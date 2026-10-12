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
    hshot [WxH]                            HighResShot in PIE; returns the new Saved\Screenshots\WindowsEditor PNG path. The 3D view and
                                           Canvas HUD only: Slate widgets (hotbar, vitals, field book) are missing; use shot for UI
    pie / unpie                            start PIE in the viewport / stop it (then poll st for worldReady)
    quit                                   stop PIE and quit the editor cleanly (releases DLL and .uasset locks)
    pyfile <path>                          run a Python file in the editor with __file__ set (plain run_python has none)
    tp <x> <y> [z]                         HomesteadTeleport: stand her on the ground at x,y (with z: on the first surface at or below z)
    sellat ["Name N,Name N"]               HomesteadSellAtStore: open the General Store's hours, stand her at its real counter, sell what she carries, open the shop; logs LogHomesteadPlaytest
    state [n] [radius]                     the full world/character dump (time, weather, pack, hotbar, lamp, equipment, position, focus, nearby with ids, store, open screen)
    goto <target> / use "<target> [item]"  walk-free go-to (store, mine, node:ID, plot:N, nearest kind...) / go-to then interact or use the hotbar item
    screen <name> / weather rain|clear / reset   open or close a screen by name / skip to rain or a clear hour / full energy and lamp, screens closed
    plog [n] [pattern]                     recent LogHomesteadPlaytest lines from this worktree's editor log (results of the commands above)
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

# hotbar: the hotbar as the player sees it (key, item, count, selected). `hotbar OilLamp` (or `hotbar Pickaxe`)
# presses that item's number key and shows the hotbar after.
function hotbar([string]$item = '') {
    $cells = (st).hotbar
    if ($item) {
        $cell = $cells | Where-Object { $_.item -like "*$item*" } | Select-Object -First 1
        if (-not $cell) { Write-Warning "No hotbar cell holds '$item'."; return $cells | Format-Table index, key, item, count, selected }
        $names = @{ '1' = 'One'; '2' = 'Two'; '3' = 'Three'; '4' = 'Four'; '5' = 'Five'; '6' = 'Six'; '7' = 'Seven'; '8' = 'Eight'; '9' = 'Nine'; '0' = 'Zero' }
        hk tap_key (@{ key = $names[$cell.key] } | ConvertTo-Json -Compress) | Out-Null
        Start-Sleep -Milliseconds 400
        $cells = (st).hotbar
    }
    $cells | Format-Table index, key, item, count, selected
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

# UI gallery in the running PIE (homestead.UIGallery): gallery <id> [Pad|KBM] [waitSeconds], then
# the editor capture path. 'gallery list' logs the ids. Every state at several resolutions:
# Scripts\Capture-UiGallery.ps1. Only on an isolated save route: Start-EditorMcp.ps1 -PreviewProfile gallery.
function gallery([string]$id = 'list', [string]$inputMode = 'KBM', [double]$wait = 6) {
    if ($id -eq 'list') { $null = con 'homestead.UIGallery list'; return 'Listed in the Output Log (LogHomesteadUIGallery).' }
    $null = con "homestead.UIGallery $id $inputMode"
    Start-Sleep -Milliseconds ([int]($wait * 1000))
    shot
}

function unpie() { py 'unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()' }

function quit() {
    py "unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()`nunreal.SystemLibrary.quit_editor()"
}

function pyfile([string]$path, [int]$timeout = 600) {
    $full = (Resolve-Path -LiteralPath $path).Path
    py "p = $(ConvertTo-Json $full)`nexec(compile(open(p, encoding='utf-8').read(), p, 'exec'), {'__file__': p, '__name__': '__main__', 'unreal': unreal})" $timeout
}

function tp([double]$x, [double]$y, $z = $null) {
    # HomesteadTeleport stands her on the ground at x,y (waiting for its collision to stream in), so she
    # never drops from a guessed height. With z she lands on the first surface at or below z (an upper floor).
    $cmd = "HomesteadTeleport $x $y" + $(if ($null -ne $z) { " $([double]$z)" } else { '' })
    $null = con $cmd
    $cmd
}

function sellat([string]$items = 'Coal 3,CopperOre 2,Quartz 2,Amethyst 1') {
    # HomesteadSellAtStore: brings the General Store's hours round (skips to 9 AM if closed or Sunday), teleports her
    # to the real village counter, sells the carried items through the shop's own sell path, opens the shop screen and
    # logs per-item prices and the purse delta (filter the log for LogHomesteadPlaytest). It sells; it does not grant,
    # so `con "HomesteadGive Coal 3"` first. The sale waits for the teleport to settle, so read the log a few seconds later.
    $cmd = "HomesteadSellAtStore $items"
    $null = con $cmd
    $cmd
}

# The test-tools commands (details and recipes: .github\skills\homestead-test-tools\SKILL.md). Each returns the
# console command it ran; the result is in the log: `plog` prints the recent LogHomesteadPlaytest lines.
function plog([int]$n = 12, [string]$pattern = 'LogHomesteadPlaytest') {
    $log = Join-Path $script:McpRoot 'Saved\Logs\SurvivalGame.log'
    if (-not (Test-Path $log)) { return "No $log (is this worktree's editor running?)" }
    $lines = [IO.File]::ReadAllLines($log) | Where-Object { $_ -match $pattern }
    $lines | Select-Object -Last $n
}

# state: the whole world/character picture in one call (st's `world` object). `state | ConvertTo-Json -Depth 6`
# to read it all; `(state).nearby` lists things with ids, kinds and distances.
function state([int]$n = 12, [int]$radius = 2000) {
    $o = mcp $script:TsPlay get_play_state "{`"nearby_count`": $n, `"radius_cm`": $radius}" | Out-String
    $s = (($o | ConvertFrom-Json).content[0].text | ConvertFrom-Json).returnValue | ConvertFrom-Json
    if ($s.world) { $s.world } else { $s }
}

function goto([string]$target) {
    # HomesteadGoto: store | counter | mine | cove | square | gate | mill | farm | home | x,y | an id such as node:520001 /
    # plot:3 | the nearest <resource, piece or plot kind>. Stands her facing it, then logs the focus.
    $cmd = "HomesteadGoto $target"
    $null = con $cmd
    $cmd
}

function use([string]$targetAndItem) {
    # HomesteadUse "<target> [item]": goto, then Interact (no item) or select the item on the hotbar and use it.
    $cmd = "HomesteadUse $targetAndItem"
    $null = con $cmd
    $cmd
}

function screen([string]$name = 'close') {
    # HomesteadScreen: close | inventory | craft | build | settings | appearance | map | shop | chest
    $cmd = "HomesteadScreen $name"
    $null = con $cmd
    $cmd
}

function weather([string]$kind) {
    # HomesteadWeather rain|clear: skip the clock to the next rain spell, or to a clear dry hour.
    $cmd = "HomesteadWeather $kind"
    $null = con $cmd
    $cmd
}

function reset() {
    # HomesteadReset: full energy, full lamp oil, screens closed, morning if night. Items, purse and placements stay.
    $null = con 'HomesteadReset'
    'HomesteadReset'
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
