[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(-1,0,1,2,3,5)][int]$CompileActionId=-1,
    [ValidateSet(-1,2,3,4,6,7,8,9)][int]$ResourceLinkActionId=-1,
    [ValidateSet('','Game','Probe')][string]$ConvertResource='',
    [string]$DerivedDllResponse='',
    [switch]$DetachedConsole,
    [switch]$FernActions
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $root 'Scripts\CompilerLeafEvidence.ps1')
$run=& (Join-Path $root 'Scripts\Development-Run.ps1') -Action Status
if (-not $run.allowWork -or ($run.completionPolicy -ne 'until-complete' -and
    [DateTimeOffset]::UtcNow.AddMinutes(2) -ge [DateTimeOffset]$run.deadlineUtc)) { throw 'Live run does not admit fixture.' }
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if (-not $output.StartsWith((Join-Path $root "Saved\Automation\$($run.id)")+'\',[StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $output)) { throw 'Fresh current-run fixture directory required.' }
$compiler='E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe'
$hash='FE251EF50A1545B1B0835EE17B1E785459712B38D79E45B5C1D3D28970A36619'
if($CompileActionId -ne -1 -and $ResourceLinkActionId -ne -1){throw 'Select only one reviewed action.'}
if($ConvertResource -and ($CompileActionId -ne -1 -or $ResourceLinkActionId -ne -1)){throw 'Conversion must be a separate leaf.'}
$dllActionIds=if($FernActions){@(2)}else{@(6,9)}
if($FernActions -and ($CompileActionId -notin @(-1,0,1) -or $ResourceLinkActionId -notin @(-1,2,3))){throw 'Only the two exported fern compiles/library/DLL are admitted.'}
if(-not $FernActions -and $ResourceLinkActionId -in @(2,3)){throw 'Fern link IDs require the pinned fern map.'}
if($DerivedDllResponse -and $ResourceLinkActionId -notin $dllActionIds){throw 'Derived response requires an approved DLL link.'}
$selectedId=if($ResourceLinkActionId -ne -1){$ResourceLinkActionId}else{$CompileActionId}
if($ConvertResource) {
    $selectedId="convert-$ConvertResource"
    $compiler='E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cvtres.exe'
    $hash='B5DA94E7B9FF60B388EA9013D9E0E3D4A2A68BFF7C668C7017583459DAC4E3C5'
}
if($ResourceLinkActionId -ne -1) {
    $compiler=if($ResourceLinkActionId -eq 8){'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe'}else{'E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe'}
    $hash=if($ResourceLinkActionId -eq 8){'43DA1503C262C30894C851589BF0155F8365D77E63A5F7BC13982320E3A6B42D'}else{'A364AF801A8539E4324D9489313DBF001D959128451FC99A27B24676BBAC058F'}
    if(@(Get-CimInstance Win32_Process -Filter "Name='mspdbsrv.exe' OR Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'").Count){throw 'Unowned PDB server or Editor prevents admission.'}
}
if ((Get-FileHash $compiler).Hash -cne $hash -or (Get-AuthenticodeSignature $compiler).Status -ne 'Valid') { throw 'Compiler identity differs.' }
$console='C:\Windows\System32\conhost.exe'
$consoleHash='E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5'
if((Get-FileHash $console).Hash -cne $consoleHash -or (Get-AuthenticodeSignature $console).Status -ne 'Valid') { throw 'Build-only console identity differs.' }
if (@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'").Count) { throw 'Existing uploader prevents an unambiguous compiler fixture.' }
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
$null=New-Item -ItemType Directory -Path $output,(Join-Path $output 'Temp')
$source=Join-Path $output 'fixture.cpp'
$action=$null;$responses=@();$produced=@();$working=$output;$exactArguments=$null;$backups=@()
if($ConvertResource) {
    $module=if($ConvertResource -eq 'Game'){'SurvivalGame'}else{'SurvivalGameEditor'}
    $source=Join-Path $root "Intermediate\Build\Win64\x64\UnrealEditor\Development\$module\Default.rc2.res"
    $sourceHash=if($ConvertResource -eq 'Game'){'6D828337158C19C252836D9549BC41A1578DFD616844AB3D715EC95EB9C97143'}else{'12892448A0741ED905D26B26CABB30A92F0C7AB3DD0C5C4D36AD086B26D08F27'}
    if((Get-FileHash $source).Hash -cne $sourceHash){throw 'Approved resource input changed.'}
    $converted=Join-Path (Split-Path $output -Parent) ($ConvertResource.ToLowerInvariant()+'-resource.obj')
    if(Test-Path $converted){throw 'Converted output must be fresh.'}
    $responses+=@{path=$source;sha256=$sourceHash}
    $exactArguments=('/MACHINE:X64 /READONLY /NOLOGO '+
        [Homestead.Authoring.LeafGuard]::Quote("/OUT:$converted")+' '+[Homestead.Authoring.LeafGuard]::Quote($source))
    $action=[pscustomobject]@{ProducedItems=@($converted)}
} elseif($selectedId -eq -1) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CompilerLeafFixture.cpp') -Destination $source
} else {
    $planPath=Join-Path $root $(if($FernActions){'Saved\Automation\20260920-182217-d1f84e39\native-build-plan-04\actions.json'}else{'Saved\Automation\20260920-182217-d1f84e39\native-build-plan-02\actions.json'})
    $planHash=if($FernActions){'612841C7EF24A780D6C89A6873A0812E4E48E4207ACE86EC9343E46C3762DA36'}else{'10BD045C4AEAD4344ECAECF061DBD65BBA94224DDA478D03F296ED564F5F10FA'}
    if((Get-FileHash $planPath).Hash -cne $planHash) { throw 'Reviewed action export changed.' }
    $plan=Get-Content $planPath -Raw|ConvertFrom-Json
    $action=@($plan.Actions|Where-Object Id -EQ $selectedId)[0]
    if($action.CommandPath -ine $compiler -or ($CompileActionId -ne -1 -and
        ($action.Type -ne 'Compile' -or $action.PrerequisiteActions.Count))) { throw 'Unreviewed action or compile dependencies.' }
    foreach($dependency in $action.PrerequisiteActions) {
        $prior=@($plan.Actions|Where-Object Id -EQ $dependency)[0]
        foreach($path in $prior.ProducedItems){if(-not(Test-Path -LiteralPath $path)){throw "Missing prerequisite:$path"}}
    }
    $exactArguments=$action.CommandArguments
    if($FernActions) {
        if((Get-FileHash (Join-Path $root 'Intermediate\Build\Win64\x64\UnrealEditor\Development\SurvivalGameEditor\SurvivalGameEditor.Shared.rsp')).Hash -cne '6EB0ED3808F77D6C0675B647A639DBA7734C2B23C20EDF7604687E4310EEF045' -or
            (Get-FileHash (Join-Path $root 'Intermediate\Build\Win64\x64\UnrealEditor\Development\SurvivalGameEditor\Definitions.h')).Hash -cne '116BDD78EDABBF8B31C806F1D01DE57BD7E4617728BBD757DF9C238749552573') { throw 'Fern shared response/definitions differ.' }
        if($action.CommandArguments -notmatch '@"([^"]+)"'){throw 'Missing fern response.'}
        $fernResponsePins=@{
            0='B57B76FE3163082B2462B90BBAFDDC1DEE9053927AC6E26718025D91CB890F1B'
            1='281BA939780E14EB53E20AF93D7E51E6362555982AB6B532145425C9DA24A5D0'
            2='9DE3CD1086EB718A2993E18BD2F136AD9328B5F26B577A0C4EE39F6F164FB309'
            3='8DC5BF3C775B5680C731DADC239B71678E0D95812EE65CF85A1A063665E2EF3A'
        }
        if((Get-FileHash $Matches[1].Replace('/','\')).Hash -cne $fernResponsePins[$selectedId]){throw 'Reviewed actual UBT fern response differs.'}
    }
    $pending=[Collections.Generic.Queue[string]]::new()
    if($action.CommandArguments -match '@"([^"]+)"') {
        $response=[IO.Path]::GetFullPath($Matches[1].Replace('/','\'));$pending.Enqueue($response)
    } elseif($ResourceLinkActionId -ne 8){throw 'Missing reviewed response file.'}
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
    if($CompileActionId -ne -1) {
        $first=(Get-Content $response -TotalCount 1).Trim('"').Replace('/','\')
        $source=[IO.Path]::GetFullPath($first)
    } elseif($ResourceLinkActionId -eq 8) {
        $source='E:\Program Files\UE_5.8\Engine\Build\Windows\Resources\Default.rc2'
    } else { $source=$response }
    if($DerivedDllResponse) {
        $derived=[IO.Path]::GetFullPath($DerivedDllResponse,$root)
        if(-not $derived.StartsWith((Join-Path $root "Saved\Automation\$($run.id)")+'\',[StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path (Split-Path $derived -Parent) -Leaf) -notmatch '^resource-conversion-[0-9]{2}$'){throw 'Derived response is outside the approved fresh recipe.'}
        $which=if($ResourceLinkActionId -eq 6){'game'}else{'probe'}
        $module=if($ResourceLinkActionId -eq 6){'SurvivalGame'}else{'SurvivalGameEditor'}
        $resource=Join-Path $root "Intermediate\Build\Win64\x64\UnrealEditor\Development\$module\Default.rc2.res"
        $converted=Join-Path (Split-Path $derived -Parent) "$which-resource.obj"
        $conversion=Get-Content (Join-Path (Split-Path $derived -Parent) "$which\result.json") -Raw|ConvertFrom-Json
        if($conversion.status -ne 'passed' -or (Get-FileHash $converted).Hash -cne $conversion.objectSha256){throw 'Converted resource is not a verified real product.'}
        $null=Read-ResourceCoff $converted
        $oldToken='"'+$resource.Replace('\','/')+'"'
        $newToken='"'+$converted+'"'
        $originalText=[IO.File]::ReadAllText($response)
        if([regex]::Matches($originalText,[regex]::Escape($oldToken)).Count -ne 1){throw 'Expected exactly one original resource token.'}
        $expected=$originalText.Replace($oldToken,$newToken)
        if([IO.File]::ReadAllText($derived) -cne $expected){throw 'Derived response changes more than the approved resource token.'}
        $responses+=@{path=$derived;sha256=(Get-FileHash $derived).Hash}
        $responses+=@{path=$converted;sha256=(Get-FileHash $converted).Hash}
        $exactArguments='@"'+$derived+'"'
    }
    $working=$action.WorkingDirectory
    foreach($path in $action.ProducedItems) {
        if(-not $path.StartsWith((Join-Path $root 'Intermediate\Build')+'\',[StringComparison]::OrdinalIgnoreCase) -and
            -not ($ResourceLinkActionId -in $dllActionIds -and $path.StartsWith((Join-Path $root 'Binaries\Win64')+'\',[StringComparison]::OrdinalIgnoreCase))) { throw 'Unexpected tool output path.' }
        if(($ResourceLinkActionId -in $dllActionIds -or $path -like '*.lib') -and (Test-Path -LiteralPath $path)) {
            $backupDirectory=Join-Path $output 'before-products'
            $null=New-Item -ItemType Directory -Path $backupDirectory -Force
            $backup=Join-Path $backupDirectory ([IO.Path]::GetFileName($path))
            $beforeHash=(Get-FileHash -LiteralPath $path).Hash
            if($path -like '*.lib'){[IO.File]::Move($path,$backup)}
            else{Copy-Item -LiteralPath $path -Destination $backup}
            if((Get-FileHash $backup).Hash -cne $beforeHash){throw 'Editor binary backup differs.'}
            $backups+=@{path=$path;backup=$backup;sha256=$beforeHash}
        }
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
    if(-not $ConvertResource){foreach($entry in $plan.Environment.PSObject.Properties){$environment[$entry.Name]=[string]$entry.Value}}
    $arguments=[string[]]@()
    $object=$action.ProducedItems[0]
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
    $guard=[Homestead.Authoring.LeafGuard]::new($compiler,$hash,$arguments,$working,$marker,(Join-Path $output 'compiler.log'),$environment,$exactArguments,[bool]$DetachedConsole)
    $guard.ArmDeadline($(if($action){100000}else{30000}),$(if($action){110000}else{40000}),(Join-Path $output 'watchdog-stop'))
    $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime
        executableSha256=$hash;arguments=$arguments;job=$guard.LastVerifiedJob;resumed=$guard.Resumed
        marker=$guard.MarkerBefore;explicitHandles=$guard.WhitelistedHandleCount;sourceSha256=(Get-FileHash $source).Hash
        guardSha256=(Get-FileHash (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')).Hash
        allowedBuildOnlyConsole=@{path=$console;sha256=$consoleHash;signature='Valid'}
        compileActionId=$CompileActionId;workingDirectory=$working;responseFiles=$responses
        expectedProducedItems=$(if($action){$action.ProducedItems}else{@($object)})
        resourceLinkActionId=$ResourceLinkActionId;exactReviewedArguments=$exactArguments;previousProductBackups=$backups
        resourceConversion=$ConvertResource;derivedDllResponse=$DerivedDllResponse
        creationFlags=$guard.CreationFlags;detachedConsole=[bool]$DetachedConsole;fernActions=[bool]$FernActions
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
            elseif($path -match '\.(lib|dll|pdb|res)$') {
                $stream=[IO.File]::OpenRead($path)
                try {
                    $header=[byte[]]::new(64);$read=$stream.Read($header,0,64)
                    if($path -like '*.lib' -and [Text.Encoding]::ASCII.GetString($header,0,8) -cne "!<arch>`n"){throw 'Invalid import library.'}
                    if($path -like '*.pdb' -and -not [Text.Encoding]::ASCII.GetString($header,0,32).StartsWith('Microsoft C/C++ MSF 7.00')){throw 'Invalid PDB.'}
                    if($path -like '*.res' -and ($read -lt 32 -or [BitConverter]::ToUInt32($header,0) -ne 0 -or [BitConverter]::ToUInt32($header,4) -ne 32)){throw 'Invalid resource file.'}
                    if($path -like '*.dll') {
                        if($read -lt 64 -or [Text.Encoding]::ASCII.GetString($header,0,2) -cne 'MZ'){throw 'Invalid DLL header.'}
                        $at=[BitConverter]::ToUInt32($header,60)
                        if($at+6 -gt $stream.Length){throw 'Invalid PE offset.'}
                        $null=$stream.Seek($at,'Begin');$pe=[byte[]]::new(6);$null=$stream.Read($pe,0,6)
                        if([BitConverter]::ToUInt32($pe,0) -ne 0x4550 -or [BitConverter]::ToUInt16($pe,4) -ne 0x8664){throw 'Invalid AMD64 PE.'}
                    }
                } finally {$stream.Dispose()}
            }
            $produced+=@{path=$path;bytes=$file.Length;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o');sha256=(Get-FileHash $path).Hash}
        }
        foreach($item in $responses) {
            if((Get-FileHash $item.path).Hash -cne $item.sha256){throw 'Compiler response file changed during execution.'}
        }
        if($ConvertResource){$coff=Read-ResourceCoff $object}
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
        coff=$coff;malformedCoffCases=$checks;markerAfter=$markerAfter;compileActionId=$CompileActionId;resourceLinkActionId=$ResourceLinkActionId;producedItems=$produced;previousProductBackups=$backups
        objectSha256=$(if(Test-Path $object){(Get-FileHash $object).Hash}else{$null})
        limits='One explicitly selected guarded build leaf with exact approved Windows console host. No application helpers/UBT/Editor/global marker. Finite endpoint/member samples, not continuous tracing. Aggregate counters do not identify denied targets.'
    }|ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'result.json')
}
if($failure){throw $failure}
if($action) {
    [pscustomobject]@{status='passed';actionId=$selectedId;producedFiles=$produced.Count;endpointSamples=$samples.Count;output=$output}
} else { Get-Content (Join-Path $output 'result.json') -Raw }
