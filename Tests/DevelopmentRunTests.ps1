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
    Assert-True ($run.completionPolicy -ceq 'bounded') 'Default run must remain bounded.'
    $awake=Join-Path $root 'Scripts\Hold-DevelopmentAwake.ps1'
    $awakeState=& $awake -RunId $run.id -StateDirectory $directory -ValidateOnly
    Assert-True ($awakeState.continue -and $awakeState.holding) 'Bounded active run should request system awake.'
    Assert-True (-not(Test-Path (Join-Path $directory 'awake.json'))) 'Awake validation must not create a hold/receipt.'
    Add-Type -TypeDefinition @'
using System;
using System.Threading.Tasks;
public static class RunControlFixtureRelease {
    public static async Task After(IDisposable handle) {
        await Task.Delay(250);
        handle.Dispose();
    }
}
'@
    $contended = [IO.File]::Open((Join-Path $directory 'run.lock'), 'Open', 'ReadWrite', 'None')
    $release = [RunControlFixtureRelease]::After($contended)
    $wait = [Diagnostics.Stopwatch]::StartNew()
    $afterContention = & $control -Action Status -StateDirectory $directory
    $null = $release.GetAwaiter().GetResult()
    Assert-True ($afterContention.allowWork -and $wait.ElapsedMilliseconds -ge 100) 'Status must wait for short legitimate lock contention.'
    $held = [IO.File]::Open((Join-Path $directory 'run.lock'), 'Open', 'ReadWrite', 'None')
    try {
        Assert-Throws { & $control -Action Status -StateDirectory $directory } 'Persistent lock contention must fail closed within the bound.'
    } finally { $held.Dispose() }
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
    $awakeState=& $awake -RunId $run.id -StateDirectory $directory -ValidateOnly
    Assert-True (-not $awakeState.continue -and -not $awakeState.holding) 'Bounded expired awake policy must release.'
    Assert-Throws { & $control -Action Resume -StateDirectory $directory } 'Resume cannot extend an expired run.'
    $stopped = & $control -Action Stop -Reason 'Test complete' -StateDirectory $directory
    Assert-True ($stopped.state -eq 'stopped' -and -not $stopped.allowWork) 'Stop must persist.'
    Assert-Throws { & $control -Action Resume -StateDirectory $directory } 'Stopped run cannot resume.'
    $newRun = & $control -Action Start -Hours 1 -CoordinatorSessionId 'test-coordinator' -StateDirectory $directory
    Assert-True ($newRun.id -ne $run.id) 'New run needs a unique ID.'
    Assert-True (Test-Path -LiteralPath (Join-Path $directory "history\$($run.id).json")) 'Previous handoff state must survive a fresh run.'
    $legacy=Get-Content $runPath -Raw|ConvertFrom-Json
    $legacy.PSObject.Properties.Remove('completionPolicy')
    $legacy|ConvertTo-Json|Set-Content $runPath
    Assert-True ((& $control -StateDirectory $directory).completionPolicy -ceq 'bounded') 'Historical missing policy means bounded.'
    $null=& $control -Action Stop -StateDirectory $directory
    $completion=& $control -Action Start -CompletionPolicy until-complete -CoordinatorSessionId 'test-coordinator' -StateDirectory $directory
    $unlimited=Get-Content $runPath -Raw|ConvertFrom-Json
    $unlimited.deadlineUtc=[DateTimeOffset]::UtcNow.AddDays(-1).ToString('o')
    $historicalDeadline=$unlimited.deadlineUtc
    $unlimited|ConvertTo-Json|Set-Content $runPath
    $permission=& $control -StateDirectory $directory
    Assert-True ($permission.allowWork -and -not $permission.deadlinePassed -and $permission.historicalDeadlinePassed) 'Explicit until-complete must ignore only the historical deadline.'
    $awakeState=& $awake -RunId $completion.id -StateDirectory $directory -ValidateOnly
    Assert-True ($awakeState.continue -and $awakeState.holding -and $awakeState.sleepMilliseconds -eq 15000) 'Until-complete awake must continue after historical deadline.'
    $null=& $control -Action Pause -StateDirectory $directory
    $awakeState=& $awake -RunId $completion.id -StateDirectory $directory -ValidateOnly
    Assert-True ($awakeState.continue -and -not $awakeState.holding) 'Pause must release the until-complete sleep hold.'
    $resumed=& $control -Action Resume -StateDirectory $directory
    Assert-True ($resumed.allowWork -and $resumed.deadlineUtc -eq $historicalDeadline) 'Resume must not rewrite historical deadline.'
    Assert-True (-not (& $awake -RunId 'different-run' -StateDirectory $directory -ValidateOnly).continue) 'Changed run identity must release awake.'
    Assert-Throws {& $control -Action Resume -CompletionPolicy bounded -StateDirectory $directory} 'Resume must not change completion policy.'
    $null=& $control -Action Stop -StateDirectory $directory
    Assert-True (-not (& $awake -RunId $completion.id -StateDirectory $directory -ValidateOnly).continue) 'Explicit stop must release until-complete awake.'
    Assert-Throws {& $control -Action Resume -StateDirectory $directory} 'Until-complete does not bypass explicit stop.'
    $invalid=Get-Content $runPath -Raw|ConvertFrom-Json
    $invalid.completionPolicy='unknown'
    $invalid|ConvertTo-Json|Set-Content $runPath
    Assert-Throws {& $control -StateDirectory $directory} 'Unknown explicit policy must fail closed.'
    Assert-Throws {& $awake -RunId $completion.id -StateDirectory $directory -ValidateOnly} 'Awake must fail closed on unknown policy.'
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
