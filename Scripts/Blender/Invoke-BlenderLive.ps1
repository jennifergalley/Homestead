[CmdletBinding(DefaultParameterSetName = 'Code')]
param(
    [Parameter(ParameterSetName = 'Code', Mandatory, Position = 0)][string]$Code,
    [Parameter(ParameterSetName = 'File', Mandatory)][string]$File,
    [Parameter(ParameterSetName = 'Ping', Mandatory)][switch]$Ping,
    [string[]]$Arguments = @(),
    [int]$TimeoutSeconds = 600
)
# Runs Python inside the live Blender window started by Start-BlenderLive.ps1.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$session = Get-Content -LiteralPath (Join-Path $root 'Saved\BlenderLive\session.json') -Raw | ConvertFrom-Json
$request = @{ token = $session.token; args = $Arguments }
switch ($PSCmdlet.ParameterSetName) {
    'Code' { $request.code = $Code }
    'File' { $request.file = (Resolve-Path -LiteralPath $File).Path }
    'Ping' { $request.ping = $true }
}
$client = [Net.Sockets.TcpClient]::new()
try {
    $client.Connect('127.0.0.1', [int]$session.port)
    $client.ReceiveTimeout = $TimeoutSeconds * 1000
    $stream = $client.GetStream()
    $bytes = [Text.Encoding]::UTF8.GetBytes(($request | ConvertTo-Json -Compress -Depth 4) + "`n")
    $stream.Write($bytes, 0, $bytes.Length)
    $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::UTF8)
    $reply = $reader.ReadLine() | ConvertFrom-Json
} finally { $client.Dispose() }
if ($reply.output) { Write-Output $reply.output.TrimEnd() }
if (-not $reply.ok) { throw "Blender live request failed:`n$($reply.error)" }
