[CmdletBinding()]
param([string]$EngineRoot, [switch]$Packaged, [switch]$WithAudio, [switch]$FullLoop, [switch]$Presentation, [switch]$HairLength, [switch]$Gathering, [switch]$Watering,
    [switch]$Weeding, [switch]$Clearing, [switch]$CameraLifecycle, [switch]$Prompts, [switch]$BookClarity, [switch]$NativeMenu, [switch]$NativeMenuQuit, [string]$NativeResumeFrom, [switch]$RequireLit, [string]$FixtureSave,
    [string]$PackageDirectory = 'Build\Windows', [string]$OutputDirectory,
    [ValidateRange(1280,7680)][int]$Width = 1920, [ValidateRange(720,4320)][int]$Height = 1080,
    [ValidateRange(50,100)][int]$RenderScale = 100,
    [ValidateRange(300,3600)][int]$TimeoutSeconds = 1200, [switch]$ShippingQA)
$ErrorActionPreference = 'Stop'
if ($NativeMenuQuit) { $NativeMenu = $true }
if ($PSBoundParameters.ContainsKey('NativeResumeFrom') -and
    (-not $NativeMenu -or $NativeMenuQuit -or [string]::IsNullOrWhiteSpace($NativeResumeFrom) -or
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
    $output = Join-Path $output 'Packaged'
    $prefix = ''
} else {
    $engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $map = Join-Path $root 'Content\SurvivalGame\Maps\Homestead.umap'
    if (-not (Test-Path -LiteralPath $map)) { throw 'Build and bootstrap the game before running engine integration tests.' }
    $executable = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $prefix = "`"$project`" /Game/SurvivalGame/Maps/Homestead -game "
}
if ($OutputDirectory) { $output = [IO.Path]::GetFullPath($OutputDirectory, $root) }
if ($ShippingQA -and (-not $Packaged -or -not $OutputDirectory -or $Weeding -or (Test-Path -LiteralPath $output))) {
    throw 'Shipping QA requires a packaged route and explicit fresh output; injected weeding fixtures are not admitted.'
}
$null = New-Item -ItemType Directory -Path $output -Force
if ($Weeding) { & (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $output }
$report = Join-Path $output 'smoke-result.txt'
$captures = @('clearing.png', 'field-book.png', 'first-foundation.png', 'heroine-long.png', 'heroine-bob.png', 'heroine-colors.png', 'heroine-ponytail.png', 'heroine-apron.png', 'heroine-willow.png', 'heroine-hazel.png')
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
if ($Weeding) { $captures = @('weeding-pull.png','weeding-recovered.png') }
if ($Clearing) { $captures = @('clearing-swing.png','clearing-recovered.png') }
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
        'native-wardrobe-dyed.png','native-wardrobe-restored.png')
}
if ($NativeResumeFrom) { $captures = @('native-wardrobe-resumed.png') }
if ($NativeMenuQuit) { $captures = @() }
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
if ($Weeding) { $loopArguments = '-HomesteadWeedingTest' }
if ($Clearing) { $loopArguments = '-HomesteadClearingTest' }
if ($CameraLifecycle) { $loopArguments += ' -HomesteadCameraLifecycle' }
if ($Prompts) { $loopArguments = '-HomesteadPromptTest' }
if ($BookClarity) { $loopArguments = '-HomesteadBookClarityTest' }
if ($NativeMenu) { $loopArguments = '-HomesteadNativeMenuTest -HomesteadRequireLit' }
if ($NativeMenuQuit) { $loopArguments += ' -HomesteadNativeQuitTest' }
if ($NativeResumeFrom) { $loopArguments += " -HomesteadNativeResumeFrom=`"$([IO.Path]::GetFullPath($NativeResumeFrom))`"" }
if ($RequireLit) { $loopArguments += ' -HomesteadRequireLit' }
$scaleArguments = if ($ShippingQA) { '' } else { "-ExecCmds=`"r.ScreenPercentage $RenderScale`"" }
$arguments = $prefix + "-HomesteadSmokeTest -HomesteadTestOutput=`"$output`" -GameUserSettingsINI=`"$graphics`" -UserDir=`"$(Join-Path $output 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height $scaleArguments -nosplash $audioArguments $loopArguments -abslog=`"$log`""
if ($ShippingQA) { $arguments += ' -HomesteadShippingQA' }
if ($ShippingQA) {
    $process = & (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $packageRoot -OutputDirectory $output -Arguments $arguments
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
