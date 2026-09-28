<#
.SYNOPSIS
Builds the editor module and opens this worktree's Unreal Editor with Epic's MCP server.
.DESCRIPTION
Playbook: .github\skills\unreal-editor-mcp\SKILL.md (read sections 0 and 0.1 first on a shared machine).
- Several worktrees share this PC. Give each editor its own -Port (for example 8766-8799) and set
  $env:UNREAL_MCP_URL = 'http://127.0.0.1:<port>/mcp' for Scripts\editor_mcp.py. The script refuses a
  port that another worktree's editor is serving.
- Don't pass -Map for the 4 km Estate map: the editor has hung at startup that way. Open it after
  MCP answers with LevelEditorSubsystem.load_level('/Game/SurvivalGame/Maps/Estate').
- The first launch after a build can take more than 10 minutes before MCP answers. Raise -TimeoutSeconds
  rather than killing it; watch Saved\Logs\SurvivalGame.log.
- Live Coding and ray tracing are off by default for agent editors (-RayTracing turns RT back on).
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
    [int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'SurvivalGame.uproject'
$url = "http://127.0.0.1:$Port/mcp"

# Editor-only plugins shipped with UE 5.8 (Experimental). They are enabled for this editor
# process only, so the .uproject, commandlets, cooking and packaged builds are unaffected.
$plugins = @(
    'ModelContextProtocol'
    'EditorToolset'
    'AutomationTestToolset'
    'ConfigSettingsToolset'
    'LiveCodingToolset'
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

$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $deadline) {
    if ($process.HasExited) { throw "Unreal Editor exited early with code $($process.ExitCode). See Saved\Logs\SurvivalGame.log." }
    if (Test-McpServer) {
        Write-Host "Unreal MCP server ready at $url (editor PID $($process.Id))."
        Write-Host "Shell client: `$env:UNREAL_MCP_URL = '$url'"
        return
    }
    Start-Sleep -Seconds 3
}
throw "Unreal Editor is running (PID $($process.Id)) but MCP did not answer at $url within $TimeoutSeconds s."
