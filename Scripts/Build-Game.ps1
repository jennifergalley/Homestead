<#
.SYNOPSIS
Builds the editor module, regenerates content and (with -Package) packages the Development game.
.DESCRIPTION
-Package              also runs UAT BuildCookRun into -ArchiveDirectory (default Build\Windows).
-PackageOnly          packages without re-running Fetch-Assets, the content bootstrap and the character/
                      locomotion imports (about 25 minutes). Use it only when generated content is already
                      current in this worktree; the editor module build still runs (a no-op when current).
-SkipAssets           skips Fetch-Assets.ps1.
UAT is single-instance machine-wide; this script waits for another worktree's package to finish
(-WaitForUATMutex) instead of failing. Output: Build\Logs\bootstrap.log and Build\Logs\package-<time>.log.
The bootstrap re-saves many tracked .uassets; review git status and restore the ones you didn't mean
to change. Shared-machine rules: .github\skills\unreal-editor-mcp\SKILL.md, sections 0 and 8.
#>
[CmdletBinding()]
param([string]$EngineRoot, [switch]$Package, [switch]$PackageOnly, [switch]$SkipAssets,
    [string]$ArchiveDirectory = 'Build\Windows',
    [ValidateSet('Development','Shipping')][string]$Configuration = 'Development',
    [switch]$ReuseCooked, [string]$ReusePakDirectory)
$ErrorActionPreference = 'Stop'
if ($PackageOnly) { $Package = [switch]$true }
$root = Split-Path $PSScriptRoot -Parent
$archive = [IO.Path]::GetFullPath($ArchiveDirectory, $root)
if ($Package) {
    $archivePrefix = $archive.TrimEnd('\') + '\'
    $running = @(Get-Process -Name SurvivalGame -ErrorAction SilentlyContinue |
        Where-Object { $_.Path -and $_.Path.StartsWith($archivePrefix, [StringComparison]::OrdinalIgnoreCase) })
    if ($running.Count) {
        throw "A player is running from $archive (PID $($running.Id -join ', ')). Use a separate -ArchiveDirectory."
    }
}
$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
& (Join-Path $PSScriptRoot 'Set-EngineEnvironment.ps1')
$project = Join-Path $root 'SurvivalGame.uproject'
if ($ReuseCooked) {
    if ($Configuration -ne 'Shipping') { throw 'The editor-free reuse path requires -Configuration Shipping.' }
    $cook = Join-Path $root 'Saved\Cooked\Windows\SurvivalGame'
    foreach ($required in @('Metadata\CookMetadata.ucookmeta','Metadata\DevelopmentAssetRegistry.bin')) {
        if (-not (Test-Path -LiteralPath (Join-Path $cook $required) -PathType Leaf)) { throw "Missing existing cook: $required" }
    }
    if (Test-Path -LiteralPath (Join-Path $cook 'ue.projectstore')) {
        throw 'This bounded offline path only supports the verified file-based cook, not a Zen project store.'
    }
    $gameConfigs = @(Get-ChildItem -LiteralPath (Join-Path $root 'Config') -Recurse -File -Filter '*.ini') +
        @(Get-Item -LiteralPath (Join-Path $engine 'Engine\Config\BaseGame.ini'), (Join-Path $engine 'Engine\Config\Windows\BaseWindowsGame.ini'))
    if ($gameConfigs | Select-String -Pattern '^\s*bEnablePakStreaming\s*=\s*(True|1)\s*$') {
        throw 'Pak streaming could launch Zen. It is not supported by this offline reuse path.'
    }
    if ($Package) {
        if (-not $archive.StartsWith((Join-Path $root 'Build\Releases') + '\', [StringComparison]::OrdinalIgnoreCase) -or
            (Test-Path -LiteralPath $archive)) { throw 'Offline packaging requires a fresh Build\Releases archive.' }
        if (-not $ReusePakDirectory) { throw 'Offline packaging requires explicit existing cooked containers; UnrealPak must not run.' }
        $pakSource = [IO.Path]::GetFullPath($ReusePakDirectory, $root)
        $paks = @(Get-ChildItem -LiteralPath $pakSource -File)
        if (-not ($paks.Extension -contains '.pak') -or -not ($paks.Extension -contains '.utoc') -or
            -not ($paks.Extension -contains '.ucas') -or ($paks | Where-Object { $_.Extension -notin '.pak','.utoc','.ucas' })) {
            throw 'ReusePakDirectory must contain only the complete existing pak/utoc/ucas container set.'
        }
        $containerHashes = @($paks | ForEach-Object { [ordered]@{ name=$_.Name; sha256=(Get-FileHash $_.FullName).Hash } })
    }
    $build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
    & $build SurvivalGame Win64 Shipping "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
    if ($LASTEXITCODE -ne 0) { throw "Shipping build failed ($LASTEXITCODE)." }
    if (-not $Package) { Write-Host 'Shipping code built; no editor, cooker, packager or game launched.'; return }
    $stage = Join-Path $archive 'Staging'
    $stagedPaks = Join-Path $stage 'Windows\SurvivalGame\Content\Paks'
    $null = New-Item -ItemType Directory -Path $stagedPaks -Force
    $paks | Copy-Item -Destination $stagedPaks
    $uat = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
    & $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Shipping -skipbuild -skipcook -skippak -stage -archive "-stagingdirectory=$stage" "-archivedirectory=$archive" -unattended -utf8output -WaitForUATMutex
    if ($LASTEXITCODE -ne 0) { throw "Shipping staging failed ($LASTEXITCODE)." }
    $packageRoot = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $archive
    foreach ($container in $containerHashes) {
        $destination = Join-Path $packageRoot "SurvivalGame\Content\Paks\$($container.name)"
        if ((Get-FileHash -LiteralPath $destination).Hash -ne $container.sha256) { throw "Cooked container changed: $($container.name)" }
    }
    Copy-Item -LiteralPath (Join-Path $root 'docs\asset-credits.md') -Destination (Join-Path $packageRoot 'asset-credits.md')
    [ordered]@{
        packagedUtc=[DateTimeOffset]::UtcNow.ToString('o'); engineRoot=$engine; configuration='Shipping'
        archiveDirectory=$archive; packageDirectory=$packageRoot
        executable='SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe'
        nativeExecutableSha256=(Get-FileHash -LiteralPath (Join-Path $packageRoot 'SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe')).Hash
        reusedCook=$cook; reusedContainers=$pakSource; containers=$containerHashes
        editorLaunched=$false; cookerLaunched=$false; unrealPakLaunched=$false
        status='Shipping build using unchanged existing cooked containers; runtime acceptance is separate.'
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $archive 'build-receipt.json') -Encoding utf8
    Write-Host "Offline Shipping package: $packageRoot"
    return
}
if ($Configuration -ne 'Development' -or $ReusePakDirectory) {
    throw 'Shipping builds require the explicit -ReuseCooked path; no editor will be launched implicitly.'
}
if (-not $SkipAssets -and -not $PackageOnly) { & (Join-Path $PSScriptRoot 'Fetch-Assets.ps1') }
$build = Join-Path $engine 'Engine\Build\BatchFiles\Build.bat'
& $build SurvivalGameEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -NoXGE -NoFASTBuild
if ($LASTEXITCODE -ne 0) { throw "Unreal editor-module build failed ($LASTEXITCODE)." }
$editor = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$offlineArguments = @(& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1'))
$bootstrap = Join-Path $PSScriptRoot 'bootstrap_unreal.py'
$logDirectory = Join-Path $root 'Build\Logs'
$null = New-Item -ItemType Directory -Path $logDirectory -Force
if (-not $PackageOnly) {
$bootstrapLog = Join-Path $logDirectory 'bootstrap.log'
Write-Host "Generating content; full output: $bootstrapLog"
& $editor $project -run=pythonscript "-script=$bootstrap" @offlineArguments -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput *> $bootstrapLog
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $bootstrapLog -Tail 85 | Write-Output
    throw "Unreal content bootstrap failed ($LASTEXITCODE). See Build\Logs\bootstrap.log."
}
$map = Join-Path $root 'Content\SurvivalGame\Maps\Homestead.umap'
if (-not (Test-Path -LiteralPath $map)) { throw 'Content bootstrap did not produce the playable map.' }
& (Join-Path $PSScriptRoot 'Import-Characters.ps1') -EngineRoot $engine
& (Join-Path $PSScriptRoot 'Import-Locomotion.ps1') -EngineRoot $engine
& (Join-Path $PSScriptRoot 'Import-Locomotion.ps1') -EngineRoot $engine -AnimationSet Gathering
& (Join-Path $PSScriptRoot 'Import-Locomotion.ps1') -EngineRoot $engine -AnimationSet Watering
& (Join-Path $PSScriptRoot 'Import-Locomotion.ps1') -EngineRoot $engine -AnimationSet Clearing
}
if ($Package) {
    # A file-based cook (-SkipZenStore): other worktrees' QA scripts stop the shared zenserver.exe, which
    # broke staging with "Failed to read oplog from Zen ... HTTP NotFound".
    Remove-Item -LiteralPath (Join-Path $root 'Saved\Cooked\Windows\ue.projectstore') -ErrorAction SilentlyContinue
    # UAT's own build step can't wait for UBT's machine-wide mutex (-UbtArgs lands inside each -Target),
    # so it fails with ConflictingInstance whenever another worktree is compiling. Build the game target
    # here with -WaitMutex and let UAT skip its build.
    & $build SurvivalGame Win64 Development "-Project=$project" -WaitMutex -NoUBA -NoXGE -NoFASTBuild
    if ($LASTEXITCODE -ne 0) { throw "Game target build failed ($LASTEXITCODE)." }
    $uat = Join-Path $engine 'Engine\Build\BatchFiles\RunUAT.bat'
    # UAT's own log folder (%APPDATA%\Unreal Engine\AutomationTool\Logs) is shared by every worktree and
    # gets overwritten, so keep this worktree's full packaging output alongside the bootstrap log.
    $packageLog = Join-Path $logDirectory "package-$(Get-Date -Format 'yyyyMMdd-HHmmss').log"
    Write-Host "Packaging; full output: $packageLog"
    & $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -skipbuild -cook -stage -pak -archive "-archivedirectory=$archive" -AdditionalCookerOptions=-SkipZenStore -prereqs -unattended -utf8output -WaitForUATMutex *>&1 |
        Tee-Object -LiteralPath $packageLog
    if ($LASTEXITCODE -ne 0) { throw "Game packaging failed ($LASTEXITCODE). See $packageLog and Saved\Logs\UnrealPak.log." }
    $packageRoot = & (Join-Path $PSScriptRoot 'Resolve-PackageDirectory.ps1') -PackageDirectory $archive
    $credits = Join-Path $packageRoot 'asset-credits.md'
    Copy-Item -LiteralPath (Join-Path $root 'docs\asset-credits.md') -Destination $credits -Force
    # Discord recognises "SurvivalGame" executables as the Steam game Outpost Zero. Players launch a
    # hard link with Homestead's own name instead (same file, no extra disk); scripts and tests keep
    # using SurvivalGame.exe.
    $binary = Join-Path $packageRoot 'SurvivalGame\Binaries\Win64\SurvivalGame.exe'
    $playerExe = Join-Path $packageRoot 'SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe'
    if (Test-Path -LiteralPath $playerExe) { Remove-Item -LiteralPath $playerExe -Force }
    $null = New-Item -ItemType HardLink -Path $playerExe -Target $binary
    [ordered]@{
        packagedUtc = [DateTimeOffset]::UtcNow.ToString('o')
        engineRoot = $engine
        configuration = 'Development'
        archiveDirectory = $archive
        packageDirectory = $packageRoot
        status = 'Prototype build; play and visual acceptance are separate.'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $archive 'build-receipt.json') -Encoding utf8
    if ($packageRoot -ne $archive) {
        Copy-Item -LiteralPath (Join-Path $archive 'build-receipt.json') -Destination (Join-Path $packageRoot 'build-receipt.json') -Force
    }
    Write-Host "Packaged game: $packageRoot. Credits are included. Players launch $playerExe."
}
else {
    Write-Host 'Editor target and content are built. Run Scripts\Start-Game.ps1.'
}
