[CmdletBinding()]
param([string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$candidates = @()
if ($EngineRoot) { $candidates += $EngineRoot }
if ($env:UE_ENGINE_ROOT) { $candidates += $env:UE_ENGINE_ROOT }
$manifest = 'C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat'
if (Test-Path -LiteralPath $manifest) {
    $entries = (Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).InstallationList
    $candidates += @($entries | Where-Object { $_.AppName -eq 'UE_5.8' } | ForEach-Object { $_.InstallLocation })
}
$candidates += @('E:\Program Files\UE_5.8', 'E:\ProgramFiles\UE_5.8', 'E:\Tools\UE_5.8', 'E:\Epic Games\UE_5.8', 'E:\UE_5.8', 'C:\Program Files\Epic Games\UE_5.8')
foreach ($candidate in ($candidates | Select-Object -Unique)) {
    if (-not $candidate) { continue }
    $build = Join-Path $candidate 'Engine\Build\Build.version'
    $editor = Join-Path $candidate 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if ((Test-Path -LiteralPath $build) -and (Test-Path -LiteralPath $editor)) {
        $pendingDirectory = Join-Path $candidate '.egstore\Pending'
        if (Test-Path -LiteralPath $pendingDirectory) {
            $pending = @(Get-ChildItem -LiteralPath $pendingDirectory -Filter '*.manifest' -File)
            if ($pending.Count -gt 0) {
                throw "Epic still has a pending installation manifest for $candidate. Wait for installation to finish; do not delete installer state or build against these partial files."
            }
        }
        $version = Get-Content -LiteralPath $build -Raw | ConvertFrom-Json
        if ($version.MajorVersion -ne 5 -or $version.MinorVersion -ne 8) {
            throw "This project targets Unreal 5.8. Found $($version.MajorVersion).$($version.MinorVersion) at $candidate. Review compatibility before changing the target."
        }
        return (Resolve-Path -LiteralPath $candidate).Path
    }
}
throw 'Unreal Engine 5.8 is not installed or the download is incomplete. Finish installation in Epic Launcher on E:, then rerun. You can pass -EngineRoot or set UE_ENGINE_ROOT; no account credentials belong in this project.'
