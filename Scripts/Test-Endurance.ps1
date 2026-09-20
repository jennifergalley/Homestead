[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$FixtureSave,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(180,2700)][int]$Seconds=2700,
    [switch]$CancelProbe
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if(-not $state.allowWork -or ([DateTimeOffset]$state.deadlineUtc - [DateTimeOffset]::UtcNow).TotalSeconds -lt $Seconds+180){
    throw 'Run state/deadline does not allow this complete bounded exercise.'
}
if($CancelProbe -and $Seconds -ne 180){throw 'Cancellation probe must use the short duration.'}
$out=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(Test-Path -LiteralPath $out){throw 'Endurance requires a fresh dedicated output directory.'}
$working=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory
$exe=Join-Path $working 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
& (Join-Path $PSScriptRoot 'Initialize-TestWorldFixture.ps1') -SourceSave $FixtureSave -OutputDirectory $out
$null=New-Item -ItemType Directory -Path (Join-Path $out 'Graphics')
$ini=Join-Path $out 'Graphics\GameUserSettings.ini'
Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $ini
$control=Join-Path $root 'Automation\run.json'
$args="-HomesteadVisualPlaytest -HomesteadEndurance -HomesteadEnduranceSeconds=$Seconds -HomesteadEnduranceControl=`"$control`" -HomesteadTestOutput=`"$out`" -GameUserSettingsINI=`"$ini`" -UserDir=`"$(Join-Path $out 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=1920 -ResY=1080 -nosound -nosplash -abslog=`"$(Join-Path $out 'engine.log')`""
$process=Start-Process -FilePath $exe -WorkingDirectory $working -ArgumentList $args -PassThru
[ordered]@{pid=$process.Id;executable=$exe;executableSha256=(Get-FileHash $exe).Hash
    arguments=$args;workingDirectory=$working;startedUtc=[DateTimeOffset]::UtcNow.ToString('o');runId=$state.id
    frozenDeadline=$state.deadlineUtc;seconds=$Seconds;cancelProbe=[bool]$CancelProbe
    stopMarker=(Join-Path $out 'stop-endurance.txt');progress=(Join-Path $out 'progress.json')
    isolation='Mapped simulated input only; existing visual observer/sandbox. Explicit synthetic graphics INI and UserDir.'
} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $out 'launch.json')
Write-Host "Owned endurance PID=$($process.Id); progress=$out\progress.json; stop=$out\stop-endurance.txt"
try {
    if($CancelProbe) {
        if($process.WaitForExit(20000)){throw 'Cancellation probe exited before marker.'}
        'Cancellation sanity probe' | Set-Content (Join-Path $out 'stop-endurance.txt')
    }
    if(-not $process.WaitForExit(($Seconds+90)*1000)) {
        'Outer timeout: request graceful stop' | Set-Content (Join-Path $out 'stop-endurance.txt')
        if(-not $process.WaitForExit(15000)){Stop-Process -Id $process.Id}
        throw 'Owned endurance process exceeded bounded timeout; run incomplete.'
    }
    $result=Get-Content (Join-Path $out 'progress.json') -Raw | ConvertFrom-Json
    $expected=if($CancelProbe){'cancelled'}else{'passed'}
    if($process.ExitCode -ne 0 -or $result.status -ne $expected){throw "Endurance $($result.status): $($result.reason). See $out"}
    if(-not $CancelProbe){
        python (Join-Path $PSScriptRoot 'Analyze-Endurance.py') $out
        if($LASTEXITCODE -ne 0){throw 'Endurance evidence analysis failed.'}
    }
    Write-Host "Endurance $($result.status); actual wallSeconds=$($result.wallSeconds)."
} finally {
    if(-not $process.HasExited){Stop-Process -Id $process.Id}
    $process.Dispose()
}
