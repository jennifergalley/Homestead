[CmdletBinding()]
param([string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script = Join-Path $root 'Scripts\Characters\import_locomotion.py'
foreach ($verify in @($false, $true)) {
    $name = if ($verify) { 'locomotion-reload' } else { 'locomotion-import' }
    $log = Join-Path $root "Build\Logs\$name.log"
    [string[]]$extra = @()
    if ($verify) { $extra = @('-LocomotionVerifyOnly') }
    & $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$script" `
        -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @extra *> $log
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $log -Pattern 'LOCOMOTION_VERIFIED' -Quiet)) {
        Get-Content -LiteralPath $log -Tail 70 | Write-Output
        throw "Locomotion import/reload failed. See $log."
    }
}
