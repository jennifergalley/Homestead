<#
.SYNOPSIS
Animation Inspector: record one heroine action frame by frame in the Character Lab and build review sheets.

.DESCRIPTION
Starts the Development editor binary as a hidden, unattended game (-game -RenderOffscreen) in the Character Lab
with the MetaHuman heroine, plays one LabAction at a fixed 1/30 s step (AHomesteadAnimInspector), captures
each view every -Every frames, then runs Scripts\anim_inspector_sheet.py for overlays, contact sheets,
keyframes.png, motion.gif and index.md (realistic-animation skill, "Seeing every frame").

It refuses to start when two Unreal processes are already running or free memory is under 6 GB, never
touches other windows or processes, and runs the game in its own kill-on-close job (Scripts\KillOnCloseJob.cs),
so a timeout or an interrupted run leaves no shader worker or crash reporter behind. Output defaults to
E:\CopilotScratch\anim-inspector\<Clip>\<stamp>\; the engine's TEMP/TMP is always under E:\CopilotScratch.

.EXAMPLE
pwsh -NoProfile -File .\Scripts\Inspect-Animation.ps1 -Clip Weeds -Recipe kneel_pull_weeds
pwsh -NoProfile -File .\Scripts\Inspect-Animation.ps1 -Clip Mow -Recipe scythe_mow -Contacts strike1,bite1 -Every 1
#>
[CmdletBinding()]
param(
    # A LabAction name: Gather, Sticks, Stones, Roots, Berries, Reeds, Pull, Pick, Eat, Craft, Water, Fill, Chop,
    # Knife, Till, Machete, Fell, Weeds, Mow, Pickaxe, AxeStrike, Billhook, LampDown, LampUp.
    [Parameter(Mandatory)][string]$Clip,
    # Capture every Nth frame (every frame is still recorded and checked).
    [ValidateRange(1, 30)][int]$Every = 2,
    [string[]]$Views = @('front', 'left', 'right', 'top', 'threequarter'),
    # A LabHold tool to carry first (Hatchet, Pail, Scythe...).
    [string]$Hold,
    # The authoring recipe whose FRAMES name the beats (homestead_agent module, e.g. kneel_pull_weeds).
    [string]$Recipe,
    # FRAMES keys of strikes or impacts, where a fast joint is only a warning.
    [string[]]$Contacts = @(),
    [ValidateRange(256, 2048)][int]$Resolution = 768,
    # Capture the lit scene instead of base colour.
    [switch]$Lit,
    [string]$OutputDirectory,
    [string]$EngineRoot,
    [ValidateRange(60, 1800)][int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'SurvivalGame.uproject'
$running = @(Get-Process UnrealEditor*, SurvivalGame*, JennysHomestead* -ErrorAction SilentlyContinue)
if ($running.Count -ge 2) { throw "Two Unreal processes are already running ($($running.Id -join ', ')); try again when one closes." }
$freeGb = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
if ($freeGb -lt 6) { throw ('Only {0:N1} GB free; the inspector needs 6 GB.' -f $freeGb) }
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path 'E:\CopilotScratch\anim-inspector' (Join-Path $Clip (Get-Date -Format 'yyyyMMdd-HHmmss'))
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$null = New-Item -ItemType Directory -Path $OutputDirectory -Force
# The engine's temp files stay on E: wherever the output goes (C: is nearly full).
$scratch = Join-Path 'E:\CopilotScratch\anim-inspector\tmp' ([Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $scratch -Force
$env:TEMP = $scratch
$env:TMP = $scratch

$engine = & (Join-Path $PSScriptRoot 'Resolve-Engine.ps1') -EngineRoot $EngineRoot
$executable = Join-Path $engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$offline = (& (Join-Path $PSScriptRoot 'Get-UnrealOfflineArguments.ps1')) -join ' '
$log = Join-Path $OutputDirectory 'engine.log'
$inspector = "-HomesteadAnimInspector=$Clip -HomesteadAnimInspectorOut=`"$OutputDirectory`" -HomesteadAnimInspectorEvery=$Every " +
    "-HomesteadAnimInspectorViews=$($Views -join ',') -HomesteadAnimInspectorResolution=$Resolution -HomesteadAnimInspectorExit"
if ($Hold) { $inspector += " -HomesteadAnimInspectorHold=$Hold" }
if ($Lit) { $inspector += ' -HomesteadAnimInspectorLit' }
$arguments = "`"$project`" /Game/SurvivalGame/Maps/Homestead -game $offline -HomesteadCharacterLab -HomesteadMetaHuman $inspector " +
    "-UserDir=`"$(Join-Path $OutputDirectory 'EngineUser')`" -unattended -RenderOffscreen -windowed -ForceRes -ResX=1280 -ResY=720 " +
    "-nosplash -nosound -abslog=`"$log`""
Add-Type -Path (Join-Path $PSScriptRoot 'KillOnCloseJob.cs')
# Its own kill-on-close job: shader workers and the crash reporter it starts end with it, also on a timeout
# or if this script is stopped. A Zen server it may have launched is spared (other sessions share it).
$spare = [string[]]@('zenserver.exe')
$job = [Homestead.Tools.KillOnCloseJob]::new($executable, "`"$executable`" $arguments", $root)
try {
    Write-Host "Animation Inspector PID $($job.ProcessId): $Clip -> $OutputDirectory"
    if (-not $job.WaitForExit($TimeoutSeconds * 1000)) {
        $ended = $job.Stop($spare)
        throw "The inspector ran past $TimeoutSeconds s; stopped its own processes ($($ended -join ', ')). See $log."
    }
    $exitCode = $job.ExitCode
}
finally {
    $null = $job.Stop($spare)
    $job.Release()
}
$result = Join-Path $OutputDirectory 'result.txt'
if (-not (Test-Path -LiteralPath (Join-Path $OutputDirectory 'frames.json'))) {
    Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
    $why = if (Test-Path -LiteralPath $result) { Get-Content -LiteralPath $result -Raw } else { "no result (exit $exitCode)" }
    throw "Nothing was recorded: $why See $log."
}
Get-Content -LiteralPath $result | Write-Host
$sheetArgs = @((Join-Path $PSScriptRoot 'anim_inspector_sheet.py'), $OutputDirectory)
if ($Recipe) { $sheetArgs += @('--recipe', $Recipe) }
if ($Contacts) { $sheetArgs += @('--contacts', ($Contacts -join ',')) }
& python @sheetArgs
$sheetExit = $LASTEXITCODE
Remove-Item -LiteralPath $scratch -Recurse -Force -ErrorAction SilentlyContinue
if ($sheetExit -ne 0) { throw "anim_inspector_sheet.py failed ($sheetExit)." }
Write-Output (Join-Path $OutputDirectory 'index.md')
