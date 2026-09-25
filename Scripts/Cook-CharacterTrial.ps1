[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory, [switch]$VitruvianTrial,
    [switch]$CMUWalkTrial, [switch]$CMUHeadLevelTrial)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or $run.completionPolicy -cne 'until-complete') {
    throw 'The completion-driven authoring run is not active.'
}
if ($CMUHeadLevelTrial -and ($CMUWalkTrial -or $VitruvianTrial)) {
    throw 'The upright-head gait uses the original heroine; do not bundle the rejected face or prior walk.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$testRoot = [IO.Path]::GetFullPath((Join-Path $root 'Saved\Automation')) + '\'
if (-not $output.StartsWith($testRoot, [StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $output)) {
    throw 'Use a fresh isolated Saved\Automation character cook directory.'
}
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1')
$exe = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if ((Get-AuthenticodeSignature -LiteralPath $exe).Status -ne 'Valid') {
    throw 'Installed Unreal Editor commandlet is not signed.'
}
$exeHash = (Get-FileHash -LiteralPath $exe).Hash
$sourceFiles = @(
    'Config\DefaultGame.ini',
    'Content\Trials\HeroineSprint_20260924_01\Animations\AN_Heroine_Sprint.uasset',
    'Content\Trials\HeroineKnife_20260924_01\Animations\AN_Heroine_KnifeCut.uasset',
    'Content\Trials\HeroineIdle_20260924_15\Animations\AN_Heroine_LivingIdle02.uasset',
    'Binaries\Win64\UnrealEditor-SurvivalGame.dll',
    'Binaries\Win64\UnrealEditor-SurvivalGameEditor.dll'
)
$trialPackages = @()
if ($VitruvianTrial) {
    $trialRelative = 'Content\Trials\HeroineVitruvian_20260924_25'
    $trialRoot = Join-Path $root $trialRelative
    if (-not (Test-Path -LiteralPath (Join-Path $trialRoot 'SK_TrialVitruvian01_Preferred_Base_Bob.uasset'))) {
        throw 'The selected isolated face trial has no imported skeletal mesh.'
    }
    $trialPackages = @(Get-ChildItem -LiteralPath $trialRoot -Recurse -File -Filter '*.uasset' |
        ForEach-Object { $_.FullName.Substring($root.Length + 1) })
    if ($trialPackages.Count -ne 12) {
        throw 'The face trial must have exactly one mesh, seven materials and four texture assets.'
    }
    $sourceFiles += $trialPackages
}
$motionPackages = @()
if ($CMUWalkTrial) {
    if (-not $VitruvianTrial) { throw 'The human mocap trial requires the licensed fitted heroine.' }
    $motionPackages = @(
        'Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUNormalWalk01.uasset',
        'Content\Trials\HeroineCMUWalk_20260924_03\Animations\AN_Heroine_CMUSlowWalk01.uasset'
    )
    if (@($motionPackages | Where-Object { -not (Test-Path -LiteralPath (Join-Path $root $_) -PathType Leaf) }).Count) {
        throw 'The original licensed human-motion clips have not completed UE import.'
    }
    $sourceFiles += $motionPackages
}
if ($CMUHeadLevelTrial) {
    $motionPackages = @(
        'Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUNormalWalk02.uasset',
        'Content\Trials\HeroineCMUWalk_20260924_04\Animations\AN_Heroine_CMUSlowWalk02.uasset'
    )
    if (@($motionPackages | Where-Object { -not (Test-Path -LiteralPath (Join-Path $root $_) -PathType Leaf) }).Count) {
        throw 'The revised licensed walking clips have not completed UE import.'
    }
    $sourceFiles += $motionPackages
}
$pins = @($sourceFiles | ForEach-Object {
    $path = Join-Path $root $_
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing character cook input: $_" }
    [ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $path).Hash}
})
$authoringFilter = "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealPak.exe' OR Name='ShaderCompileWorker.exe' OR Name='CrashReportClientEditor.exe' OR Name='CrashReportClient.exe' OR Name='UnrealTraceServer.exe' OR Name='zenserver.exe' OR Name='VCTIP.EXE'"
$others = @(Get-CimInstance Win32_Process -Filter $authoringFilter)
if ($others.Count) { throw 'A competing authoring or compiler telemetry process prevents the guarded cook.' }

. (Join-Path $PSScriptRoot 'AuthoringProbePolicy.ps1')
. (Join-Path $PSScriptRoot 'FernSpikePolicy.ps1')
Add-Type -Path (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')
$policy = Get-FernOperationPolicy -Mode Cook -RenderProfile CompletionDriven
$marker = 'C:\ProgramData\Epic\NotAllowedUnattendedBugReports'
$before = [Homestead.Authoring.LeafGuard]::InspectMarker($marker)
$null = New-Item -ItemType Directory -Path $output, (Join-Path $output 'Config'),
    (Join-Path $output 'Temp'), (Join-Path $output 'EngineUser')
$ddc = Join-Path $output 'DDC'
$null = New-Item -ItemType Directory -Path $ddc
$configs = [ordered]@{}
foreach ($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings',
    'GameUserSettings','Game','Input')) {
    $configs[$name] = Join-Path $output "Config\$name.ini"
    [IO.File]::WriteAllText($configs[$name], '')
}
$args = @(Get-FernProbeArguments (Join-Path $root 'SurvivalGame.uproject') $output $ddc $configs 'Cook')
if ('-SkipZenStore' -notin $args -or '-noshaderworker' -notin $args -or
    '-DisablePython' -notin $args -or '-RunAsCookCommandlet' -notin $args -or
    '-nullrhi' -notin $args) {
    throw 'The reviewed single-process loose-file cook arguments differ.'
}
$environment = [Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP'] = Join-Path $output 'Temp'
$environment['TMP'] = Join-Path $output 'Temp'
$environment['UE_PYTHONPATH'] = $null
$environment['UE_PIPINSTALL_PATH'] = Join-Path $output 'PipMustRemainAbsent'
$environment['UE_SKIP_UBT_SDK_SETUP'] = '1'
$environment['UE-LocalDataCachePath'] = $ddc
$environment['HOMESTEAD_PROBE_OUTPUT'] = $output
$environment['HOMESTEAD_PROBE_COMPLETION_POLICY'] = 'until-complete'
$environment['HOMESTEAD_PROBE_DEADLINE'] = $null
$stop = Join-Path $output 'stop-probe.txt'
$guard = $null
$failure = $null
$cooked = $null
$settings = $null
$final = $null
$cleanup = [Collections.Generic.List[string]]::new()
$samples = [Collections.Generic.List[object]]::new()
try {
    $guard = [Homestead.Authoring.LeafGuard]::new($exe,$exeHash,[string[]]$args,
        $root,$marker,(Join-Path $output 'stdout.log'),$environment,$true)
    $guard.ArmDeadline(0,0,$stop,$policy.profile)
    $job = $guard.ObserveJobPolicy()
    Assert-AuthoringRootObservation $job @($guard.ObserveJobMembers()) $guard.ObserveHeldRoot() `
        $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime
    [ordered]@{
        pid=$guard.ProcessId;created=$guard.ProcessCreationTime;image=$exe;sha256=$exeHash
        arguments=$args;marker=$before;job=$job;sourcePins=$pins;output=$output;policy=$policy.profile
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
    $guard.Resume()
    while (-not $guard.Wait(0)) {
        $job = $guard.ObserveJobPolicy()
        Assert-AuthoringRootObservation $job @($guard.ObserveJobMembers()) $guard.ObserveHeldRoot() `
            $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime
        $null = $guard.VerifyMarker()
        $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($guard.ProcessId)")
        $other = @(Get-CimInstance Win32_Process -Filter $authoringFilter |
            Where-Object ProcessId -NE $guard.ProcessId)
        if ($children.Count -or $other.Count) { throw 'Unexpected child or helper process in guarded Cook.' }
        $tcp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$($guard.ProcessId)")
        $udp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$($guard.ProcessId)")
        Assert-AuthoringEndpoints $tcp $udp $guard.ProcessId
        $samples.Add([ordered]@{utc=[DateTimeOffset]::UtcNow.ToString('o');tcp=$tcp;udp=$udp;job=$job})
        $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -cne $run.id -or
            $state.completionPolicy -cne 'until-complete') {
            throw 'The authoring run stopped while cooking.'
        }
        if (-not $settings -and (Test-Path -LiteralPath (Join-Path $output 'effective-settings.json'))) {
            $settings = Get-Content -LiteralPath (Join-Path $output 'effective-settings.json') -Raw |
                ConvertFrom-Json
            if (-not $settings.valid -or $settings.pid -ne $guard.ProcessId -or
                [uint64]$settings.processCreationTime -ne $guard.ProcessCreationTime -or
                $settings.cachedSendReports -or $settings.cachedSendUsage -or
                $settings.jobFlags -ne 8200 -or $settings.jobProcessLimit -ne 1 -or
                -not $settings.completionDriven) {
                throw 'Native identity/privacy/job settings did not match the guarded Cook.'
            }
            Assert-AuthoringPythonState $settings.pythonEntry
            Assert-AuthoringDdc $settings.ddcStores $ddc
            Assert-AuthoringConfigBranches $settings.configBranches $output
            [IO.File]::WriteAllText((Join-Path $output 'operation-admitted.txt'),'Cook')
        }
        if ($settings -and -not $cooked -and
            (Test-Path -LiteralPath (Join-Path $output 'cook-result.json'))) {
            $cooked = Get-Content -LiteralPath (Join-Path $output 'cook-result.json') -Raw |
                ConvertFrom-Json
            Assert-HomesteadCookOutput $cooked $output -AdditionalPackages (@(
                'Content\Trials\HeroineSprint_20260924_01\Animations\AN_Heroine_Sprint.uasset',
                'Content\Trials\HeroineKnife_20260924_01\Animations\AN_Heroine_KnifeCut.uasset',
                'Content\Trials\HeroineIdle_20260924_15\Animations\AN_Heroine_LivingIdle02.uasset') +
                $trialPackages + $motionPackages)
            [IO.File]::WriteAllText($stop,'complete')
        }
        Start-Sleep -Milliseconds 100
    }
    if (-not $settings -or -not $cooked -or $guard.ExitCode -ne 0 -or
        $guard.HardTerminated -or $guard.DeadlineError) {
        throw 'Native character cook did not complete cleanly.'
    }
    $exit = Get-Content -LiteralPath (Join-Path $output 'native-exit.json') -Raw |
        ConvertFrom-Json
    if (-not $exit.passed -or -not $exit.cooperative -or $exit.stopReason.Trim() -ne 'complete') {
        throw 'Native cook did not cooperatively finish.'
    }
    Assert-AuthoringPythonState $exit.pythonExit
    if (Test-Path -LiteralPath $environment['UE_PIPINSTALL_PATH']) {
        throw 'Disabled Python touched the pip output.'
    }
} catch {
    $failure = $_.ToString()
} finally {
    if ($guard) {
        try {
            if (-not $guard.Wait(0)) {
                [IO.File]::WriteAllText($stop,'cancelled')
                if (-not $guard.Wait(3000)) { $guard.HardStop(96) }
            }
            if (-not $guard.Wait(5000)) { throw 'Cook process death was not observed.' }
            $final = $guard.CaptureExitedJob()
            Assert-AuthoringRootObservation $final @($guard.ObserveJobMembers()) $guard.ObserveHeldRoot() `
                $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime -Final
            $null = $guard.VerifyMarker()
            $guard.Dispose()
            $after = [Homestead.Authoring.LeafGuard]::InspectMarker($marker)
            if (($after | ConvertTo-Json -Compress) -cne ($before | ConvertTo-Json -Compress)) {
                throw 'Global marker changed after guarded Cook.'
            }
        } catch { $cleanup.Add($_.ToString()) }
    }
    foreach ($pin in $pins) {
        if ((Get-FileHash -LiteralPath (Join-Path $root $pin.path)).Hash -cne $pin.sha256) {
            $cleanup.Add("Cook changed protected source: $($pin.path)")
        }
    }
    [ordered]@{
        status=$(if (-not $failure -and -not $cleanup.Count -and $settings -and $cooked) {
            'passed'
        } else { 'failed' })
        error=$failure;cleanupErrors=@($cleanup);observations=$samples.Count
        cooked=$cooked;finalJob=$final;sourcePins=$pins
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'character-cook-result.json')
}
if ($failure -or $cleanup.Count -or -not $settings -or -not $cooked) {
    throw "Character Cook failed: $failure $($cleanup -join ' | ')"
}
Write-Output "Verified isolated loose Cook: $(Join-Path $output 'Cooked')"
