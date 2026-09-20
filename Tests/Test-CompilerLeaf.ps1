[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory)
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
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CompilerLeafFixture.cpp') -Destination $source
$marker=Join-Path $output 'dummy-marker'
[IO.File]::WriteAllBytes($marker,[byte[]]@())
$object=Join-Path $output 'fixture.obj'
$environment=[Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP']=Join-Path $output 'Temp';$environment['TMP']=$environment['TEMP']
$environment['CL']=$null;$environment['_CL_']=$null
$arguments=[string[]]@('/nologo','/c','/Z7','/O2',"/Fo$object",$source)
$samples=[Collections.Generic.List[object]]::new()
$memberSamples=[Collections.Generic.List[object]]::new()
$guard=$null;$failure=$null;$code=$null;$coff=$null;$afterJob=$null;$checks=0;$markerAfter=$null;$members=@()
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$clock=[Diagnostics.Stopwatch]::StartNew()
try {
    $state=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
    if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Live admission changed.' }
    $guard=[Homestead.Authoring.LeafGuard]::new($compiler,$hash,$arguments,$output,$marker,(Join-Path $output 'compiler.log'),$environment)
    $guard.ArmDeadline(30000,40000,(Join-Path $output 'watchdog-stop'))
    $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime
        executableSha256=$hash;arguments=$arguments;job=$guard.LastVerifiedJob;resumed=$guard.Resumed
        marker=$guard.MarkerBefore;explicitHandles=$guard.WhitelistedHandleCount;sourceSha256=(Get-FileHash $source).Hash
        guardSha256=(Get-FileHash (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')).Hash
        allowedBuildOnlyConsole=@{path=$console;sha256=$consoleHash;signature='Valid'}
    }|ConvertTo-Json -Depth 8|Set-Content (Join-Path $output 'launch.json')
    $guard.Resume()
    do {
        $children=@(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($guard.ProcessId)" |
            Select-Object ProcessId,ParentProcessId,ExecutablePath,CreationDate)
        $uploaders=@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'" | Select-Object ProcessId,ExecutablePath,CreationDate)
        $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
        $members=@($memberSamples)
        $ids=@($guard.ProcessId)+@($members.Members.Pid) | Sort-Object -Unique
        $filter=($ids|ForEach-Object{"OwningProcess=$_"}) -join ' OR '
        $tcp=@(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter $filter |
            Select-Object OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
        $udp=@(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter $filter |
            Select-Object OwningProcess,LocalAddress,LocalPort)
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;children=$children;uploaders=$uploaders;tcp=$tcp;udp=$udp})
        if (@($children|Where-Object ExecutablePath -INE $console).Count -or $uploaders.Count -or $tcp.Count -or $udp.Count) { throw 'Unexpected process or endpoint observed.' }
        $state=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id -or $clock.Elapsed.TotalSeconds -gt 35) { throw 'Fixture cancelled by run/deadline.' }
    } while (-not $guard.Wait(10))
    $code=$guard.ExitCode
    $afterJob=$guard.CaptureExitedJob()
    if ($code -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw "Compiler fixture failed:$code" }
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
    $members=@($memberSamples)
    Assert-CompilerLeafAccounting $afterJob $members $guard.ProcessId $compiler
    if((Get-FileHash $console).Hash -cne $consoleHash) { throw 'Console image changed.' }
} catch { $failure=$_.ToString() }
finally {
    if ($guard) {
        try {
            if (-not $guard.Wait(0)) { $guard.HardStop(98) }
            $markerAfter=$guard.VerifyMarker()
        } catch { $failure=(@($failure,$_.ToString())|Where-Object {$_}) -join ' | ' }
        finally { $guard.Dispose() }
    }
    $previous=0.0;$gaps=@()
    foreach($sample in $samples){$gaps+=$sample.elapsedMs-$previous;$previous=$sample.elapsedMs}
    $gaps+=$clock.Elapsed.TotalMilliseconds-$previous
    [ordered]@{status=$(if($failure){'failed'}else{'passed'});error=$failure;exitCode=$code;elapsedSeconds=$clock.Elapsed.TotalSeconds
        samples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum;exitedJob=$afterJob;jobMemberSamples=$members
        coff=$coff;malformedCoffCases=$checks;markerAfter=$markerAfter
        objectSha256=$(if(Test-Path $object){(Get-FileHash $object).Hash}else{$null})
        limits='One guarded compile-only fixture with exact approved build-only Windows console host. No link/UBT/Editor/global marker. Finite endpoint/member samples, not continuous tracing. Aggregate counters do not identify denied targets.'
    }|ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'result.json')
}
if($failure){throw $failure}
Get-Content (Join-Path $output 'result.json') -Raw
