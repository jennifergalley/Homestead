[CmdletBinding()]
param(
    [ValidateSet('Start','Status','AssignWorker','Pause','Resume','Stop')]
    [string]$Action = 'Status',
    [ValidateRange(1,24)][int]$Hours = 8,
    [ValidateSet('bounded','until-complete')][string]$CompletionPolicy = 'bounded',
    [string]$CoordinatorSessionId,
    [string]$WorkerSessionId,
    [string]$Reason,
    [string]$StateDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Automation')
)
$ErrorActionPreference = 'Stop'
if($PSBoundParameters.ContainsKey('CompletionPolicy') -and $Action -ne 'Start') {
    throw 'CompletionPolicy is a Start option; existing-run policy belongs to the explicitly authorized coordinator.'
}
$null = New-Item -ItemType Directory -Path $StateDirectory -Force
$path = Join-Path $StateDirectory 'run.json'
$lock = $null
$lockWait = [Diagnostics.Stopwatch]::StartNew()
while (-not $lock) {
    try { $lock = [IO.File]::Open((Join-Path $StateDirectory 'run.lock'), 'OpenOrCreate', 'ReadWrite', 'None') }
    catch [IO.IOException] {
        if (($_.Exception.HResult -band 0xffff) -notin @(32,33) -or $lockWait.Elapsed.TotalSeconds -ge 2) { throw }
        Start-Sleep -Milliseconds 10
    }
}
try {
    $now = [DateTimeOffset]::UtcNow
    $run = if (Test-Path -LiteralPath $path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json } else { $null }
    if ($run) {
        foreach ($field in @('startedUtc','deadlineUtc','updatedUtc')) {
            $run.$field = ([DateTimeOffset]$run.$field).ToUniversalTime().ToString('o')
        }
        $policy=if($run.PSObject.Properties.Name -contains 'completionPolicy'){$run.completionPolicy}else{'bounded'}
        if($policy -cnotin @('bounded','until-complete')){throw 'Unknown completion policy; refusing work.'}
    }
    if ($Action -eq 'Start') {
        if ($run -and $run.state -ne 'stopped') { throw 'Stop the previous run explicitly before starting another.' }
        if (-not $CoordinatorSessionId) { throw 'Start requires -CoordinatorSessionId.' }
        if ($run) {
            $history = Join-Path $StateDirectory 'history'
            $null = New-Item -ItemType Directory -Path $history -Force
            Copy-Item -LiteralPath $path -Destination (Join-Path $history "$($run.id).json")
        }
        $run = [pscustomobject][ordered]@{
            schemaVersion = 1
            id = $now.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
            state = 'running'
            completionPolicy = $CompletionPolicy
            startedUtc = $now.ToString('o')
            deadlineUtc = $now.AddHours($Hours).ToString('o')
            updatedUtc = $now.ToString('o')
            coordinatorSessionId = $CoordinatorSessionId
            workerSessionId = ''
            reason = if($CompletionPolicy -eq 'until-complete'){'Explicit completion-driven autonomous run.'}else{'Bounded autonomous character-quality iteration.'}
        }
        $policy=$CompletionPolicy
    } elseif (-not $run) {
        throw 'No run exists. Start a bounded run first.'
    } elseif ($Action -eq 'AssignWorker') {
        if ($run.state -eq 'stopped') { throw 'Cannot assign a worker to a stopped run.' }
        if (-not $WorkerSessionId) { throw 'AssignWorker requires -WorkerSessionId.' }
        $run.workerSessionId = $WorkerSessionId
    } elseif ($Action -in @('Pause','Resume')) {
        if ($run.state -eq 'stopped') { throw 'A stopped run cannot be resumed; start a new bounded run.' }
        if ($policy -eq 'bounded' -and $now -ge [DateTimeOffset]::Parse($run.deadlineUtc)) { throw 'Run deadline has passed; stop and hand off.' }
        $run.state = if ($Action -eq 'Pause') { 'paused' } else { 'running' }
    } elseif ($Action -eq 'Stop') {
        $run.state = 'stopped'
    }
    if ($Action -ne 'Status') {
        $run.updatedUtc = $now.ToString('o')
        if ($Reason) { $run.reason = $Reason }
        $temporary = "$path.tmp"
        [IO.File]::WriteAllText($temporary, ($run | ConvertTo-Json -Depth 8))
        [IO.File]::Move($temporary, $path, $true)
    }
    $historicalDeadlinePassed=$now -ge [DateTimeOffset]::Parse($run.deadlineUtc)
    $deadlinePassed=$policy -eq 'bounded' -and $historicalDeadlinePassed
    $allowed = $run.state -eq 'running' -and -not $deadlinePassed
    $run | Add-Member -NotePropertyName completionPolicy -NotePropertyValue $policy -Force
    $run | Add-Member -NotePropertyName allowWork -NotePropertyValue $allowed -Force
    $run | Add-Member -NotePropertyName deadlinePassed -NotePropertyValue $deadlinePassed -Force
    $run | Add-Member -NotePropertyName historicalDeadlinePassed -NotePropertyValue $historicalDeadlinePassed -Force
    $run
} finally {
    $lock.Dispose()
}
