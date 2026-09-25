[CmdletBinding(DefaultParameterSetName = 'Recipe')]
param(
    [Parameter(ParameterSetName = 'Recipe', Mandatory, Position = 0)][string]$Recipe,
    [Parameter(ParameterSetName = 'Blend', Mandatory)][string]$Blend,
    [Parameter(ParameterSetName = 'FromLive', Mandatory)][string]$FromLive,
    [Parameter(ParameterSetName = 'Recipe')][switch]$Live,
    [string]$Blender,
    [string]$OutDirectory,
    [switch]$NoPreview,
    [switch]$NoBeauty,
    [int]$BeautySamples = 256,
    [switch]$KeepPivot,
    [switch]$Show,
    [switch]$Open
)
# Builds a static prop. See docs\blender-assets.md.
#   -Recipe name       headless build of Scripts\Blender\Recipes\<name>.py
#   -Recipe name -Live build it inside the visible Blender (Start-BlenderLive.ps1)
#   -Blend file        export SM_* meshes from a saved .blend
#   -FromLive Name     export SM_* meshes currently in the visible Blender as asset set Name
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$builder = Join-Path $PSScriptRoot 'build_prop.py'
$liveClient = Join-Path $PSScriptRoot 'Invoke-BlenderLive.ps1'

$exe = @($Blender, $env:HOMESTEAD_BLENDER, 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe',
    (Get-Command blender -ErrorAction SilentlyContinue).Source, 'E:\Tools\blender-4.5.14-windows-x64\blender.exe') |
    Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } | Select-Object -First 1
if (-not $exe) { throw 'Blender not found. Pass -Blender or set HOMESTEAD_BLENDER.' }

$arguments = @()
switch ($PSCmdlet.ParameterSetName) {
    'Recipe' {
        $path = $Recipe
        if (-not (Test-Path -LiteralPath $path)) { $path = Join-Path $PSScriptRoot "Recipes\$Recipe.py" }
        if (-not (Test-Path -LiteralPath $path)) { throw "Recipe not found: $Recipe" }
        $arguments += @('--recipe', (Resolve-Path -LiteralPath $path).Path)
        $label = [IO.Path]::GetFileNameWithoutExtension($path)
    }
    'Blend' {
        $arguments += @('--blend', (Resolve-Path -LiteralPath $Blend).Path)
        $label = [IO.Path]::GetFileNameWithoutExtension($Blend)
    }
    'FromLive' {
        # Snapshot the live scene so finalizing never touches the user's working file.
        $snapshot = Join-Path $root "Saved\BlenderLive\$FromLive.blend"
        $escaped = $snapshot.Replace('\', '\\')
        & $liveClient -Code "import bpy; bpy.ops.wm.save_as_mainfile(filepath='$escaped', copy=True, compress=True)" | Out-Null
        $arguments += @('--blend', $snapshot, '--name', $FromLive)
        $label = $FromLive
    }
}
if ($OutDirectory) { $arguments += @('--out', [IO.Path]::GetFullPath($OutDirectory, $root)) }
if ($NoPreview) { $arguments += '--no-preview' }
if ($KeepPivot) { $arguments += '--keep-pivot' }

$logDirectory = Join-Path $root 'Build\Logs\Blender'
$null = New-Item -ItemType Directory -Force $logDirectory
$log = Join-Path $logDirectory "$label.log"
if ($Live) {
    try { & $liveClient -File $builder -Arguments $arguments *> $log; $exitCode = 0 }
    catch { $_.Exception.Message | Out-File -Append -LiteralPath $log; $exitCode = 1 }
} else {
    & $exe --background --factory-startup --disable-autoexec --offline-mode --python-exit-code 1 `
        --python $builder -- @arguments *> $log
    $exitCode = $LASTEXITCODE
}
$built = Select-String -LiteralPath $log -Pattern 'HOMESTEAD_PROP_BUILT (.+)$' | Select-Object -Last 1
if ($exitCode -ne 0 -or -not $built) {
    Get-Content -LiteralPath $log -Tail 40 | Write-Output
    throw "Blender prop build failed. Full log: $log"
}
Select-String -LiteralPath $log -Pattern 'HOMESTEAD_PROP_(MESH|WARNING) ' | ForEach-Object { $_.Line.Trim() }
$report = $built.Matches[0].Groups[1].Value.Trim()
if ($PSCmdlet.ParameterSetName -eq 'FromLive') {
    Copy-Item -LiteralPath $snapshot -Destination (Join-Path (Split-Path $report) "$FromLive.blend") -Force
}
Write-Host "Report: $report"
$reportData = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if ($reportData.textures -and -not $NoPreview -and -not $NoBeauty) {
    $blendFile = Get-ChildItem -LiteralPath (Split-Path $report) -Filter '*.blend' | Select-Object -First 1
    $beautyLog = Join-Path $logDirectory "$label-beauty.log"
    Write-Host "Rendering Cycles review images ($BeautySamples samples)..."
    & $exe --background $blendFile.FullName --disable-autoexec --offline-mode --python-exit-code 1 `
        --python (Join-Path $PSScriptRoot 'render_beauty.py') -- --samples $BeautySamples *> $beautyLog
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $beautyLog -Pattern 'HOMESTEAD_BEAUTY_DONE' -Quiet)) {
        Get-Content -LiteralPath $beautyLog -Tail 30 | Write-Output
        throw "Beauty render failed. Full log: $beautyLog"
    }
    Select-String -LiteralPath $beautyLog -Pattern '^HOMESTEAD_BEAUTY ' | ForEach-Object { $_.Line.Trim() }
}
if ($Show) { & (Join-Path $PSScriptRoot 'Show-Prop.ps1') $reportData.name | Out-Null }
if ($Open) {
    $blendFile = Get-ChildItem -LiteralPath (Split-Path $report) -Filter '*.blend' | Select-Object -First 1
    Start-Process -FilePath $exe -ArgumentList "`"$($blendFile.FullName)`""
}
