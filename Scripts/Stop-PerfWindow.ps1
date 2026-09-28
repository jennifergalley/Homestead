<#
.SYNOPSIS
Releases the perf window claimed with Start-PerfWindow.ps1.
.DESCRIPTION
Deletes E:\CopilotScratch\homestead-perf.lock if this worktree owns it (or it's stale). -Force removes
another worktree's lock; only do that when its owner is gone.
#>
[CmdletBinding()]
param([switch]$Force)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'PerfLock.ps1')
$worktree = Split-Path (Split-Path $PSScriptRoot -Parent) -Leaf
$lock = Get-PerfLock
if (-not $lock) {
    if (Test-Path -LiteralPath $PerfLockPath) { Remove-Item -LiteralPath $PerfLockPath -Force; Write-Host 'Removed an unreadable perf lock.' }
    else { Write-Host 'No perf window is claimed.' }
    return
}
if ($lock.worktree -ne $worktree -and -not $lock.stale -and -not $Force) {
    throw "The perf window belongs to $($lock.worktree) ($($lock.ageMinutes) min old). Use -Force only if that session is gone."
}
Remove-Item -LiteralPath $PerfLockPath -Force
Write-Host "Perf window released (was held by $($lock.worktree) for $($lock.ageMinutes) min)."
