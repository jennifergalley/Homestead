[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'AuthoringProbePolicy.ps1')
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or [DateTimeOffset]::UtcNow.AddMinutes(3) -ge [DateTimeOffset]$run.deadlineUtc -or
    $run.authoringApproval.proposalSha256 -cne 'EA25571F37A6F3109BEECCA56B54E56006D8F61F0C0B07A95BA5DE077DB0DBCC') {
    throw 'Live run/approval/deadline does not admit the conditional settings probe.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$runRoot = Join-Path $root "Saved\Automation\$($run.id)"
if (-not $output.StartsWith($runRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)) {
    throw 'Fresh current-run probe output required.'
}
$priorReservation=Join-Path $runRoot 'native-settings-attempt.json'
$priorResult=Join-Path $runRoot 'native-settings-01\probe-result.json'
$supersession=@{approval='Coordinator explicit corrected settings-only third reservation, 2026-09-20 17:57 Arizona; Python execution disabled, dependency modules permitted; no retroactive pass'
    reservationSha256='733810D16F65E4DCC00E83BF210E9E307CFD99DA64579DC12299672AE419402A'
    resultSha256='F0AE7B69A68A2DA0A2261984F52B2321FFEF6569F7B4044F206ED34768D64DF7'
    secondReservationSha256='D90A8DBF29952F7BDBF0322C85A20127EF54023F80285ABDE2B8490E459C9E8A'
    secondResultSha256='A671447286868FC3857FD39EC756DF6AB48F591A48CD45E227AC4F0B649F06F7'}
function Assert-NamedSupersession {
    if($run.id -cne '20260920-182217-d1f84e39' -or
        $output -ine (Join-Path $runRoot 'native-settings-03') -or
        (Get-FileHash $priorReservation).Hash -cne $supersession.reservationSha256 -or
        (Get-FileHash $priorResult).Hash -cne $supersession.resultSha256 -or
        (Get-FileHash (Join-Path $runRoot 'native-settings-attempt-02.json')).Hash -cne $supersession.secondReservationSha256 -or
        (Get-FileHash (Join-Path $runRoot 'native-settings-02\probe-result.json')).Hash -cne $supersession.secondResultSha256) {
        throw 'Only the explicitly authorized third attempt with both prior failures preserved is admitted.'
    }
}
Assert-NamedSupersession
$attempt = Join-Path $runRoot 'native-settings-attempt-03.json'
if (Test-Path -LiteralPath $attempt) { throw 'The single native settings attempt is already reserved; no automatic retry.' }
$engine = 'E:\Program Files\UE_5.8\Engine\Binaries\Win64'
$exe = Join-Path $engine 'UnrealEditor-Cmd.exe'
$module = Join-Path $root 'Binaries\Win64\UnrealEditor-SurvivalGameEditor.dll'
$approved = [ordered]@{
    'UnrealEditor-Cmd.exe' = 'AE92F55952A3C9A7DEF90983FCF5AEDCE52D8EE8E4565FBD923985E9D9D3E05E'
    'UnrealEditor-Core.dll' = '3EAF66A3AA55FEB0BEEF9BD88411A577A0589AFADB60724F3D6651D2E46632DB'
    'UnrealEditor-TraceLog.dll' = '0759373042151249445A0260039DFA61E502DACD0E9347CDEFB26A0B6743160C'
}
foreach ($name in $approved.Keys) {
    $path = Join-Path $engine $name
    if ((Get-FileHash -LiteralPath $path).Hash -cne $approved[$name] -or
        (Get-AuthenticodeSignature -LiteralPath $path).Status -ne 'Valid') { throw "Engine identity differs:$name" }
}
$moduleHash = (Get-FileHash -LiteralPath $module).Hash
$buildReceiptPath = Join-Path $root 'docs\research\environment-assets\guarded-correction-01\receipt.json'
$buildReceiptHash = '3F4B00F44B25F3F02207A7A39A94C6F107FB9E3BB43893A3932E30E03570A53B'
$pythonPins = @{
    'python3.dll'='3C7ECFB999333AAF5BA9DDF4C5BFB8676B63CFCEC3DC5370CBC255A83063962F'
    'python311.dll'='3E5A5C012CDDB3D156D147ACAD59BB489C0716B87DAD274CB5BF20EEC3B68192'
}
if ((Get-FileHash $buildReceiptPath).Hash -cne $buildReceiptHash) { throw 'Accepted native build receipt differs.' }
$acceptedBuild = Get-Content $buildReceiptPath -Raw | ConvertFrom-Json
$productPins = @($acceptedBuild.products)
$requiredProducts = @('UnrealEditor-SurvivalGame.dll','UnrealEditor-SurvivalGame.pdb',
    'UnrealEditor-SurvivalGameEditor.dll','UnrealEditor-SurvivalGameEditor.pdb',
    'UnrealEditor.modules','SurvivalGameEditor.target') | ForEach-Object { "Binaries\Win64\$_" }
if ($productPins.Count -ne $requiredProducts.Count -or
    @($requiredProducts | Where-Object { $_ -cnotin $productPins.path }).Count) {
    throw 'Accepted native product map differs.'
}
function Assert-AcceptedNativeProducts {
    if ((Get-FileHash $buildReceiptPath).Hash -cne $buildReceiptHash) { throw 'Accepted receipt changed.' }
    foreach ($product in $productPins) {
        $path = Join-Path $root $product.path
        if ((Get-Item $path).Length -ne $product.bytes -or (Get-FileHash $path).Hash -cne $product.sha256) {
            throw "Accepted native product differs:$($product.path)"
        }
    }
    foreach ($name in $pythonPins.Keys) {
        if ((Get-FileHash (Join-Path (Split-Path (Split-Path $engine -Parent) -Parent) "Binaries\ThirdParty\Python3\Win64\$name")).Hash -cne $pythonPins[$name]) {
            throw 'Installed Python dependency identity differs.'
        }
    }
}
Assert-AcceptedNativeProducts
$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\research\environment-assets\authoring-preflight-01\receipt.json') -Raw | ConvertFrom-Json
$expectedRules = @($baseline.existingInboundAllowRules | Where-Object { $_.program -ieq $exe })
function Assert-ExistingNetworkPermission {
    $profiles = @(Get-NetConnectionProfile)
    if ($profiles.Count -ne $baseline.activeProfiles.Count) { throw 'Active network profile count changed.' }
    foreach ($profile in $profiles) {
        if (-not ($baseline.activeProfiles | Where-Object {
            $_.InterfaceAlias -eq $profile.InterfaceAlias -and $_.networkCategory -eq $profile.NetworkCategory.ToString()
        })) { throw 'Active network profile differs from the approved observation.' }
    }
    if ($expectedRules.Count -ne 2) { throw 'Expected exact existing Editor-Cmd TCP/UDP rules.' }
    $matchingRules = @(Get-NetFirewallApplicationFilter -Program $exe | Get-NetFirewallRule)
    if ($matchingRules.Count -ne $expectedRules.Count -or
        @($matchingRules | Where-Object Name -NotIn $expectedRules.name).Count) {
        throw 'The complete set of executable-specific authoring rules changed.'
    }
    foreach ($expected in $expectedRules) {
        $rule = Get-NetFirewallRule -Name $expected.name
        $app = $rule | Get-NetFirewallApplicationFilter
        $port = $rule | Get-NetFirewallPortFilter
        $address = $rule | Get-NetFirewallAddressFilter
        if ($app.Program -ine $exe -or $rule.Enabled.ToString() -ne 'True' -or
            $rule.Direction.ToString() -ne $expected.direction -or $rule.Action.ToString() -ne $expected.action -or
            $rule.Profile.ToString() -ne $expected.profile -or $port.Protocol.ToString() -ne $expected.protocol -or
            (@($port.LocalPort) -join ',') -ne ($expected.localPort -join ',') -or
            (@($port.RemotePort) -join ',') -ne ($expected.remotePort -join ',') -or
            (@($address.LocalAddress) -join ',') -ne ($expected.localAddress -join ',') -or
            (@($address.RemoteAddress) -join ',') -ne ($expected.remoteAddress -join ',')) {
            throw 'An existing authoring permission changed; do not click any security dialog.'
        }
    }
}
function Get-AuthoringProcesses {
    @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealPak.exe' OR Name='ShaderCompileWorker.exe' OR Name='CrashReportClientEditor.exe' OR Name='CrashReportClient.exe' OR Name='UnrealTraceServer.exe' OR Name='zenserver.exe' OR Name='UnrealInsights.exe'" |
        Select-Object ProcessId,ParentProcessId,ExecutablePath,CreationDate)
}
Assert-ExistingNetworkPermission
if (@(Get-AuthoringProcesses).Count) { throw 'Another authoring process prevents the bounded shared-marker lock.' }
$protected = @(Get-Content -LiteralPath (Join-Path $root 'Assets\Environment\woodland-preparation-01\protected-before.json') -Raw | ConvertFrom-Json)
$before = @($protected | ForEach-Object {
    $hash = (Get-FileHash -LiteralPath (Join-Path $root $_.path)).Hash
    if ($hash -cne $_.sha256 -and $_.path -notin @('SurvivalGame.uproject','Source\SurvivalGameEditor.Target.cs',
        'Scripts\Development-Run.ps1','Tests\DevelopmentRunTests.ps1')) {
        throw "Unrelated protected input changed:$($_.path)"
    }
    @{ path=$_.path;sha256=$hash }
})
$markerPath = 'C:\ProgramData\Epic\NotAllowedUnattendedBugReports'
$markerFile = Get-Item -LiteralPath $markerPath
if ($markerFile.Length -ne 0 -or ($markerFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Global marker metadata differs.' }
if ($ValidateOnly) {
    [ordered]@{ eligibleForGuardConstruction=$true;executable=$exe;module=$module;moduleSha256=$moduleHash
        buildReceiptSha256=$buildReceiptHash;productPins=$productPins;creationFlags=0x0008040C
        buildMonitoringQualification=$acceptedBuild.qualification
        output=$output;globalMarkerReadLocked=$false;runtimeVerified=$false;attemptConsumed=$false }
    return
}
Add-Type -Path (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')
$null = New-Item -ItemType Directory -Path $output,(Join-Path $output 'Config'),(Join-Path $output 'Temp'),
    (Join-Path $output 'EngineUser'),(Join-Path $output 'DDC')
$configs = [ordered]@{}
foreach ($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')) {
    $configs[$name] = Join-Path $output "Config\$name.ini"
    [IO.File]::WriteAllText($configs[$name], '')
}
$ddc = Join-Path $output 'DDC'
$arguments = @((Join-Path $root 'SurvivalGame.uproject'),'-run=HomesteadAuthoringProbe',"-EvidenceDirectory=$output",
    '-notraceserver','-traceautostart=0','-unattended','-nop4','-nosplash','-nullrhi','-stdout','-FullStdOutLogOutput',
    '-DisablePython','-DisablePlugins=PythonScriptPlugin,EditorScriptingUtilities,UdpMessaging,TcpMessaging',
    '-noshaderworker',"-UserDir=$(Join-Path $output 'EngineUser')","-abslog=$(Join-Path $output 'editor.log')",
    "-DDC=(Local=(Type=FileSystem,Path=$ddc,ReadOnly=false,Clean=false,Flush=false,DeleteUnused=false))",
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnabledByDefault=False',
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTransport=False',
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTunnel=False',
    '-ini:Engine:[/Script/TcpMessaging.TcpMessagingSettings]:EnableTransport=False',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=False',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRunPipInstallOnStartup=False',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bIsolateInterpreterEnvironment=True',
    '-ini:Engine:[DevOptions.Shaders]:bAllowCompilingThroughWorkers=False',
    '-ini:Engine:[ConsoleVariables]:r.Shaders.AllowCompilingThroughWorkers=0',
    '-ini:EditorSettings:[/Script/UnrealEd.CrashReportsPrivacySettings]:bSendUnattendedBugReports=False',
    '-ini:EditorSettings:[/Script/UnrealEd.AnalyticsPrivacySettings]:bSendUsageData=False')
foreach ($name in $configs.Keys) { $arguments += "-${name}INI=$($configs[$name])" }
$environment = [Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP'] = Join-Path $output 'Temp'
$environment['TMP'] = Join-Path $output 'Temp'
$environment['UE_PYTHONPATH'] = $null
$environment['UE_PIPINSTALL_PATH'] = Join-Path $output 'PipMustRemainAbsent'
$environment['UE_SKIP_UBT_SDK_SETUP'] = '1'
$environment['UE-LocalDataCachePath'] = $ddc
$environment['HOMESTEAD_PROBE_OUTPUT'] = $output
$environment['HOMESTEAD_PROBE_DEADLINE'] = [DateTimeOffset]::UtcNow.AddSeconds(110).ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$guard = $null
$failure = $null
$samples = [Collections.Generic.List[object]]::new()
$jobSamples = [Collections.Generic.List[object]]::new()
$native = $null
$started = [DateTimeOffset]::UtcNow
$clock = [Diagnostics.Stopwatch]::StartNew()
$stopPath = Join-Path $output 'stop-probe.txt'
function Request-ProbeStop([string]$Reason) {
    if (-not (Test-Path -LiteralPath $stopPath)) { [IO.File]::WriteAllText($stopPath,$Reason) }
}
function Read-ProbeJob([string]$Phase, [switch]$Final) {
    $job = if ($Final) { $guard.CaptureExitedJob() } else { $guard.ObserveJobPolicy() }
    $members = @($guard.ObserveJobMembers())
    $heldRoot = $guard.ObserveHeldRoot()
    $jobSamples.Add(@{phase=$Phase;utc=[DateTimeOffset]::UtcNow.ToString('o')
        job=$job;members=$members;heldRoot=$heldRoot})
    Assert-AuthoringRootObservation $job $members $heldRoot $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime -Final:$Final
    return $heldRoot.Exited
}
try {
    $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
    if (-not $state.allowWork -or $state.id -ne $run.id -or @(Get-AuthoringProcesses).Count) { throw 'Admission changed before guard construction.' }
    Assert-ExistingNetworkPermission
    Assert-AcceptedNativeProducts
    Assert-NamedSupersession
    $reservation = [IO.File]::Open($attempt,'CreateNew','Write','Read')
    try {
        $bytes=[Text.Encoding]::UTF8.GetBytes((@{utc=$started.ToString('o');output=$output;runId=$run.id;supersession=$supersession}|ConvertTo-Json))
        $reservation.Write($bytes)
    } finally { $reservation.Dispose() }
    $guard = [Homestead.Authoring.LeafGuard]::new($exe,$approved['UnrealEditor-Cmd.exe'],[string[]]$arguments,
        $root,$markerPath,(Join-Path $output 'stdout.log'),$environment,$true)
    if ($guard.CreationFlags -ne 0x0008040C -or $guard.WhitelistedHandleCount -ne 3) {
        throw 'Approved Editor detached-console/stdio creation policy differs.'
    }
    $guard.ArmDeadline(100000,110000,$stopPath)
    $null = Read-ProbeJob 'suspended-before-resume'
    [ordered]@{pid=$guard.ProcessId;creationTime=$guard.ProcessCreationTime;image=$guard.ImagePath
        executableSha256=$approved['UnrealEditor-Cmd.exe'];moduleSha256=$moduleHash;arguments=$arguments
        markerBefore=$guard.MarkerBefore;job=$guard.LastVerifiedJob;explicitInheritedHandles=$guard.WhitelistedHandleCount
        expectedRules=$expectedRules;deadlineUtc=$environment['HOMESTEAD_PROBE_DEADLINE']
        creationFlags=$guard.CreationFlags;buildReceiptSha256=$buildReceiptHash;productPins=$productPins;supersession=$supersession
        buildMonitoringQualification=$acceptedBuild.qualification
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
    $guard.Resume()
    $readyAt = $null
    $lastPermission = 0.0
    while (-not $guard.Wait(0)) {
        if (Read-ProbeJob 'live') { break }
        $null = $guard.VerifyMarker()
        $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($guard.ProcessId)" |
            Select-Object ProcessId,ParentProcessId,ExecutablePath,CreationDate)
        $other = @(Get-AuthoringProcesses | Where-Object ProcessId -NE $guard.ProcessId)
        $tcp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$($guard.ProcessId)" |
            Select-Object OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
        $udp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$($guard.ProcessId)" |
            Select-Object OwningProcess,LocalAddress,LocalPort)
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;utc=[DateTimeOffset]::UtcNow.ToString('o');tcp=$tcp;udp=$udp;children=$children;otherAuthoring=$other})
        Assert-AuthoringEndpoints $tcp $udp $guard.ProcessId
        if ($children.Count -or $other.Count) { throw 'Executed child or competing authoring process observed.' }
        $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Run pause/stop/deadline observed.' }
        if ($clock.Elapsed.TotalSeconds - $lastPermission -gt 3) {
            Assert-ExistingNetworkPermission
            $lastPermission = $clock.Elapsed.TotalSeconds
        }
        if (-not $native -and (Test-Path -LiteralPath (Join-Path $output 'effective-settings.json'))) {
            $native = Get-Content -LiteralPath (Join-Path $output 'effective-settings.json') -Raw | ConvertFrom-Json
            if (-not $native.valid -or $native.pid -ne $guard.ProcessId -or
                [uint64]$native.processCreationTime -ne $guard.ProcessCreationTime -or
                [IO.Path]::GetFullPath($native.executable) -ine $guard.ImagePath -or
                $native.marker.volume -ne $guard.MarkerBefore.Volume -or $native.marker.indexHigh -ne $guard.MarkerBefore.IndexHigh -or
                $native.marker.indexLow -ne $guard.MarkerBefore.IndexLow -or
                $native.cachedSendReports -or $native.cachedSendUsage -or
                $native.jobFlags -ne 8200 -or $native.jobProcessLimit -ne 1 -or $native.jobActiveProcesses -ne 1) {
                throw 'Actual native identity/config/privacy/guard evidence failed.'
            }
            Assert-AuthoringDdc $native.ddcStores $ddc
            Assert-AuthoringConfigBranches $native.configBranches $output
            Assert-AuthoringPythonState $native.pythonEntry
            $priorPlugins = (Get-Content (Join-Path $runRoot 'native-settings-02\effective-settings.json') -Raw | ConvertFrom-Json).enabledPlugins
            if (@($native.enabledPlugins | Where-Object { $_ -cnotin $priorPlugins }).Count) {
                throw 'An additional unreviewed plugin became enabled.'
            }
            $readyAt = $clock.Elapsed.TotalSeconds
        }
        if ($null -ne $readyAt -and $clock.Elapsed.TotalSeconds - $readyAt -ge 10 -and $samples.Count -ge 10) {
            Request-ProbeStop 'complete'
        }
        if ($clock.Elapsed.TotalSeconds -gt 105) { throw 'Bounded settings probe timeout.' }
        Start-Sleep -Milliseconds 100
    }
    if (-not $native -or $guard.ExitCode -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw 'Native settings probe failed or was hard-terminated.' }
    $exit = Get-Content -LiteralPath (Join-Path $output 'native-exit.json') -Raw | ConvertFrom-Json
    if (-not $exit.passed -or -not $exit.cooperative -or $exit.stopReason.Trim() -ne 'complete') { throw 'Native cooperative stop was not proved.' }
    Assert-AuthoringPythonState $exit.pythonExit
    if (Test-Path -LiteralPath $environment['UE_PIPINSTALL_PATH']) { throw 'Disabled Python unexpectedly touched the pip output.' }
} catch {
    $failure = $_.ToString()
} finally {
    $cleanupErrors = [Collections.Generic.List[string]]::new()
    $markerBefore=$null;$markerAfter=$null;$released=$null;$code=$null;$hard=$false;$watchdog=$null
    $subjectExited=$null;$guardDisposed=$false
    if ($guard) {
        $markerBefore = $guard.MarkerBefore
        try {
            if (-not $guard.Wait(0)) {
                try { Request-ProbeStop 'cancelled' } catch { $cleanupErrors.Add("Stop request failed:$_") }
                if (-not $guard.Wait(3000)) { $guard.HardStop(96) }
            }
        } catch {
            $cleanupErrors.Add("Owned-process cleanup failed:$_")
            try { $guard.HardStop(97) } catch { $cleanupErrors.Add("Owned-job fallback failed:$_") }
        }
        try { $subjectExited=$guard.Wait(5000) } catch { $cleanupErrors.Add("Exit observation failed:$_") }
        if ($subjectExited) {
            try { $null = Read-ProbeJob 'after-observed-death' -Final } catch { $cleanupErrors.Add("Final job evidence failed:$_") }
            try { $markerAfter=$guard.VerifyMarker() } catch { $cleanupErrors.Add("Guarded marker verification failed:$_") }
            try {
                $code=$guard.ExitCode;$hard=$guard.HardTerminated
                $watchdog=@{requested=$guard.DeadlineStopRequested;hardStop=$guard.DeadlineHardStop;error=$guard.DeadlineError}
            } catch { $cleanupErrors.Add("Exit evidence failed:$_") }
            try { $guard.Dispose();$guardDisposed=$true } catch { $cleanupErrors.Add("Guard disposal failed:$_") }
            try {
                $released=[Homestead.Authoring.LeafGuard]::InspectMarker($markerPath)
                if (($released|ConvertTo-Json -Compress) -cne ($markerBefore|ConvertTo-Json -Compress)) {
                    throw 'Post-release marker identity/metadata/ACL differs.'
                }
            } catch { $cleanupErrors.Add("Released marker verification failed:$_") }
        } else {
            $cleanupErrors.Add('Subject death was not observed; guard was not explicitly released.')
        }
    }
    foreach ($item in $before) {
        try {
            if ((Get-FileHash -LiteralPath (Join-Path $root $item.path)).Hash -cne $item.sha256) { throw "Protected file changed:$($item.path)" }
        } catch { $cleanupErrors.Add($_.ToString()) }
    }
    try { Assert-AcceptedNativeProducts } catch { $cleanupErrors.Add("Accepted build changed:$_") }
    try { Assert-NamedSupersession } catch { $cleanupErrors.Add("Original failed attempt changed:$_") }
    if ($cleanupErrors.Count) { $failure=(@($failure)+@($cleanupErrors) | Where-Object { $_ }) -join ' | ' }
    $gaps = @()
    $previous = 0.0
    foreach ($sample in $samples) { $gaps += $sample.elapsedMs-$previous; $previous=$sample.elapsedMs }
    $gaps += $clock.Elapsed.TotalMilliseconds-$previous
    [ordered]@{status=$(if($failure){'failed'}else{'passed'});error=$failure;cleanupErrors=$cleanupErrors
        subjectExited=$subjectExited;guardDisposed=$guardDisposed;exitCode=$code;hardTerminated=$hard;watchdog=$watchdog
        elapsedSeconds=$clock.Elapsed.TotalSeconds;samples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum
        markerBefore=$markerBefore;markerAfter=$markerAfter;markerAfterRelease=$released
        native=$native;protectedFiles=$before;executableSha256=$approved['UnrealEditor-Cmd.exe'];moduleSha256=$moduleHash
        jobSamples=$jobSamples;buildReceiptSha256=$buildReceiptHash;productPins=$productPins;supersession=$supersession
        buildMonitoringQualification=$acceptedBuild.qualification;approvedCreationFlags=0x0008040C
        limits='One NullRHI settings/stop probe. Sampled endpoints/processes, not packet/continuous broker tracing. Security prompts require human/coordinator stop; never click Allow. No shader workload/render/import/cook/Pak/Shipping-QA proof.'
    } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $output 'probe-result.json')
}
if ($failure) { throw $failure }
"Native settings/stop probe passed:$output"
