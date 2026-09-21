param(
    [ValidateSet('LongWave', 'Bob', 'All')][string]$Style = 'All',
    [switch]$Modular,
    [switch]$Preview,
    [string]$Blender = 'E:\Tools\blender-4.5.14-windows-x64\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$out = Join-Path $root 'Build\CharacterPreview\HairstyleRefinement'
New-Item -ItemType Directory -Force $out | Out-Null
$saved = @{}
foreach ($name in @('BLENDER_USER_RESOURCES', 'TEMP', 'TMP')) {
    $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
try {
    $env:BLENDER_USER_RESOURCES = Join-Path $out 'Resources'
    $env:TEMP = $out
    $env:TMP = $out
    $styles = if ($Style -eq 'All') { @('LongWave', 'Bob') } else { @($Style) }
    if ($Modular) { $styles = @('Modular') }
    foreach ($selected in $styles) {
        $script = 'build_hairstyle_refinement.py'
        $arguments = @('--style', $selected)
        if ($selected -eq 'Bob') {
            $script = 'build_hairstyle_bob_reuse.py'
            $arguments = @()
        }
        if ($selected -eq 'Modular') {
            $script = 'build_modular_hairstyle_refinement.py'
            $arguments = @()
        }
        if ($Preview) { $arguments += '--preview' }
        & $Blender --background --factory-startup --disable-autoexec --offline-mode --threads 2 `
            --python-exit-code 1 --python "$PSScriptRoot\$script" -- @arguments `
            *> "$out\$selected-build.log"
        if ($LASTEXITCODE -ne 0) {
            Get-Content "$out\$selected-build.log" -Tail 30
            throw "Hairstyle source export failed: $selected"
        }
        if ($selected -eq 'Bob') {
            & $Blender --background --factory-startup --disable-autoexec --offline-mode --threads 2 `
                --python-exit-code 1 --python "$PSScriptRoot\finalize_hairstyle_bundle.py" `
                *> "$out\Bob-consolidation.log"
            if ($LASTEXITCODE -ne 0) {
                Get-Content "$out\Bob-consolidation.log" -Tail 30
                throw 'Canonical stock Bob consolidation failed'
            }
        }
    }
}
finally {
    foreach ($name in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process')
    }
}
