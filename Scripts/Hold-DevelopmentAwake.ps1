[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RunId,
    [string]$StateDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Automation'),
    [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
$control=Join-Path $PSScriptRoot 'Development-Run.ps1'
function Get-AwakePolicy {
    $run=& $control -Action Status -StateDirectory $StateDirectory
    $continue=$run.id -eq $RunId -and $run.state -in @('running','paused') -and -not $run.deadlinePassed
    $remaining=([DateTimeOffset]$run.deadlineUtc-[DateTimeOffset]::UtcNow).TotalSeconds
    [pscustomobject]@{continue=$continue;holding=($continue -and $run.allowWork);completionPolicy=$run.completionPolicy;
        deadlineUtc=$run.deadlineUtc;sleepMilliseconds=$(if($run.completionPolicy -eq 'until-complete'){15000}else{[int][Math]::Max(1,[Math]::Min(15000,$remaining*1000))})}
}
if($ValidateOnly){Get-AwakePolicy;return}
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class DevelopmentAwake {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern uint SetThreadExecutionState(uint flags);
}
'@
$receipt = Join-Path $StateDirectory 'awake.json'
$holding = $false
try {
    while ($true) {
        $policy=Get-AwakePolicy
        if(-not $policy.continue){break}
        $holding=$policy.holding
        $flags = if ($holding) { [uint32]2147483649 } else { [uint32]2147483648 }
        if ([DevelopmentAwake]::SetThreadExecutionState($flags) -eq 0) {
            throw "System-awake request failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())."
        }
        [ordered]@{
            runId = $RunId; pid = $PID; holding = $holding
            updatedUtc = [DateTimeOffset]::UtcNow.ToString('o'); deadlineUtc = $policy.deadlineUtc
            completionPolicy=$policy.completionPolicy
        } | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
        Start-Sleep -Milliseconds $policy.sleepMilliseconds
    }
} finally {
    $null = [DevelopmentAwake]::SetThreadExecutionState([uint32]2147483648)
    [ordered]@{
        runId = $RunId; pid = $PID; holding = $false
        updatedUtc = [DateTimeOffset]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
}
