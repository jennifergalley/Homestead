[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
@(
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
