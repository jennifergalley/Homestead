[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$ExportActions
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or [DateTimeOffset]::UtcNow.AddMinutes(12) -ge [DateTimeOffset]$run.deadlineUtc) {
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
if ((Get-FileHash "$env:SystemRoot\System32\conhost.exe").Hash -cne 'E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5' -or
    (Get-AuthenticodeSignature "$env:SystemRoot\System32\conhost.exe").Status -ne 'Valid') {
    throw 'The separately approved build-only console-host identity differs.'
}
$identities = @($allowed + $ubt | ForEach-Object {
    $file = Get-Item -LiteralPath $_
    [ordered]@{ path = $file.FullName; sha256 = (Get-FileHash -LiteralPath $_).Hash; bytes = $file.Length }
})
$arguments = @($ubt,'SurvivalGameEditor','Win64','Development',"-Project=$(Join-Path $root 'SurvivalGame.uproject')",
    '-WaitMutex','-NoHotReloadFromIDE','-NoUBA','-NoXGE','-NoFASTBuild','-NoSNDBS','-NoArtifactReads',
    '-NoArtifactWrites','-NoEngineChanges',"-Log=$(Join-Path $output 'ubt.log')")
if ($ExportActions) { $arguments += "-WriteOutdatedActions=$(Join-Path $output 'actions.json')" }
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
        limitation = 'Sampled local build orchestration; no engine runtime or remote executor is authorized.'
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
    while (-not $process.HasExited) {
        if ($clock.Elapsed.TotalMinutes -gt 10) { throw 'Local build exceeded ten minutes.' }
        $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Live run stopped during build.' }
        $filter = (@($owned.Keys) | ForEach-Object { "ParentProcessId=$_" }) -join ' OR '
        foreach ($child in Get-CimInstance Win32_Process -Filter $filter) {
            if ($owned.ContainsKey([int]$child.ProcessId)) { continue }
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
        observedChildren = $observed; samples = $samples
    } | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'result.json')
    foreach ($held in $owned.Values) { $held.Dispose() }
}
if ($failure) { throw $failure }
Get-Content -LiteralPath (Join-Path $output 'stdout.log') -Tail 25
