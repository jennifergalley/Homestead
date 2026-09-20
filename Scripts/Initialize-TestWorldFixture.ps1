[CmdletBinding()]
param([Parameter(Mandatory)][string]$SourceSave,
    [Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = [IO.Path]::GetFullPath($SourceSave, $root)
$testRoot = (Join-Path $root 'Saved\Automation') + '\'
if (-not $source.StartsWith($testRoot, [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw 'The disclosed fixture must be an existing dedicated save under Saved\Automation, never a player save.'
}
$bytes = [IO.File]::ReadAllBytes($source)
if ($bytes.Length -lt 16 -or [Text.Encoding]::ASCII.GetString($bytes, 0, 8) -ne 'HOMESAV1') {
    throw 'Fixture does not have the existing Homestead save envelope.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$saves = Join-Path $output 'SmokeSave'
if ((Test-Path -LiteralPath $saves) -and @(Get-ChildItem -LiteralPath $saves -Force).Count) {
    throw 'Fixture requires an empty isolated SmokeSave directory; existing saves will not be replaced.'
}
$null = New-Item -ItemType Directory -Path $saves -Force
$destination = Join-Path $saves 'Homestead_Manual.sav'
Copy-Item -LiteralPath $source -Destination $destination
$hash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $hash) {
    throw 'Copied test-world fixture hash differs from its source.'
}
[ordered]@{
    source = $source
    destination = $destination
    sha256 = $hash
    copiedUtc = [DateTime]::UtcNow.ToString('o')
    setup = 'Preexisting functional-test world, including its saved location/appearance. Its setup used test teleports and ordinary sleep. Not a fresh-start ordinary-play setup; subsequent visual approach/action uses mapped controls only.'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'fixture.json') -Encoding utf8
