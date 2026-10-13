<#
.SYNOPSIS
Marks planner cards "On main, awaiting build": done and merged to main, not yet in a delivered build.
.DESCRIPTION
Jenny doesn't need to keep editing or prioritising an item once its work is on main. This records that in
docs\handoff\priority.json (`merged`: card id -> {sha, mergedAt, note}); the planner canvas then shows the card in its own
"Done, ships in the next build" group with a SHA/date badge, out of Next build / Build after next / unscheduled and out of
prioritisation, while the Backlog search still finds it. It is idempotent: the same card and SHA changes nothing.

Run it right after merging a lane to main (Integration), or yourself when you push dev-only work straight to main. Then commit
and push docs\handoff\priority.json (a docs-only commit may go straight to main). Scripts\Close-Delivery.ps1 clears the state
when the card ships; with -ShippedCards omitted it ships every merged card.

.PARAMETER Cards
Planner card IDs: `backlog:<inbox id>` or an OpenSpec change name. Comma- or space-separated.
.PARAMETER Sha
The merge/commit SHA on main (7-64 hex digits, or any git ref that resolves in this repo). Stored in full.
.PARAMETER Note
Short note shown on the card (e.g. "dev-only, no new build needed").
.PARAMETER MergedAt
When it reached main (ISO or local time); default now.
.PARAMETER Reopen
Clear the merged state instead (back to the normal backlog); -Sha is not needed.
.PARAMETER Root
Repo root to operate on (default: this repo); point it at a scratch copy to rehearse.
.EXAMPLE
Scripts\Mark-Merged.ps1 -Cards backlog:jenny-mv62x9e8-k3g6qz -Sha 049a7282f -Note "planner search box" -WhatIf
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][string[]]$Cards,
    [string]$Sha = '',
    [string]$Note,
    [string]$MergedAt = '',
    [switch]$Reopen,
    [string]$Root = ''
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (-not $Root) { $Root = $repo }
$Root = (Resolve-Path $Root).Path
$ids = @($Cards | ForEach-Object { $_ -split '[,\s]+' } | Where-Object { $_ })
if (-not $ids) { throw 'Give at least one card ID.' }
$dryRun = [bool]$WhatIfPreference

if (-not $Reopen) {
    if (-not $Sha) { throw 'Give -Sha (the SHA the work landed on main as).' }
    # Store the full SHA when this repo can resolve it; a scratch -Root or an unknown ref falls back to the text given.
    $full = (& git -C $repo rev-parse --verify --quiet "$Sha^{commit}" 2>$null)
    if ($LASTEXITCODE -eq 0 -and $full) {
        $Sha = $full.Trim()
        & git -C $repo merge-base --is-ancestor $Sha origin/main 2>$null
        if ($LASTEXITCODE -ne 0) { Write-Warning "$($Sha.Substring(0, 9)) is not on origin/main (yet). Push the merge so the planner's badge points at real work." }
    } elseif ($Sha -notmatch '^[0-9a-fA-F]{7,64}$') {
        throw "'$Sha' is neither a commit in this repo nor 7-64 hex digits."
    }
}

$nodeArgs = @((Join-Path $repo '.github\extensions\openspec-task-planner\mark-merged.mjs'), '--root', $Root, '--cards') + $ids
if ($Reopen) { $nodeArgs += '--clear' } else { $nodeArgs += @('--sha', $Sha) }
if ($PSBoundParameters.ContainsKey('Note')) { $nodeArgs += @('--note', $Note) }
if ($MergedAt) { $nodeArgs += @('--merged-at', ([datetime]::Parse($MergedAt)).ToUniversalTime().ToString('o')) }
if ($dryRun) { $nodeArgs += '--dry-run' }
& node @nodeArgs
if ($LASTEXITCODE -ne 0) { throw "mark-merged failed with exit code $LASTEXITCODE." }
if (-not $dryRun) { Write-Host 'Next: commit docs\handoff\priority.json (docs-only commit; push to main) so the planner on every checkout shows it.' }
