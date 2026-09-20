[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $root 'DerivedDataCache'
$null = New-Item -ItemType Directory -Path $cache -Force
[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $cache, [EnvironmentVariableTarget]::Process)
Write-Verbose "Unreal local cache for this process: $cache"
