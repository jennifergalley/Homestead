[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$manifest = Get-Content (Join-Path $root 'Assets\asset-manifest.json') -Raw | ConvertFrom-Json
$receiptPath = Join-Path $root 'Assets\download-receipt.json'
$knownHashes = @{}
if (Test-Path -LiteralPath $receiptPath) {
    foreach ($entry in (Get-Content -LiteralPath $receiptPath -Raw | ConvertFrom-Json)) {
        $knownHashes["$($entry.asset)/$($entry.file)"] = $entry.sha256
    }
}
$receipt = [System.Collections.Generic.List[object]]::new()
function Add-AssetReceipt($Asset, [string]$Name, [string]$Path, [string]$Url) {
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    $key = "$($Asset.id)/$Name"
    if ($knownHashes.ContainsKey($key) -and $knownHashes[$key] -ne $hash) {
        throw "Asset content changed since the recorded download: $key. Review it before importing."
    }
    $receipt.Add([ordered]@{
        asset = $Asset.id
        file = $Name
        bytes = (Get-Item -LiteralPath $Path).Length
        sha256 = $hash
        source = $Url
        license = $Asset.license
    })
}
foreach ($asset in $manifest.assets) {
    $directory = Join-Path $root "Assets\Source\$($asset.id)"
    $null = New-Item -ItemType Directory -Path $directory -Force
    foreach ($file in $asset.files) {
        $destination = Join-Path $directory $file.name
        if (-not (Test-Path -LiteralPath $destination)) {
            Write-Host "Downloading $($asset.id)/$($file.name)"
            $temporary = "$destination.download"
            Invoke-WebRequest -Uri $file.url -OutFile $temporary
            if ((Get-Item -LiteralPath $temporary).Length -ne $file.bytes) {
                throw "Unexpected size for $($file.name); retained $temporary for inspection, not imported."
            }
            Move-Item -LiteralPath $temporary -Destination $destination
        }
        if ((Get-Item -LiteralPath $destination).Length -ne $file.bytes) {
            throw "Existing $destination does not match the verified source size."
        }
        Add-AssetReceipt $asset $file.name $destination $file.url
        if ($file.extract) {
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $archive = [IO.Compression.ZipFile]::OpenRead($destination)
            try {
                foreach ($member in $file.extract) {
                    if ([IO.Path]::GetFileName($member.name) -ne $member.name) {
                        throw "Unsafe extraction destination in asset manifest: $($member.name)"
                    }
                    $entry = $archive.GetEntry($member.member)
                    if (-not $entry) { throw "Archive does not contain required member $($member.member)." }
                    $target = Join-Path $directory $member.name
                    if (-not (Test-Path -LiteralPath $target)) {
                        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $target)
                    }
                    if ((Get-Item -LiteralPath $target).Length -ne $entry.Length) {
                        throw "Extracted source size differs from the archive: $target"
                    }
                    Add-AssetReceipt $asset $member.name $target ($file.url + '#' + $member.member)
                }
            } finally {
                $archive.Dispose()
            }
        }
    }
}
$receipt | ConvertTo-Json -Depth 4 | Set-Content $receiptPath -Encoding utf8
Write-Host "Verified $($receipt.Count) asset files. Licenses and provenance: Assets\asset-manifest.json."
