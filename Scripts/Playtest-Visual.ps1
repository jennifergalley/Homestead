[CmdletBinding()]
param([string]$EngineRoot, [ValidateRange(1280,3840)][int]$Width=1280,
    [ValidateRange(720,2160)][int]$Height=720)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$engine=& (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$output=Join-Path $root ('Saved\VisualPlaytests\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$null=New-Item -ItemType Directory -Path $output -Force
$editor=Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$project=Join-Path $root 'SurvivalGame.uproject'
$log=Join-Path $output 'engine.log'
$args="`"$project`" /Game/SurvivalGame/Maps/Homestead -game -HomesteadVisualPlaytest -HomesteadTestOutput=`"$output`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=$Width -ResY=$Height -nosound -nosplash -abslog=`"$log`""
$process=Start-Process -FilePath $editor -ArgumentList $args -PassThru
Write-Host "Isolated visual playtest PID=$($process.Id); output=$output"
if(-not $process.WaitForExit(600000)) {
    Stop-Process -Id $process.Id
    throw 'Visual playtest timed out; only its own process was stopped.'
}
$telemetry=Join-Path $output 'telemetry.csv'
$observations=Join-Path $output 'observations.txt'
if(-not (Test-Path $telemetry) -or -not (Test-Path $observations)) {
    throw "Playtest did not finish its capture. See $log."
}
$rows=Import-Csv -LiteralPath $telemetry
foreach($row in $rows) {
    $frame=Join-Path $output ('Frames\frame-{0:D5}.png' -f [int]$row.frame)
    if(-not (Test-Path -LiteralPath $frame)) { throw "Missing recorded frame: $frame" }
}
Get-Content -LiteralPath $observations
Write-Host "Recorded $($rows.Count) frames. Review the frames/telemetry; completion is not visual approval."
