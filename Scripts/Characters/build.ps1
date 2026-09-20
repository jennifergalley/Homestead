param([switch]$SkipRender, [switch]$PreviewOnly, [string]$ToolsRoot = 'E:\Tools')
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$blender = Join-Path $ToolsRoot 'blender-4.5.14-windows-x64\blender.exe'
if (!(Test-Path $blender)) { throw 'Run setup.ps1 to acquire the verified portable tools first.' }
$env:BLENDER_USER_RESOURCES = Join-Path $ToolsRoot 'BlenderCharacterUser'
function Invoke-Stage($Script, $Log, $Extra = @()) {
    & $blender --background --python-exit-code 1 --python "$PSScriptRoot\$Script" -- @Extra *> "$root\Build\CharacterPreview\$Log"
    if ($LASTEXITCODE -ne 0) {
        Get-Content "$root\Build\CharacterPreview\$Log" -Tail 40
        throw "Blender stage $Script failed ($LASTEXITCODE)"
    }
    Write-Host "$Script completed. Log: Build\CharacterPreview\$Log"
}
if (!$PreviewOnly) {
    $extra = @()
    if ($SkipRender) { $extra += '--skip-render' }
    Invoke-Stage 'build_heroine.py' 'authoring.log' $extra
    Invoke-Stage 'export_heroine.py' 'export.log'
}
if (!$SkipRender) { Invoke-Stage 'preview_exports.py' 'preview.log' }
