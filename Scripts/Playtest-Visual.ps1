[CmdletBinding()]
param([string]$EngineRoot, [ValidateRange(1280,3840)][int]$Width=1280,
    [ValidateRange(720,2160)][int]$Height=720,
    [switch]$Packaged, [switch]$Watering, [switch]$Weeding, [switch]$Clearing, [switch]$PresentationDiagnostics,
    [string]$FixtureSave, [string]$PackageDirectory='Build\Windows',
    [string]$OutputDirectory, [switch]$ShippingQA)
$ErrorActionPreference='Stop'
if($PresentationDiagnostics -and ($Watering -or $Weeding -or $Clearing)) { throw 'Presentation diagnostics require a separate motion route.' }
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
    $prefix="`"$project`" /Game/SurvivalGame/Maps/Homestead -game "
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
if($ShippingQA) {
    $process=& (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $packageRoot -OutputDirectory $output -Arguments $arguments
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
$required=if($PresentationDiagnostics) {
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
if($outcome.Contains('FAILED ') -or -not $outcome.Contains($required)) {
    throw "Recorded route did not meet its actual gameplay outcome. See $observations."
}
$rows=Import-Csv -LiteralPath $telemetry
if(-not $rows.Count) { throw "Visual playtest produced no telemetry: $telemetry" }
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
