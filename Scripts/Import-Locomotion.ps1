[CmdletBinding()]
param([string]$EngineRoot, [ValidateSet('Locomotion','Gathering','Watering','Clearing','Chopping','Tilling')][string]$AnimationSet='Locomotion')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$offlineArguments = @(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1'))
$script = Join-Path $root 'Scripts\Characters\import_locomotion.py'
foreach ($verify in @($false, $true)) {
    $name = $AnimationSet.ToLower() + $(if ($verify) { '-reload' } else { '-import' })
    $log = Join-Path $root "Build\Logs\$name.log"
    [string[]]$extra = @()
    if ($verify) { $extra = @('-LocomotionVerifyOnly') }
    if ($AnimationSet -eq 'Gathering') { $extra += '-GatheringAnimations' }
    if ($AnimationSet -eq 'Watering') { $extra += '-WateringAnimations' }
    if ($AnimationSet -eq 'Clearing') { $extra += '-ClearingAnimations' }
    if ($AnimationSet -eq 'Chopping') { $extra += '-ChoppingAnimations' }
    if ($AnimationSet -eq 'Tilling') { $extra += '-TillingAnimations' }
    & $editor (Join-Path $root 'SurvivalGame.uproject') -run=pythonscript "-script=$script" `
        @offlineArguments -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @extra *> $log
    $marker = $AnimationSet.ToUpper() + '_VERIFIED'
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $log -Pattern $marker -Quiet)) {
        Get-Content -LiteralPath $log -Tail 70 | Write-Output
        throw "Locomotion import/reload failed. See $log."
    }
}
