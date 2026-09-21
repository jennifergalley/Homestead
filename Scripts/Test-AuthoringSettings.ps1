[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$ValidateOnly,
    [ValidateSet('Settings','Import','Render')][string]$Mode='Settings'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'AuthoringProbePolicy.ps1')
. (Join-Path $PSScriptRoot 'FernSpikePolicy.ps1')
$softSeconds=if($Mode -eq 'Import'){150}elseif($Mode -eq 'Render'){480}else{100}
$hardSeconds=if($Mode -eq 'Import'){180}elseif($Mode -eq 'Render'){510}else{110}
$ceilingSeconds=if($Mode -eq 'Import'){210}elseif($Mode -eq 'Render'){540}else{120}
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or [DateTimeOffset]::UtcNow.AddSeconds($ceilingSeconds+60) -ge [DateTimeOffset]$run.deadlineUtc -or
    $run.authoringApproval.proposalSha256 -cne 'EA25571F37A6F3109BEECCA56B54E56006D8F61F0C0B07A95BA5DE077DB0DBCC') {
    throw 'Live run/approval/deadline does not admit the conditional settings probe.'
}
Add-Type -Path (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')
$deadlineProfile=if($Mode -eq 'Settings'){'Default'}else{$Mode}
[Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile($softSeconds*1000,$hardSeconds*1000,$deadlineProfile)
$wrapperHash=(Get-FileHash $PSCommandPath).Hash
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
        $output -ine (Join-Path $runRoot $(if($Mode -eq 'Settings'){'native-settings-03'}elseif($Mode -eq 'Import'){'fern-import-02'}else{'fern-render-01'})) -or
        (Get-FileHash $priorReservation).Hash -cne $supersession.reservationSha256 -or
        (Get-FileHash $priorResult).Hash -cne $supersession.resultSha256 -or
        (Get-FileHash (Join-Path $runRoot 'native-settings-attempt-02.json')).Hash -cne $supersession.secondReservationSha256 -or
        (Get-FileHash (Join-Path $runRoot 'native-settings-02\probe-result.json')).Hash -cne $supersession.secondResultSha256) {
        throw 'Only the explicitly authorized third attempt with both prior failures preserved is admitted.'
    }
    if($Mode -ne 'Settings' -and (
        (Get-FileHash (Join-Path $runRoot 'native-settings-attempt-03.json')).Hash -cne 'F45A17352F06F0F029090C3ACAE9A4043F58E8EA1C0642B581B30337E96C2046' -or
        (Get-FileHash (Join-Path $runRoot 'native-settings-03\probe-result.json')).Hash -cne 'FB6E92F924EBD7F841D52B6FF408B4ED708B49AFB3569595E32B7A8228EB5D7C')) {
        throw 'Accepted settings subgate changed.'
    }
    if($Mode -ne 'Settings' -and (
        (Get-FileHash (Join-Path $runRoot 'fern-import-attempt-01.json')).Hash -cne '3D799DBE1E344B703B468A7D5016E082C3271700A6C2E50D073EDD3BBC1508C2' -or
        (Get-FileHash (Join-Path $runRoot 'fern-import-01\probe-result.json')).Hash -cne '6D6F56576EE6ECCABF0918B99D6168E7C410213387ED11EADE11FB00A49D3910')) {
        throw 'Original pre-resume fern failure changed.'
    }
    if($Mode -eq 'Render' -and (
        (Get-FileHash (Join-Path $runRoot 'fern-import-02\probe-result.json')).Hash -cne '31477873B75CC0BD90C7557E0A96D21BC474BE87D278D7787143D55A66720624' -or
        (Get-FileHash (Join-Path $runRoot 'fern-import-02\asset-admission.json')).Hash -cne 'C818E55DBD142CA6AA28D26CEB9BA42E6ECAEBD81F2A5FDDD04D7C64B2846B33' -or
        (Get-FileHash (Join-Path $runRoot 'fern-import-attempt-02.json')).Hash -cne 'B3955E859FEEE605CF10248F3B5808B55925FB3FF8BED7454C1CC1C394CADA52')) {
        throw 'Actual successful import evidence changed.'
    }
}
Assert-NamedSupersession
$attempt = Join-Path $runRoot $(if($Mode -eq 'Settings'){'native-settings-attempt-03.json'}elseif($Mode -eq 'Import'){'fern-import-attempt-02.json'}else{'fern-render-attempt-01.json'})
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
if($Mode -ne 'Settings') {
    $buildReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-native-build-01\receipt.json'
    $buildReceiptHash='86DEE9CA2EA8CCC3CF6EC810F8F4967A60BBE249F3622032DEE3356D07821F6F'
}
$pythonPins = @{
    'python3.dll'='3C7ECFB999333AAF5BA9DDF4C5BFB8676B63CFCEC3DC5370CBC255A83063962F'
    'python311.dll'='3E5A5C012CDDB3D156D147ACAD59BB489C0716B87DAD274CB5BF20EEC3B68192'
}
if ((Get-FileHash $buildReceiptPath).Hash -cne $buildReceiptHash) { throw 'Accepted native build receipt differs.' }
$acceptedBuild = Get-Content $buildReceiptPath -Raw | ConvertFrom-Json
$supervisorReceipt=$null;$supervisorReceiptHash=$null
if($Mode -ne 'Settings') {
    $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-supervisor-02\receipt.json'
    $supervisorReceiptHash='7D930D038282AB18A14855EDA4E798B2847CC1DEAF7723F69B46B87099E9C761'
    if((Get-FileHash $supervisorReceiptPath).Hash -cne $supervisorReceiptHash){throw 'Supervisor revision receipt differs.'}
    $supervisorReceipt=Get-Content $supervisorReceiptPath -Raw|ConvertFrom-Json
}
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
        if($Mode -ne 'Settings') {
            foreach($sourcePin in $acceptedBuild.sources) {
                if($sourcePin.path -ceq 'Scripts\AuthoringLeafGuard.cs'){continue}
                if((Get-FileHash (Join-Path $root $sourcePin.path)).Hash -cne $sourcePin.sha256){throw "Built fern source/guard changed:$($sourcePin.path)"}
            }
            if((Get-FileHash $supervisorReceiptPath).Hash -cne $supervisorReceiptHash -or
                (Get-FileHash $PSCommandPath).Hash -cne $wrapperHash){throw 'Supervisor revision changed.'}
            foreach($pin in @($supervisorReceipt.supervisorSources)+@($supervisorReceipt.originalAttemptFiles)) {
                if((Get-FileHash (Join-Path $root $pin.path)).Hash -cne $pin.sha256){throw "Reviewed supervisor or original failed attempt changed:$($pin.path)"}
            }
        }
    }
    foreach ($name in $pythonPins.Keys) {
        if ((Get-FileHash (Join-Path (Split-Path (Split-Path $engine -Parent) -Parent) "Binaries\ThirdParty\Python3\Win64\$name")).Hash -cne $pythonPins[$name]) {
            throw 'Installed Python dependency identity differs.'
        }
    }
}
Assert-AcceptedNativeProducts
$trial=Join-Path $root 'Content\Trials\Fern02_20260920_01'
$trialIdentity=$null;$trialBefore=@();$fernResult=$null;$fernInputs=@();$fernSourceInventory=$null
$contentBefore=@();$assetAdmission=$null
if($Mode -ne 'Settings') {
    $sourceReceiptPath=Join-Path $root 'Assets\Environment\woodland-preparation-01\download-receipt.json'
    $inventoryPath=Join-Path $root 'Assets\Environment\woodland-preparation-01\source-inventory.json'
    if((Get-FileHash $sourceReceiptPath).Hash -cne 'AACF677F7B8CA07A5656842EA80AFEDC80845E509AFE8731F198DA973F339ECF' -or
        (Get-FileHash $inventoryPath).Hash -cne 'B38DCE70440F97817B3A8C8372D25EFC6B72BEE47E7078FC1D8ED207C1C0AAF4') {
        throw 'Reviewed source receipt/inventory changed.'
    }
    $fernInputs=@((Get-Content $sourceReceiptPath -Raw|ConvertFrom-Json).files|Where-Object asset -CEQ 'fern_02')
    $fernSourceInventory=((Get-Content $inventoryPath -Raw|ConvertFrom-Json).files|
        Where-Object { $_.asset -ceq 'fern_02' -and $_.file -ceq 'fern_02_1k.fbx' }).sourceInspection
    $sourceRoot=Join-Path $root 'Assets\Source\woodland-preparation-20260920-182217-d1f84e39\fern_02'
    if($fernInputs.Count -ne 6 -or @(Get-ChildItem $sourceRoot -Force).Count -ne 6 -or
        @($fernSourceInventory.externalTextureReferencesNotFollowed).Count -or
        @($fernSourceInventory.objectTypes.PSObject.Properties|Where-Object Name -NotIn @('Geometry','Model','Material')).Count) {
        throw 'Exact reference-free six-file fern whitelist differs.'
    }
    Assert-FernOrdinaryTree $sourceRoot
    foreach($inputFile in $fernInputs) {
        $file=Join-Path $sourceRoot $inputFile.file
        if((Get-Item $file).Length -ne $inputFile.bytes -or (Get-FileHash $file).Hash -cne $inputFile.sha256){throw 'Original fern source changed.'}
    }
    Assert-FernOrdinaryTree (Join-Path $root 'Content')
    $contentBefore=@(Get-ChildItem (Join-Path $root 'Content') -Recurse -File -Force |
        Where-Object { -not $_.FullName.StartsWith($trial+'\',[StringComparison]::OrdinalIgnoreCase) } |
        ForEach-Object { @{path=$_.FullName;sha256=(Get-FileHash $_.FullName).Hash} })
    if($Mode -eq 'Import') {
        if(Test-Path -LiteralPath $trial){throw 'Fresh fern trial namespace required; no overwrite/reuse.'}
        $failed=Get-Content (Join-Path $runRoot 'fern-import-01\probe-result.json') -Raw|ConvertFrom-Json
        $quarantine=Join-Path $runRoot 'fern-import-01\discarded-content'
        Assert-FernDirectoryIdentity $failed.trialIdentity $quarantine
        if(@(Get-ChildItem $quarantine -Force).Count){throw 'Only the verifiably empty original trial may be replaced.'}
    } else {
        $priorImport=Get-Content (Join-Path $runRoot 'fern-import-02\probe-result.json') -Raw|ConvertFrom-Json
        $assetAdmission=Get-Content (Join-Path $runRoot 'fern-import-02\asset-admission.json') -Raw|ConvertFrom-Json
        if($priorImport.status -cne 'passed' -or -not $priorImport.subjectExited -or $priorImport.hardTerminated){throw 'Render requires passed actual import.'}
        Assert-FernNativeInventory $assetAdmission.inventory $fernSourceInventory
        $trialIdentity=$assetAdmission.directoryIdentity
        Assert-FernDirectoryIdentity $trialIdentity $trial
        $trialBefore=@(Get-FernPackageFiles $trial -Complete)
        Assert-FernPackagePins @($assetAdmission.packages) $trialBefore
    }
}
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
$configs = [ordered]@{}
foreach ($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')) {
    $configs[$name] = Join-Path $output "Config\$name.ini"
}
$ddc = Join-Path $output 'DDC'
if($Mode -eq 'Render'){$ddc=Join-Path $runRoot 'fern-import-02\DDC';Assert-FernOrdinaryTree $ddc}
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
if($Mode -ne 'Settings'){$arguments+="-FernMode=$Mode"}
if($Mode -eq 'Render'){$arguments=@($arguments|Where-Object {$_ -cne '-nullrhi'})+@('-AllowCommandletRendering','-RenderOffScreen')}
$environment = [Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP'] = Join-Path $output 'Temp'
$environment['TMP'] = Join-Path $output 'Temp'
$environment['UE_PYTHONPATH'] = $null
$environment['UE_PIPINSTALL_PATH'] = Join-Path $output 'PipMustRemainAbsent'
$environment['UE_SKIP_UBT_SDK_SETUP'] = '1'
$environment['UE-LocalDataCachePath'] = $ddc
$environment['HOMESTEAD_PROBE_OUTPUT'] = $output
$environment['HOMESTEAD_PROBE_DEADLINE'] = [DateTimeOffset]::UtcNow.AddSeconds($hardSeconds).ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
if(($Mode -eq 'Render' -and ('-nullrhi' -in $arguments -or '-AllowCommandletRendering' -notin $arguments -or '-RenderOffScreen' -notin $arguments)) -or
    ($Mode -ne 'Render' -and '-nullrhi' -notin $arguments) -or '-DisablePython' -notin $arguments -or
    '-noshaderworker' -notin $arguments -or $configs.Count -ne 7 -or
    ($Mode -ne 'Settings' -and "-FernMode=$Mode" -notin $arguments)){throw 'Mode-specific launch configuration differs.'}
if ($ValidateOnly) {
    [ordered]@{eligibleForGuardConstruction=$true;moduleSha256=$moduleHash;buildReceiptSha256=$buildReceiptHash
        supervisorReceiptSha256=$supervisorReceiptHash;creationFlags=0x0008040C;mode=$Mode;deadlineProfile=$deadlineProfile
        softMilliseconds=$softSeconds*1000;hardMilliseconds=$hardSeconds*1000;ceilingSeconds=$ceilingSeconds
        nativeAbsoluteDeadline=$environment['HOMESTEAD_PROBE_DEADLINE'];ddc=$ddc;arguments=$arguments
        output=$output;globalMarkerReadLocked=$false;runtimeVerified=$false;attemptConsumed=$false}
    return
}
$null=New-Item -ItemType Directory -Path $output,(Join-Path $output 'Config'),(Join-Path $output 'Temp'),
    (Join-Path $output 'EngineUser'),(Join-Path $output 'DDC')
foreach($name in $configs.Keys){[IO.File]::WriteAllText($configs[$name],'')}
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
    if (-not $state.allowWork -or $state.id -ne $run.id -or @(Get-AuthoringProcesses).Count -or
        [DateTimeOffset]::UtcNow.AddSeconds($ceilingSeconds+60) -ge [DateTimeOffset]$state.deadlineUtc) { throw 'Admission changed before guard construction.' }
    Assert-ExistingNetworkPermission
    Assert-AcceptedNativeProducts
    Assert-NamedSupersession
    [Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile($softSeconds*1000,$hardSeconds*1000,$deadlineProfile)
    $reservation = [IO.File]::Open($attempt,'CreateNew','Write','Read')
    try {
        $bytes=[Text.Encoding]::UTF8.GetBytes((@{utc=$started.ToString('o');output=$output;runId=$run.id;supersession=$supersession
            mode=$Mode;supervisorReceiptSha256=$supervisorReceiptHash
            priorFernReservationSha256='3D799DBE1E344B703B468A7D5016E082C3271700A6C2E50D073EDD3BBC1508C2'
            priorFernResultSha256='6D6F56576EE6ECCABF0918B99D6168E7C410213387ED11EADE11FB00A49D3910'
            approval='Coordinator explicit replacement import02 after exact deadline-profile regression; original failed-before-resume attempt immutable. Conditional single render reads import02.'}|ConvertTo-Json))
        $reservation.Write($bytes)
    } finally { $reservation.Dispose() }
    if($Mode -eq 'Import') {
        if(Test-Path -LiteralPath $trial){throw 'Trial appeared before reservation.'}
        $null=New-Item -ItemType Directory -Path $trial
        $trialIdentity=[Homestead.Authoring.LeafGuard]::InspectDirectory($trial)
    }
    $environment['HOMESTEAD_PROBE_DEADLINE']=[DateTimeOffset]::UtcNow.AddSeconds($hardSeconds).ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
    $guard = [Homestead.Authoring.LeafGuard]::new($exe,$approved['UnrealEditor-Cmd.exe'],[string[]]$arguments,
        $root,$markerPath,(Join-Path $output 'stdout.log'),$environment,$true)
    if ($guard.CreationFlags -ne 0x0008040C -or $guard.WhitelistedHandleCount -ne 3) {
        throw 'Approved Editor detached-console/stdio creation policy differs.'
    }
    $guard.ArmDeadline($softSeconds*1000,$hardSeconds*1000,$stopPath,$deadlineProfile)
    if($guard.DeadlineProfile -cne $deadlineProfile -or $guard.SoftDeadlineMilliseconds -ne $softSeconds*1000 -or
        $guard.HardDeadlineMilliseconds -ne $hardSeconds*1000){throw 'Armed production deadline profile differs.'}
    $null = Read-ProbeJob 'suspended-before-resume'
    [ordered]@{pid=$guard.ProcessId;creationTime=$guard.ProcessCreationTime;image=$guard.ImagePath
        executableSha256=$approved['UnrealEditor-Cmd.exe'];moduleSha256=$moduleHash;arguments=$arguments
        markerBefore=$guard.MarkerBefore;job=$guard.LastVerifiedJob;explicitInheritedHandles=$guard.WhitelistedHandleCount
        expectedRules=$expectedRules;deadlineUtc=$environment['HOMESTEAD_PROBE_DEADLINE']
        creationFlags=$guard.CreationFlags;buildReceiptSha256=$buildReceiptHash;productPins=$productPins;supersession=$supersession
        buildMonitoringQualification=$acceptedBuild.qualification
        mode=$Mode;softSeconds=$softSeconds;hardSeconds=$hardSeconds;ceilingSeconds=$ceilingSeconds;trialIdentity=$trialIdentity
        deadlineProfile=$guard.DeadlineProfile;supervisorReceiptSha256=$supervisorReceiptHash;wrapperSha256=$wrapperHash
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
            if($Mode -ne 'Settings') {
                Assert-FernDirectoryIdentity $trialIdentity $trial
                [IO.File]::WriteAllText((Join-Path $output 'operation-admitted.txt'),$Mode)
            }
        }
        if($Mode -ne 'Settings' -and -not $fernResult -and (Test-Path (Join-Path $output 'fern-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'fern-result.json') -Raw|ConvertFrom-Json
            Assert-FernNativeInventory $fernResult $fernSourceInventory
            if($Mode -eq 'Import'){$null=Get-FernPackageFiles $trial -Complete}
            else{Assert-FernRenderImages $fernResult $output}
        }
        if ($null -ne $readyAt -and (($Mode -eq 'Settings' -and $clock.Elapsed.TotalSeconds - $readyAt -ge 10 -and $samples.Count -ge 10) -or
            ($Mode -ne 'Settings' -and $fernResult))) {
            Request-ProbeStop 'complete'
        }
        if ($clock.Elapsed.TotalSeconds -gt $softSeconds+5) { throw 'Bounded authoring operation timeout.' }
        Start-Sleep -Milliseconds 100
    }
    if (-not $native -or $guard.ExitCode -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw 'Native settings probe failed or was hard-terminated.' }
    $exit = Get-Content -LiteralPath (Join-Path $output 'native-exit.json') -Raw | ConvertFrom-Json
    if (-not $exit.passed -or -not $exit.cooperative -or $exit.stopReason.Trim() -ne 'complete') { throw 'Native cooperative stop was not proved.' }
    Assert-AuthoringPythonState $exit.pythonExit
    if (Test-Path -LiteralPath $environment['UE_PIPINSTALL_PATH']) { throw 'Disabled Python unexpectedly touched the pip output.' }
    if($Mode -ne 'Settings' -and -not $fernResult){throw 'Native fern operation has no verified inventory.'}
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
    if($Mode -ne 'Settings') {
        try {
            foreach($inputFile in $fernInputs) {
                if((Get-FileHash (Join-Path $sourceRoot $inputFile.file)).Hash -cne $inputFile.sha256){throw 'Original fern input changed during operation.'}
            }
            $contentAfter=@(Get-ChildItem (Join-Path $root 'Content') -Recurse -File -Force |
                Where-Object {-not $_.FullName.StartsWith($trial+'\',[StringComparison]::OrdinalIgnoreCase)})
            if($contentAfter.Count -ne $contentBefore.Count){throw 'Unexpected Content output outside the exact trial.'}
            foreach($file in $contentBefore){if((Get-FileHash $file.path).Hash -cne $file.sha256){throw "Existing content changed:$($file.path)"}}
            if($trialIdentity) {
                Assert-FernDirectoryIdentity $trialIdentity $trial
                if($Mode -eq 'Render'){Assert-FernPackagePins $trialBefore @(Get-FernPackageFiles $trial -Complete)}
            }
        } catch {$cleanupErrors.Add("Fern protection check failed:$_")}
    }
    if ($cleanupErrors.Count) { $failure=(@($failure)+@($cleanupErrors) | Where-Object { $_ }) -join ' | ' }
    if($Mode -eq 'Import' -and $trialIdentity) {
        try {
            Assert-FernDirectoryIdentity $trialIdentity $trial
            if($failure -and ($subjectExited -or -not $guard)) {
                $partial=@(Get-ChildItem $trial -Recurse -File -Force|ForEach-Object {
                    @{path=[IO.Path]::GetRelativePath($trial,$_.FullName);bytes=$_.Length;sha256=(Get-FileHash $_.FullName).Hash}
                })
                $partial|ConvertTo-Json -Depth 5|Set-Content (Join-Path $output 'partial-content.json')
                [IO.Directory]::Move($trial,(Join-Path $output 'discarded-content'))
                Assert-FernDirectoryIdentity $trialIdentity (Join-Path $output 'discarded-content')
            } elseif(-not $failure) {
                @{inventory=$fernResult;directoryIdentity=$trialIdentity;packages=@(Get-FernPackageFiles $trial -Complete);
                    sourceInputs=$fernInputs;sourceGraph=$fernSourceInventory}|ConvertTo-Json -Depth 20|Set-Content (Join-Path $output 'asset-admission.json')
            }
        } catch {$failure=(@($failure,"Fern disposition failed:$_")|Where-Object {$_}) -join ' | '}
    }
    if($Mode -eq 'Render' -and $failure -and ($subjectExited -or -not $guard)) {
        try {
            foreach($name in @('fern-a-front.png','fern-a-back.png')) {
                $image=Join-Path $output $name
                if(Test-Path -LiteralPath $image) {
                    Assert-FernOrdinaryTree $output
                    $discard=Join-Path $output 'discarded-images'
                    if(-not(Test-Path $discard)){$null=New-Item -ItemType Directory -Path $discard}
                    [IO.File]::Move($image,(Join-Path $discard $name))
                }
            }
        } catch {$failure=(@($failure,"Partial image quarantine failed:$_")|Where-Object {$_}) -join ' | '}
    }
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
        mode=$Mode;fernInventory=$fernResult;trialIdentity=$trialIdentity;softSeconds=$softSeconds;hardSeconds=$hardSeconds
        supervisorReceiptSha256=$supervisorReceiptHash;wrapperSha256=$wrapperHash
        limits='One specifically reserved settings/import/offscreen-render operation. Sampled endpoints/processes, not continuous tracing or filesystem/network isolation. No cook/Pak/Shipping/4K/performance proof. Never click security Allow.'
    } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $output 'probe-result.json')
}
if ($failure) { throw $failure }
"Native settings/stop probe passed:$output"
