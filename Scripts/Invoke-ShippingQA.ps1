[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$Arguments,
    [switch]$CompletionDriven
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
function Assert-QACompletionRoute([string]$CommandLine,[bool]$Completion,[string]$Policy) {
    if(-not $Completion){return}
    if($Policy -cne 'until-complete'){throw 'Completion-driven Shipping QA requires live until-complete authority.'}
    if($CommandLine -match '(?i)(?:^|\s)-HomesteadFullLoop(?=\s|$)'){
        foreach($flag in 'HomesteadFullLoop','HomesteadSmokeTest','HomesteadShippingQA'){
            if([regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=\s|$)").Count -ne 1){
                throw "Full-loop QA requires one explicit $flag."
            }
        }
        if($CommandLine -match '(?i)(?:^|\s)-Homestead(VisualPlaytest|Endurance|GeneratedWoodland|NativeMenuTest|ClearingTest|PresentationTest|ForageRenewal)(?:\s|$)'){
            throw 'Full-loop QA cannot mix another acceptance route.'
        }
        return
    }
    if($CommandLine -match '(?i)(?:^|\s)-HomesteadGeneratedWoodland(?=\s|$)'){
        foreach($flag in 'HomesteadGeneratedWoodland','HomesteadSmokeTest','HomesteadShippingQA'){
            if([regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=\s|$)").Count -ne 1){
                throw "Generated woodland QA requires one explicit $flag."
            }
        }
        if($CommandLine -match '(?i)(?:^|\s)-Homestead(VisualPlaytest|Endurance|NativeMenuTest|FullLoop|ClearingTest|PresentationTest|ForageRenewal)(?:\s|$)'){
            throw 'Generated woodland QA cannot mix another acceptance route.'
        }
        return
    }
    if($CommandLine -match '(?i)(?:^|\s)-HomesteadNativeMenuTest(?=\s|$)'){
        foreach($flag in 'HomesteadNativeMenuTest','HomesteadSmokeTest','HomesteadShippingQA'){
            if([regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=\s|$)").Count -ne 1){
                throw "Native-menu QA requires one explicit $flag."
            }
        }
        if($CommandLine -match '(?i)(?:^|\s)-Homestead(VisualPlaytest|Endurance|GeneratedWoodland|FullLoop|ClearingTest|PresentationTest|ForageRenewal)(?:\s|$)'){
            throw 'Native-menu QA cannot mix another acceptance route.'
        }
        return
    }
    if($CommandLine -notmatch '(?i)(?:^|\s)-HomesteadEndurance(?=\s|$)'){
        foreach($flag in 'HomesteadVisualPlaytest','HomesteadShippingQA'){
            if([regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=\s|$)").Count -ne 1){
                throw "Ordinary completion-driven capture requires one explicit $flag."
            }
        }
        if($CommandLine -match '(?i)(?:^|\s)-Homestead(SmokeTest|EnduranceFresh|ForageRenewal|NativeMenuTest|NativeQuitTest|FullLoop|ClearingTest|PresentationTest|WateringPlaytest|WeedingPlaytest|ClearingPlaytest|PresentationDiagnostics)(?:\s|$)'){
            throw 'Ordinary completion-driven capture cannot mix another route.'
        }
        return
    }
    foreach($flag in 'HomesteadEndurance','HomesteadEnduranceFresh','HomesteadVisualPlaytest','HomesteadShippingQA'){
        if([regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=\s|$)").Count -ne 1){
            throw "Completion-driven Shipping QA requires one explicit $flag."
        }
    }
    if($CommandLine -match '(?i)(?:^|\s)-Homestead(SmokeTest|ForageRenewal|WateringPlaytest|WeedingPlaytest|ClearingPlaytest|PresentationDiagnostics)(?:\s|$)'){
        throw 'Completion-driven Shipping QA cannot mix endurance with another route.'
    }
}
function Get-NativeResumeSource([string]$CommandLine,[string]$Output,[string]$AutomationRoot,[switch]$Generated) {
    $flag=if($Generated){'HomesteadGeneratedResumeFrom'}else{'HomesteadNativeResumeFrom'}
    $other=if($Generated){'HomesteadNativeResumeFrom'}else{'HomesteadGeneratedResumeFrom'}
    if($CommandLine -match "(?i)(?:^|\s)-$other(?=[=\s]|$)"){throw 'Resume source flag does not match the selected route.'}
    $tokens=[regex]::Matches($CommandLine,"(?i)(?:^|\s)-$flag(?=[=\s]|$)")
    if(-not $tokens.Count){return $null}
    $match=[regex]::Match($CommandLine,"(?i)(?:^|\s)-$flag="+ '"([^"\r\n]+)"(?=\s|$)')
    $route=if($Generated){'HomesteadGeneratedWoodland'}else{'HomesteadNativeMenuTest'}
    $forbidden=if($Generated){'NativeMenuTest|NativeQuitTest|VisualPlaytest|FullLoop|ClearingTest'}else{'NativeQuitTest|GeneratedWoodland'}
    if($tokens.Count -ne 1 -or -not $match.Success -or
        [regex]::Matches($CommandLine,"(?i)(?:^|\s)-$route(?=\s|$)").Count -ne 1 -or
        $CommandLine -match "(?i)(?:^|\s)-Homestead($forbidden)(?:\s|$)"){
        throw 'Resume requires one explicit quoted producer path and one unmixed matching route.'
    }
    $source=$match.Groups[1].Value
    if(-not [IO.Path]::IsPathFullyQualified($source)){throw 'Resume producer must be absolute.'}
    $source=[IO.Path]::GetFullPath($source)
    $allowed=[IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($AutomationRoot))+'\'
    if(-not $source.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or
        -not $Output.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::TrimEndingDirectorySeparator($source) -ieq [IO.Path]::TrimEndingDirectorySeparator($Output) -or
        [IO.DriveInfo]::new([IO.Path]::GetPathRoot($source)).DriveType -ne [IO.DriveType]::Fixed){
        throw 'Resume requires distinct producer/consumer paths within the same fixed local Automation root.'
    }
    $names=if($Generated){@('generated-woodland-fixture.json','generated-woodland-fixture.sav')}else{@('native-wardrobe-fixture.json','native-wardrobe-fixture.sav')}
    foreach($name in $names){
        $file=Get-Item -LiteralPath (Join-Path $source $name)
        $limit=if($name.EndsWith('.sav')){if($Generated){20MB}else{4MB}}else{8MB}
        if($file -is [IO.DirectoryInfo] -or $file.Length -le 0 -or $file.Length -gt $limit){throw 'Resume fixture size/type differs.'}
        for($item=$file;$null -ne $item;$item=if($item -is [IO.DirectoryInfo]){$item.Parent}else{$item.Directory}){
            if($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::Device)){
                throw 'Resume fixture/ancestor must be ordinary.'
            }
        }
    }
    return $source
}
$run=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if(-not $run.allowWork -or ($run.completionPolicy -ne 'until-complete' -and
    [DateTimeOffset]::UtcNow.AddSeconds(180) -ge [DateTimeOffset]$run.deadlineUtc)){throw 'Run does not admit the short Shipping QA route.'}
Assert-QACompletionRoute $Arguments ([bool]$CompletionDriven) $run.completionPolicy
$package=& (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $PackageDirectory -Details
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
$generatedRoute=$Arguments -match '(?i)(?:^|\s)-HomesteadGeneratedWoodland(?=\s|$)'
$resumeSource=Get-NativeResumeSource $Arguments $output (Join-Path $root 'Saved\Automation') -Generated:$generatedRoute
$resumeNames=if($generatedRoute){@('generated-woodland-fixture.json','generated-woodland-fixture.sav')}else{@('native-wardrobe-fixture.json','native-wardrobe-fixture.sav')}
$resumePins=@(if($resumeSource){foreach($name in $resumeNames){
    $path=Join-Path $resumeSource $name
    @{path=$path;sha256=(Get-FileHash $path).Hash}
}})
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
    if($CompletionDriven){$guard.ArmDeadline(0,0,$stop,'RenderCompletionDriven')}
    else{$guard.ArmDeadline(100000,110000,$stop)}
    $null=Read-QAJob
    $state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
    if(-not $state.allowWork -or $state.id -cne $run.id -or
        ($CompletionDriven -and $state.completionPolicy -cne 'until-complete') -or
        @(Get-CimInstance Win32_Process -Filter $authoringFilter).Count) {
        throw 'Shipping QA admission changed before first-thread resume.'
    }
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime;executableSha256=$imageHash;
        arguments=$Arguments;resumeSourcePins=$resumePins;markerBefore=$before;job=$guard.LastVerifiedJob;creationFlags=$guard.CreationFlags;
        handles=$guard.WhitelistedHandleCount;guardSha256=(Get-FileHash (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')).Hash;
        adapterSha256=(Get-FileHash $PSCommandPath).Hash;completionDriven=[bool]$CompletionDriven;
        deadlineProfile=$(if($CompletionDriven){'RenderCompletionDriven'}else{'Default'});
        softSeconds=$(if($CompletionDriven){0}else{100});hardSeconds=$(if($CompletionDriven){0}else{110});
        wrapperSeconds=$(if($CompletionDriven){0}else{120})} |
        ConvertTo-Json -Depth 8|Set-Content $launch
    $guard.Resume()
    do {
        $null=Read-QAJob
        $endpoints=@([Homestead.Authoring.LeafGuard]::ObserveEndpoints([uint32[]]@($guard.ProcessId)))
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;endpoints=$endpoints})
        if($endpoints.Count){throw 'Unexpected Shipping TCP/UDP endpoint; there is no TraceControl exception.'}
        if(@(Get-CimInstance Win32_Process -Filter $authoringFilter).Count){throw 'Competing authoring/helper process appeared.'}
        $state=& (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if(-not $state.allowWork -or $state.id -cne $run.id -or
            ($CompletionDriven -and $state.completionPolicy -cne 'until-complete')){throw 'Shipping QA cancelled by live run control.'}
        if(-not $CompletionDriven -and $clock.Elapsed.TotalSeconds -gt 115){throw 'Shipping QA exceeded its short wrapper bound.'}
    } while(-not $guard.Wait(100))
    $exitCode=$guard.ExitCode
    $final=Read-QAJob -Final
    if($exitCode -ne 0 -or $guard.HardTerminated -or $guard.DeadlineStopRequested -or $guard.DeadlineError){
        throw "Shipping QA failed/cancelled: exit=$exitCode; deadline=$($guard.DeadlineError)"
    }
    if((Get-FileHash $package.executable).Hash -cne $imageHash){throw 'Staged executable changed during QA.'}
    foreach($pin in $resumePins){
        if((Get-FileHash $pin.path).Hash -cne $pin.sha256){throw 'Producer resume fixture changed during consumer QA.'}
    }
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
        completionDriven=[bool]$CompletionDriven;
        limits='One root-only Shipping QA case. Completion-driven endurance reuses the existing zero-timer RenderCompletionDriven lifetime profile; live stop and owned-job cancellation remain enforced. Finite process/endpoint samples, not continuous history or a filesystem/network sandbox. Runtime route/image checks remain the caller responsibility.'} |
        ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'qa-guard-result.json')
}
if($failure){throw $failure}
[pscustomobject]@{ExitCode=$exitCode;Id=$guard.ProcessId}
