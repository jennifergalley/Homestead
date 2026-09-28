[CmdletBinding()]
param([string]$EngineRoot, [switch]$Packaged, [switch]$WithAudio, [switch]$FullLoop, [switch]$Presentation, [switch]$HairLength, [switch]$Gathering, [switch]$Watering, [switch]$Creek, [switch]$HeroineSequence, [switch]$UprightGait, [switch]$Crafting, [switch]$LivingIdle,
    [switch]$Weeding, [switch]$Clearing, [switch]$CameraLifecycle, [switch]$GeneratedWoodland, [string]$GeneratedResumeFrom, [switch]$Prompts, [switch]$BookClarity, [switch]$Hotbar, [switch]$NativeMenu, [switch]$DirectionalNavigation, [switch]$NativeMenuQuit, [switch]$NativeSaveRetry, [string]$NativeResumeFrom, [switch]$RequireLit, [string]$FixtureSave,
    [string]$PackageDirectory = 'Build\Windows', [string]$OutputDirectory,
    [ValidateRange(1280,7680)][int]$Width = 1920, [ValidateRange(720,4320)][int]$Height = 1080,
    [ValidateRange(50,100)][int]$RenderScale = 100,
    [ValidateRange(300,3600)][int]$TimeoutSeconds = 1200, [switch]$ShippingQA,
    [switch]$DisableChunkPreparation)
$ErrorActionPreference = 'Stop'
if ($LivingIdle -and -not $NativeMenu) {
    throw 'The living-idle proof requires the native-menu route.'
}
if ($GeneratedWoodland) {
    if (-not $ShippingQA -or $FullLoop -or $Presentation -or $HairLength -or $Gathering -or $Watering -or
        $Weeding -or $Clearing -or $CameraLifecycle -or $Prompts -or $BookClarity -or $NativeMenu -or
        $DirectionalNavigation -or $NativeMenuQuit -or $NativeResumeFrom -or $FixtureSave -or $WithAudio) {
        throw 'Generated woodland is one isolated controlled Shipping gameplay route, not another fixture combination.'
    }
    if ($DisableChunkPreparation -and -not $GeneratedWoodland) {
        throw 'Chunk-preparation baseline switch is limited to the generated woodland route.'
    }
    $RequireLit = $true
}
if ($PSBoundParameters.ContainsKey('GeneratedResumeFrom') -and
    (-not $GeneratedWoodland -or [string]::IsNullOrWhiteSpace($GeneratedResumeFrom) -or
        -not [IO.Path]::IsPathFullyQualified($GeneratedResumeFrom) -or $GeneratedResumeFrom -match '["\r\n]')) {
    throw 'Generated resume requires its explicit woodland route and a quoted-safe absolute producer directory.'
}
if ($DirectionalNavigation) {
    if ($NativeMenuQuit -or $PSBoundParameters.ContainsKey('NativeResumeFrom')) {
        throw 'Directional navigation is a separate native menu fixture, not a quit/resume route.'
    }
    $NativeMenu = $true
}
if ($NativeMenuQuit -or $NativeSaveRetry) { $NativeMenu = $true }
if ($PSBoundParameters.ContainsKey('NativeResumeFrom') -and
    (-not $NativeMenu -or $NativeMenuQuit -or $NativeSaveRetry -or [string]::IsNullOrWhiteSpace($NativeResumeFrom) -or
        -not [IO.Path]::IsPathFullyQualified($NativeResumeFrom) -or $NativeResumeFrom -match '["\r\n]')) {
    throw 'Native resume requires NativeMenu, a quoted-safe absolute producer directory, and no NativeMenuQuit.'
}
if ($NativeMenu -and ($BookClarity -or $Prompts -or $Clearing -or $Weeding -or $Gathering -or $Watering -or $Presentation -or $HairLength -or $FullLoop -or $WithAudio)) {
    throw 'Native menu checks run separately from other acceptance modes.'
}
if ($ShippingQA -and $RenderScale -ne 100) { throw 'Shipping QA preserves normal resolution policy; render-scale console overrides are not admitted.' }
if ($BookClarity -and ($Prompts -or $Clearing -or $Weeding -or $Gathering -or $Watering -or $Presentation -or $HairLength -or $FullLoop -or $WithAudio)) {
    throw 'Book clarity fixtures run separately from other acceptance modes.'
}
if ($Prompts -and ($Clearing -or $Weeding -or $Gathering -or $Watering -or $Presentation -or $HairLength -or $FullLoop -or $WithAudio)) {
    throw 'Prompt-intent fixtures run separately from other acceptance modes.'
}
if ($CameraLifecycle -and -not $Clearing) { throw 'Camera lifecycle uses the existing controlled clearing fixture.' }
if ($Hotbar -and ($NativeMenu -or $FullLoop -or $GeneratedWoodland -or $Clearing -or $Watering -or $Weeding -or $Gathering)) {
    throw 'Hotbar checks require their own isolated gameplay route.'
}
if ($Crafting -and ($NativeMenu -or $Hotbar -or $FullLoop -or $GeneratedWoodland -or $Clearing -or
    $Watering -or $Weeding -or $Gathering -or $Presentation -or $BookClarity -or $Prompts)) {
    throw 'Crafting checks require their own isolated menu route.'
}
if ($Clearing -and ($Weeding -or $Gathering -or $Watering -or $Presentation -or $HairLength -or $FullLoop -or $WithAudio)) {
    throw 'Clearing lifecycle checks run separately from other acceptance modes.'
}
if ($Weeding -and ($Gathering -or $Watering -or $Presentation -or $HairLength -or $FullLoop -or $WithAudio)) {
    throw 'Weeding lifecycle checks require their own disclosed test-world fixture run.'
}
if ([bool]$Weeding -ne [bool]$FixtureSave) { throw 'Use -Weeding together with an explicit -FixtureSave.' }
if ($HairLength -and $Presentation) { throw 'Choose either the hair-length or face-presentation fixture.' }
if ($HairLength) { $Presentation = $true }
if ($Gathering -and ($Presentation -or $FullLoop -or $WithAudio)) {
    throw 'Gathering lifecycle checks run separately from presentation, full-loop and audio acceptance.'
}
if ($Watering -and ($Gathering -or $Presentation -or $FullLoop -or $WithAudio)) {
    throw 'Watering lifecycle checks run separately from other acceptance modes.'
}
if ($Creek -and ($Watering -or $Gathering -or $Presentation -or $HairLength -or $FullLoop -or
    $WithAudio -or $Weeding -or $Clearing -or $GeneratedWoodland -or $Prompts -or $BookClarity -or $NativeMenu)) {
    throw 'Creek presentation requires its own ordinary traversal route.'
}
if ($HeroineSequence -and (-not $Creek -or ($Packaged -and -not $ShippingQA))) {
    throw 'The combined heroine sequence requires its own creek route and packaged Shipping QA admission.'
}
if ($UprightGait -and -not $HeroineSequence) {
    throw 'The upright-head movement sequence requires the combined heroine creek route.'
}
if ($Presentation -and ($FullLoop -or $WithAudio)) {
    throw 'Presentation fixtures are separate from full-loop and audio acceptance.'
}
$root = Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$project = Join-Path $root 'SurvivalGame.uproject'
$output = Join-Path $root 'Saved\Automation'
if ($Packaged) {
    $package = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
    $packageRoot = $package.packageDirectory
    $executable = $package.executable
    if (($package.configuration -eq 'Shipping') -ne [bool]$ShippingQA) { throw 'Shipping automated smoke requires explicit -ShippingQA; it is not a Development override.' }
    if (-not (Test-Path -LiteralPath $executable)) { throw 'Package the Windows game before running packaged integration tests.' }
    if ($LivingIdle -and -not (Test-Path -LiteralPath (Join-Path $packageRoot 'SurvivalGame\Content\Trials\HeroineIdle_20260924_15\Animations\AN_Heroine_LivingIdle02.uasset'))) {
        throw 'The requested full living-idle proof requires a package with the admitted five-second clip.'
    }
    if ($HeroineSequence) {
        $trialAssets=if($UprightGait) {@(
            'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUNormalWalk02.uasset',
            'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUSlowWalk02.uasset'
        )}else{@(
            'SurvivalGame\Content\Trials\HeroineVitruvian_20260924_25\SK_TrialVitruvian01_Preferred_Base_Bob.uasset',
            'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUNormalWalk01.uasset'
        )}
        foreach($relative in $trialAssets) {
            if(-not (Test-Path -LiteralPath (Join-Path $packageRoot $relative) -PathType Leaf)) {
                throw "Combined heroine Shipping QA requires the real cooked face and human walk: $relative"
            }
        }
    }
    $output = Join-Path $output 'Packaged'
    # The default map is the Estate; these suites still run on the woodland map until each is retargeted.
    $prefix = '/Game/SurvivalGame/Maps/Homestead '
} else {
    $engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $map = Join-Path $root 'Content\SurvivalGame\Maps\Homestead.umap'
    if (-not (Test-Path -LiteralPath $map)) { throw 'Build and bootstrap the game before running engine integration tests.' }
    $executable = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $offlineArguments = (& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1')) -join ' '
    $prefix = "`"$project`" /Game/SurvivalGame/Maps/Homestead -game $offlineArguments "
}
if ($OutputDirectory) { $output = [IO.Path]::GetFullPath($OutputDirectory, $root) }
if ($ShippingQA -and (-not $Packaged -or -not $OutputDirectory -or $Weeding -or (Test-Path -LiteralPath $output))) {
    throw 'Shipping QA requires a packaged route and explicit fresh output; injected weeding fixtures are not admitted.'
}
$null = New-Item -ItemType Directory -Path $output -Force
# Every run starts from an empty save sandbox: saves left by an older build can't load after an
# item or save-format change, and LoadLatest then reports "An unreadable save was skipped".
$sandboxSaves = Join-Path $output 'SmokeSave'
if (Test-Path -LiteralPath $sandboxSaves) { Remove-Item -LiteralPath $sandboxSaves -Recurse -Force }
if ($Weeding) { & (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $output }
$report = Join-Path $output 'smoke-result.txt'
$captures = @('clearing.png', 'field-book.png', 'first-foundation.png', 'heroine-long.png', 'heroine-bob.png', 'heroine-colors.png', 'heroine-ponytail.png')
if ($FullLoop) { $captures += @('garden.png', 'shelter-night.png', 'failure-retry.png') }
if ($Presentation) {
    $captures = @('face-day-front.png','face-day-angle.png','face-night-front.png','face-night-angle.png','face-day-colors.png')
}
if ($HairLength) {
    $captures = foreach ($body in @('preferred','willow','hazel')) {
        foreach ($outfit in @('tunic','apron')) {
            foreach ($view in @('back','angle')) { "hair-$body-$outfit-$view.png" }
        }
    }
}
if ($Gathering) { $captures = @('gather-reach.png','gather-recovered.png') }
if ($Watering) { $captures = @('watering-pour.png','watering-recovered.png') }
if ($Creek) { $captures = @('creek-approach.png','creek-bank.png','creek-along.png','creek-crossed.png',
    'creek-reeds.png','creek-knife-gesture.png','creek-reeds-depleted.png') }
if ($HeroineSequence) { $captures += 'heroine-sequence-clear.png' }
if ($Weeding) { $captures = @('weeding-pull.png','weeding-recovered.png') }
if ($Clearing) { $captures = @('clearing-swing.png','clearing-recovered.png') }
if ($CameraLifecycle) { $captures += 'camera-safe-sapling.png' }
if ($GeneratedWoodland) { $captures = @('generated-untouched.png','generated-cleared-site.png','generated-boundary.png','generated-reloaded.png') }
if ($GeneratedResumeFrom) { $captures = @('generated-reloaded.png') }
if ($Prompts) {
    $captures = @('prompts-book-before.png','prompts-book-after.png','prompts-book-keyboard.png')
    foreach ($surface in @('context','settings','look','planning')) {
        foreach ($device in @('gamepad','keyboard')) { $captures += "prompts-$surface-$device.png" }
    }
}
if ($BookClarity) {
    $captures = @('book-pack-tool.png','book-recipes-missing.png','book-plans.png','book-pack-food.png','book-recipes-supplied.png',
        'book-recipes-keyboard.png','book-plans-keyboard.png','book-storage-only.png','book-storage-carried.png','book-pack-empty.png')
}
if ($NativeMenu) {
    $captures = @('native-settings.png','native-exit-confirm.png','native-save-error.png',
        'native-inventory.png','native-crafting.png','native-recovery-exit.png',
        'native-storage-two-grid.png','native-storage-transactions.png','native-storage-full.png','native-test-reset.png',
        'native-world-drop.png',
        'native-build.png','native-guidebook.png','native-appearance.png',
        'native-base-only-0.png','native-base-only-1.png','native-base-only-2.png',
        'native-wardrobe-layered.png','native-wardrobe-dyed.png','native-wardrobe-restored.png')
    foreach($style in @('wave','bob-blonde')) {
        foreach($body in 0..2) {
            foreach($view in @('back','three-quarter','side')) {
                $captures += "native-hair-$style-body$body-$view.png"
            }
        }
    }
}
if ($DirectionalNavigation) {
    $captures = @('native-navigation-equipment.png','native-navigation-scrolled.png','native-navigation-drag.png')
}
if ($NativeResumeFrom) { $captures = @('native-wardrobe-resumed.png') }
if ($NativeMenuQuit) { $captures = @() }
if ($NativeSaveRetry) { $captures = @() }
if ($Hotbar) { $captures = @('hotbar-gameplay.png') }
if ($Crafting) { $captures = @('craft-requirements-ready.png','craft-hold-progress.png','craft-requirements-blocked.png') }
$frameReports = @($captures | ForEach-Object { $_ -replace '\.png$', '.frame.txt' })
$previous = (@('smoke-result.txt', 'game-audio.wav', 'game-audio.json') + $captures + $frameReports) |
    ForEach-Object { Join-Path $output $_ } |
    Where-Object { Test-Path -LiteralPath $_ }
if ($previous) {
    $history = Join-Path $output ('History\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    $null = New-Item -ItemType Directory -Path $history -Force
    foreach ($file in $previous) {
        Move-Item -LiteralPath $file -Destination (Join-Path $history (Split-Path $file -Leaf))
    }
}
$log = Join-Path $output 'engine.log'
$graphics = Join-Path $output 'Graphics\GameUserSettings.ini'
$null = New-Item -ItemType Directory -Path (Split-Path $graphics -Parent) -Force
if (-not (Test-Path -LiteralPath $graphics)) {
    Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $graphics
}
$audioArguments = if ($WithAudio) { '-HomesteadAudioProof' } else { '-nosound' }
$loopArguments = if ($FullLoop) { '-HomesteadFullLoop' } else { '' }
if ($Presentation) { $loopArguments = '-HomesteadPresentationTest' }
if ($HairLength) { $loopArguments += ' -HomesteadHairLengthTest' }
if ($Gathering) { $loopArguments = '-HomesteadGatheringTest' }
if ($Watering) { $loopArguments = '-HomesteadWateringTest' }
if ($Creek) { $loopArguments = '-HomesteadCreekTest -HomesteadRequireLit' }
if ($HeroineSequence) {
    $loopArguments += ' -HomesteadHeroineSequence'
    if($UprightGait) {
        $loopArguments += ' -HomesteadTrialCMUWalk01 -HomesteadTrialCMULevelHead'
    }else{
        $loopArguments += ' -HomesteadHeroineTrialVitruvian01'
        if($Packaged) { $loopArguments += ' -HomesteadTrialCMUWalk01' }
    }
}
if ($Weeding) { $loopArguments = '-HomesteadWeedingTest' }
if ($Clearing) { $loopArguments = '-HomesteadClearingTest' }
if ($CameraLifecycle) { $loopArguments += ' -HomesteadCameraLifecycle' }
if ($GeneratedWoodland) { $loopArguments = '-HomesteadGeneratedWoodland' }
if ($DisableChunkPreparation) { $loopArguments += ' -HomesteadDisableChunkPreparation' }
if ($GeneratedResumeFrom) { $loopArguments += " -HomesteadGeneratedResumeFrom=`"$([IO.Path]::GetFullPath($GeneratedResumeFrom))`"" }
if ($Prompts) { $loopArguments = '-HomesteadPromptTest' }
if ($BookClarity) { $loopArguments = '-HomesteadBookClarityTest' }
if ($NativeMenu) { $loopArguments = '-HomesteadNativeMenuTest -HomesteadRequireLit' }
if ($LivingIdle) { $loopArguments += ' -HomesteadIdleExtended' }
if ($NativeSaveRetry) { $loopArguments += ' -HomesteadNativeSaveRetryTest' }
if ($DirectionalNavigation) { $loopArguments += ' -HomesteadDirectionalNavigationTest' }
if ($NativeMenuQuit) { $loopArguments += ' -HomesteadNativeQuitTest' }
if ($NativeResumeFrom) { $loopArguments += " -HomesteadNativeResumeFrom=`"$([IO.Path]::GetFullPath($NativeResumeFrom))`"" }
# The estate tools are MetaHuman props (the legacy heroine only ever carried the knife), so the hotbar
# route presents them on the MetaHuman heroine.
if ($Hotbar) { $loopArguments = '-HomesteadHotbarTest -HomesteadMetaHuman -HomesteadRequireLit' }
if ($Crafting) { $loopArguments = '-HomesteadCraftingTest -HomesteadRequireLit' }
if ($RequireLit) { $loopArguments += ' -HomesteadRequireLit' }
$scaleArguments = if ($ShippingQA) { '' } else { "-ExecCmds=`"r.ScreenPercentage $RenderScale`"" }
$arguments = $prefix + "-HomesteadSmokeTest -HomesteadTestOutput=`"$output`" -GameUserSettingsINI=`"$graphics`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height $scaleArguments -nosplash $audioArguments $loopArguments -abslog=`"$log`""
if ($ShippingQA) { $arguments += ' -HomesteadShippingQA' }
if ($ShippingQA) {
    $process = & (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $packageRoot -OutputDirectory $output -Arguments $arguments -CompletionDriven:($GeneratedWoodland -or $FullLoop -or $NativeMenu)
} else {
    $process = Start-Process -FilePath $executable -ArgumentList $arguments -PassThru
    Write-Host "Engine smoke-test PID: $($process.Id). Log: $log"
}
Write-Host "Requested output: ${Width}x${Height}; 3D resolution policy: $(if($ShippingQA){'unchanged Shipping defaults'}else{$RenderScale})."
if (-not $ShippingQA -and -not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id
    throw "Engine smoke test exceeded $TimeoutSeconds seconds. Stopped only its process $($process.Id)."
}
if (-not (Test-Path -LiteralPath $report)) {
    if ($ShippingQA -and $process.ExitCode -eq 2) { throw 'Shipping QA admission rejected: explicit single route and fresh isolated output/graphics/user/save directories are required.' }
    throw "The game exited without a smoke-test report (exit $($process.ExitCode)). See $log."
}
if ($ShippingQA -and (Get-Content (Join-Path $output 'qa-admission.txt') -Raw) -notmatch 'shipping=1\r?\ntrace_compiled=0\r?\nroute=smoke') {
    throw 'Actual Shipping QA/trace-disabled admission evidence is missing.'
}
$result = Get-Content -LiteralPath $report -Raw
Write-Output $result
if ($process.ExitCode -ne 0 -or $result -notmatch '(?m)^SUCCESS ') {
    throw "Game smoke test failed (exit $($process.ExitCode)). See $output."
}
if ($NativeResumeFrom -and $result -notmatch '(?m)^NATIVE_RESUME producer_pid=[1-9]\d* consumer_pid=[1-9]\d* ') {
    throw 'Distinct-process current-save resume evidence is missing.'
}
if ($RequireLit -and $result -notmatch '(?m)^LIT_GUARD samples=[1-9]\d* final_mode=3 shader_complexity=0 ') {
    throw 'The requested sustained Lit guard did not produce successful runtime evidence.'
}
foreach ($name in $captures) {
    $image = Join-Path $output $name
    if (-not (Test-Path -LiteralPath $image) -or (Get-Item -LiteralPath $image).Length -eq 0) {
        throw "Functional checks passed but required rendered capture is missing: $name."
    }
    Add-Type -AssemblyName System.Drawing
    $decoded = [Drawing.Image]::FromFile($image)
    try {
        if ($decoded.Width -ne $Width -or $decoded.Height -ne $Height) {
            throw "Capture $name is $($decoded.Width)x$($decoded.Height), not the requested${Width}x${Height}; render verification cannot use a silently resized viewport."
        }
    } finally {
        $decoded.Dispose()
    }
}
if ($WithAudio) {
    $recording = Join-Path $output 'game-audio.wav'
    if (-not (Test-Path -LiteralPath $recording)) { throw 'No game-only audio proof was exported.' }
    python (Join-Path $PSScriptRoot 'analyze_audio.py') $recording
    if ($LASTEXITCODE -ne 0) { throw 'Game audio waveform validation failed.' }
}
if ($Presentation) {
    Write-Host "Fixed presentation fixture captured: $output. This is not ordinary-play or full-loop acceptance."
} else {
    Write-Host "Game integration smoke test passed. Captures: $output. Visual and audio review are separate."
}
