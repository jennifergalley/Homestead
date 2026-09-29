# Shared by Start-PerfWindow.ps1, Stop-PerfWindow.ps1 and Start-EditorMcp.ps1 (dot-source it).
# Jenny's rule: perf and frame-rate measurements run with only ONE Unreal process on the machine and no
# UBT/cl.exe builds going; with several editors and builds, readings swung about 5x (render thread 20 ms
# vs 97-118 ms). The lock tells other sessions not to launch or build while someone measures.

$PerfLockPath = 'E:\CopilotScratch\homestead-perf.lock'
$PerfLockStaleMinutes = 20

function Get-PerfLock {
    # Returns the lock (worktree, session, startedUtc, ageMinutes, stale) or $null when there is none.
    if (-not (Test-Path -LiteralPath $PerfLockPath)) { return $null }
    try { $lock = Get-Content -LiteralPath $PerfLockPath -Raw | ConvertFrom-Json } catch { return $null }
    $started = [DateTimeOffset]::Parse($lock.startedUtc, [Globalization.CultureInfo]::InvariantCulture)
    $age = ([DateTimeOffset]::UtcNow - $started).TotalMinutes
    [pscustomobject]@{
        worktree = $lock.worktree; session = $lock.session; purpose = $lock.purpose
        startedUtc = $started.UtcDateTime.ToString('yyyy-MM-dd HH:mm') + ' UTC'; ageMinutes = [math]::Round($age, 1); stale = $age -ge $PerfLockStaleMinutes
    }
}

function Get-UnrealProcesses {
    @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'SurvivalGame%' OR Name LIKE 'JennysHomestead%'" -ErrorAction SilentlyContinue)
}

function Get-BuildProcesses {
    # Blender counts too: headless asset builds from other lanes load the CPU and skew frame times.
    @(Get-CimInstance Win32_Process -Filter "Name = 'dotnet.exe' OR Name = 'cl.exe' OR Name = 'link.exe' OR Name = 'UnrealBuildTool.exe' OR Name = 'blender.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne 'dotnet.exe' -or $_.CommandLine -match 'UnrealBuildTool|AutomationTool' })
}

function Get-WorktreeName([string]$CommandLine) {
    if ($CommandLine -match 'copilot-worktrees\\SurvivalGame\\([^\\"/]+)') { return $Matches[1] }
    if ($CommandLine -match 'copilot-worktrees/SurvivalGame/([^\\"/]+)') { return $Matches[1] }
    if ($CommandLine -match 'HomesteadMVP') { return 'HomesteadMVP' }
    '?'
}
