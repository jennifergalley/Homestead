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
Variables: $E $S $L $SL $H $PY (toolset names). Full playbook: .github\skills\unreal-editor-mcp\SKILL.md.
#>
param([Parameter(Mandatory)][int]$Port)

$script:McpRoot = Split-Path $PSScriptRoot -Parent
$script:McpClient = Join-Path $PSScriptRoot 'editor_mcp.py'
$script:McpArgsDir = Join-Path $script:McpRoot 'Saved\McpArgs'
$null = New-Item -ItemType Directory -Force $script:McpArgsDir
$env:UNREAL_MCP_URL = "http://127.0.0.1:$Port/mcp"

$E = 'EditorToolset.EditorAppToolset'
$S = 'editor_toolset.toolsets.scene.SceneTools'
$L = 'EditorToolset.LogsToolset'
$SL = 'SlateInspectorToolset.SlateInspectorToolset'
$H = 'homestead_agent.toolset.HomesteadPlayTools'
$PY = 'homestead_agent.toolset.HomesteadEditorPython'

function mcp([string]$ts, [string]$tool, $a = '{}', [int]$timeout = 600) {
    $arguments = if ($a -is [string]) { ConvertFrom-Json $a -AsHashtable } else { $a }
    $json = @{ toolset_name = $ts; tool_name = $tool; arguments = $arguments } | ConvertTo-Json -Depth 20 -Compress
    # A unique file per call: parallel sessions and sub-agents must not share one args file.
    $file = Join-Path $script:McpArgsDir "args-$PID-$([guid]::NewGuid().ToString('N')).json"
    [IO.File]::WriteAllText($file, $json)
    try { python $script:McpClient --timeout $timeout call call_tool "@$file" }
    finally { Remove-Item -LiteralPath $file -ErrorAction SilentlyContinue }
}

function hk([string]$tool, $a = '{}') { (mcp $H $tool $a | Out-String | ConvertFrom-Json).content[0].text }

function st([int]$n = 6) {
    $o = mcp $H get_play_state "{`"nearby_count`": $n, `"radius_cm`": 4000}" | Out-String
    (($o | ConvertFrom-Json).content[0].text | ConvertFrom-Json).returnValue | ConvertFrom-Json
}

function py([string]$code, [int]$timeout = 600) {
    $o = mcp $PY run_python @{ code = $code } $timeout | Out-String
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
    $o = mcp $E CaptureEditorImage | Out-String
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
