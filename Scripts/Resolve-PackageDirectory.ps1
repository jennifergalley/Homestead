[CmdletBinding()]
param([string]$PackageDirectory = 'Build\Windows')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$archive = [IO.Path]::GetFullPath($PackageDirectory, $root)
$binaries = @('SurvivalGame\Binaries\Win64\SurvivalGame.exe', 'SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe')
$candidates = @($archive, (Join-Path $archive 'Windows'))
$found = @($candidates | Where-Object {
    $candidate = $_
    @($binaries | Where-Object { Test-Path -LiteralPath (Join-Path $candidate $_) }).Count -gt 0
})
if ($found.Count -eq 0) { throw "No packaged game found at $archive or its Windows subdirectory." }
if ($found.Count -gt 1) { throw "Multiple packaged games exist under $archive. Pass the exact Windows platform directory to avoid testing an older build." }
if (@($binaries | Where-Object { Test-Path -LiteralPath (Join-Path $found[0] $_) }).Count -ne 1) {
    throw 'Mixed Development/Shipping executables in one package are not supported.'
}
$found[0]
