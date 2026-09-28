<#
.SYNOPSIS
Shell helpers for driving this worktree's Unreal Editor over MCP. Dot-source it once per command.
.DESCRIPTION
    . .\Scripts\McpHelpers.ps1 -Port 8768

Point -Port at the port you gave Start-EditorMcp.ps1. Each worktree's editor needs its own port, and
8765 may belong to someone else. The script sets $env:UNREAL_MCP_URL for this process only.

Functions:
    mcp <toolset> <tool> [json] [timeout]  call_tool; returns the raw JSON result
    hk <tool> [json]                       HomesteadPlayTools call; returns its text
    st [nearby]                            parsed get_play_state
    py <code> [timeout]                    run_python (needs Start-EditorMcp.ps1 -AllowPython); returns output text
    con <command>                          console command in PIE (or the editor world), with the player controller
    shot                                   CaptureEditorImage; returns the PNG path (can fail: "Failed to capture any editor windows")
    hshot [WxH]                            HighResShot in PIE; returns the new Saved\Screenshots\WindowsEditor PNG path (more reliable)
    pie / unpie                            start PIE in the viewport / stop it (then poll st for worldReady)
    quit                                   stop PIE and quit the editor cleanly (releases DLL and .uasset locks)
    pyfile <path>                          run a Python file in the editor with __file__ set (plain run_python has none)
    tp <x> <y> [z]                         move the player pawn (z default 200; she drops to the ground)
    click <x> <y>                          real Win32 left click at editor-window pixels (Slate clicks don't reach game widgets)
Variables: $McpEditor $McpScene $McpLogs $McpSlate $McpPlay $McpPython (toolset names for your own mcp calls).
Short aliases $E $S $L $SL $H $PY are set too, but only if you haven't already defined those names; dot-sourcing
never overwrites your variables. Full playbook: .github\skills\unreal-editor-mcp\SKILL.md.
#>
param([Parameter(Mandatory)][int]$Port)

$script:McpRoot = Split-Path $PSScriptRoot -Parent
$script:McpClient = Join-Path $PSScriptRoot 'editor_mcp.py'
$script:McpArgsDir = Join-Path $script:McpRoot 'Saved\McpArgs'
$null = New-Item -ItemType Directory -Force $script:McpArgsDir
$env:UNREAL_MCP_URL = "http://127.0.0.1:$Port/mcp"

# Toolset names. The helpers use the $script:Ts* copies, so a caller's own $s/$e/$h/$l can't break
# them. For your own mcp calls use the $Mcp* names. The short $E/$S/$L/$SL/$H/$PY aliases are set only
# if the caller hasn't already defined them: PowerShell names are case-insensitive, and dot-sourcing
# must never overwrite a caller's $s = 'E:\scratch'.
$script:TsEditor = 'EditorToolset.EditorAppToolset'
$script:TsPlay = 'homestead_agent.toolset.HomesteadPlayTools'
$script:TsPython = 'homestead_agent.toolset.HomesteadEditorPython'
$McpEditor = $script:TsEditor
$McpScene = 'editor_toolset.toolsets.scene.SceneTools'
$McpLogs = 'EditorToolset.LogsToolset'
$McpSlate = 'SlateInspectorToolset.SlateInspectorToolset'
$McpPlay = $script:TsPlay
$McpPython = $script:TsPython
foreach ($alias in @(@('E', $McpEditor), @('S', $McpScene), @('L', $McpLogs), @('SL', $McpSlate), @('H', $McpPlay), @('PY', $McpPython))) {
    if (-not (Get-Variable -Name $alias[0] -Scope 0 -ErrorAction SilentlyContinue)) {
        Set-Variable -Name $alias[0] -Value $alias[1] -Scope 0
    }
}

function mcp([string]$ts, [string]$tool, $a = '{}', [int]$timeout = 600) {
    $arguments = if ($a -is [string]) { ConvertFrom-Json $a -AsHashtable } else { $a }
    $json = @{ toolset_name = $ts; tool_name = $tool; arguments = $arguments } | ConvertTo-Json -Depth 20 -Compress
    # A unique file per call: parallel sessions and sub-agents must not share one args file.
    $file = Join-Path $script:McpArgsDir "args-$PID-$([guid]::NewGuid().ToString('N')).json"
    [IO.File]::WriteAllText($file, $json)
    try { python $script:McpClient --timeout $timeout call call_tool "@$file" }
    finally { Remove-Item -LiteralPath $file -ErrorAction SilentlyContinue }
}

function hk([string]$tool, $a = '{}') { (mcp $script:TsPlay $tool $a | Out-String | ConvertFrom-Json).content[0].text }

function st([int]$n = 6) {
    $o = mcp $script:TsPlay get_play_state "{`"nearby_count`": $n, `"radius_cm`": 4000}" | Out-String
    (($o | ConvertFrom-Json).content[0].text | ConvertFrom-Json).returnValue | ConvertFrom-Json
}

function py([string]$code, [int]$timeout = 600) {
    $o = mcp $script:TsPython run_python @{ code = $code } $timeout | Out-String
    try {
        $text = ($o | ConvertFrom-Json).content[0].text
        try { ($text | ConvertFrom-Json).returnValue } catch { $text }
    } catch { $o }
}

function con([string]$command) {
    py @"
import unreal
sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = sub.get_game_world() or sub.get_editor_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0) if sub.get_game_world() else None
unreal.SystemLibrary.execute_console_command(w, $(ConvertTo-Json $command), pc)
"@
}

function shot() {
    $o = mcp $script:TsEditor CaptureEditorImage | Out-String
    if ($o -match 'saved to ([^>]+?\.png)') { $Matches[1].Replace('\\\\', '\') } else { $o }
}

function hshot([string]$res = '1920x1080') {
    $dir = Join-Path $script:McpRoot 'Saved\Screenshots\WindowsEditor'
    $before = @(Get-ChildItem $dir -Filter *.png -ErrorAction SilentlyContinue | ForEach-Object FullName)
    $null = con "HighResShot $res"
    for ($i = 0; $i -lt 30; $i++) {
        Start-Sleep 1
        $new = Get-ChildItem $dir -Filter *.png -ErrorAction SilentlyContinue |
            Where-Object { $before -notcontains $_.FullName } | Select-Object -First 1
        if ($new) { Start-Sleep 1; return $new.FullName }
    }
    'HighResShot wrote nothing within 30 s (is PIE running?)'
}

function pie() {
    $null = mcp $script:TsEditor StartPIE '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":5}}'
    'PIE requested; StartPIE may report a timeout while it loads. Poll st until worldReady.'
}

function unpie() { py 'unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()' }

function quit() {
    py "unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()`nunreal.SystemLibrary.quit_editor()"
}

function pyfile([string]$path, [int]$timeout = 600) {
    $full = (Resolve-Path -LiteralPath $path).Path
    py "p = $(ConvertTo-Json $full)`nexec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__file__': p, '__name__': '__main__', 'unreal': unreal})" $timeout
}

function tp([double]$x, [double]$y, [double]$z = 200) {
    py ("w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()`n" +
        "pawn = unreal.GameplayStatics.get_player_pawn(w, 0) if w else None`n" +
        "print(pawn.set_actor_location(unreal.Vector($x, $y, $z), False, True) if pawn else 'No PIE pawn')")
}

$script:McpUser32 = @"
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
[DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint d, UIntPtr e);
[DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint f, UIntPtr e);
public struct RECT { public int Left, Top, Right, Bottom; }
"@

function click([int]$x, [int]$y) {
    if (-not ('HomesteadMcp.User32' -as [type])) { Add-Type -Namespace HomesteadMcp -Name User32 -MemberDefinition $script:McpUser32 }
    $u = [HomesteadMcp.User32]
    $port = ([uri]$env:UNREAL_MCP_URL).Port
    $owner = Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" |
        Where-Object { $_.CommandLine -match "-ModelContextProtocolPort=$port(\D|$)" } | Select-Object -First 1
    if (-not $owner) { return "No editor found on port $port" }
    $hwnd = (Get-Process -Id $owner.ProcessId).MainWindowHandle
    $null = $u::SetProcessDPIAware()
    $u::keybd_event(0x12, 0, 0, [UIntPtr]::Zero); $u::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)  # an Alt tap lets SetForegroundWindow work
    $null = $u::SetForegroundWindow($hwnd)
    $r = New-Object 'HomesteadMcp.User32+RECT'; $null = $u::GetWindowRect($hwnd, [ref]$r)
    $null = $u::SetCursorPos($r.Left + $x, $r.Top + $y); Start-Sleep -Milliseconds 80
    $u::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero); Start-Sleep -Milliseconds 60
    $u::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
    "Clicked ($x, $y) in the editor window at ($($r.Left), $($r.Top))"
}
