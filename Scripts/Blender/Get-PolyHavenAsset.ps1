[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)][string]$Id,
    [string]$Resolution = '4k',
    [ValidateSet('blend', 'hdri')][string]$Kind = 'blend'
)
# Fetches one CC0 Poly Haven asset into Assets\Source\Blender\polyhaven (git-ignored),
# verifying every file against the publisher's MD5 and writing receipt.json.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$cache = Join-Path $root 'Assets\Source\Blender\polyhaven'
$info = Invoke-RestMethod "https://api.polyhaven.com/info/$Id"
$files = Invoke-RestMethod "https://api.polyhaven.com/files/$Id"
$receipt = [ordered]@{ id = $Id; name = $info.name; authors = $info.authors; license = 'CC0-1.0'
    source = "https://polyhaven.com/a/$Id"; resolution = $Resolution; acquiredUtc = [DateTimeOffset]::UtcNow.ToString('o'); files = @() }

function Get-Verified($Url, $Path, $Md5) {
    $null = New-Item -ItemType Directory -Force (Split-Path $Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        & curl.exe --fail --location --retry 3 --silent --show-error --output $Path $Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $Url" }
    }
    if ((Get-FileHash -Algorithm MD5 -LiteralPath $Path).Hash -ne $Md5.ToUpper()) {
        Remove-Item -LiteralPath $Path
        throw "Publisher MD5 mismatch; removed $Path"
    }
    $script:receipt.files += [ordered]@{ path = $Path.Substring($cache.Length + 1); url = $Url; md5 = $Md5
        sha256 = (Get-FileHash -LiteralPath $Path).Hash }
}

if ($Kind -eq 'hdri') {
    $entry = $files.hdri.$Resolution.exr
    $target = Join-Path $cache "${Id}_$Resolution.exr"
    Get-Verified $entry.url $target $entry.md5
    $receiptPath = Join-Path $cache "${Id}_$Resolution.receipt.json"
} else {
    $entry = $files.blend.$Resolution.blend
    if (-not $entry) { throw "$Id has no $Resolution .blend download." }
    $folder = Join-Path $cache "${Id}_$Resolution"
    $target = Join-Path $folder "${Id}_$Resolution.blend"
    Get-Verified $entry.url $target $entry.md5
    foreach ($include in $entry.include.PSObject.Properties) {
        Get-Verified $include.Value.url (Join-Path $folder $include.Name) $include.Value.md5
    }
    $receiptPath = Join-Path $folder 'receipt.json'
}
$receipt | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $receiptPath -Encoding utf8
Write-Host "$($info.name) ($Resolution) verified: $target"
