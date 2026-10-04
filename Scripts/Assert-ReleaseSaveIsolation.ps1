[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CandidatePackage,
    [Parameter(Mandatory)][string]$RollbackPackage,
    [Parameter(Mandatory)][string]$CandidateArguments,
    [Parameter(Mandatory)][string]$RollbackArguments
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path $PSScriptRoot -Parent
function Assert-OrdinaryPath([string]$Path) {
    for ($item = Get-Item -LiteralPath $Path -Force; $null -ne $item; $item = $item.Parent) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Release save routing cannot use a reparse point: $($item.FullName)"
        }
        if ($item.FullName -eq [IO.Path]::GetPathRoot($item.FullName)) { break }
    }
}
function Get-ReleaseSaveRoot([string]$Package, [string]$Arguments, [string]$Role) {
    if ($Arguments -match '(?i)(?:^|\s)-HomesteadPreviewProfile(?:=|\s|$)') {
        throw "$Role launch cannot use HomesteadPreviewProfile."
    }
    $matches = [regex]::Matches($Arguments, '(?i)(?:^|\s)-UserDir=(?:"([^"]+)"|([^\s]+))')
    if ($matches.Count -ne 1) { throw "$Role launch requires exactly one explicit -UserDir." }
    $value = if ($matches[0].Groups[1].Success) { $matches[0].Groups[1].Value } else { $matches[0].Groups[2].Value }
    if (-not [IO.Path]::IsPathFullyQualified($value)) { throw "$Role UserDir must be absolute." }
    $platform = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $Package
    $expected = [IO.Path]::GetFullPath((Join-Path $platform 'SurvivalGame'))
    $actual = [IO.Path]::GetFullPath($value)
    if (-not [string]::Equals($actual, $expected, [StringComparison]::OrdinalIgnoreCase)) {
        throw "$Role UserDir must be the exact package-local SurvivalGame directory."
    }
    $saveRoot = [IO.Path]::GetFullPath((Join-Path $actual 'Saved\SaveGames'))
    if (-not (Test-Path -LiteralPath $saveRoot -PathType Container)) {
        throw "$Role SaveGames root is missing before launch: $saveRoot"
    }
    Assert-OrdinaryPath $saveRoot
    return [pscustomobject]@{ Package = $platform; UserDir = $actual; SaveRoot = $saveRoot }
}

$candidate = Get-ReleaseSaveRoot $CandidatePackage $CandidateArguments 'Candidate'
$rollback = Get-ReleaseSaveRoot $RollbackPackage $RollbackArguments 'Rollback'
if ([string]::Equals($candidate.SaveRoot, $rollback.SaveRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Candidate and rollback SaveGames roots must be physically distinct.'
}
[pscustomobject]@{ Candidate = $candidate; Rollback = $rollback }
