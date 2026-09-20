[CmdletBinding()]
param([string]$PackageDirectory = 'Build\Windows')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$archive = [IO.Path]::GetFullPath($PackageDirectory, $root)
$binary = 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
$candidates = @($archive, (Join-Path $archive 'Windows'))
$found = @($candidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ $binary) })
if ($found.Count -eq 0) { throw "No packaged game found at $archive or its Windows subdirectory." }
if ($found.Count -gt 1) { throw "Multiple packaged games exist under $archive. Pass the exact Windows platform directory to avoid testing an older build." }
$found[0]
