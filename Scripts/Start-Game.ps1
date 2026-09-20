[CmdletBinding()]
param([string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$project = Join-Path $root 'SurvivalGame.uproject'
if (-not (Test-Path (Join-Path $root 'Content\SurvivalGame\Maps\Homestead.umap'))) {
    throw 'The map has not been generated. Run Scripts\Build-Game.ps1 first.'
}
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
& $editor $project /Game/SurvivalGame/Maps/Homestead -game -windowed -ForceRes -ResX=1920 -ResY=1080 -log
if ($LASTEXITCODE -ne 0) { throw "Game exited with code $LASTEXITCODE." }
