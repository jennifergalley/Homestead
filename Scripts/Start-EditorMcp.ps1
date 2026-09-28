<#
.SYNOPSIS
Builds the editor module and opens this worktree's Unreal Editor with Epic's MCP server.
.DESCRIPTION
Playbook: .github\skills\unreal-editor-mcp\SKILL.md (read sections 0 and 0.1 first on a shared machine).
- Several worktrees share this PC. Give each editor its own -Port (for example 8766-8799) and set
  $env:UNREAL_MCP_URL = 'http://127.0.0.1:<port>/mcp' for Scripts\editor_mcp.py. The script refuses a
  port that another worktree's editor is serving.
- The editor opens the Estate by default (EditorStartupMap), so -Map isn't needed; an explicit
  -Map for the Estate once hung startup. Load the old woodland with LevelEditorSubsystem.load_level
  ('/Game/SurvivalGame/Maps/Homestead') after MCP answers.
- The first launch after a build can take more than 10 minutes before MCP answers. Raise -TimeoutSeconds
  rather than killing it; watch Saved\Logs\SurvivalGame.log.
- While it waits, the script answers the editor's "Wait for ZenServer?" dialog with Yes, and stops the
  editor's own `Build.bat -Mode=ValidatePlatforms` child if it's still running after 2 minutes (it queues
  behind other worktrees' UBT builds and can hold startup for 10+ minutes). Before launching it removes a
  stale Saved\Autosaves\PackageRestoreData.json. All three otherwise block MCP with no log output.
- Live Coding and ray tracing are off by default for agent editors (-RayTracing turns RT back on).
- Refuses to launch when 3 or more Unreal processes (editors, games, commandlets) are already running on
  the machine, and lists them with their worktree. -Force overrides.
#>
[CmdletBinding()]
param(
    [string]$EngineRoot,
    [int]$Port = 8765,
    [string]$Map,
    [string[]]$ExtraPlugins = @(),
    [switch]$AllowPython,
    [switch]$RayTracing,
    [switch]$SkipBuild,
    [switch]$Force,
    [int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'SurvivalGame.uproject'
$url = "http://127.0.0.1:$Port/mcp"

# Editor-only plugins shipped with UE 5.8 (Experimental). They are enabled for this editor
# process only, so the .uproject, commandlets, cooking and packaged builds are unaffected.
# LiveCodingToolset is left out on purpose: its CompileLiveCoding tool calls
# ILiveCodingModule::Compile, which switches Live Coding on for the session regardless of the
# setting (LiveCodingModule.cpp: Compile -> EnableForSession(true)). Pass
# -ExtraPlugins LiveCodingToolset only when you're alone on the machine.
$plugins = @(
    'ModelContextProtocol'
    'EditorToolset'
    'AutomationTestToolset'
    'ConfigSettingsToolset'
    'SlateInspectorToolset'
    'PluginToolset'
    'AnimationAssistantToolset'
    'PhysicsToolsets'
) + $ExtraPlugins

function Test-McpServer {
    $body = '{"jsonrpc":"2.0","id":1,"method":"ping"}'
    try {
        $null = Invoke-WebRequest -Uri $url -Method Post -Body $body -ContentType 'application/json' `
            -Headers @{ Accept = 'application/json, text/event-stream' } -TimeoutSec 3 -SkipHttpErrorCheck
        return $true
    } catch { return $false }
}

if (Test-McpServer) {
    # Several worktrees run editors at once; make sure the one on this port is ours.
    $owners = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -match "-ModelContextProtocolPort=$Port(\D|$)" })
    $ours = @($owners | Where-Object { $_.CommandLine.IndexOf($project, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    if ($owners.Count -and -not $ours.Count) {
        $other = ($owners[0].CommandLine -split '"')[3]
        throw "MCP port $Port belongs to another worktree's editor (PID $($owners[0].ProcessId), $other). Pass a free -Port and set `$env:UNREAL_MCP_URL to match."
    }
    Write-Host "Unreal MCP server is already answering at $url$(if ($ours.Count) { " (this worktree's editor, PID $($ours[0].ProcessId))" } else { ' (owner not identified)' })"
    Write-Host "Shell client: `$env:UNREAL_MCP_URL = '$url'"
    return
}

$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')

# Machine rule: at most 3 Unreal processes in total (editors, packaged games, commandlets). More have
# reset the GPU driver and exhausted VRAM for every session. Checked right before launching, so a
# listing earlier in the same command can't go stale.
$unreal = @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'SurvivalGame%' OR Name LIKE 'JennysHomestead%'" -ErrorAction SilentlyContinue)
if ($unreal.Count -ge 3 -and -not $Force) {
    $list = ($unreal | ForEach-Object {
        $where = if ($_.CommandLine -match 'copilot-worktrees\\SurvivalGame\\([^\\"]+)') { $Matches[1] } elseif ($_.ExecutablePath -match 'HomesteadMVP') { 'HomesteadMVP' } else { '?' }
        "  PID $($_.ProcessId) $($_.Name) $([int]($_.WorkingSetSize / 1MB)) MB since $($_.CreationDate.ToString('HH:mm')) ($where)"
    }) -join "`n"
    throw "$($unreal.Count) Unreal processes are already running (the machine limit is 3):`n$list`nWait for one to finish, close your own, or ask its owner. A tiny editor that's been up a long time may be stuck on a dialog. -Force overrides this check."
}

# Belt and braces for the -ini: Live Coding opt-out below: also write it into this worktree's saved
# per-project settings, so a launch that bypasses this script (a hand-run editor, a -game run) starts
# with Live Coding off too. One active Live Coding session anywhere blocks every worktree's build,
# because UBT checks a mutex named after the shared UnrealEditor.exe path (HotReload.cs).
$userSettings = Join-Path $root 'Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini'
$null = New-Item -ItemType Directory -Force (Split-Path $userSettings)
$lines = [Collections.Generic.List[string]]::new()
if (Test-Path -LiteralPath $userSettings) { foreach ($l in Get-Content -LiteralPath $userSettings) { $lines.Add($l) } }
$section = '[/Script/LiveCoding.LiveCodingSettings]'
$at = $lines.IndexOf($section)
if ($at -lt 0) {
    if ($lines.Count -and $lines[$lines.Count - 1] -ne '') { $lines.Add('') }
    $lines.Add($section); $lines.Add('bEnabled=False')
} else {
    $end = $at + 1
    while ($end -lt $lines.Count -and -not $lines[$end].StartsWith('[')) { $end++ }
    $key = -1
    for ($k = $at + 1; $k -lt $end; $k++) { if ($lines[$k] -match '^\s*bEnabled\s*=') { $key = $k } }
    if ($key -ge 0) { $lines[$key] = 'bEnabled=False' } else { $lines.Insert($at + 1, 'bEnabled=False') }
}
Set-Content -LiteralPath $userSettings -Value $lines -Encoding utf8

# A killed editor leaves Saved\Autosaves\PackageRestoreData.json, and the next launch then stops on a
# modal "Restore Packages" dialog before MCP starts (it doesn't take synthetic input). Agents don't
# recover autosaves, so clear it when no editor for this worktree is running.
$restore = Join-Path $root 'Saved\Autosaves\PackageRestoreData.json'
if (Test-Path -LiteralPath $restore) {
    $mine = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -and $_.CommandLine.IndexOf($project, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    if (-not $mine.Count) {
        Remove-Item -LiteralPath $restore -Force
        Write-Host 'Removed a stale Saved\Autosaves\PackageRestoreData.json (the last editor was killed); no restore prompt this launch.'
    }
}

if (-not $SkipBuild) {
    $build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
    & $build SurvivalGameEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
    if ($LASTEXITCODE -ne 0) { throw "Unreal editor-module build failed ($LASTEXITCODE)." }
}

$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$arguments = @("`"$project`"")
if ($Map) { $arguments += $Map }
$arguments += @(
    "-EnablePlugins=$($plugins -join ',')"
    '-ModelContextProtocolStartServer'
    "-ModelContextProtocolPort=$Port"
    # Agents drive the editor while another window has focus; don't throttle PIE in the background.
    '-ini:EditorSettings:[/Script/UnrealEd.EditorPerformanceSettings]:bThrottleCPUWhenNotForeground=False'
    # Several worktrees share this machine; an active Live Coding session blocks every other
    # worktree's SurvivalGameEditor build, so agent editors run without it.
    '-ini:EditorPerProjectUserSettings:[/Script/LiveCoding.LiveCodingSettings]:bEnabled=False'
    # Blender and other sessions write under Assets\ constantly; the "source content changed, import?"
    # toast covers captures. Agents import explicitly (import_props.py), so don't watch for changes.
    '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorLoadingSavingSettings]:bMonitorContentDirectories=False'
    '-nosplash'
) + @(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1'))
# Several agent editors share one GPU. Building ray-tracing pipelines in all of them at once has
# reset the driver (DXGI_ERROR_DEVICE_REMOVED / DRIVER_INTERNAL_ERROR), taking every Unreal process
# down with it, so agent editors render without ray tracing unless -RayTracing is passed.
if (-not $RayTracing) { $arguments += '-DPCVars=r.RayTracing.Enable=0' }
# Opt-in: registers homestead_agent.toolset.HomesteadEditorPython.run_python (arbitrary editor Python).
if ($AllowPython) { $arguments += '-HomesteadAgentPython' }

$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru
Write-Host "Started Unreal Editor (PID $($process.Id)); waiting for MCP at $url ..."

# When this worktree's DerivedDataCache\Zen differs from the zenserver already on port 8558, the
# editor restarts zenserver on its own data dir and can stop at a native "Wait for ZenServer?"
# Yes/No dialog (ZenServerInterface.cpp) with nothing in the log. Answer Yes (keep waiting) by
# sending the dialog its own IDC_YES command (1003, Launch\Resources\Windows\resource.h).
if (-not ('HomesteadMcp.EditorDialogs' -as [type])) {
    Add-Type -Namespace HomesteadMcp -Name EditorDialogs -MemberDefinition @'
public delegate bool EnumProc(System.IntPtr hwnd, System.IntPtr lParam);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, System.IntPtr lParam);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(System.IntPtr hwnd, out uint pid);
[System.Runtime.InteropServices.DllImport("user32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)] public static extern int GetWindowText(System.IntPtr hwnd, System.Text.StringBuilder text, int max);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern bool IsWindowVisible(System.IntPtr hwnd);
[System.Runtime.InteropServices.DllImport("user32.dll")] public static extern System.IntPtr PostMessage(System.IntPtr hwnd, uint msg, System.IntPtr w, System.IntPtr l);
public static int AnswerYes(uint processId, string title) {
    int answered = 0;
    EnumWindows((h, l) => {
        uint pid; GetWindowThreadProcessId(h, out pid);
        if (pid != processId || !IsWindowVisible(h)) return true;
        var text = new System.Text.StringBuilder(256); GetWindowText(h, text, 256);
        if (text.ToString() == title) { PostMessage(h, 0x0111, (System.IntPtr)1003, System.IntPtr.Zero); answered++; }
        return true;
    }, System.IntPtr.Zero);
    return answered;
}
'@
}

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline) {
    if ($process.HasExited) { throw "Unreal Editor exited early with code $($process.ExitCode). See Saved\Logs\SurvivalGame.log." }
    if (Test-McpServer) {
        Write-Host "Unreal MCP server ready at $url (editor PID $($process.Id))."
        Write-Host "Shell client: `$env:UNREAL_MCP_URL = '$url'"
        return
    }
    if ([HomesteadMcp.EditorDialogs]::AnswerYes([uint32]$process.Id, 'Wait for ZenServer?')) {
        Write-Host 'Answered "Wait for ZenServer?" with Yes (zenserver was restarting for this worktree''s cache).'
    }
    # Every editor start runs `Build.bat -Mode=ValidatePlatforms` (TargetPlatformManagerModule.cpp). It's a
    # single-instance UBT mode, so it queues behind any other worktree's UBT build and can hold startup for
    # 10+ minutes. It normally takes seconds; stopping a stuck one lets the editor continue (lanes did this
    # by hand). Only this editor's own child tree is touched.
    $stuck = @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $($process.Id)" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -match 'ValidatePlatforms' -and ((Get-Date) - $_.CreationDate).TotalSeconds -gt 120 })
    foreach ($child in $stuck) {
        $tree = @($child.ProcessId)
        for ($i = 0; $i -lt $tree.Count -and $i -lt 64; $i++) {
            $tree += @(Get-CimInstance Win32_Process -Filter "ParentProcessId = $($tree[$i])" -ErrorAction SilentlyContinue |
                ForEach-Object { $_.ProcessId })
        }
        [array]::Reverse($tree)
        foreach ($id in $tree) { Stop-Process -Id $id -Force -ErrorAction SilentlyContinue }
        Write-Host "Stopped the editor's ValidatePlatforms check (PID $($child.ProcessId), over 2 min; probably waiting on another worktree's UBT build)."
    }
    Start-Sleep -Seconds 3
}
throw "Unreal Editor is running (PID $($process.Id)) but MCP did not answer at $url within $TimeoutSeconds s."
