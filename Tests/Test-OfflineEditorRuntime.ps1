[CmdletBinding()]
param(
    [ValidateRange(30,900)][int]$TimeoutSeconds = 300,
    [string]$OutputDirectory = 'Saved\Automation\20260921-033354-2d257ba0\offline-editor-runtime'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$null = New-Item -ItemType Directory -Path $output -Force
$started = [DateTimeOffset]::UtcNow
$deadline = $started.AddSeconds($TimeoutSeconds)
$editor = $null
while ([DateTimeOffset]::UtcNow -lt $deadline -and -not $editor) {
    $editor = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'" |
        Where-Object {
            $_.CommandLine -like '*E:\Repos\SurvivalGame\SurvivalGame.uproject*' -and
            $_.CommandLine -like '*-notraceserver*' -and
            $_.CommandLine -like '*-traceautostart=0*'
        } | Select-Object -First 1
    if (-not $editor) { Start-Sleep -Milliseconds 500 }
}
if (-not $editor) { throw "No hardened SurvivalGame Editor launch appeared within $TimeoutSeconds seconds." }
Start-Sleep -Seconds 6
if (-not (Get-Process -Id $editor.ProcessId -ErrorAction SilentlyContinue)) {
    throw "Hardened Editor process $($editor.ProcessId) exited before endpoint sampling."
}

$traceServers = @(Get-CimInstance Win32_Process -Filter "Name='UnrealTraceServer.exe'" |
    Where-Object { $_.CommandLine -match "(?i)(--sponsor|--owner-pid)\s+$($editor.ProcessId)(?:\s|$)" })
$tcp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection `
    -Filter "OwningProcess=$($editor.ProcessId)" -ErrorAction SilentlyContinue |
    Select-Object OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
$udp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint `
    -Filter "OwningProcess=$($editor.ProcessId)" -ErrorAction SilentlyContinue |
    Select-Object OwningProcess,LocalAddress,LocalPort)
$unexpectedTcp = @($tcp | Where-Object {
    $loopback = $_.LocalAddress -in @('127.0.0.1','::1','::') -and
        $_.RemoteAddress -in @('127.0.0.1','::1','::')
    $traceControl = $_.LocalPort -eq 1985 -and $_.State -eq 2
    -not ($loopback -or $traceControl)
})
$result = [ordered]@{
    observedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    editor = [ordered]@{
        processId = $editor.ProcessId
        executablePath = $editor.ExecutablePath
        commandLine = $editor.CommandLine
    }
    traceServerChildren = @($traceServers | Select-Object ProcessId,ParentProcessId,CommandLine)
    tcp = $tcp
    udp = $udp
    unexpectedTcp = $unexpectedTcp
    admittedHardCodedListener = @($tcp | Where-Object { $_.LocalPort -eq 1985 -and $_.State -eq 2 }).Count
    pass = $traceServers.Count -eq 0 -and $udp.Count -eq 0 -and $unexpectedTcp.Count -eq 0
    limitation = 'Unreal TraceLog opens in-process TCP TraceControl port 1985 unconditionally in this installed Development Editor. The project does not claim this listener is disabled.'
}
$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'probe.json')
if (-not $result.pass) {
    throw "Hardened Editor endpoint policy failed. See $output\probe.json."
}
Write-Output "PASS hardened Editor PID $($editor.ProcessId): no sponsored TraceServer, no UDP, no unexpected TCP; admitted TraceControl1985=$($result.admittedHardCodedListener)."
