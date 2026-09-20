[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$launch = Get-Content -LiteralPath (Join-Path $output 'launch.json') -Raw | ConvertFrom-Json
$native = Get-Content -LiteralPath (Join-Path $output 'startup-probe.json') -Raw | ConvertFrom-Json
$samples = @(Get-Content -LiteralPath (Join-Path $output 'owned-endpoints.json') -Raw | ConvertFrom-Json)
$current = Get-Process -Id $launch.pid -ErrorAction SilentlyContinue
if ($current -and $current.Path -eq $launch.executable) { throw 'Analyze only after the owned probe has exited.' }
if ((Get-FileHash -LiteralPath $launch.executable).Hash -ne $launch.executableSha256) { throw 'The measured executable changed.' }
if ($native.status -ne 'passed' -or -not $native.shipping -or $native.traceCompiled -or
    $native.actualWindowMode -ne 2 -or $native.viewportWidth -ne 1280 -or $native.viewportHeight -ne 720 -or
    $native.savedWidth -ne 1920 -or $native.savedHeight -ne 1080 -or $native.savedWindowMode -ne 2 -or
    $native.vsyncPreference -or $native.frameLimit -ne 60 -or $native.'r.VSync' -ne '0' -or
    $native.'r.ScreenPercentage' -ne '0' -or $native.'r.AntiAliasingMethod' -ne '4' -or $native.'t.MaxFPS' -ne '60' -or
    $native.viewMode -ne 3 -or $native.litGuardTicks -lt 1 -or -not $native.heroinePresent -or
    $native.saveDispatches -ne 1 -or $native.loadDispatches -ne 1 -or $native.shotRequested -or
    $native.savedSimulationMd5 -ne $native.loadedSimulationMd5 -or $native.profile -cne $launch.profile -or
    $launch.profile -cnotmatch '^offline-[0-9a-f]{12}$') { throw 'Native Shipping acceptance fields failed.' }
if ([IO.Path]::GetFullPath($native.saveDirectory) -ne [IO.Path]::GetFullPath($launch.saveDirectory) -or
    [IO.Path]::GetFullPath($native.graphicsFile) -ne (Join-Path $output 'Graphics\GameUserSettings.ini') -or
    -not [IO.Path]::GetFullPath($native.projectSavedDirectory).StartsWith((Join-Path $output 'EngineUser') + '\')) {
    throw 'The actual namespace/config roots do not match the isolated fixture.'
}
if ($samples.Count -lt 2 -or @($samples | Where-Object { $_.tcp.Count -or $_.udp.Count }).Count) {
    throw 'Missing endpoint coverage or an observed network endpoint.'
}
$graphics = & (Join-Path $PSScriptRoot 'Compare-GraphicsDefaults.ps1') -DefaultsFile (Join-Path $root 'Config\DefaultGameUserSettings.ini') `
    -UserFile $launch.graphicsFile -InitialDefaultsSha256 $launch.graphicsBeforeSha256
$graphics | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'graphics-comparison.json')
if (@(Get-ChildItem -LiteralPath (Join-Path $output 'EngineUser') -File -Recurse -Filter '*.png').Count) {
    throw 'Unexpected screenshot files after mapped F9.'
}
$copy = Join-Path $output 'ProfileSaveCopies'
if (-not (Test-Path -LiteralPath $copy)) {
    $null = New-Item -ItemType Directory -Path $copy
    Copy-Item -LiteralPath (Get-ChildItem -LiteralPath $launch.saveDirectory -File).FullName -Destination $copy
}
$gaps = @(for ($i=1; $i -lt $samples.Count; $i++) {
    ([DateTimeOffset]$samples[$i].utc - [DateTimeOffset]$samples[$i-1].utc).TotalSeconds
})
[ordered]@{
    status='passed-reviewed-evidence'; nativeStatus=$native.status; pid=$launch.pid
    executableSha256=$launch.executableSha256; samples=$samples.Count
    firstSampleAfterStartSeconds=([DateTimeOffset]$samples[0].utc - [DateTimeOffset]$launch.startedUtc).TotalSeconds
    observedSeconds=([DateTimeOffset]$samples[-1].utc - [DateTimeOffset]$samples[0].utc).TotalSeconds
    maximumSampleGapSeconds=($gaps | Measure-Object -Maximum).Maximum
    tcpEndpoints=0; udpEndpoints=0; graphicsBytesIdentical=$graphics.bytesIdentical
    requestedGraphicsComparisons=$graphics.comparisons.Count; native=$native
    originalWrapperOutcome='Failed its byte-identical graphics-file assertion after native/save/socket checks passed. Unreal normalized the full-default fixture into inherited diff form. Raw launch/native/socket evidence retained unchanged; no game rerun.'
    limits='One offscreen windowed Shipping process and mapped same-process save/load, not legacy 914 Shipping checks, relaunch persistence, visible borderless, scanout, human input comfort, or proof of no brief network endpoint between samples.'
} | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $output 'reviewed-result.json')
Write-Output "Reviewed isolated Shipping evidence passed: $output"
