[CmdletBinding()]
param([string]$EngineRoot, [switch]$Force)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$source = Join-Path $root 'Assets\Characters\Heroine'
$importer = Join-Path $root 'Scripts\Characters\import_unreal.py'
$variantSource = Join-Path $root 'Assets\Characters\Variants'
$variantImporter = Join-Path $root 'Scripts\Characters\import_variants.py'
$bodySource = Join-Path $root 'Assets\Characters\BodyPresets'
$bodyImporter = Join-Path $root 'Scripts\Characters\import_body_presets.py'
$verifier = Join-Path $root 'Scripts\Characters\verify_runtime_assets.py'
$required = @('SK_Heroine_LongWave.fbx','SK_Heroine_Bob.fbx','AN_Heroine_Idle.fbx','AN_Heroine_Walk.fbx','materials.json')
foreach ($name in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $name))) {
        throw "Missing character export $name. Run Scripts\Characters\setup.ps1 and Scripts\Characters\build.ps1 first."
    }
}
$files = @($required | ForEach-Object { Get-Item -LiteralPath (Join-Path $source $_) })
$files += @(Get-ChildItem -LiteralPath (Join-Path $source 'Textures') -File)
$files += Get-Item -LiteralPath $importer
$variantNames = @('SK_Heroine_Ponytail','SK_Heroine_LongWave_Apron','SK_Heroine_Bob_Apron','SK_Heroine_Ponytail_Apron')
foreach ($name in $variantNames) {
    $file = Join-Path $variantSource "$name.fbx"
    if (-not (Test-Path -LiteralPath $file)) { throw "Missing wardrobe variant: $file. Run Scripts\Characters\extend_variants.py with the authoring toolchain." }
    $files += Get-Item -LiteralPath $file
}
$files += Get-Item -LiteralPath (Join-Path $variantSource 'materials.json'), (Join-Path $variantSource 'variant-manifest.json'), $variantImporter
$files += @(Get-ChildItem -LiteralPath (Join-Path $variantSource 'Textures') -File)
$bodyManifestPath = Join-Path $bodySource 'body-preset-manifest.json'
$bodyManifest = Get-Content -LiteralPath $bodyManifestPath -Raw | ConvertFrom-Json
$bodyNames = @()
foreach ($preset in $bodyManifest.presets.PSObject.Properties) {
    foreach ($entry in $preset.Value.exports.PSObject.Properties) {
        $bodyNames += $entry.Name
        $files += Get-Item -LiteralPath (Join-Path (Join-Path $bodySource $preset.Name) $entry.Value.fbx)
    }
}
$files += Get-Item -LiteralPath $bodyManifestPath, $bodyImporter, $verifier
$lines = foreach ($file in ($files | Sort-Object FullName)) {
    "$($file.FullName.Substring($root.Length))=$((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash)"
}
$hasher = [Security.Cryptography.SHA256]::Create()
try {
    $fingerprint = [BitConverter]::ToString($hasher.ComputeHash([Text.Encoding]::UTF8.GetBytes(($lines -join "`n")))).Replace('-','')
} finally {
    $hasher.Dispose()
}
$receipt = Join-Path $root 'Build\CharacterPreview\unreal-import-receipt.json'
$assetRoot = Join-Path $root 'Content\SurvivalGame\Characters\Heroine'
$outputAssets = @('SK_Heroine_LongWave.uasset','SK_Heroine_Bob.uasset','Animations\AN_Heroine_Idle.uasset','Animations\AN_Heroine_Walk.uasset')
$outputAssets += @($variantNames | ForEach-Object { "$_.uasset" })
$outputAssets += @($bodyNames | ForEach-Object { "$_.uasset" })
$outputsExist = @($outputAssets | Where-Object { -not (Test-Path -LiteralPath (Join-Path $assetRoot $_)) }).Count -eq 0
if (-not $Force -and $outputsExist -and (Test-Path -LiteralPath $receipt)) {
    $previous = Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    if ($previous.sourceFingerprint -eq $fingerprint -and $previous.engineRoot -eq $engine) {
        Write-Host 'Character sources and importer are unchanged; retaining existing imported assets.'
        return
    }
}
$logs = Join-Path $root 'Build\Logs'
$null = New-Item -ItemType Directory -Path $logs -Force
$log = Join-Path $logs 'character-import.log'
$report = Join-Path $root 'Build\CharacterPreview\unreal-import-report.json'
if (Test-Path -LiteralPath $report) {
    $history = Join-Path $root ('Build\CharacterPreview\ImportHistory\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    $null = New-Item -ItemType Directory -Path $history -Force
    Move-Item -LiteralPath $report -Destination (Join-Path $history 'unreal-import-report.json')
}
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
Write-Host "Importing the character assets; log: $log"
& $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$importer" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 80 | Write-Output
    throw "Unreal character import failed ($LASTEXITCODE)."
}
if (-not (Test-Path -LiteralPath $report)) { throw 'Character importer did not produce a fresh validation report.' }
$variantReport = Join-Path $root 'Build\CharacterPreview\Variants\unreal-import-report.json'
$variantLog = Join-Path $logs 'variant-import.log'
& $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$variantImporter" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $variantLog
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $variantReport)) {
    Get-Content -LiteralPath $variantLog -Tail 80 | Write-Output
    throw "Unreal wardrobe import failed ($LASTEXITCODE)."
}
$bodyReport = Join-Path $root 'Build\CharacterPreview\BodyPresets\unreal-import-report.json'
$bodyLog = Join-Path $logs 'body-preset-import.log'
& $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$bodyImporter" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $bodyLog
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $bodyReport)) {
    Get-Content -LiteralPath $bodyLog -Tail 80 | Write-Output
    throw "Unreal body-preset import failed ($LASTEXITCODE)."
}
$verificationLog = Join-Path $logs 'character-reference-verification.log'
& $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$verifier" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $verificationLog
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $verificationLog -Tail 80 | Write-Output
    throw "Persisted character references failed a fresh-process check ($LASTEXITCODE)."
}
foreach ($name in $outputAssets) {
    if (-not (Test-Path -LiteralPath (Join-Path $assetRoot $name))) { throw "Character import output is missing: $name" }
}
[ordered]@{
    sourceFingerprint = $fingerprint
    engineRoot = $engine
    importedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    report = 'Build\CharacterPreview\unreal-import-report.json'
} | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
Write-Host 'Heroine meshes and animations imported; source fingerprint recorded.'
