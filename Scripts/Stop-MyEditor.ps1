<#
.SYNOPSIS
Closes this worktree's Unreal Editor, and only that one, even when several worktrees have editors open.
.DESCRIPTION
Finds UnrealEditor.exe processes whose command line contains this worktree's SurvivalGame.uproject
(a plain substring match: regex -match on paths trips over the backslashes). It first asks the editor
to quit cleanly through MCP when -Port is given (stop PIE, then quit_editor, which leaves no restore
prompt), waits up to -WaitSeconds, and only then stops the process by PID.

    .\Scripts\Stop-MyEditor.ps1 -Port 8768     # clean quit via MCP, then force if it hangs
    .\Scripts\Stop-MyEditor.ps1                # force stop by PID (Start-EditorMcp clears the restore file next launch)
    .\Scripts\Stop-MyEditor.ps1 -WhatIf        # show what would be closed
#>
[CmdletBinding(SupportsShouldProcess)]
param([int]$Port, [int]$WaitSeconds = 60)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'SurvivalGame.uproject'

function Get-MyEditors {
    @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -in 'UnrealEditor.exe', 'UnrealEditor-Cmd.exe' -and $_.CommandLine -and
            $_.CommandLine.IndexOf($project, [StringComparison]::OrdinalIgnoreCase) -ge 0 })
}

$mine = Get-MyEditors
if (-not $mine.Count) { Write-Host "No Unreal Editor is running for $root."; return }
foreach ($p in $mine) { Write-Host "This worktree's editor: PID $($p.ProcessId) $($p.Name), since $($p.CreationDate.ToString('HH:mm'))" }
if (-not $PSCmdlet.ShouldProcess("PID $(($mine.ProcessId) -join ', ')", 'Close editor')) { return }

if ($Port) {
    $env:UNREAL_MCP_URL = "http://127.0.0.1:$Port/mcp"
    $code = "unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()`nunreal.SystemLibrary.quit_editor()"
    $json = @{ toolset_name = 'homestead_agent.toolset.HomesteadEditorPython'; tool_name = 'run_python'; arguments = @{ code = $code } } |
        ConvertTo-Json -Compress
    $argsDir = Join-Path $root 'Saved\McpArgs'
    $null = New-Item -ItemType Directory -Force $argsDir
    $file = Join-Path $argsDir "quit-$PID.json"
    [IO.File]::WriteAllText($file, $json)
    try { $null = python (Join-Path $PSScriptRoot 'editor_mcp.py') --timeout 30 call call_tool "@$file" 2>&1 }
    catch { Write-Host "MCP quit request failed ($_); will stop by PID." }
    finally { Remove-Item -LiteralPath $file -ErrorAction SilentlyContinue }
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while ((Get-Date) -lt $deadline -and (Get-MyEditors).Count) { Start-Sleep 2 }
}

foreach ($p in Get-MyEditors) {
    Write-Host "Stopping PID $($p.ProcessId)."
    Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue
}
if ((Get-MyEditors).Count) { throw 'The editor is still running.' }
Write-Host 'Closed. Live Coding patch files can stay locked for about a minute after exit.'
