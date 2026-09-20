[CmdletBinding()]
param([string]$EngineRoot, [switch]$Package, [switch]$SkipAssets)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$project = Join-Path $root 'SurvivalGame.uproject'
if (-not $SkipAssets) { & (Join-Path $PSScriptRoot 'Fetch-Assets.ps1') }
$build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
& $build SurvivalGameEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
if ($LASTEXITCODE -ne 0) { throw "Unreal editor-module build failed ($LASTEXITCODE)." }
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$bootstrap = Join-Path $PSScriptRoot 'bootstrap_unreal.py'
$logDirectory = Join-Path $root 'Build\Logs'
$null = New-Item -ItemType Directory -Path $logDirectory -Force
$bootstrapLog = Join-Path $logDirectory 'bootstrap.log'
Write-Host "Generating content; full output: $bootstrapLog"
& $editor $project -run=pythonscript "-script=$bootstrap" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $bootstrapLog
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $bootstrapLog -Tail 85 | Write-Output
    throw "Unreal content bootstrap failed ($LASTEXITCODE). See Build\Logs\bootstrap.log."
}
$map = Join-Path $root 'Content\SurvivalGame\Maps\Homestead.umap'
if (-not (Test-Path -LiteralPath $map)) { throw 'Content bootstrap did not produce the playable map.' }
& (Join-Path $PSScriptRoot 'Import-Characters.ps1') -EngineRoot $engine
if ($Package) {
    $uat = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
    $archive = Join-Path $root 'Build\Windows'
    & $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$archive" "-UbtArgs=-NoUBA -NoXGE -NoFASTBuild" -prereqs -unattended -utf8output
    if ($LASTEXITCODE -ne 0) { throw "Game packaging failed ($LASTEXITCODE)." }
    $credits = Join-Path $archive 'asset-credits.md'
    Copy-Item -LiteralPath (Join-Path $root 'docs\asset-credits.md') -Destination $credits -Force
    [ordered]@{
        packagedUtc = [DateTimeOffset]::UtcNow.ToString('o')
        engineRoot = $engine
        configuration = 'Development'
        status = 'Prototype build; play and visual acceptance are separate.'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $archive 'build-receipt.json') -Encoding utf8
    Write-Host "Packaged game: $archive. Credits are included."
}
else {
    Write-Host 'Editor target and content are built. Run Scripts\Start-Game.ps1.'
}
