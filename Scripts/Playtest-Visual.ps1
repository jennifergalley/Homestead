[CmdletBinding()]
param([string]$EngineRoot, [ValidateRange(1280,3840)][int]$Width=1280,
    [ValidateRange(720,2160)][int]$Height=720,
    [switch]$Packaged, [string]$PackageDirectory='Build\Windows',
    [string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$output=Join-Path $root ('Saved\VisualPlaytests\' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if($OutputDirectory) { $output=[IO.Path]::GetFullPath($OutputDirectory, $root) }
if(Test-Path -LiteralPath (Join-Path $output 'telemetry.csv')) {
    throw "Use a fresh output directory; a previous visual playtest exists at $output."
}
$null=New-Item -ItemType Directory -Path $output -Force
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
