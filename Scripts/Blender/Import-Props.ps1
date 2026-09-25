[CmdletBinding()]
param([string[]]$Name, [string]$EngineRoot)
# Imports Blender-built props from Assets\Props into Unreal. The project must not
# be open in another editor instance. See docs\blender-assets.md.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$scripts = Join-Path $root 'Scripts'
$engine = & (Join-Path $scripts 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $scripts 'Set-EngineEnvironment.ps1')
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$offlineArguments = @(& (Join-Path $scripts 'Get-UnrealOfflineArguments.ps1'))
$extra = @()
if ($Name) { $extra += "-HomesteadProps=$($Name -join ',')" }
$log = Join-Path $root 'Build\Logs\prop-import.log'
$null = New-Item -ItemType Directory -Force (Split-Path $log)
& $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$(Join-Path $PSScriptRoot 'import_props.py')" `
    @offlineArguments -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @extra *> $log
if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $log -Pattern 'HOMESTEAD_PROPS_IMPORTED' -Quiet)) {
    Get-Content -LiteralPath $log -Tail 60 | Write-Output
    throw "Prop import failed. See $log."
}
Get-Content -LiteralPath (Join-Path $root 'Build\Logs\prop-import.json')
