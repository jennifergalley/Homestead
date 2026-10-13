<#
.SYNOPSIS
Closes a Homestead delivery in one run: usage report, planner clean-up, promotion and the builds.md card.
.DESCRIPTION
README "Playtest builds" step 4 by hand took Integration most of an evening. This does it in one command:

  1. Exports the allocated local call records (Scripts\Export-BuildUsage.py) and builds
     docs\handoff\accounting\reports\<BuildId>.json (Scripts\Report-BuildUsage.mjs). Needs the build's
     docs\handoff\accounting\<BuildId>-allocation.json, which Integration writes at the freeze.
  2. Moves the shipped cards out of the active planner: priority.json order/slots -> `removed`, their inbox
     entries -> backlog-archive.json with the receipt, and the backlog.md mirror refreshed the way the planner
     server does it.
  3. Promotes `after-next` slots to `next`.
  4. Inserts a delivered-card skeleton in builds.md (heading from -PromotedAt, Build ID, Package source SHA,
     Status: delivered, Configuration: Development, placeholder Ships bullets) that the planner parses. Re-running
     never adds a second card.

Then it loads the result with the planner's own loader and checks the build shows on the Measured build cost tab.
The planner half lives in .github\extensions\openspec-task-planner\close-delivery.mjs and reuses the planner's code.
Nothing here needs Unreal or a heavy slot. Afterwards: fill in the Ships bullets, review the `## Next build` and
`## Build after next` cards in builds.md, commit.

.PARAMETER BuildId
The delivered build, e.g. 20261011-next-01.
.PARAMETER ShippedCards
Planner card IDs that shipped: `backlog:<inbox id>` or an OpenSpec change name. Comma- or space-separated.
.PARAMETER PackageSha
Package source SHA (7-64 hex digits). Omit to write `pending`.
.PARAMETER PromotedAt
Local promotion time for the card heading: "2026-10-12 1:58 AM", "2026-10-12 13:58" or an ISO timestamp.
Default: now.
.PARAMETER Root
Repo root to operate on (default: this repo). Point it at a scratch copy to rehearse.
.PARAMETER UsageDatabase
Copilot session store with assistant_usage_events (default: ~\.copilot\session-store.db).
.PARAMETER SkipUsage
Skip step 1 (use a report that is already in place, or none).
.EXAMPLE
Scripts\Close-Delivery.ps1 -BuildId 20261011-next-01 -ShippedCards open-estate-mine-shallow-level,backlog:jenny-mut6zr63-iu7hm9 -PackageSha 6c7b1f8e6136 -PromotedAt "2026-10-12 1:58 AM" -WhatIf
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Mandatory)][ValidatePattern('^[\w-]+$')][string]$BuildId,
    [Parameter(Mandatory)][string[]]$ShippedCards,
    [string]$PackageSha = '',
    [string]$PromotedAt = '',
    [string]$Root = '',
    [string]$UsageDatabase = (Join-Path $env:USERPROFILE '.copilot\session-store.db'),
    [switch]$SkipUsage
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (-not $Root) { $Root = $repo }
$Root = (Resolve-Path $Root).Path
$handoff = Join-Path $Root 'docs\handoff'
$accounting = Join-Path $handoff 'accounting'
$allocation = Join-Path $accounting "$BuildId-allocation.json"
$usage = Join-Path $accounting "$BuildId-usage.json"
$report = Join-Path $accounting "reports\$BuildId.json"
$cards = @($ShippedCards | ForEach-Object { $_ -split '[,\s]+' } | Where-Object { $_ })
if (-not $cards) { throw 'Give at least one shipped card ID.' }
if ($PackageSha -and $PackageSha -notmatch '^[0-9a-fA-F]{7,64}$') { throw "PackageSha '$PackageSha' is not 7-64 hex digits." }
$dryRun = [bool]$WhatIfPreference

function Invoke-Tool([string]$exe, [string[]]$toolArgs) {
    & $exe @toolArgs
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE." }
}

# 1. Usage export and cost report.
if ($SkipUsage) {
    Write-Host "Step 1 skipped (-SkipUsage); report $(if (Test-Path $report) { 'is in place' } else { 'is missing' })."
} elseif ($PSCmdlet.ShouldProcess($report, 'export usage records and build the cost report')) {
    if (-not (Test-Path $allocation)) { throw "Missing $allocation. Write the build's allocation first (see the earlier *-allocation.json files), or pass -SkipUsage." }
    if (-not (Test-Path $UsageDatabase)) { throw "No usage database at $UsageDatabase. Pass -UsageDatabase or -SkipUsage." }
    Invoke-Tool python @((Join-Path $PSScriptRoot 'Export-BuildUsage.py'), '--database', $UsageDatabase, '--allocation', $allocation, '--out', $usage)
    Invoke-Tool node @((Join-Path $PSScriptRoot 'Report-BuildUsage.mjs'), '--allocation', $allocation, '--input', $usage, '--out', $report)
} else {
    Write-Host "Would export usage with $UsageDatabase and write $report (needs $allocation)."
}

# 2-4. Planner clean-up, promotion and the builds.md card.
$planner = @((Join-Path $repo '.github\extensions\openspec-task-planner\close-delivery.mjs'),
    '--root', $Root, '--build-id', $BuildId, '--shipped') + $cards
if ($PackageSha) { $planner += @('--package-sha', $PackageSha) }
if ($PromotedAt) { $planner += @('--promoted-at', $PromotedAt) }
if ($dryRun) { $planner += '--dry-run' }
Invoke-Tool node $planner
if (-not $dryRun) {
    Write-Host 'Next: replace the placeholder Ships bullets in docs\handoff\builds.md, check the Next build / Build after next cards there, then commit.'
}
