[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CookDirectory,
    [Parameter(Mandatory)][string]$ArchiveDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$cook = [IO.Path]::GetFullPath($CookDirectory, $root)
$archive = [IO.Path]::GetFullPath($ArchiveDirectory, $root)
$releases = [IO.Path]::GetFullPath((Join-Path $root 'Build\Releases')) + '\'
$automation = [IO.Path]::GetFullPath((Join-Path $root 'Saved\Automation')) + '\'
if (-not $archive.StartsWith($releases, [StringComparison]::OrdinalIgnoreCase) -or
    -not $cook.StartsWith($automation, [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $archive)) {
    throw 'Fresh Build\Releases output and isolated Saved\Automation Cook input are required.'
}
$resultFile = Join-Path $cook 'character-cook-result.json'
if (-not (Test-Path -LiteralPath $resultFile -PathType Leaf)) {
    throw 'An actual guarded character Cook result is required.'
}
$result = Get-Content -LiteralPath $resultFile -Raw | ConvertFrom-Json
if ($result.status -cne 'passed' -or -not $result.cooked.passed -or
    $result.cooked.exitCode -ne 0 -or -not $result.cooked.skipZenStore -or
    $result.cooked.targetPlatform -cne 'Windows') {
    throw 'Only a verified loose Windows Cook may be staged.'
}
foreach ($pin in $result.sourcePins) {
    $source = Join-Path $root $pin.path
    if (-not (Test-Path -LiteralPath $source -PathType Leaf) -or
        (Get-FileHash -LiteralPath $source).Hash -cne $pin.sha256) {
        throw "Cook source changed before Shipping staging: $($pin.path)"
    }
}
$cookedRoot = Join-Path $cook 'Cooked'
$game = Join-Path $cookedRoot 'SurvivalGame'
$engineCooked = Join-Path $cookedRoot 'Engine'
$trialPackages = @($result.sourcePins | Where-Object {
    $_.path -like 'Content\Trials\HeroineVitruvian_20260924_25\*.uasset'
})
if ($trialPackages.Count -notin 0,12) {
    throw 'The guarded Cook has incomplete isolated face source pins.'
}
$motionPackages = @($result.sourcePins | Where-Object {
    $_.path -like 'Content\Trials\HeroineCMUWalk_20260924_03\Animations\*.uasset'
})
$headLevelPackages = @($result.sourcePins | Where-Object {
    $_.path -like 'Content\Trials\HeroineCMUWalk_20260924_04\Animations\*.uasset'
})
if ($motionPackages.Count -notin 0,2 -or ($motionPackages.Count -eq 2 -and $trialPackages.Count -ne 12)) {
    throw 'The guarded Cook has incomplete licensed motion or heroine source pins.'
}
if ($headLevelPackages.Count -notin 0,2 -or ($headLevelPackages.Count -eq 2 -and
    ($motionPackages.Count -ne 0 -or $trialPackages.Count -ne 0))) {
    throw 'The original-face upright-head motion must be complete and separate from the rejected heroine trial.'
}
$required = @(
    'Metadata\CookMetadata.ucookmeta',
    'Content\SurvivalGame\Maps\Homestead.umap',
    'Content\Trials\HeroineSprint_20260924_01\Animations\AN_Heroine_Sprint.uasset',
    'Content\Trials\HeroineKnife_20260924_01\Animations\AN_Heroine_KnifeCut.uasset',
    'Content\Trials\HeroineIdle_20260924_15\Animations\AN_Heroine_LivingIdle02.uasset'
) + @($trialPackages | ForEach-Object { $_.path }) +
    @($motionPackages | ForEach-Object { $_.path }) +
    @($headLevelPackages | ForEach-Object { $_.path })
$pins = @($required | ForEach-Object {
    $file = Join-Path $game $_
    if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or
        (Get-Item -LiteralPath $file).Length -eq 0) {
        throw "Cooked character package is missing: $_"
    }
    [ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $file).Hash}
})
if (Test-Path -LiteralPath (Join-Path $cookedRoot 'Windows\ue.projectstore')) {
    throw 'Zen cooked output cannot be staged as loose files.'
}
if (-not (Test-Path -LiteralPath $engineCooked -PathType Container)) {
    throw 'Cooked Engine assets are missing from the file-based Cook.'
}
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1')
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$project = Join-Path $root 'SurvivalGame.uproject'
$build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
& $build SurvivalGame Win64 Shipping "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
if ($LASTEXITCODE -ne 0) { throw "Genuine Shipping code build failed ($LASTEXITCODE)." }
$uat = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
$cookInput = Join-Path $archive 'CookInput'
$platformCook = Join-Path $cookInput 'Windows'
$null = New-Item -ItemType Directory -Path $platformCook -Force
Copy-Item -LiteralPath $game, $engineCooked -Destination $platformCook -Recurse
foreach ($pin in $pins) {
    if ((Get-FileHash -LiteralPath (Join-Path $platformCook "SurvivalGame\$($pin.path)")).Hash -cne $pin.sha256) {
        throw "Fresh UAT input differs from the guarded Cook: $($pin.path)"
    }
}
$stage = Join-Path $archive 'Staging'
$log = Join-Path $archive 'uat-stage.log'
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Shipping `
    -skipbuild -skipcook -skipiostore -stage -archive "-CookOutputDir=$cookInput" `
    "-stagingdirectory=$stage" "-archivedirectory=$archive" -unattended -utf8output *> $log
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $log -Tail 65
    throw "Standard loose-file UAT staging failed ($LASTEXITCODE)."
}
$package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $archive -Details
if ($package.configuration -cne 'Shipping') { throw 'Staged executable is not Shipping.' }
$containers = @(Get-ChildItem -LiteralPath $package.packageDirectory -Recurse -File |
    Where-Object { $_.Extension -in '.pak','.utoc','.ucas' })
if ($containers.Count) { throw 'Loose-file candidate unexpectedly contains cooked containers.' }
foreach ($relative in (@(
    'SurvivalGame\Content\SurvivalGame\Maps\Homestead.umap',
    'SurvivalGame\Content\Trials\HeroineSprint_20260924_01\Animations\AN_Heroine_Sprint.uasset',
    'SurvivalGame\Content\Trials\HeroineKnife_20260924_01\Animations\AN_Heroine_KnifeCut.uasset',
    'SurvivalGame\Content\Trials\HeroineIdle_20260924_15\Animations\AN_Heroine_LivingIdle02.uasset'
) + @($trialPackages | ForEach-Object { 'SurvivalGame\' + $_.path }) +
    @($motionPackages | ForEach-Object { 'SurvivalGame\' + $_.path }) +
    @($headLevelPackages | ForEach-Object { 'SurvivalGame\' + $_.path }))) {
    $staged = Join-Path $package.packageDirectory $relative
    $source = Join-Path $game ($relative -replace '^SurvivalGame\\','')
    if (-not (Test-Path -LiteralPath $staged -PathType Leaf) -or
        (Get-FileHash -LiteralPath $staged).Hash -cne (Get-FileHash -LiteralPath $source).Hash) {
        throw "Staged cooked content does not match the fresh Cook: $relative"
    }
}
$registry = @(Get-ChildItem -LiteralPath $package.packageDirectory -Recurse -File -Filter 'AssetRegistry.bin')
if (-not $registry.Count) { throw 'Staged package is missing its real runtime asset registry.' }
foreach ($pin in $pins) {
    if ((Get-FileHash -LiteralPath (Join-Path $game $pin.path)).Hash -cne $pin.sha256) {
        throw "Cook input changed during staging: $($pin.path)"
    }
}
Copy-Item -LiteralPath (Join-Path $root 'docs\asset-credits.md') `
    -Destination (Join-Path $package.packageDirectory 'asset-credits.md')
[ordered]@{
    packagedUtc=[DateTimeOffset]::UtcNow.ToString('o')
    configuration='Shipping'
    archiveDirectory=$archive
    packageDirectory=$package.packageDirectory
    executable='SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe'
    nativeExecutableSha256=(Get-FileHash -LiteralPath $package.executable).Hash
    cookResult=$resultFile
    cookedPackages=$pins
    motionGaitReview=($motionPackages.Count -eq 2 -or $headLevelPackages.Count -eq 2)
    levelHeadMotion=($headLevelPackages.Count -eq 2)
    status='Genuine fresh loose-file Cook and Shipping stage; gameplay acceptance and preview promotion are separate.'
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $archive 'build-receipt.json')
Write-Output "Fresh unselected loose-file Shipping candidate: $($package.packageDirectory)"
