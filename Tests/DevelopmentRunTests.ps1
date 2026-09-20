[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$control = Join-Path $root 'Scripts\Development-Run.ps1'
$status = Join-Path $root 'Scripts\Update-DevelopmentStatus.ps1'
$directory = Join-Path $root ('Saved\RunControlTests\' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $directory -Force
$checks = 0
function Assert-True([bool]$condition, [string]$message) {
    if (-not $condition) { throw $message }
    $script:checks++
}
function Assert-Throws([scriptblock]$operation, [string]$message) {
    $threw = $false
    try { & $operation | Out-Null } catch { $threw = $true }
    Assert-True $threw $message
}
try {
    Assert-Throws { & $control -StateDirectory $directory } 'Missing run must be explicit.'
    $run = & $control -Action Start -Hours 8 -CoordinatorSessionId 'test-coordinator' -StateDirectory $directory
    Assert-True $run.allowWork 'Fresh run should allow work.'
    $duration = [DateTimeOffset]::Parse($run.deadlineUtc) - [DateTimeOffset]::Parse($run.startedUtc)
    Assert-True ($duration.TotalHours -eq 8) 'Deadline must be exactly eight hours from activation.'
    Assert-Throws { & $control -Action Start -CoordinatorSessionId 'duplicate' -StateDirectory $directory } 'Duplicate run must fail.'
    $assigned = & $control -Action AssignWorker -WorkerSessionId 'test-worker' -StateDirectory $directory
    Assert-True ($assigned.workerSessionId -eq 'test-worker') 'Worker assignment must persist.'
    Assert-True ($assigned.deadlineUtc.EndsWith('+00:00')) 'Read and rewrite must retain explicit UTC timestamps.'
    $paused = & $control -Action Pause -StateDirectory $directory
    Assert-True (-not $paused.allowWork) 'Paused run must refuse work.'
    $resumed = & $control -Action Resume -StateDirectory $directory
    Assert-True ($resumed.allowWork -and $resumed.deadlineUtc -eq $run.deadlineUtc) 'Resume must preserve the deadline.'
    $markdown = Join-Path $directory 'status.md'
    $null = & $status -RunId $run.id -Task 'Example task' -Phase 'testing' -Summary 'Actual result' -Evidence 'Passed evidence' -StateDirectory $directory -StatusPath $markdown
    $saved = Get-Content -LiteralPath (Join-Path $directory 'status.json') -Raw | ConvertFrom-Json
    Assert-True ($saved.task -eq 'Example task' -and $saved.evidence[0] -eq 'Passed evidence') 'Status evidence must persist.'
    Assert-True ((Get-Content -LiteralPath $markdown -Raw).Contains('test-worker')) 'Visible status must identify the worker.'
    Assert-Throws { & $status -RunId 'stale-run' -Task 'x' -Phase 'x' -Summary 'x' -StateDirectory $directory -StatusPath $markdown } 'Stale worker must not overwrite status.'
    $runPath = Join-Path $directory 'run.json'
    $expired = Get-Content -LiteralPath $runPath -Raw | ConvertFrom-Json
    $expired.deadlineUtc = [DateTimeOffset]::UtcNow.AddSeconds(-1).ToString('o')
    $expired | ConvertTo-Json | Set-Content -LiteralPath $runPath
    $permission = & $control -Action Status -StateDirectory $directory
    Assert-True ($permission.deadlinePassed -and -not $permission.allowWork) 'Expired run must refuse work even before a scheduler tick.'
    Assert-Throws { & $control -Action Resume -StateDirectory $directory } 'Resume cannot extend an expired run.'
    $stopped = & $control -Action Stop -Reason 'Test complete' -StateDirectory $directory
    Assert-True ($stopped.state -eq 'stopped' -and -not $stopped.allowWork) 'Stop must persist.'
    Assert-Throws { & $control -Action Resume -StateDirectory $directory } 'Stopped run cannot resume.'
    $newRun = & $control -Action Start -Hours 1 -CoordinatorSessionId 'test-coordinator' -StateDirectory $directory
    Assert-True ($newRun.id -ne $run.id) 'New run needs a unique ID.'
    Assert-True (Test-Path -LiteralPath (Join-Path $directory "history\$($run.id).json")) 'Previous handoff state must survive a fresh run.'
    $resolvePackage = Join-Path $root 'Scripts\Resolve-PackageDirectory.ps1'
    $archive = Join-Path $directory 'candidate'
    Assert-Throws { & $resolvePackage -PackageDirectory $archive } 'Missing package must fail.'
    $platform = Join-Path $archive 'Windows'
    $binaryDirectory = Join-Path $platform 'SurvivalGame\Binaries\Win64'
    $null = New-Item -ItemType Directory -Path $binaryDirectory -Force
    $null = New-Item -ItemType File -Path (Join-Path $binaryDirectory 'SurvivalGame.exe')
    Assert-True ((& $resolvePackage -PackageDirectory $archive) -eq $platform) 'Archive root should resolve its platform folder.'
    Assert-True ((& $resolvePackage -PackageDirectory $platform) -eq $platform) 'Exact package path should resolve unchanged.'
    $ambiguousBinary = Join-Path $archive 'SurvivalGame\Binaries\Win64'
    $null = New-Item -ItemType Directory -Path $ambiguousBinary -Force
    $null = New-Item -ItemType File -Path (Join-Path $ambiguousBinary 'SurvivalGame.exe')
    Assert-Throws { & $resolvePackage -PackageDirectory $archive } 'Ambiguous old/new packages must not silently select a stale executable.'
    Write-Output "Development run controls: $checks checks passed."
} finally {
    # Only this test's explicitly allocated directory is eligible for cleanup.
    Get-ChildItem -LiteralPath $directory -File -Recurse | Remove-Item -Force
    Get-ChildItem -LiteralPath $directory -Directory -Recurse |
        Sort-Object { $_.FullName.Length } -Descending | Remove-Item
    Remove-Item -LiteralPath $directory
}
