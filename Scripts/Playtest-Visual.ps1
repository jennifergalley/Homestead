[CmdletBinding()]
param([string]$EngineRoot, [ValidateRange(1280,3840)][int]$Width=1280,
    [ValidateRange(720,2160)][int]$Height=720,
    [switch]$Packaged, [switch]$Watering, [switch]$Weeding, [switch]$Clearing, [switch]$Sprint, [switch]$LivingIdle, [switch]$TrialFootLock, [switch]$TrialCMUWalk, [switch]$TrialCMULevelHead, [switch]$CMUGaitReview, [switch]$PresentationDiagnostics,
    [string]$FixtureSave, [string]$PackageDirectory='Build\Windows',
    [string]$OutputDirectory, [switch]$ShippingQA, [switch]$CompletionDriven,
    [ValidateRange(-1,2)][int]$BodyPreset=-1, [ValidateRange(-1,2)][int]$HairStyle=-1,
    [ValidateRange(-1,4)][int]$HairColor=-1,
    [ValidateRange(-1,3)][int]$SkinTone=-1, [ValidateRange(-1,3)][int]$EyeColor=-1,
    [switch]$HeroineTrialSculpt01,
    [switch]$HeroineTrialSculpt02, [switch]$HeroineTrialBob02, [switch]$HeroineTrialVitruvian01,
    [switch]$HeroineTrialPaintedBrows)
$ErrorActionPreference='Stop'
function Assert-VisualPlaytestOutcome {
    param([string]$Outcome,[string]$Required,[switch]$RequireTree)
    if($Outcome.Contains('FAILED ') -or -not $Outcome.Contains($Required)) {
        throw 'Recorded route did not meet its actual gameplay outcome.'
    }
    $treeLines=@($Outcome -split '\r?\n'|Where-Object {$_ -like 'Tree ready=*'})
    if($RequireTree -or $treeLines.Count) {
        if($treeLines.Count -ne 1 -or
            $treeLines[0] -cne 'Tree ready=1; ordinary approach=1; actual trunk blocked walking=1; ordinary retreat=1') {
            throw 'Recorded tree readiness/approach/trunk-contact/retreat outcome failed or is missing.'
        }
    }
}
if($PresentationDiagnostics -and ($Watering -or $Weeding -or $Clearing -or $Sprint)) { throw 'Presentation diagnostics require a separate motion route.' }
if($Sprint -and ($Watering -or $Weeding -or $Clearing)){throw 'Sprint requires its own ordinary motion route.'}
if($LivingIdle -and -not $Sprint){throw 'The extended five-second idle route requires ordinary Sprint transition checks.'}
if($CMUGaitReview -and (-not $TrialCMUWalk -or $Sprint -or $PresentationDiagnostics)) {
    throw 'Slow/full human-gait review requires the isolated CMU walk without Sprint.'
}
if($TrialCMULevelHead -and (-not $TrialCMUWalk -or $HeroineTrialVitruvian01)) {
    throw 'The upright-head motion trial uses the original heroine, not the rejected face.'
}
if((@($BodyPreset,$HairStyle,$HairColor)|Where-Object {$_ -ge 0}).Count -notin 0,3) {
    throw 'Hair review requires BodyPreset, HairStyle and HairColor together.'
}
if(($SkinTone -ge 0 -or $EyeColor -ge 0) -and $BodyPreset -lt 0) {
    throw 'Skin and iris review must specify the complete body and hairstyle first.'
}
if($CompletionDriven -and (-not $ShippingQA -or $Watering -or $Weeding -or $Clearing -or $Sprint -or $PresentationDiagnostics)) {
    throw 'Completion-driven visual capture is only the ordinary isolated Shipping route.'
}
if($Watering -and $Weeding) { throw 'Choose one ordinary action route.' }
if($Clearing -and ($Watering -or $Weeding)) { throw 'Choose one ordinary action route.' }
if([bool]$Weeding -ne [bool]$FixtureSave) { throw 'Use -Weeding together with its explicit -FixtureSave.' }
$root=Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$output=Join-Path $root ('Saved\VisualPlaytests\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if($OutputDirectory) { $output=[IO.Path]::GetFullPath($OutputDirectory, $root) }
if($ShippingQA -and (-not $Packaged -or -not $OutputDirectory -or $Weeding -or (Test-Path -LiteralPath $output))) {
    throw 'Shipping QA requires a packaged route and explicit fresh output; injected weeding fixtures are not admitted.'
}
if(Test-Path -LiteralPath (Join-Path $output 'telemetry.csv')) {
    throw "Use a fresh output directory; a previous visual playtest exists at $output."
}
$null=New-Item -ItemType Directory -Path $output -Force
if($Weeding) { & (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $output }
$project=Join-Path $root 'SurvivalGame.uproject'
if($Packaged) {
    $package=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
    $packageRoot=$package.packageDirectory
    $executable=$package.executable
    if(($package.configuration -eq 'Shipping') -ne [bool]$ShippingQA){throw 'Shipping automated capture requires explicit -ShippingQA; it is not a Development override.'}
    $prefix=''
    $workingDirectory=$packageRoot
} else {
    $engine=& (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $executable=Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $offlineArguments=(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1')) -join ' '
    $prefix="`"$project`" /Game/SurvivalGame/Maps/Homestead -game $offlineArguments "
    $workingDirectory=$root
}
if(-not (Test-Path -LiteralPath $executable)) { throw "Missing game executable: $executable" }
$log=Join-Path $output 'engine.log'
$arguments=$prefix+"-HomesteadVisualPlaytest -HomesteadTestOutput=`"$output`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height -nosound -nosplash -abslog=`"$log`""
if($ShippingQA) {
    $graphics=Join-Path $output 'Graphics\GameUserSettings.ini'
    $null=New-Item -ItemType Directory -Path (Split-Path $graphics -Parent)
    Copy-Item (Join-Path $root 'Config\DefaultGameUserSettings.ini') $graphics
    $arguments+=" -HomesteadShippingQA -GameUserSettingsINI=`"$graphics`" -UserDir=`"$(Join-Path $output 'EngineUser')`""
}
if($Watering) { $arguments += ' -HomesteadWateringPlaytest' }
if($Weeding) { $arguments += ' -HomesteadWeedingPlaytest' }
if($Clearing) { $arguments += ' -HomesteadClearingPlaytest' }
if($Sprint) { $arguments += ' -HomesteadSprintPlaytest' }
if($CMUGaitReview) { $arguments += ' -HomesteadCMUGaitReview' }
if($TrialCMUWalk) {
    if(-not ($HeroineTrialVitruvian01 -or $TrialCMULevelHead)) {
        throw 'Select an explicit old-face or upright-head CMU gait trial.'
    }
    if($CMUGaitReview -and $Width -ge 2560) { $arguments += ' -HomesteadSparseGaitCapture' }
    if($Packaged) {
        $receiptPath=Join-Path (Split-Path $packageRoot -Parent) 'build-receipt.json'
        $receipt=if(Test-Path -LiteralPath $receiptPath){Get-Content -LiteralPath $receiptPath -Raw|ConvertFrom-Json}
        if($CMUGaitReview -and (-not $receipt -or -not $receipt.motionGaitReview)) {
            throw 'This Shipping candidate predates the authored slow/full gait review.'
        }
        if($TrialCMULevelHead -and (-not $receipt -or -not $receipt.levelHeadMotion -or
            -not (Test-Path -LiteralPath (Join-Path $packageRoot 'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUNormalWalk02.uasset') -PathType Leaf))) {
            throw 'This Shipping candidate does not contain the original-face upright-head CMU gait.'
        }
        if(-not ($Sprint -or $PresentationDiagnostics -or $CMUGaitReview)) {
            throw 'The CMU walk package needs a specified supported motion route.'
        }
        $walk=if($TrialCMULevelHead) {
            'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUNormalWalk02.uasset'
        } else {
            'SurvivalGame\Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUNormalWalk01.uasset'
        }
        if(-not $ShippingQA -or
            -not (Test-Path -LiteralPath (Join-Path $packageRoot $walk) -PathType Leaf)) {
            throw 'The staged walk trial requires its cooked CMU clip in a genuine Shipping QA route.'
        }
    }
    $arguments += ' -HomesteadTrialCMUWalk01'
    if($TrialCMULevelHead) { $arguments += ' -HomesteadTrialCMULevelHead' }
}
if($TrialFootLock) {
    if(-not $Sprint -or $Packaged) { throw 'The planted-stop experiment requires the isolated Editor Sprint route.' }
    $arguments += ' -HomesteadTrialFootLock'
}
if($LivingIdle) { $arguments += ' -HomesteadIdleExtended' }
if($Sprint -and $Width -ge 2560) { $arguments += ' -HomesteadSparseSprintCapture' }
if($PresentationDiagnostics) {
    $arguments += ' -HomesteadPresentationDiagnostics'
    & (Join-Path $PSScriptRoot 'Read-DisplayMode.ps1') | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath (Join-Path $output 'display-mode-before.json')
    [ordered]@{
        executable=$executable; executableSha256=(Get-FileHash -LiteralPath $executable).Hash
        arguments=$arguments; workingDirectory=$workingDirectory
        setup='Fresh test-sandbox world; ordinary mapped input, no teleport/state/time edits.'
        pipeline='Offscreen game framebuffer only; no desktop capture, physical scanout or DXGI Present tracing.'
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'diagnostic-launch.json')
}
if($BodyPreset -ge 0) {
    $arguments += " -HomesteadVisualBodyPreset=$BodyPreset -HomesteadVisualHairStyle=$HairStyle -HomesteadVisualHairColor=$HairColor"
    if($SkinTone -ge 0) { $arguments += " -HomesteadVisualSkinTone=$SkinTone" }
    if($EyeColor -ge 0) { $arguments += " -HomesteadVisualEyeColor=$EyeColor" }
}
if($HeroineTrialSculpt01) {
    if($HeroineTrialBob02 -or $HeroineTrialVitruvian01 -or $Packaged -or $BodyPreset -ne 0 -or $HairStyle -ne 1) {
        throw 'The isolated sculpt trial requires nonpackaged Preferred/Bob appearance.'
    }
    $arguments += ' -HomesteadHeroineTrialSculpt01'
}
if($HeroineTrialSculpt02) {
    if($HeroineTrialSculpt01 -or $HeroineTrialBob02 -or $HeroineTrialVitruvian01 -or $Packaged -or $BodyPreset -ne 0 -or $HairStyle -ne 1) {
        throw 'The second isolated sculpt requires nonpackaged Preferred/Bob and no competing trial.'
    }
    $arguments += ' -HomesteadHeroineTrialSculpt02'
}
if($HeroineTrialBob02) {
    if($HeroineTrialSculpt01 -or $HeroineTrialSculpt02 -or $HeroineTrialVitruvian01 -or $Packaged -or
        $BodyPreset -ne 0 -or $HairStyle -ne 1) {
        throw 'The CC0 bob02 trial requires nonpackaged Preferred/Bob and no competing trial.'
    }
    $arguments += ' -HomesteadHeroineTrialBob02'
}
if($HeroineTrialVitruvian01) {
    if($HeroineTrialSculpt01 -or $HeroineTrialSculpt02 -or $HeroineTrialBob02 -or
        $BodyPreset -notin 0,1,2 -or $HairStyle -notin 0,1,2 -or
        ($BodyPreset -ge 1 -and $HairStyle -ne 1)) {
        throw 'The licensed CC0 face trial currently fits all three Bob bodies and Preferred/LongWave/Ponytail.'
    }
    if($Packaged) {
        $trial='SurvivalGame\Content\Trials\HeroineVitruvian_20260924_25\SK_TrialVitruvian01_Preferred_Base_Bob.uasset'
        if(-not $ShippingQA -or $BodyPreset -ne 0 -or $HairStyle -ne 1 -or
            -not (Test-Path -LiteralPath (Join-Path $packageRoot $trial) -PathType Leaf)) {
            throw 'Only the cooked Preferred/Bob trial is available for staged Shipping QA.'
        }
    }
    $arguments += ' -HomesteadHeroineTrialVitruvian01'
    if(-not $Packaged -and ($BodyPreset -ne 0 -or $HairStyle -ne 1)) {
        $arguments += ' -HomesteadHeroineVariantTrials'
    }
}
if($HeroineTrialPaintedBrows) {
    if(-not $HeroineTrialVitruvian01 -or $Packaged) {
        throw 'The painted brow material trial requires only the uncooked licensed face in Editor.'
    }
    $arguments += ' -HomesteadTrialPaintedBrows'
}
if($ShippingQA) {
    $process=& (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $packageRoot -OutputDirectory $output -Arguments $arguments -CompletionDriven:$CompletionDriven
} else {
    $process=Start-Process -FilePath $executable -WorkingDirectory $workingDirectory -ArgumentList $arguments -PassThru
    Write-Host "Isolated visual playtest PID=$($process.Id); output=$output"
    if(-not $process.WaitForExit(600000)) {
        Stop-Process -Id $process.Id
        throw 'Visual playtest timed out; only its own process was stopped.'
    }
}
$telemetry=Join-Path $output 'telemetry.csv'
$observations=Join-Path $output 'observations.txt'
if($process.ExitCode -ne 0 -or -not (Test-Path $telemetry) -or -not (Test-Path $observations)) {
    if($ShippingQA -and $process.ExitCode -eq 2){throw 'Shipping QA admission rejected: explicit single route and fresh isolated output/graphics/user/save directories are required.'}
    throw "Playtest did not finish its capture. See $log."
}
if($ShippingQA -and (Get-Content (Join-Path $output 'qa-admission.txt') -Raw) -notmatch 'shipping=1\r?\ntrace_compiled=0\r?\nroute=visual') {
    throw 'Actual Shipping QA/trace-disabled admission evidence is missing.'
}
$outcome=Get-Content -LiteralPath $observations -Raw
$requireTree=$false
if($ShippingQA -and -not ($Watering -or $Weeding -or $Clearing -or $Sprint -or $PresentationDiagnostics)) {
    $buildReceipt=Get-Content (Join-Path (Split-Path $packageRoot -Parent) 'build-receipt.json') -Raw|ConvertFrom-Json
    $requireTree=$buildReceipt.PSObject.Properties.Name -contains 'treeDiagnostic' -and $buildReceipt.treeDiagnostic
}
$required=if($Sprint) {
    'Sprint mapped keyboard=1 controller=1 recovered=1 stationary_no_sprint_drain=1 menu_cancel=1 save_reload=1'
} elseif($CMUGaitReview) {
    'CMU slow/full/turn review completed=1;'
} elseif($PresentationDiagnostics) {
    'Presentation diagnostic route completed=1;'
} elseif($Clearing) {
    'Cleared actual sapling=1; action observed=1; swung hatchet observed=1; recovered and hidden=1'
} elseif($Weeding) {
    'Weeded existing planted plot=1; action observed=1; recovered to idle=1'
} elseif($Watering) {
    'Watered real planted plot=1; action observed=1; tilted tool observed=1; recovered and hidden=1'
} else {
    'Forage target reached=1; resources actually gathered=1'
}
Assert-VisualPlaytestOutcome $outcome $required -RequireTree:$requireTree
$rows=Import-Csv -LiteralPath $telemetry
if(-not $rows.Count) { throw "Visual playtest produced no telemetry: $telemetry" }
if($TrialCMUWalk -and -not $Sprint -and -not $PresentationDiagnostics) {
    $slow=@($rows|Where-Object { $_.pass -eq 'slow-walk' -and [double]$_.speed -ge 60 }|
        ForEach-Object { [double]$_.slow_weight }|Sort-Object)
    $full=@($rows|Where-Object { $_.pass -eq 'full-walk' -and [double]$_.speed -ge 150 }|
        ForEach-Object { [double]$_.slow_weight }|Sort-Object)
    $minimum=if($CMUGaitReview -and $Width -ge 2560){1}else{5}
    if($slow.Count -lt $minimum -or $full.Count -lt $minimum -or
        $slow[[int][Math]::Floor($slow.Count/2)] -le .55 -or
        $full[[int][Math]::Floor($full.Count/2)] -ge .15) {
        throw 'Natural input never selected distinct authored slow and full human walk phases.'
    }
}
Add-Type -AssemblyName System.Drawing
foreach($row in $rows) {
    $frame=Join-Path $output ('Frames\frame-{0:D5}.png' -f [int]$row.frame)
    if(-not (Test-Path -LiteralPath $frame)) { throw "Missing recorded frame: $frame" }
    $image=[Drawing.Image]::FromFile($frame)
    try {
        if($image.Width -ne $Width -or $image.Height -ne $Height) {
            throw "Recorded frame dimensions do not match ${Width}x${Height}: $frame"
        }
    } finally { $image.Dispose() }
}
Get-Content -LiteralPath $observations
if($PresentationDiagnostics) {
    & (Join-Path $PSScriptRoot 'Read-DisplayMode.ps1') | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath (Join-Path $output 'display-mode-after.json')
    python (Join-Path $PSScriptRoot 'Analyze-PresentationDiagnostics.py') $output
    if($LASTEXITCODE -ne 0) { throw 'Presentation diagnostic evidence validation failed.' }
}
Write-Host "Recorded $($rows.Count) frames. Review the frames/telemetry; completion is not visual approval."
