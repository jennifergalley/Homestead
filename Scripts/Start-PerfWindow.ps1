<#
.SYNOPSIS
Claims the machine for a perf or frame-rate measurement (Jenny's rule: one Unreal process, no builds).
.DESCRIPTION
Refuses unless this worktree's editor, or the one -ProcessId you name (for example a standalone
-game you launched), is the only Unreal process running and no UBT, cl.exe or link.exe build is going.
Then writes E:\CopilotScratch\homestead-perf.lock with this worktree, the session and a UTC time.
While the lock is fresh (under 20 minutes), Start-EditorMcp.ps1 in other worktrees refuses to launch.
Measure, then run Scripts\Stop-PerfWindow.ps1. A lock older than 20 minutes counts as stale; run this
again to refresh it for a longer measurement.

    .\Scripts\Start-PerfWindow.ps1 -Purpose 'Estate 4K frame pacing'
    .\Scripts\Start-PerfWindow.ps1 -ProcessId 12345        # a standalone game you launched
#>
[CmdletBinding()]
param([string]$Purpose = '', [int]$ProcessId, [string]$Session = $env:SESSION_ID)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PerfLock.ps1')
$root = Split-Path $PSScriptRoot -Parent
$worktree = Split-Path $root -Leaf
$project = Join-Path $root 'SurvivalGame.uproject'

$lock = Get-PerfLock
if ($lock -and -not $lock.stale -and $lock.worktree -ne $worktree) {
    throw "Another session holds the perf window: $($lock.worktree) since $($lock.startedUtc) ($($lock.ageMinutes) min, '$($lock.purpose)'). Wait until it's released or older than $PerfLockStaleMinutes min."
}

$unreal = Get-UnrealProcesses
$others = @($unreal | Where-Object {
    if ($ProcessId) { $_.ProcessId -ne $ProcessId } else { -not ($_.CommandLine -and $_.CommandLine.IndexOf($project, [StringComparison]::OrdinalIgnoreCase) -ge 0) }
})
$builds = Get-BuildProcesses
$problems = @()
if (-not $unreal.Count) { $problems += 'No Unreal process is running yet: start the one you will measure first.' }
if ($ProcessId -and -not ($unreal | Where-Object ProcessId -eq $ProcessId)) { $problems += "Process $ProcessId isn't a running Unreal process." }
foreach ($p in $others) { $problems += "Unreal process from another worktree or launch: PID $($p.ProcessId) $($p.Name) ($(Get-WorktreeName $p.CommandLine))" }
if (@($unreal).Count -gt 1 -and -not $others.Count) { $problems += "More than one Unreal process is yours; close all but the one you'll measure." }
foreach ($b in $builds) { $problems += "Build running: PID $($b.ProcessId) $($b.Name) ($(Get-WorktreeName $b.CommandLine))" }
if ($problems) {
    throw "Not a clean perf window (Jenny's rule: one Unreal process, no builds):`n  $($problems -join "`n  ")`nAsk the owners (mailbox_send) to close or pause, then try again."
}

$null = New-Item -ItemType Directory -Force (Split-Path $PerfLockPath)
[ordered]@{
    worktree = $worktree; session = $Session; purpose = $Purpose
    startedUtc = [DateTimeOffset]::UtcNow.ToString('o'); processId = @($unreal)[0].ProcessId
} | ConvertTo-Json | Set-Content -LiteralPath $PerfLockPath -Encoding utf8
Write-Host "Perf window claimed for $worktree ($PerfLockPath). Measure now, then run Scripts\Stop-PerfWindow.ps1."
Write-Host "It counts as stale after $PerfLockStaleMinutes min; run this again to refresh a longer measurement."
