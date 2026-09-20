param([string]$ToolsRoot = 'E:\Tools')
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source = Join-Path $root 'Assets\Characters\Source'
$downloads = Join-Path $ToolsRoot 'Downloads'
New-Item -ItemType Directory -Force $source, $downloads, "$source\Licenses", "$root\Build\CharacterPreview" | Out-Null

function Get-VerifiedArchive($Url, $Path, $Expected) {
    if (!(Test-Path $Path)) {
        & curl.exe --fail --location --retry 3 --silent --show-error --output $Path $Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $Url" }
    }
    $actual = (Get-FileHash -Algorithm SHA256 $Path).Hash.ToLowerInvariant()
    if ($actual -ne $Expected) { throw "Checksum mismatch; archive not executed: $Path" }
    Write-Host "SHA256 verified: $Path $actual"
}

# Pin the stable authoring environment. Check tools before acquiring anything.
$blenderDir = Join-Path $ToolsRoot 'blender-4.5.14-windows-x64'
$blender = Join-Path $blenderDir 'blender.exe'
$blenderBase = 'https://download.blender.org/release/Blender4.5'
if (!(Test-Path $blender)) {
    $available = Get-Command blender -ErrorAction SilentlyContinue
    if ($available) { throw "Blender is already available at $($available.Source). Review its version rather than downloading another copy." }
    $hashes = Invoke-WebRequest "$blenderBase/blender-4.5.14.sha256"
    $hashes.Content | Set-Content "$source\blender-4.5.14.sha256"
    $expected = (($hashes.Content -split "`n" | Where-Object { $_ -match 'blender-4.5.14-windows-x64.zip\s*$' }) -split '\s+')[0]
    if ($expected -ne 'b9533d2397ac1984db4466fb23a7a4649391cca93f6e84209f9bcc60d071c8b9') { throw 'Official Blender checksum changed; review required.' }
    $zip = Join-Path $downloads 'blender-4.5.14-windows-x64.zip'
    Get-VerifiedArchive "$blenderBase/blender-4.5.14-windows-x64.zip" $zip $expected
    Expand-Archive $zip $ToolsRoot -Force
}

$env:BLENDER_USER_RESOURCES = Join-Path $ToolsRoot 'BlenderCharacterUser'
$extension = Join-Path $env:BLENDER_USER_RESOURCES 'extensions\user_default\mpfb\blender_manifest.toml'
if (!(Test-Path $extension)) {
    $listing = Invoke-RestMethod 'https://extensions.blender.org/api/v1/extensions/' -Headers @{Accept='application/json'}
    $package = $listing.data | Where-Object { $_.id -eq 'mpfb' -and $_.version -eq '2.0.17' }
    if (!$package) { throw 'Pinned MPFB release is no longer listed. Use retained verified archive; do not silently upgrade.' }
    $package | ConvertTo-Json -Depth 8 | Set-Content "$source\mpfb-official-release.json"
    $expected = $package.archive_hash -replace '^sha256:', ''
    if ($expected -ne '4f0a879d64a39bf646fbf5f53601ac678855da329d650617dca5737548239a87') { throw 'Official MPFB checksum changed; review required.' }
    $zip = Join-Path $downloads 'add-on-mpfb-v2.0.17.zip'
    Get-VerifiedArchive $package.archive_url $zip $expected
    & $blender --background --factory-startup --command extension install-file --repo user_default --enable $zip
    if ($LASTEXITCODE -ne 0) { throw 'Offline MPFB install failed' }
}

$pack = Join-Path $source 'makehuman_system_assets_cc0.zip'
if (!(Test-Path "$source\SystemAssets\hair\long01\long01.mhclo")) {
    # Publisher does not list a separate checksum for this data-only asset pack.
    # This pinned digest is our recorded HTTPS acquisition digest, NOT a publisher signature.
    Get-VerifiedArchive 'https://files2.makehumancommunity.org/asset_packs/makehuman_system_assets/makehuman_system_assets_cc0.zip' $pack 'b542127a8e25547c7c29c19f2d1d2adb9a664c80396ecd694095dbc8028a0107'
    Expand-Archive $pack "$source\SystemAssets" -Force
}
foreach ($name in @('LICENSE.md','LICENSE.ASSETS.md','LICENSE.CODE.md')) {
    if (!(Test-Path "$source\Licenses\$name")) {
        Invoke-WebRequest "https://raw.githubusercontent.com/makehumancommunity/mpfb2/v2.0.17/$name" -OutFile "$source\Licenses\$name"
    }
}
Write-Host 'Offline character tools and source assets are ready. Run Scripts\Characters\build.ps1.'
