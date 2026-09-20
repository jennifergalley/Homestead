[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RunId,
    [string]$StateDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Automation')
)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class DevelopmentAwake {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint SetThreadExecutionState(uint flags);
}
'@
$path = Join-Path $StateDirectory 'run.json'
$receipt = Join-Path $StateDirectory 'awake.json'
$holding = $false
try {
    while ($true) {
        $run = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
        $remaining = ([DateTimeOffset]::Parse($run.deadlineUtc) - [DateTimeOffset]::UtcNow).TotalSeconds
        if ($run.id -ne $RunId -or $run.state -eq 'stopped' -or $remaining -le 0) { break }
        $holding = $run.state -eq 'running'
        $flags = if ($holding) { [uint32]2147483649 } else { [uint32]2147483648 }
        if ([DevelopmentAwake]::SetThreadExecutionState($flags) -eq 0) {
            throw "System-awake request failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())."
        }
        [ordered]@{
            runId = $RunId; pid = $PID; holding = $holding
            updatedUtc = [DateTimeOffset]::UtcNow.ToString('o'); deadlineUtc = $run.deadlineUtc
        } | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
        Start-Sleep -Milliseconds ([int][Math]::Max(1, [Math]::Min(15000, $remaining * 1000)))
    }
} finally {
    $null = [DevelopmentAwake]::SetThreadExecutionState([uint32]2147483648)
    [ordered]@{
        runId = $RunId; pid = $PID; holding = $false
        updatedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
}
