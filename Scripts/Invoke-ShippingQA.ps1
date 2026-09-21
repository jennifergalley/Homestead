[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Arguments
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
$run=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if(-not $run.allowWork -or ($run.completionPolicy -ne 'until-complete' -and
    [DateTimeOffset]::UtcNow.AddSeconds(180) -ge [DateTimeOffset]$run.deadlineUtc)){throw 'Run does not admit the short Shipping QA route.'}
$package=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
$launch=Join-Path $output 'qa-launch.json'
if($package.configuration -cne 'Shipping' -or (Test-Path $launch) -or
    $Arguments -notmatch '(?i)(?:^|\s)-HomesteadShippingQA(?:\s|$)' -or
    $Arguments -notmatch '(?i)(?:^|\s)-RenderOffscreen(?:\s|$)' -or
    $Arguments -notmatch '(?i)(?:^|\s)-windowed(?:\s|$)' -or
    $Arguments -match '(?i)(?:^|\s)-(ExecCmds|run|HomesteadStartupProbe)[=\s]' -or
    -not $Arguments.Contains("-HomesteadTestOutput=`"$output`"")){throw 'Only an explicit fresh offscreen Shipping QA route is admitted.'}
$receiptPath=Join-Path (Split-Path $package.packageDirectory -Parent) 'build-receipt.json'
$receipt=Get-Content $receiptPath -Raw|ConvertFrom-Json
$imageHash=(Get-FileHash $package.executable).Hash
if($receipt.configuration -cne 'Shipping' -or $receipt.nativeExecutableSha256 -cne $imageHash){throw 'Genuine staged Shipping receipt differs.'}
Add-Type -Path (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')
. (Join-Path $PSScriptRoot 'AuthoringProbePolicy.ps1')
$null=[Homestead.Authoring.LeafGuard]::InspectDirectory($output)
$marker='C:\ProgramData\Epic\NotAllowedUnattendedBugReports'
$before=[Homestead.Authoring.LeafGuard]::InspectMarker($marker)
$authoringFilter="Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealPak.exe' OR Name='ShaderCompileWorker.exe' OR Name='CrashReportClientEditor.exe' OR Name='CrashReportClient.exe' OR Name='UnrealTraceServer.exe' OR Name='zenserver.exe' OR Name='UnrealInsights.exe' OR Name='VCTIP.EXE'"
if(@(Get-CimInstance Win32_Process -Filter $authoringFilter).Count){throw 'Competing authoring process prevents the bounded marker lock.'}
$environment=[Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP']=Join-Path $output 'Temp';$environment['TMP']=$environment['TEMP']
$null=New-Item -ItemType Directory -Path $environment['TEMP']
$stop=Join-Path $output 'stop-qa.txt'
$guard=$null;$failure=$null;$exitCode=$null;$dead=$false;$disposed=$false;$released=$null;$final=$null;$hard=$false
$samples=[Collections.Generic.List[object]]::new()
$jobs=[Collections.Generic.List[object]]::new()
$cleanup=[Collections.Generic.List[string]]::new()
$clock=[Diagnostics.Stopwatch]::StartNew()
function Read-QAJob([switch]$Final) {
    $job=if($Final){$guard.CaptureExitedJob()}else{$guard.ObserveJobPolicy()}
    $members=@($guard.ObserveJobMembers());$held=$guard.ObserveHeldRoot()
    $jobs.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;job=$job;members=$members;heldRoot=$held})
    Assert-AuthoringRootObservation $job $members $held $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime -Final:$Final
    return $job
}
try {
    $guard=[Homestead.Authoring.LeafGuard]::new($package.executable,$imageHash,[string[]]@(),$package.packageDirectory,
        $marker,(Join-Path $output 'qa-native.log'),$environment,$Arguments,$true)
    if(($guard.MarkerBefore|ConvertTo-Json -Compress) -cne ($before|ConvertTo-Json -Compress)){throw 'Marker identity changed before admission.'}
    $guard.ArmDeadline(100000,110000,$stop)
    $null=Read-QAJob
    $state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
    if(-not $state.allowWork -or $state.id -cne $run.id -or @(Get-CimInstance Win32_Process -Filter $authoringFilter).Count) {
        throw 'Shipping QA admission changed before first-thread resume.'
    }
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime;executableSha256=$imageHash;
        arguments=$Arguments;markerBefore=$before;job=$guard.LastVerifiedJob;creationFlags=$guard.CreationFlags;
        handles=$guard.WhitelistedHandleCount;guardSha256=(Get-FileHash (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')).Hash;
        adapterSha256=(Get-FileHash $PSCommandPath).Hash;softSeconds=100;hardSeconds=110;wrapperSeconds=120} |
        ConvertTo-Json -Depth 8|Set-Content $launch
    $guard.Resume()
    do {
        $null=Read-QAJob
        $endpoints=@([Homestead.Authoring.LeafGuard]::ObserveEndpoints([uint32[]]@($guard.ProcessId)))
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;endpoints=$endpoints})
        if($endpoints.Count){throw 'Unexpected Shipping TCP/UDP endpoint; there is no TraceControl exception.'}
        if(@(Get-CimInstance Win32_Process -Filter $authoringFilter).Count){throw 'Competing authoring/helper process appeared.'}
        $state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if(-not $state.allowWork -or $state.id -cne $run.id){throw 'Shipping QA cancelled by live run control.'}
        if($clock.Elapsed.TotalSeconds -gt 115){throw 'Shipping QA exceeded its short wrapper bound.'}
    } while(-not $guard.Wait(100))
    $exitCode=$guard.ExitCode
    $final=Read-QAJob -Final
    if($exitCode -ne 0 -or $guard.HardTerminated -or $guard.DeadlineStopRequested -or $guard.DeadlineError){
        throw "Shipping QA failed/cancelled: exit=$exitCode; deadline=$($guard.DeadlineError)"
    }
    if((Get-FileHash $package.executable).Hash -cne $imageHash){throw 'Staged executable changed during QA.'}
} catch {$failure=$_.ToString()}
finally {
    if($guard) {
        try {
            if(-not $guard.Wait(0)) {
                [IO.File]::WriteAllText($stop,'Cancelled by the owned Shipping QA supervisor.')
                if(-not $guard.Wait(3000)){$guard.HardStop(96)}
            }
            $dead=$guard.Wait(1000)
            if(-not $dead){throw 'Owned subject death was not observed; inherited marker protection is retained.'}
            $exitCode=$guard.ExitCode;$hard=$guard.HardTerminated
            $final=Read-QAJob -Final
        } catch {$cleanup.Add($_.ToString())}
        if($dead) {
            try {$null=$guard.VerifyMarker()}catch{$cleanup.Add($_.ToString())}
            try {$guard.Dispose();$disposed=$true}catch{$cleanup.Add($_.ToString())}
            try {
                $released=[Homestead.Authoring.LeafGuard]::InspectMarker($marker)
                if(($released|ConvertTo-Json -Compress) -cne ($before|ConvertTo-Json -Compress)){throw 'Released marker identity/bytes/metadata/ACL changed.'}
            } catch {$cleanup.Add($_.ToString())}
        }
    }
    if($cleanup.Count){$failure=(@($failure)+@($cleanup)|Where-Object {$_}) -join ' | '}
    $last=0.0;$gaps=@()
    foreach($sample in $samples){$gaps+=$sample.elapsedMs-$last;$last=$sample.elapsedMs}
    $gaps+=$clock.Elapsed.TotalMilliseconds-$last
    [ordered]@{status=$(if($failure){'failed'}else{'passed'});error=$failure;exitCode=$exitCode;
        elapsedSeconds=$clock.Elapsed.TotalSeconds;subjectExited=$dead;guardDisposed=$disposed;hardTerminated=$hard;
        markerBefore=$before;markerAfterRelease=$released;cleanupErrors=$cleanup;finalJob=$final;jobSamples=$jobs;
        endpointSamples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum;
        limits='One root-only short Shipping QA case. Finite process/endpoint samples, not continuous history or a filesystem/network sandbox. Runtime route/image checks remain the caller responsibility.'} |
        ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'qa-guard-result.json')
}
if($failure){throw $failure}
[pscustomobject]@{ExitCode=$exitCode;Id=$guard.ProcessId}
