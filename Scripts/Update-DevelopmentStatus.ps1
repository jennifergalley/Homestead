[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RunId,
    [Parameter(Mandatory)][string]$Task,
    [Parameter(Mandatory)][string]$Phase,
    [Parameter(Mandatory)][string]$Summary,
    [string[]]$Evidence = @(),
    [string]$Blocker = 'None',
    [string]$Candidate = '',
    [string]$StateDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Automation'),
    [string]$StatusPath = (Join-Path (Split-Path $PSScriptRoot -Parent) 'docs\development-status.md')
)
$ErrorActionPreference = 'Stop'
$run = Get-Content -LiteralPath (Join-Path $StateDirectory 'run.json') -Raw | ConvertFrom-Json
if ($run.id -ne $RunId) { throw 'Status update belongs to a different run.' }
$deadline = ([DateTimeOffset]$run.deadlineUtc).ToUniversalTime().ToString('o')
$now = [DateTimeOffset]::UtcNow.ToString('o')
$status = [ordered]@{
    runId = $RunId; updatedUtc = $now; task = $Task; phase = $Phase
    summary = $Summary; evidence = $Evidence; blocker = $Blocker; candidate = $Candidate
}
$jsonPath = Join-Path $StateDirectory 'status.json'
[IO.File]::WriteAllText("$jsonPath.tmp", ($status | ConvertTo-Json -Depth 8))
[IO.File]::Move("$jsonPath.tmp", $jsonPath, $true)
$proof = if ($Evidence.Count) { ($Evidence | ForEach-Object { "- $_" }) -join "`n" } else { 'No new verification claimed.' }
$markdown = @"
# Development status

**Task:** $Task

**Phase:** $Phase | **Updated (UTC):** $now

$Summary

## Evidence

$proof

**Blocker:** $Blocker

**Candidate:** $Candidate

Run: $RunId. Deadline (UTC): $deadline.
Worker session: $($run.workerSessionId).
This is the latest worker report, not a live process monitor.
Control state and permission to continue: Scripts\Development-Run.ps1 -Action Status.
The original Build\Windows player build is not automatically replaced.
"@
[IO.File]::WriteAllText("$StatusPath.tmp", $markdown)
[IO.File]::Move("$StatusPath.tmp", $StatusPath, $true)
$status
