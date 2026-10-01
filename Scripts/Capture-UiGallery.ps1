<#
.SYNOPSIS
Captures the UI gallery: every UI surface (field book pages and dialogs, shop, notices, interaction
hints, HUD) as Jenny would see it, Slate included, at each requested resolution and input.

.DESCRIPTION
For each resolution and input this runs one hidden Development game (Test-Game.ps1 -UIGallery: the
editor binary with -game on the Estate map, -unattended -RenderOffscreen, sandboxed saves and user
settings, TEMP on E:). The game walks the gallery states (Source/SurvivalGame/HomesteadUIGallery.h)
and captures its own window with Slate, so no desktop window is opened or grabbed.

Output: <OutputRoot>\<stamp>\<res>[-pad][-world][-noheroine]\<id>.png, view\<id>.jpg (viewable copies), contact.jpg,
and <stamp>\index.md (id, what she should see, a link per resolution).

Two Unreal processes at most share this PC (the integration session holds one): the script refuses to
start while two are running or with under 6 GB of free memory.

.EXAMPLE
pwsh -File Scripts\Capture-UiGallery.ps1 -Res 720p,4K
pwsh -File Scripts\Capture-UiGallery.ps1 -Ids toast-success,focus-gather -Res 1080p -Input KBM,Pad
#>
[CmdletBinding()]
param(
    [string[]]$Ids = @('all'),
    [ValidateSet('720p', '1080p', '1440p', '4K')][string[]]$Res = @('720p', '1080p', '4K'),
    [Alias('Input')][ValidateSet('KBM', 'Pad')][string[]]$InputMode = @('KBM'),
    # Plain (default, Jenny 2026-09-30): every UI element in its screen position over a flat warm-grey
    # backdrop with the world hidden, the heroine kept for scale. World: over the game scene.
    [ValidateSet('Plain', 'World')][string[]]$Backdrop = @('Plain'),
    [switch]$NoHeroine,
    # The UI theme trial: parchment (the branch default) and/or classic.
    [ValidateSet('parchment', 'classic')][string[]]$Theme = @('parchment'),
    # Where the stamp folders go; defaults to E:\CopilotScratch\<SessionId>\ui-gallery.
    [string]$OutputRoot,
    [string]$SessionId = $(if ($env:COPILOT_SESSION_ID) { $env:COPILOT_SESSION_ID } else { 'ui-gallery' }),
    [ValidateRange(600, 3600)][int]$TimeoutSeconds = 3000,
    [double]$MinFreeGB = 6.0)
$ErrorActionPreference = 'Stop'
$sizes = @{ '720p' = @(1280, 720); '1080p' = @(1920, 1080); '1440p' = @(2560, 1440); '4K' = @(3840, 2160) }
if (-not $OutputRoot) { $OutputRoot = Join-Path 'E:\CopilotScratch' (Join-Path $SessionId 'ui-gallery') }
$stamp = Join-Path $OutputRoot (Get-Date -Format 'yyyyMMdd-HHmmss')
$null = New-Item -ItemType Directory -Force -Path $stamp
# Heavy temp writers stay off C:.
$temp = Join-Path (Split-Path $OutputRoot -Parent) 'tmp'
$null = New-Item -ItemType Directory -Force -Path $temp
$env:TEMP = $temp; $env:TMP = $temp

$failures = @()
foreach ($resolution in $Res) {
    foreach ($mode in $InputMode) {
      foreach ($scene in $Backdrop) {
       foreach ($look in $Theme) {
        $running = @(Get-Process UnrealEditor*, SurvivalGame*, JennysHomestead* -ErrorAction SilentlyContinue)
        $freeGB = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
        if ($running.Count -ge 2) { throw "Two Unreal processes are already running ($($running.Name -join ', ')); try again later." }
        if ($freeGB -lt $MinFreeGB) { throw ("Only {0:N1} GB of memory is free (need {1}); try again later." -f $freeGB, $MinFreeGB) }
        $name = $resolution + "-$look" + $(if ($mode -eq 'Pad') { '-pad' } else { '' }) + $(if ($scene -eq 'World') { '-world' } else { '' }) + $(if ($NoHeroine) { '-noheroine' } else { '' })
        $folder = Join-Path $stamp $name
        $size = $sizes[$resolution]
        Write-Host "UI gallery $resolution $mode -> $folder"
        try {
            & (Join-Path $PSScriptRoot 'Test-Game.ps1') -UIGallery -UIGalleryIds ($Ids -join ',') -UIGalleryInput $mode `
                -UIGalleryBackdrop $scene -UIGalleryNoHeroine:$NoHeroine -UITheme $look `
                -Width $size[0] -Height $size[1] -OutputDirectory $folder -TimeoutSeconds $TimeoutSeconds | Out-Null
        } catch {
            $failures += "$resolution $mode`: $($_.Exception.Message)"
            Write-Warning $failures[-1]
        }
        $result = Join-Path $folder 'smoke-result.txt'
        if (Test-Path -LiteralPath $result) {
            Select-String -LiteralPath $result -Pattern '^UI_GALLERY_(SKIPPED|MISSING|PENDING)' | ForEach-Object { Write-Warning $_.Line }
        }
       }
      }
    }
}
$index = python (Join-Path $PSScriptRoot 'ui_gallery_sheet.py') $stamp
if ($LASTEXITCODE -ne 0) { throw "Building the contact sheets failed: $index" }
Write-Output "Index: $index"
Get-ChildItem -LiteralPath $stamp -Recurse -Filter contact.jpg | ForEach-Object { Write-Output "Contact sheet: $($_.FullName)" }
if ($failures) { throw "UI gallery runs failed: $($failures -join '; ')" }
