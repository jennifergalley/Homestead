[CmdletBinding()]
param([string]$SelectionFile = 'Preview.json', [AllowEmptyString()][string]$Profile)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
function Read-Object([string]$Path, [string[]]$Required) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Required preview metadata is missing: $Path" }
    $value = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json -AsHashtable
    if ($value -isnot [Collections.IDictionary]) { throw "Preview metadata must be a JSON object: $Path" }
    foreach ($key in $Required) {
        if (-not $value.Contains($key)) { throw "Missing preview metadata field '$key': $Path" }
    }
    return $value
}
function Relative-Path([string]$Base, [string]$Relative) {
    if ([string]::IsNullOrWhiteSpace($Relative) -or [IO.Path]::IsPathRooted($Relative) `
        -or $Relative -match '(^|[\\/])\.\.?([\\/]|$)|[:*?"<>|\x00-\x1f]') {
        throw "Preview paths must be ordinary relative paths without traversal: $Relative"
    }
    $path = [IO.Path]::GetFullPath($Relative, $Base)
    if (-not $path.StartsWith($Base.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Preview path escapes its known root: $Relative"
    }
    return $path
}
function Reject-Links([string]$Path) {
    $cursor = $Path
    while ($cursor -and $cursor.Length -ge $root.Length) {
        $item = Get-Item -LiteralPath $cursor -Force
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Preview selection may not follow reparse points: $cursor"
        }
        if ($cursor -eq $root) { break }
        $cursor = Split-Path $cursor -Parent
    }
}
$selectionPath = [IO.Path]::GetFullPath($SelectionFile, $root)
Reject-Links $selectionPath
$selection = Read-Object $selectionPath @('schemaVersion','candidateDirectory','profile','executableSha256')
if ($selection.schemaVersion -isnot [long] -and $selection.schemaVersion -isnot [int]) { throw 'Invalid preview selection schema type.' }
if ($selection.schemaVersion -ne 1) { throw 'Unsupported preview selection schema.' }
$chosenProfile = if ($PSBoundParameters.ContainsKey('Profile')) { $Profile } else { $selection.profile }
if ($chosenProfile -isnot [string] -or $chosenProfile -cnotmatch '^[a-z][a-z0-9-]{0,31}\z') {
    throw 'Preview profile must be 1-32 ASCII lowercase letters/digits/hyphens, starting with a letter.'
}
$archive = Relative-Path $root $selection.candidateDirectory
$releases = Join-Path $root 'Build\Releases'
if (-not $archive.StartsWith($releases + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Preview candidate must be explicitly selected inside Build\Releases, never the original Build\Windows.'
}
Reject-Links $archive
$platform = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $archive
$executable = Join-Path $platform 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
Reject-Links $executable
$cursor = $platform
while ($cursor.Length -ge $releases.Length) {
    if (Get-ChildItem -LiteralPath $cursor -File -Filter 'REJECTED*') { throw "Rejected preview candidate: $cursor" }
    if ($cursor -eq $releases) { break }
    $cursor = Split-Path $cursor -Parent
}
$receiptDirectory = if ($platform -eq $archive -and (Split-Path $archive -Leaf) -eq 'Windows') {
    Split-Path $archive -Parent
} else { $archive }
$receiptPath = Join-Path $receiptDirectory 'acceptance-receipt.json'
Reject-Links $receiptPath
$receipt = Read-Object $receiptPath @('schemaVersion','verificationStatus','previewSaveRoutingVersion',
    'executableSha256','checkpoint','privateRemoteVerified','proofIndex','proofIndexSha256')
if (($receipt.schemaVersion -isnot [long] -and $receipt.schemaVersion -isnot [int]) `
    -or ($receipt.previewSaveRoutingVersion -isnot [long] -and $receipt.previewSaveRoutingVersion -isnot [int]) `
    -or $receipt.schemaVersion -ne 2 -or $receipt.verificationStatus -cne 'passed' `
    -or $receipt.previewSaveRoutingVersion -ne 1 -or $receipt.privateRemoteVerified -isnot [bool] `
    -or -not $receipt.privateRemoteVerified -or $receipt.checkpoint -cnotmatch '^[0-9a-f]{40}\z') {
    throw 'Candidate lacks a passed, checkpointed preview-routing-v1 acceptance receipt. Old or unverified packages cannot safely use preview flags.'
}
foreach ($hash in @($selection.executableSha256, $receipt.executableSha256, $receipt.proofIndexSha256)) {
    if ($hash -isnot [string] -or $hash -notmatch '^[0-9a-fA-F]{64}\z') { throw 'Malformed SHA-256 in preview metadata.' }
}
$actual = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
if ($actual -ne $selection.executableSha256 -or $actual -ne $receipt.executableSha256) {
    throw 'Preview executable does not match both the explicit selection and acceptance receipt.'
}
$proof = Relative-Path $receiptDirectory $receipt.proofIndex
Reject-Links $proof
if ((Get-FileHash -LiteralPath $proof -Algorithm SHA256).Hash -ne $receipt.proofIndexSha256) {
    throw 'Preview acceptance proof index does not match its receipt.'
}
[pscustomobject]@{
    Executable = $executable
    WorkingDirectory = $platform
    Arguments = @("-HomesteadPreviewProfile=$chosenProfile")
    Profile = $chosenProfile
    Candidate = $selection.candidateDirectory
    Checkpoint = $receipt.checkpoint
    ExecutableSha256 = $actual
    Receipt = $receiptPath
}
