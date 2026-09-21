[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [string]$FixtureSave,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(180,2700,4200)][int]$Seconds=2700,
    [switch]$FreshWorld,
    [switch]$ShippingQA,
    [switch]$CancelProbe,
    [switch]$LitFailureProbe,
    [DateTimeOffset]$LatestStartUtc = [DateTimeOffset]::MaxValue
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if($FreshWorld -or $ShippingQA){
    if(-not ($FreshWorld -and $ShippingQA) -or $FixtureSave -or $CancelProbe -or $LitFailureProbe -or
        -not $state.allowWork -or $state.completionPolicy -cne 'until-complete' -or
        [DateTimeOffset]::UtcNow -ge $LatestStartUtc){
        throw 'Fresh endurance requires Shipping QA, live completion-driven authority and no fixture/debug-mode/cancellation injection.'
    }
    $out=[IO.Path]::GetFullPath($OutputDirectory,$root)
    if(Test-Path -LiteralPath $out){throw 'Endurance requires a fresh dedicated output directory.'}
    $package=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
    if($package.configuration -cne 'Shipping'){throw 'Fresh Shipping endurance requires a real Shipping package.'}
    $null=New-Item -ItemType Directory -Path (Join-Path $out 'Graphics')
    $ini=Join-Path $out 'Graphics\GameUserSettings.ini'
    Copy-Item -LiteralPath (Join-Path $root 'Config\DefaultGameUserSettings.ini') -Destination $ini
    $control=Join-Path $root 'Automation\run.json'
    $arguments="-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadEndurance -HomesteadEnduranceFresh -HomesteadEnduranceSeconds=$Seconds -HomesteadEnduranceControl=`"$control`" -HomesteadTestOutput=`"$out`" -GameUserSettingsINI=`"$ini`" -UserDir=`"$(Join-Path $out 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=1920 -ResY=1080 -nosound -nosplash -abslog=`"$(Join-Path $out 'engine.log')`""
    [ordered]@{runId=$state.id;seconds=$Seconds;freshWorld=$true;completionDriven=$true;
        arguments=$arguments;fixture=$null;executable=$package.executable;
        executableSha256=(Get-FileHash $package.executable).Hash;
        setup='Fresh isolated world; mapped travel, gathering, eating and exact save/load; natural clock only. No state, time or lighting edits.';
        timing='Existing finite observation duration, not an external timed kill. Existing root-only lifetime guard and live run stop remain active.'} |
        ConvertTo-Json -Depth 5|Set-Content (Join-Path $out 'launch.json')
    $process=& (Join-Path $PSScriptRoot 'Invoke-ShippingQA.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $out -Arguments $arguments -CompletionDriven
    $result=Get-Content (Join-Path $out 'progress.json') -Raw|ConvertFrom-Json
    if($process.ExitCode -ne 0 -or $result.status -cne 'passed' -or -not $result.freshWorld){
        throw "Fresh endurance $($result.status): $($result.reason). See $out"
    }
    python (Join-Path $PSScriptRoot 'Analyze-Endurance.py') $out
    if($LASTEXITCODE -ne 0){throw 'Endurance evidence analysis failed.'}
    Write-Host "Fresh-world endurance passed; actual wallSeconds=$($result.wallSeconds)."
    return
}
if(-not $FixtureSave -or $Seconds -eq 4200){throw 'Legacy endurance requires its disclosed fixture and original duration.'}
if(-not $state.allowWork -or ([DateTimeOffset]$state.deadlineUtc - [DateTimeOffset]::UtcNow).TotalSeconds -lt $Seconds+180){
    throw 'Run state/deadline does not allow this complete bounded exercise.'
}
if($CancelProbe -and $Seconds -ne 180){throw 'Cancellation probe must use the short duration.'}
if($LitFailureProbe -and ($Seconds -ne 180 -or $CancelProbe)){throw 'Use the separate short Lit failure probe.'}
if([DateTimeOffset]::UtcNow -ge $LatestStartUtc){
    throw 'The explicit latest-start limit has passed; do not begin this exercise.'
}
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
if($LitFailureProbe){$args+=' -ExecCmds="viewmode shadercomplexity"'}
$process=Start-Process -FilePath $exe -WorkingDirectory $working -ArgumentList $args -PassThru
[ordered]@{pid=$process.Id;executable=$exe;executableSha256=(Get-FileHash $exe).Hash
    arguments=$args;workingDirectory=$working;startedUtc=[DateTimeOffset]::UtcNow.ToString('o');runId=$state.id
    frozenDeadline=$state.deadlineUtc;latestStartUtc=$LatestStartUtc.ToString('o');seconds=$Seconds
    cancelProbe=[bool]$CancelProbe;litFailureProbe=[bool]$LitFailureProbe
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
    $expected=if($LitFailureProbe){'failed'}elseif($CancelProbe){'cancelled'}else{'passed'}
    [ordered]@{processExitCode=$process.ExitCode;actualStatus=$result.status;expectedStatus=$expected
        litFailureProbe=[bool]$LitFailureProbe} | ConvertTo-Json | Set-Content (Join-Path $out 'process-exit.json')
    # UE's graceful Windows quit may return0 despite RequestExitWithStatus(false,1).
    $allowedExit=if($LitFailureProbe){@(0,1)}else{@(0)}
    if($process.ExitCode -notin $allowedExit -or $result.status -ne $expected -or $result.litGuardVersion -ne 1){
        throw "Endurance $($result.status), exit$($process.ExitCode): $($result.reason). See $out"
    }
    if($LitFailureProbe -and ($result.reason -ne 'Endurance requires normal Lit/lighting without ShaderComplexity.' -or
        $result.presentation.viewMode -ne 8 -or -not $result.presentation.shaderComplexity)){
        throw 'Negative probe did not observe and reject the deliberate real debug mode.'
    }
    if(-not $CancelProbe -and -not $LitFailureProbe){
        python (Join-Path $PSScriptRoot 'Analyze-Endurance.py') $out
        if($LASTEXITCODE -ne 0){throw 'Endurance evidence analysis failed.'}
    }
    Write-Host "Endurance $($result.status); actual wallSeconds=$($result.wallSeconds)."
} finally {
    if(-not $process.HasExited){Stop-Process -Id $process.Id}
    $process.Dispose()
}
