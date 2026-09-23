[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$helper = Join-Path $root 'Scripts\Get-UnrealOfflineArguments.ps1'
$arguments = @(& $helper)
$required = @(
    '-notraceserver'
    '-traceautostart=0'
    '-DisablePlugins=UdpMessaging,TcpMessaging,AndroidFileServer'
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnabledByDefault=False'
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTransport=False'
    '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTunnel=False'
    '-ini:Engine:[/Script/TcpMessaging.TcpMessagingSettings]:EnableTransport=False'
    '-ini:Engine:[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]:bEnablePlugin=False'
    '-ini:Engine:[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]:bAllowNetworkConnection=False'
)
if ($arguments.Count -ne $required.Count) { throw 'Offline Editor argument count differs.' }
foreach ($argument in $required) {
    if ($argument -cnotin $arguments) { throw "Missing offline Editor argument: $argument" }
}
foreach ($script in @('Build-Game.ps1','Import-Characters.ps1','Import-Locomotion.ps1','Test-Game.ps1',
    'Playtest-Visual.ps1','Test-PreviewSaves.ps1','Test-VideoSync.ps1','Start-Game.ps1')) {
    $content = Get-Content -LiteralPath (Join-Path $root "Scripts\$script") -Raw
    if ($content -notmatch [regex]::Escape('Get-UnrealOfflineArguments.ps1')) {
        throw "$script does not reuse the offline Editor argument policy."
    }
}
$engine = Get-Content -LiteralPath (Join-Path $root 'Config\DefaultEngine.ini') -Raw
$pluginDisabled = $engine -match '(?ms)\[/Script/AndroidFileServerEditor\.AndroidFileServerRuntimeSettings\].*?^bEnablePlugin=False\s*$'
$networkDisabled = $engine -match '(?ms)\[/Script/AndroidFileServerEditor\.AndroidFileServerRuntimeSettings\].*?^bAllowNetworkConnection=False\s*$'
if (-not ($pluginDisabled -and $networkDisabled)) {
    throw 'Android File Server remains enabled for Editor launches.'
}
$authoring = Get-Content -LiteralPath (Join-Path $root 'Scripts\FernSpikePolicy.ps1') -Raw
foreach ($token in @('-notraceserver','-traceautostart=0','AndroidFileServer',
    'bEnablePlugin=False','bAllowNetworkConnection=False')) {
    if ($authoring -notmatch [regex]::Escape($token)) {
        throw "Bounded authoring policy is missing offline control: $token"
    }
}
Write-Output "PASS offline Editor launch policy: $($required.Count) arguments, 8 launch scripts, bounded authoring, Android File Server disabled."
