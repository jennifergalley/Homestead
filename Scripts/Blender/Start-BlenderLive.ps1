[CmdletBinding()]
param([string]$File, [string]$Blender, [int]$Port = 9876)
# Opens a visible Blender window with the live bridge so scripts (and agents)
# can build assets while you watch. Optional -File opens a .blend.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$exe = @($Blender, $env:HOMESTEAD_BLENDER, 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    (Get-Command blender -ErrorAction SilentlyContinue).Source) |
    Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } | Select-Object -First 1
if (-not $exe) { throw 'Blender not found. Pass -Blender or set HOMESTEAD_BLENDER.' }

$state = Join-Path $root 'Saved\BlenderLive'
$null = New-Item -ItemType Directory -Force $state
$token = [Convert]::ToHexString([Security.Cryptography.RandomNumberGenerator]::GetBytes(24))
[ordered]@{ port = $Port; token = $token } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $state 'session.json')

$env:HOMESTEAD_BLENDER_TOKEN = $token
$env:HOMESTEAD_BLENDER_PORT = "$Port"
$arguments = @()
if ($File) { $arguments += "`"$((Resolve-Path -LiteralPath $File).Path)`"" }
$arguments += @('--python', "`"$(Join-Path $PSScriptRoot 'live_bridge.py')`"")
$process = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru
Remove-Item Env:HOMESTEAD_BLENDER_TOKEN

$deadline = (Get-Date).AddSeconds(60)
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 500
    try {
        $version = & (Join-Path $PSScriptRoot 'Invoke-BlenderLive.ps1') -Ping
        Write-Host "Live Blender $version ready (PID $($process.Id), port $Port)."
        return
    } catch { if ($process.HasExited) { throw "Blender exited ($($process.ExitCode))." } }
}
throw 'Blender started but the live bridge did not answer within 60 s.'
