[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$ExportActions,
    [switch]$WriteMetadataOnly,
    [string]$ProjectDirectory = (Split-Path $PSScriptRoot -Parent),
    [switch]$ShippingActions,
    [switch]$HairWaveCandidate,
    [switch]$WardrobeCandidate
)
$ErrorActionPreference = 'Stop'
$authorityRoot = Split-Path $PSScriptRoot -Parent
if($HairWaveCandidate -and (-not $ShippingActions -or -not $WriteMetadataOnly)){throw 'Hair-wave metadata requires the genuine Shipping metadata path.'}
if($WardrobeCandidate -and (-not $ShippingActions -or -not $WriteMetadataOnly -or $HairWaveCandidate)){throw 'Wardrobe metadata requires exclusive genuine Shipping metadata mode.'}
$shippingBuildName=if($WardrobeCandidate){'wardrobe-shipping-build-01'}elseif($HairWaveCandidate){'hair-shipping-build-01'}else{'clearing-shipping-build-03'}
$shippingLinkFolder=if($WardrobeCandidate){'link2'}else{'link1'}
$manifestAttempt=if($HairWaveCandidate -or $WardrobeCandidate){'manifest-01'}else{'manifest-02'}
$root = [IO.Path]::GetFullPath($ProjectDirectory).TrimEnd('\')
if ($root -ine $authorityRoot -and
    (-not $ExportActions -or $WriteMetadataOnly -or
     $root -ine 'E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-solid-memory')) {
    throw 'Only the coordinated isolated UI worktree action export is admitted.'
}
if ($ShippingActions -and ((-not $ExportActions -and -not $WriteMetadataOnly) -or $root -ine $authorityRoot)) {
    throw 'Shipping admits only action export or verified metadata-only writing.'
}
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or ($run.completionPolicy -ne 'until-complete' -and
    [DateTimeOffset]::UtcNow.AddMinutes(12) -ge [DateTimeOffset]$run.deadlineUtc)) {
    throw 'Run does not permit a bounded local Editor-module build.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
if (-not $output.StartsWith((Join-Path $root "Saved\Automation\$($run.id)") + '\') -or (Test-Path -LiteralPath $output)) {
    throw 'Use a fresh current-run build-evidence directory.'
}
$null = New-Item -ItemType Directory -Path $output,(Join-Path $output 'Temp'),(Join-Path $output 'DotNetHome')
$engine = 'E:\Program Files\UE_5.8'
$dotnet = Join-Path $engine 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
$ubt = Join-Path $engine 'Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll'
$compiler = 'E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64'
$sdk = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64'
$allowed = @($dotnet, "$compiler\cl.exe", "$compiler\link.exe", "$compiler\cvtres.exe",
    "$compiler\mspdbsrv.exe", "$sdk\rc.exe", "$env:SystemRoot\System32\cmd.exe", "$env:SystemRoot\System32\conhost.exe")
$ispc = Join-Path $engine 'Engine\Source\ThirdParty\Intel\ISPC\bin\Windows\ispc.exe'
if ($ExportActions) {
    if ((Get-FileHash $ispc).Hash -cne 'D2F6C922DAE9257293615AD453EE79789F76B0DB895F129457F80058C2EAB874' -or
        (Get-Item $ispc).Length -ne 97822136 -or (Get-AuthenticodeSignature $ispc).Status -ne 'Valid') {
        throw 'Approved export-only ISPC version-query identity differs.'
    }
    $allowed += $ispc
}
if ($WriteMetadataOnly) {
    if ($ExportActions) { throw 'Metadata and build-export modes are exclusive.' }
    $allowed = @($dotnet, "$env:SystemRoot\System32\conhost.exe")
    $metadataPath = Join-Path $root 'Intermediate\Build\Win64\x64\SurvivalGameEditor\Development\TargetMetadata.json'
    $metadataHash = '2550557136B7AE6F947A2746AFFBE1FEC6538735D296EB7E342B549120294924'
    if($ShippingActions) {
        $metadataPath=Join-Path $root 'Intermediate\Build\Win64\x64\SurvivalGame\Shipping\TargetMetadata.json'
        $metadataHash='EB8CF0F674F80C64F5FC0AB400A4EC834BCA07059C8E651FA542BC61D5A28522'
    }
    if ((Get-FileHash $metadataPath).Hash -cne $metadataHash -or
        (Get-FileHash $dotnet).Hash -cne '0AF909A3DB0C02BD736F3008E2A9B20E7BA4E87EF27DD35619A7A9EC588191A8' -or
        (Get-FileHash $ubt).Hash -cne 'A513EE9E22291D8827C5390C1ED5BBAD7AC564DA6CF82F3BD57B4E27509E7182') {
        throw 'Approved metadata input/tool identity differs.'
    }
    $metadata = Get-Content $metadataPath -Raw | ConvertFrom-Json
    $versionPath = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.version'
    $versionHash = '2C94D8C30DF424622504FF9CBEBB1A6DE62E3083BEE0910394DDF4BDE0A3840F'
    $manifestPath = Join-Path $root 'Binaries\Win64\UnrealEditor.modules'
    $receiptPath = Join-Path $root 'Binaries\Win64\SurvivalGameEditor.target'
    if($ShippingActions) {
        $receiptPath=Join-Path $root 'Binaries\Win64\SurvivalGame-Win64-Shipping.target'
        if($null -ne $metadata.Version -or $null -ne $metadata.VersionFile -or $metadata.ReceiptFile -cne $receiptPath -or
            $metadata.Receipt.TargetName -cne 'SurvivalGame' -or $metadata.Receipt.Configuration -cne 'Shipping' -or
            $metadata.Receipt.Platform -cne 'Win64' -or $metadata.Receipt.Version.BuildId -cne '55116800' -or
            @($metadata.FileToManifest.PSObject.Properties).Count -or @($metadata.FileToLoadOrderManifest.PSObject.Properties).Count) {
            throw 'Shipping metadata is not the genuine project-only monolithic receipt.'
        }
    } elseif ($null -ne $metadata.Version -or $metadata.VersionFile -cne $versionPath -or
        $metadata.ReceiptFile -cne $receiptPath -or (Get-FileHash $versionPath).Hash -cne $versionHash -or
        $metadata.Receipt.Version.BuildId -cne '55116800' -or
        @($metadata.FileToManifest.PSObject.Properties).Count -ne 1 -or
        @($metadata.FileToManifest.PSObject.Properties)[0].Name -cne $manifestPath -or
        @($metadata.FileToLoadOrderManifest.PSObject.Properties).Count -ne 0) {
        throw 'Metadata engine-version or project-only write-map condition differs.'
    }
    $products = @()
    $modules=if($ShippingActions){@('SurvivalGame-Win64-Shipping')}else{@('SurvivalGame','SurvivalGameEditor')}
    foreach ($module in $modules) {
        $dll = Join-Path $root $(if($ShippingActions){"Binaries\Win64\$module.exe"}else{"Binaries\Win64\UnrealEditor-$module.dll"})
        $pdb = [IO.Path]::ChangeExtension($dll, '.pdb')
        $inspectionArgs=@($dll,$pdb)
        if($ShippingActions) {
            $inspectionArgs+=@('--executable','--manifest-input',
                (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\$shippingLinkFolder\link-generated.manifest"),
                '--manifest-input',(Join-Path $engine 'Engine\Build\Windows\Resources\Default-Win64.manifest'))
            $embedded=Get-Content (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\$manifestAttempt\result.json") -Raw|ConvertFrom-Json
            if($embedded.status -cne 'passed' -or (Get-FileHash $dll).Hash -cne $embedded.objectSha256){throw 'Successful genuine manifest embedding is missing.'}
        }
        $inspection = & python (Join-Path $PSScriptRoot 'Inspect-NativeModule.py') @inspectionArgs
        if ($LASTEXITCODE -ne 0) { throw "Native module/PDB verification failed:$module" }
        $product = $inspection | ConvertFrom-Json
        if ($product.moduleName -cne [IO.Path]::GetFileName($dll) -or
            ($module -eq 'SurvivalGameEditor' -and @($product.probeExports).Count -ne 1)) {
            throw 'Linked module identity or commandlet export differs.'
        }
        $products += $product
        if($ShippingActions){continue}
        $library = Join-Path $root "Intermediate\Build\Win64\x64\UnrealEditor\Development\$module\UnrealEditor-$module.lib"
        $stream = [IO.File]::OpenRead($library)
        try { $header = [byte[]]::new(8); $stream.ReadExactly($header) } finally { $stream.Dispose() }
        if ([Text.Encoding]::ASCII.GetString($header) -cne "!<arch>`n") { throw 'Invalid actual import library.' }
    }
    $products | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'native-products.json')
    $metadataOutputs=if($ShippingActions){@($receiptPath)}else{@($receiptPath,$manifestPath)}
    foreach ($path in $metadataOutputs) {
        if (Test-Path -LiteralPath $path) { Copy-Item -LiteralPath $path -Destination (Join-Path $output ([IO.Path]::GetFileName($path)+'.before')) }
    }
}
if ((Get-FileHash "$env:SystemRoot\System32\conhost.exe").Hash -cne 'E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5' -or
    (Get-AuthenticodeSignature "$env:SystemRoot\System32\conhost.exe").Status -ne 'Valid') {
    throw 'The separately approved build-only console-host identity differs.'
}
$identities = @($allowed + $ubt | ForEach-Object {
    $file = Get-Item -LiteralPath $_
    [ordered]@{ path = $file.FullName; sha256 = (Get-FileHash -LiteralPath $_).Hash; bytes = $file.Length }
})
$targetName=if($ShippingActions){'SurvivalGame'}else{'SurvivalGameEditor'}
$configuration=if($ShippingActions){'Shipping'}else{'Development'}
$arguments = @($ubt,$targetName,'Win64',$configuration,"-Project=$(Join-Path $root 'SurvivalGame.uproject')",
    '-WaitMutex','-NoHotReloadFromIDE','-NoUBA','-NoXGE','-NoFASTBuild','-NoSNDBS','-NoArtifactReads',
    '-NoArtifactWrites','-NoEngineChanges',"-Log=$(Join-Path $output 'ubt.log')")
if ($ExportActions) { $arguments += "-WriteOutdatedActions=$(Join-Path $output 'actions.json')" }
if ($WriteMetadataOnly) {
    $arguments = @($ubt,'-Mode=WriteMetadata',"-Input=$metadataPath",'-Version=2',"-Log=$(Join-Path $output 'ubt.log')")
}
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$start = [Diagnostics.ProcessStartInfo]::new($dotnet)
$start.UseShellExecute = $false
$start.WorkingDirectory = Join-Path $engine 'Engine\Source'
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$start.CreateNoWindow = $true
foreach ($argument in $arguments) { $start.ArgumentList.Add($argument) }
$start.Environment['DOTNET_CLI_TELEMETRY_OPTOUT'] = '1'
$start.Environment['DOTNET_SKIP_FIRST_TIME_EXPERIENCE'] = '1'
$start.Environment['DOTNET_CLI_HOME'] = Join-Path $output 'DotNetHome'
if ($ExportActions) { $start.Environment['UnrealBuildTool_SourceFileWorkingSet__Provider'] = 'None' }
$null = $start.Environment.Remove('UBT_EXTRA_ARGS')
$start.Environment['TEMP'] = Join-Path $output 'Temp'
$start.Environment['TMP'] = Join-Path $output 'Temp'
$process = [Diagnostics.Process]::Start($start)
$null = $process.Handle
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
$owned = [Collections.Generic.Dictionary[int,object]]::new()
$owned.Add($process.Id, $process)
$observed = [Collections.Generic.List[object]]::new()
$samples = [Collections.Generic.List[object]]::new()
$clock = [Diagnostics.Stopwatch]::StartNew()
$failure = $null
try {
    [ordered]@{ runId = $run.id; rootPid = $process.Id; startedUtc = $process.StartTime.ToUniversalTime().ToString('o')
        arguments = $arguments; identities = $identities; leafJobApplied = $false
        sourceWorkingSetProvider=$(if($ExportActions){'None (operation-local environment XML configuration)'}else{'Not queried by metadata-only mode'})
        limitation = 'Observational sampled build processes/endpoints, not a preventive child allowlist or complete history. Export-only ISPC --version is admitted; no ISPC compilation, runtime or remote executor.'
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
    while (-not $process.HasExited) {
        if ($clock.Elapsed.TotalMinutes -gt 10) { throw 'Local build exceeded ten minutes.' }
        $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Live run stopped during build.' }
        $filter = (@($owned.Keys) | ForEach-Object { "ParentProcessId=$_" }) -join ' OR '
        foreach ($child in Get-CimInstance Win32_Process -Filter $filter) {
            if ($owned.ContainsKey([int]$child.ProcessId)) { continue }
            if ($child.ExecutablePath -ieq $ispc -and (-not $ExportActions -or
                $child.CommandLine -notmatch '^"?[^"]*ispc\.exe"?\s+--version\s*$')) {
                throw 'Only the source-established export ISPC --version command is admitted.'
            }
            try { $held = [Diagnostics.Process]::GetProcessById($child.ProcessId) }
            catch [ArgumentException] {
                $observed.Add(@{ pid = $child.ProcessId; parentPid = $child.ParentProcessId; path = $child.ExecutablePath
                    observation = 'Exited before a process handle could be acquired; no retained-handle proof for this sample.' })
                if (-not $child.ExecutablePath -or $child.ExecutablePath -notin $allowed) { throw 'Unadmitted exited build child.' }
                continue
            }
            $null = $held.Handle
            if ($held.MainModule.FileName -ine $child.ExecutablePath -or
                $held.StartTime.ToUniversalTime() -lt $process.StartTime.ToUniversalTime()) {
                $held.Dispose(); throw 'Build child identity differs.'
            }
            $owned.Add([int]$child.ProcessId, $held)
            $observation = @{ pid = $child.ProcessId; parentPid = $child.ParentProcessId; path = $child.ExecutablePath
                startUtc = $held.StartTime.ToUniversalTime().ToString('o'); arguments = $child.CommandLine
                admitted = $false; sha256 = $null }
            $observed.Add($observation)
            if (-not $child.ExecutablePath -or $child.ExecutablePath -notin $allowed) {
                throw "Unadmitted local-build child PID$($child.ProcessId):$($child.ExecutablePath)"
            }
            $expected = $identities | Where-Object { $_.path -ieq $child.ExecutablePath }
            if ((Get-FileHash -LiteralPath $child.ExecutablePath).Hash -cne $expected.sha256) {
                throw 'Build child image changed.'
            }
            $observation.sha256 = $expected.sha256
            $observation.admitted = $true
        }
        $filter = (@($owned.Keys) | ForEach-Object { "OwningProcess=$_" }) -join ' OR '
        $tcp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter $filter |
            Select-Object OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
        $udp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter $filter |
            Select-Object OwningProcess,LocalAddress,LocalPort)
        $samples.Add(@{ elapsedMs = $clock.Elapsed.TotalMilliseconds; tcp = $tcp; udp = $udp })
        if ($tcp.Count -or $udp.Count) { throw 'Unexpected local-build network activity; no next operation admitted.' }
        Start-Sleep -Milliseconds 100
    }
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { throw "Direct local UBT failed:$($process.ExitCode)" }
    $exportCorroboration = $null
    if ($ExportActions) {
        $actual = Get-Content (Join-Path $output 'actions.json') -Raw | ConvertFrom-Json
        if (@($actual.Actions | Where-Object CommandPath -IEQ $ispc).Count) { throw 'ISPC compilation is not authorized.' }
        $log = $stdout.GetAwaiter().GetResult()
        if ($log -match 'Using ISPC compiler' -and $log -notmatch 'ISPC\), 1\.24\.0') { throw 'ISPC version output differs.' }
        $exportCorroboration = @{versionQueryReported=($log -match 'Using ISPC compiler')
            version='1.24.0';sampledIsNotCompleteHistory=$true
            source='ISPCToolChain.cs233-247 invokes --version through a process-local Lazy cache'
            observation='A log-confirmed brief version-query child may be absent from process samples; no retroactive identity/endpoint claim.'}
    }
    if ($WriteMetadataOnly) {
        if ((Get-FileHash $metadataPath).Hash -cne $metadataHash -or (Get-FileHash $versionPath).Hash -cne $versionHash) {
            throw 'Metadata input or installed engine version changed.'
        }
        $receipt = Get-Content $receiptPath -Raw | ConvertFrom-Json
        if($ShippingActions) {
            if($receipt.TargetName -cne 'SurvivalGame' -or $receipt.Configuration -cne 'Shipping' -or
                $receipt.Platform -cne 'Win64' -or $receipt.Version.BuildId -cne '55116800'){throw 'Actual Shipping receipt differs.'}
        } else {
            $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
            if ($manifest.BuildId -cne '55116800' -or $receipt.Version.BuildId -cne '55116800' -or
            $manifest.Modules.SurvivalGame -cne 'UnrealEditor-SurvivalGame.dll' -or
            $manifest.Modules.SurvivalGameEditor -cne 'UnrealEditor-SurvivalGameEditor.dll') {
                throw 'Actual UBT manifest/receipt differs from the linked module contract.'
            }
        }
        foreach ($product in $products) {
            $image=if($ShippingActions){$product.exe}else{$product.dll}
            foreach ($file in @($image,$product.pdb)) {
                if ((Get-FileHash $file.path).Hash -cne $file.sha256) { throw 'Linked product changed during metadata.' }
            }
        }
        @($metadataOutputs)+@($versionPath) | ForEach-Object {
            @{path=$_;sha256=(Get-FileHash $_).Hash}
        } | ConvertTo-Json | Set-Content (Join-Path $output 'metadata-products.json')
    }
} catch {
    $failure = $_.ToString()
    throw
} finally {
    foreach ($held in $owned.Values) {
        if (-not $held.HasExited) {
            if (-not $failure) { $failure = 'An observed build helper outlived the root; hard-stopped owned identity.' }
            Stop-Process -Id $held.Id
            if (-not $held.WaitForExit(5000)) { throw 'Owned build process did not exit.' }
        }
    }
    [IO.File]::WriteAllText((Join-Path $output 'stdout.log'), $stdout.GetAwaiter().GetResult())
    [IO.File]::WriteAllText((Join-Path $output 'stderr.log'), $stderr.GetAwaiter().GetResult())
    [ordered]@{ status = $(if ($failure) { 'failed' } else { 'passed' }); error = $failure
        exitCode = $process.ExitCode; elapsedSeconds = $clock.Elapsed.TotalSeconds
        observedChildren = $observed; samples = $samples; exportCorroboration=$exportCorroboration
    } | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'result.json')
    foreach ($held in $owned.Values) { $held.Dispose() }
}
if ($failure) { throw $failure }
Get-Content -LiteralPath (Join-Path $output 'stdout.log') -Tail 25
