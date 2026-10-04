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
function Test-FParseWhitespace([char]$Character) {
    return $Character -eq ' ' -or $Character -eq "`r" -or $Character -eq "`n" -or $Character -eq "`t"
}
function Get-UserDirArguments([string]$Arguments) {
    $tokens = @()
    $inQuotes = $false
    for ($index = 0; $index -lt $Arguments.Length; ++$index) {
        if ($Arguments[$index] -eq '"') {
            $inQuotes = -not $inQuotes
            continue
        }
        $precededByAsciiAlnum = $false
        if ($index -gt 0) {
            $previousCode = [int][char]$Arguments[$index - 1]
            $precededByAsciiAlnum = ($previousCode -ge 48 -and $previousCode -le 57) -or
                ($previousCode -ge 65 -and $previousCode -le 90) -or
                ($previousCode -ge 97 -and $previousCode -le 122)
        }
        if ($inQuotes -or $index + 8 -gt $Arguments.Length -or
            $precededByAsciiAlnum -or
            -not $Arguments.Substring($index, 8).Equals('UserDir=', [StringComparison]::OrdinalIgnoreCase)) {
            continue
        }
        $valueStart = $index + 8
        $quotedValue = $valueStart -lt $Arguments.Length -and $Arguments[$valueStart] -eq '"'
        while ($valueStart -lt $Arguments.Length -and (Test-FParseWhitespace $Arguments[$valueStart])) { ++$valueStart }
        if ($valueStart -ge $Arguments.Length) { throw 'UserDir requires a value.' }
        if ($quotedValue) {
            $valueEnd = $Arguments.IndexOf('"', $valueStart + 1)
            if ($valueEnd -lt 0) { throw 'UserDir has an unterminated quoted value.' }
            $value = $Arguments.Substring($valueStart + 1, $valueEnd - $valueStart - 1)
        } else {
            $valueEnd = $valueStart
            while ($valueEnd -lt $Arguments.Length -and
                -not (Test-FParseWhitespace $Arguments[$valueEnd]) -and
                $Arguments[$valueEnd] -ne ',' -and $Arguments[$valueEnd] -ne ')') {
                ++$valueEnd
            }
            $value = $Arguments.Substring($valueStart, $valueEnd - $valueStart)
        }
        $tokens += [pscustomobject]@{ Value = $value }
    }
    return $tokens
}
function Get-ReleaseSaveRoot([string]$Package, [string]$Arguments, [string]$Role) {
    if ($Package.Replace('/', '\') -match '^(?:\\\\\?\\|\\\\\.\\)') {
        throw "$Role package path cannot use an extended/device alias."
    }
    if ($Arguments -match '(?i)(?:^|\s)"?-HomesteadPreviewProfile(?:=|\s|$)') {
        throw "$Role launch cannot use HomesteadPreviewProfile."
    }
    # Match FParse::Value("UserDir="): quoted text is ignored and competing values are ambiguous.
    $tokens = @(Get-UserDirArguments $Arguments)
    if ($tokens.Count -ne 1) { throw "$Role launch requires exactly one recognized UserDir argument." }
    $value = $tokens[0].Value
    if (-not [IO.Path]::IsPathFullyQualified($value)) { throw "$Role UserDir must be absolute." }
    if ($value.Replace('/', '\') -match '^(?:\\\\\?\\|\\\\\.\\)') { throw "$Role UserDir cannot use an extended/device alias." }
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
