[CmdletBinding()]
param(
    [ValidateSet('Start','Status','AssignWorker','Pause','Resume','Stop')]
    [string]$Action = 'Status',
    [ValidateRange(1,24)][int]$Hours = 8,
    [string]$CoordinatorSessionId,
    [string]$WorkerSessionId,
    [string]$Reason,
    [string]$StateDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Automation')
)
$ErrorActionPreference = 'Stop'
$null = New-Item -ItemType Directory -Path $StateDirectory -Force
$path = Join-Path $StateDirectory 'run.json'
$lock = [IO.File]::Open((Join-Path $StateDirectory 'run.lock'), 'OpenOrCreate', 'ReadWrite', 'None')
try {
    $now = [DateTimeOffset]::UtcNow
    $run = if (Test-Path -LiteralPath $path) { Get-Content -LiteralPath $path -Raw | ConvertFrom-Json } else { $null }
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
            startedUtc = $now.ToString('o')
            deadlineUtc = $now.AddHours($Hours).ToString('o')
            updatedUtc = $now.ToString('o')
            coordinatorSessionId = $CoordinatorSessionId
            workerSessionId = ''
            reason = 'Bounded autonomous character-quality iteration.'
        }
    } elseif (-not $run) {
        throw 'No run exists. Start a bounded run first.'
    } elseif ($Action -eq 'AssignWorker') {
        if ($run.state -eq 'stopped') { throw 'Cannot assign a worker to a stopped run.' }
        if (-not $WorkerSessionId) { throw 'AssignWorker requires -WorkerSessionId.' }
        $run.workerSessionId = $WorkerSessionId
    } elseif ($Action -in @('Pause','Resume')) {
        if ($run.state -eq 'stopped') { throw 'A stopped run cannot be resumed; start a new bounded run.' }
        if ($now -ge [DateTimeOffset]::Parse($run.deadlineUtc)) { throw 'Run deadline has passed; stop and hand off.' }
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
    $allowed = $run.state -eq 'running' -and $now -lt [DateTimeOffset]::Parse($run.deadlineUtc)
    $run | Add-Member -NotePropertyName allowWork -NotePropertyValue $allowed -Force
    $run | Add-Member -NotePropertyName deadlinePassed -NotePropertyValue ($now -ge [DateTimeOffset]::Parse($run.deadlineUtc)) -Force
    $run
} finally {
    $lock.Dispose()
}
