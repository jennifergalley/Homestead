[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$FixtureSave,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet('none','graphics','overlap')][string]$GuardCase='none',
    [switch]$CancelProbe
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$out=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(Test-Path -LiteralPath $out){throw 'Use a fresh isolated renewal output directory.'}
if($CancelProbe -and $GuardCase -ne 'none'){throw 'Cancellation and guard probes are separate cases.'}
$state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if(-not $state.allowWork -or ([DateTimeOffset]$state.deadlineUtc - [DateTimeOffset]::UtcNow).TotalSeconds -lt 1020){throw 'Run state/deadline cannot admit bounded renewal.'}
$working=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
$exe=Join-Path $working 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
& (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $out
$null=New-Item -ItemType Directory -Path (Join-Path $out 'Graphics')
$ini=Join-Path $out 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $ini
$phases=if($CancelProbe -or $GuardCase -ne 'none'){@('write')}else{@('write','reload')}
$saveHashes=@{}
foreach($phase in $phases) {
    $state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
    if(-not $state.allowWork){throw 'Run stopped before next renewal process; incomplete.'}
    $flags=if($phase -eq 'reload'){'-HomesteadRenewalReadOnly'}elseif($GuardCase -eq 'overlap'){'-HomesteadEndurance'}else{''}
    $config=$ini
    if($GuardCase -eq 'graphics'){
        $config=Join-Path $out 'Graphics\deliberately-wrong.ini'
        Copy-Item -LiteralPath $ini -Destination $config
    }
    $args="-HomesteadVisualPlaytest -HomesteadForageRenewal $flags -HomesteadRenewalControl=`"$(Join-Path $root 'Automation\run.json')`" -HomesteadTestOutput=`"$out`" -GameUserSettingsINI=`"$config`" -UserDir=`"$(Join-Path $out 'EngineUser')`" -RenderOffscreen -windowed -ForceRes -ResX=1920 -ResY=1080 -nosound -unattended -nosplash -abslog=`"$(Join-Path $out "$phase.log")`""
    $process=Start-Process -FilePath $exe -WorkingDirectory $working -ArgumentList $args -PassThru
    @{pid=$process.Id;phase=$phase;arguments=$args;workingDirectory=$working;exeSha256=(Get-FileHash $exe).Hash
        startedUtc=[DateTimeOffset]::UtcNow.ToString('o');stopMarker=(Join-Path $out 'stop-renewal.txt')}|
        ConvertTo-Json|Set-Content (Join-Path $out "$phase-launch.json")
    Write-Host "Owned renewal $phase PID=$($process.Id); output=$out"
    try {
        if($CancelProbe) {
            if($process.WaitForExit(20000)){throw 'Probe exited before cancellation marker.'}
            'Explicit cancellation probe'|Set-Content (Join-Path $out 'stop-renewal.txt')
        }
        if(-not $process.WaitForExit(960000)) {
            'Outer timeout; incomplete'|Set-Content (Join-Path $out 'stop-renewal.txt')
            if(-not $process.WaitForExit(15000)){Stop-Process -Id $process.Id}
            throw 'Bounded renewal timeout.'
        }
        $result=Get-Content (Join-Path $out "$phase-result.json") -Raw|ConvertFrom-Json
        $expected=if($CancelProbe){'cancelled'}elseif($GuardCase -ne 'none'){'failed'}else{'passed'}
        if($result.status -ne $expected){throw "Renewal $($result.status):$($result.reason)"}
        if($GuardCase -eq 'graphics' -and $result.reason -notlike 'Graphics configuration*'){throw 'Wrong graphics guard failure.'}
        if($GuardCase -eq 'overlap' -and $result.reason -notlike 'Overlapping automation*'){throw 'Wrong overlap guard failure.'}
        if($expected -eq 'passed' -and $process.ExitCode -ne 0){throw 'Unexpected native process exit.'}
    } finally {
        if(-not $process.HasExited){Stop-Process -Id $process.Id}
        $process.Dispose()
    }
    if($phase -eq 'write'){
        foreach($file in Get-ChildItem (Join-Path $out 'SmokeSave') -File){$saveHashes[$file.FullName]=(Get-FileHash $file.FullName).Hash}
    }else{
        foreach($path in $saveHashes.Keys){if((Get-FileHash $path).Hash -ne $saveHashes[$path]){throw 'Read-only relaunch changed test saves.'}}
        if((Get-ChildItem (Join-Path $out 'SmokeSave') -File).Count -ne $saveHashes.Count){throw 'Read-only relaunch created a save.'}
    }
    Write-Host "$phase result=$($result.status); checks=$($result.checks); sleeps=$($result.sleeps); harvests=$($result.harvests)"
}
@{status=if($CancelProbe){'cancelled'}elseif($GuardCase -ne 'none'){'guard-rejected'}else{'passed'}
    guard=$GuardCase;separateProcessReload=$phases.Count -eq 2;unchangedRelaunchSaveFiles=$saveHashes.Count
    graphicsDestination=$ini;saveSha256=$saveHashes;fixture=(Get-Content (Join-Path $out 'fixture.json') -Raw|ConvertFrom-Json)}|
    ConvertTo-Json -Depth 6|Set-Content (Join-Path $out 'renewal-result.json')
