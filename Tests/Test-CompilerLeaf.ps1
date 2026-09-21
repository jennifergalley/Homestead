[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(-1,0,1,2,3,5)][int]$CompileActionId=-1
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $root 'Scripts\CompilerLeafEvidence.ps1')
$run=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
if (-not $run.allowWork -or [DateTimeOffset]::UtcNow.AddMinutes(2) -ge [DateTimeOffset]$run.deadlineUtc) { throw 'Live run does not admit fixture.' }
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if (-not $output.StartsWith((Join-Path $root "Saved\Automation\$($run.id)")+'\',[StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $output)) { throw 'Fresh current-run fixture directory required.' }
$compiler='E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe'
$hash='FE251EF50A1545B1B0835EE17B1E785459712B38D79E45B5C1D3D28970A36619'
if ((Get-FileHash $compiler).Hash -cne $hash -or (Get-AuthenticodeSignature $compiler).Status -ne 'Valid') { throw 'Compiler identity differs.' }
$console='C:\Windows\System32\conhost.exe'
$consoleHash='E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5'
if((Get-FileHash $console).Hash -cne $consoleHash -or (Get-AuthenticodeSignature $console).Status -ne 'Valid') { throw 'Build-only console identity differs.' }
if (@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'").Count) { throw 'Existing uploader prevents an unambiguous compiler fixture.' }
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
$null=New-Item -ItemType Directory -Path $output,(Join-Path $output 'Temp')
$source=Join-Path $output 'fixture.cpp'
$action=$null;$responses=@();$produced=@();$working=$output
if($CompileActionId -eq -1) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CompilerLeafFixture.cpp') -Destination $source
} else {
    $planPath=Join-Path $root 'Saved\Automation\20260920-182217-d1f84e39\native-build-plan-02\actions.json'
    if((Get-FileHash $planPath).Hash -cne '10BD045C4AEAD4344ECAECF061DBD65BBA94224DDA478D03F296ED564F5F10FA') { throw 'Reviewed action export changed.' }
    $plan=Get-Content $planPath -Raw|ConvertFrom-Json
    $action=@($plan.Actions|Where-Object Id -EQ $CompileActionId)[0]
    if($action.CommandPath -ine $compiler -or $action.Type -ne 'Compile' -or $action.PrerequisiteActions.Count -or
        $action.CommandArguments -notmatch '^@"([^"]+)"\s*$') { throw 'Unreviewed compiler action or dependencies.' }
    $responseArgument='@'+$Matches[1]
    $response=[IO.Path]::GetFullPath($Matches[1].Replace('/','\'))
    $pending=[Collections.Generic.Queue[string]]::new();$pending.Enqueue($response)
    $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    while($pending.Count) {
        $path=$pending.Dequeue()
        if(-not $seen.Add($path)){continue}
        if($seen.Count -gt 16 -or -not $path.StartsWith((Join-Path $root 'Intermediate')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected response-file graph.' }
        $responses+=@{path=$path;sha256=(Get-FileHash $path).Hash}
        foreach($line in Get-Content $path) {
            if($line -match '^@"([^"]+)"$'){$pending.Enqueue([IO.Path]::GetFullPath($Matches[1].Replace('/','\')))}
        }
    }
    $first=(Get-Content $response -TotalCount 1).Trim('"').Replace('/','\')
    $source=[IO.Path]::GetFullPath($first)
    $working=$action.WorkingDirectory
    foreach($path in $action.ProducedItems) {
        if(-not $path.StartsWith((Join-Path $root 'Intermediate\Build')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected compiler output path.' }
    }
}
$marker=Join-Path $output 'dummy-marker'
[IO.File]::WriteAllBytes($marker,[byte[]]@())
$object=Join-Path $output 'fixture.obj'
$environment=[Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP']=Join-Path $output 'Temp';$environment['TMP']=$environment['TEMP']
$environment['CL']=$null;$environment['_CL_']=$null
$arguments=[string[]]@('/nologo','/c','/Z7','/O2',"/Fo$object",$source)
if($action) {
    foreach($entry in $plan.Environment.PSObject.Properties){$environment[$entry.Name]=[string]$entry.Value}
    $arguments=[string[]]@($responseArgument)
    $object=@($action.ProducedItems|Where-Object {$_ -like '*.obj'})[0]
}
$samples=[Collections.Generic.List[object]]::new()
$memberSamples=[Collections.Generic.List[object]]::new()
$observedPids=[Collections.Generic.HashSet[uint32]]::new()
$verifiedPids=[Collections.Generic.HashSet[uint32]]::new()
$guard=$null;$failure=$null;$code=$null;$coff=$null;$afterJob=$null;$checks=0;$markerAfter=$null;$members=@()
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$clock=[Diagnostics.Stopwatch]::StartNew()
try {
    $state=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
    if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Live admission changed.' }
    $guard=[Homestead.Authoring.LeafGuard]::new($compiler,$hash,$arguments,$working,$marker,(Join-Path $output 'compiler.log'),$environment)
    $guard.ArmDeadline($(if($action){100000}else{30000}),$(if($action){110000}else{40000}),(Join-Path $output 'watchdog-stop'))
    $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime
        executableSha256=$hash;arguments=$arguments;job=$guard.LastVerifiedJob;resumed=$guard.Resumed
        marker=$guard.MarkerBefore;explicitHandles=$guard.WhitelistedHandleCount;sourceSha256=(Get-FileHash $source).Hash
        guardSha256=(Get-FileHash (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')).Hash
        allowedBuildOnlyConsole=@{path=$console;sha256=$consoleHash;signature='Valid'}
        compileActionId=$CompileActionId;workingDirectory=$working;responseFiles=$responses
        expectedProducedItems=$(if($action){$action.ProducedItems}else{@($object)})
    }|ConvertTo-Json -Depth 8|Set-Content (Join-Path $output 'launch.json')
    $guard.Resume()
    $null=$observedPids.Add($guard.ProcessId)
    $null=$verifiedPids.Add($guard.ProcessId)
    do {
        $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
        foreach($member in $memberSamples[-1].Members){$null=$observedPids.Add($member.Pid)}
        $ids=[uint32[]]$observedPids
        $endpoints=@([Homestead.Authoring.LeafGuard]::ObserveEndpoints([uint32[]]$ids))
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;endpoints=$endpoints;queriedPids=$ids})
        if($endpoints.Count){throw 'Unexpected owned IPv4/IPv6 TCP/UDP endpoint.'}
        foreach($member in $memberSamples[-1].Members) {
            if($member.Error -and $member.NativeError -eq 87 -and $verifiedPids.Contains($member.Pid)){continue}
            if($member.Error -or $member.Image -notin @($compiler,$console)){throw "Unexpected or unidentified live job member:$($member|ConvertTo-Json -Compress)"}
            $null=$verifiedPids.Add($member.Pid)
        }
        $state=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id -or $clock.Elapsed.TotalSeconds -gt $(if($action){105}else{35})) { throw 'Compiler leaf cancelled by run/deadline.' }
    } while (-not $guard.Wait(10))
    $code=$guard.ExitCode
    if (@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'").Count) { throw 'Uploader appeared during compiler fixture.' }
    $afterJob=$guard.CaptureExitedJob()
    if ($code -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw "Compiler fixture failed:$code" }
    if($action) {
        foreach($path in $action.ProducedItems) {
            $file=Get-Item -LiteralPath $path
            if($file.LastWriteTimeUtc.ToFileTimeUtc() -lt $guard.ProcessCreationTime) { throw "Compiler output is stale:$path" }
            if($path -like '*.obj') {
                $stream=[IO.File]::OpenRead($path)
                try { $header=[byte[]]::new(56);$read=$stream.Read($header,0,56) } finally {$stream.Dispose()}
                $normal=$read -ge 20 -and [BitConverter]::ToUInt16($header,0) -eq 0x8664
                $big=$read -eq 56 -and [BitConverter]::ToUInt16($header,0) -eq 0 -and
                    [BitConverter]::ToUInt16($header,2) -eq 0xffff -and [BitConverter]::ToUInt16($header,6) -eq 0x8664
                if(-not $normal -and -not $big){throw 'Output is not an AMD64 COFF object.'}
            }
            $produced+=@{path=$path;bytes=$file.Length;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o');sha256=(Get-FileHash $path).Hash}
        }
        foreach($item in $responses) {
            if((Get-FileHash $item.path).Hash -cne $item.sha256){throw 'Compiler response file changed during execution.'}
        }
    } else {
        $bytes=[IO.File]::ReadAllBytes($object)
        $coff=Read-CompilerLeafCoff $bytes
        foreach ($case in @('truncated','wrong-machine','symbol-overflow','bad-strings','wrong-code')) {
        $bad=[byte[]]$bytes.Clone()
        switch ($case) {
            'truncated' { $bad=[byte[]]@(0,1,2) }
            'wrong-machine' { $bad[0]=0 }
            'symbol-overflow' { [Array]::Copy([BitConverter]::GetBytes([uint32]::MaxValue),0,$bad,12,4) }
            'bad-strings' {
                $at=[BitConverter]::ToUInt32($bad,8)+18*[BitConverter]::ToUInt32($bad,12)
                [Array]::Copy([BitConverter]::GetBytes([uint32]::MaxValue),0,$bad,$at,4)
            }
            'wrong-code' { $section=$coff.sections|Where-Object name -EQ $coff.function.section|Select-Object -First 1;$bad[$section.offset+$coff.function.offset]=0 }
        }
        $rejected=$false
        try { $null=Read-CompilerLeafCoff $bad } catch { $rejected=$true }
        if (-not $rejected) { throw "Malformed COFF accepted:$case" }
            $checks++
        }
    }
    $members=@($memberSamples)
    Assert-CompilerLeafAccounting $afterJob $members $guard.ProcessId $compiler
    if((Get-FileHash $console).Hash -cne $consoleHash) { throw 'Console image changed.' }
} catch { $failure=$_.ToString() }
finally {
    $members=@($memberSamples)
    if ($guard) {
        try {
            if (-not $guard.Wait(0)) { $guard.HardStop(98) }
            if($null -eq $code){$code=$guard.ExitCode}
            $markerAfter=$guard.VerifyMarker()
        } catch { $failure=(@($failure,$_.ToString())|Where-Object {$_}) -join ' | ' }
        finally { $guard.Dispose() }
    }
    $previous=0.0;$gaps=@()
    foreach($sample in $samples){$gaps+=$sample.elapsedMs-$previous;$previous=$sample.elapsedMs}
    $gaps+=$clock.Elapsed.TotalMilliseconds-$previous
    [ordered]@{status=$(if($failure){'failed'}else{'passed'});error=$failure;exitCode=$code;elapsedSeconds=$clock.Elapsed.TotalSeconds
        samples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum;exitedJob=$afterJob;jobMemberSamples=$members
        coff=$coff;malformedCoffCases=$checks;markerAfter=$markerAfter;compileActionId=$CompileActionId;producedItems=$produced
        objectSha256=$(if(Test-Path $object){(Get-FileHash $object).Hash}else{$null})
        limits='One guarded compile-only fixture with exact approved build-only Windows console host. No link/UBT/Editor/global marker. Finite endpoint/member samples, not continuous tracing. Aggregate counters do not identify denied targets.'
    }|ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'result.json')
}
if($failure){throw $failure}
if($action) {
    [pscustomobject]@{status='passed';actionId=$CompileActionId;producedFiles=$produced.Count;endpointSamples=$samples.Count;output=$output}
} else { Get-Content (Join-Path $output 'result.json') -Raw }
