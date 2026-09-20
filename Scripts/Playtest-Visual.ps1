[CmdletBinding()]
param([string]$EngineRoot, [ValidateRange(1280,3840)][int]$Width=1280,
    [ValidateRange(720,2160)][int]$Height=720,
    [switch]$Packaged, [switch]$Watering, [switch]$Weeding, [string]$FixtureSave, [string]$PackageDirectory='Build\Windows',
    [string]$OutputDirectory)
$ErrorActionPreference='Stop'
if($Watering -and $Weeding) { throw 'Choose one ordinary action route.' }
if([bool]$Weeding -ne [bool]$FixtureSave) { throw 'Use -Weeding together with its explicit -FixtureSave.' }
$root=Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$output=Join-Path $root ('Saved\VisualPlaytests\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if($OutputDirectory) { $output=[IO.Path]::GetFullPath($OutputDirectory, $root) }
if(Test-Path -LiteralPath (Join-Path $output 'telemetry.csv')) {
    throw "Use a fresh output directory; a previous visual playtest exists at $output."
}
$null=New-Item -ItemType Directory -Path $output -Force
if($Weeding) { & (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $output }
$project=Join-Path $root 'SurvivalGame.uproject'
if($Packaged) {
    $packageRoot=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
    $executable=Join-Path $packageRoot 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
    $prefix=''
} else {
    $engine=& (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
    $executable=Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
    $prefix="`"$project`" /Game/SurvivalGame/Maps/Homestead -game "
}
if(-not (Test-Path -LiteralPath $executable)) { throw "Missing game executable: $executable" }
$log=Join-Path $output 'engine.log'
$arguments=$prefix+"-HomesteadVisualPlaytest -HomesteadTestOutput=`"$output`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height -nosound -nosplash -abslog=`"$log`""
if($Watering) { $arguments += ' -HomesteadWateringPlaytest' }
if($Weeding) { $arguments += ' -HomesteadWeedingPlaytest' }
$process=Start-Process -FilePath $executable -ArgumentList $arguments -PassThru
Write-Host "Isolated visual playtest PID=$($process.Id); output=$output"
if(-not $process.WaitForExit(600000)) {
    Stop-Process -Id $process.Id
    throw 'Visual playtest timed out; only its own process was stopped.'
}
$telemetry=Join-Path $output 'telemetry.csv'
$observations=Join-Path $output 'observations.txt'
if($process.ExitCode -ne 0 -or -not (Test-Path $telemetry) -or -not (Test-Path $observations)) {
    throw "Playtest did not finish its capture. See $log."
}
$outcome=Get-Content -LiteralPath $observations -Raw
$required=if($Weeding) {
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
Write-Host "Recorded $($rows.Count) frames. Review the frames/telemetry; completion is not visual approval."
