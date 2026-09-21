[CmdletBinding()]
param(
    [switch]$VerifyOnly,
    [switch]$Render,
    [string]$ToolsRoot = 'E:\Tools'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$blender = Join-Path $ToolsRoot 'blender-4.5.14-windows-x64\blender.exe'
$sourceAddon = Join-Path $ToolsRoot 'BlenderCharacterUser\extensions\user_default\mpfb'
$work = Join-Path $root 'Build\CharacterPreview\ModularClothing'
$profile = Join-Path $work 'profile'
$addonParent = Join-Path $profile 'extensions\user_default'
$temp = Join-Path $work 'temp'
if (!(Test-Path -LiteralPath $blender)) { throw 'Verified Blender 4.5.14 is required.' }
New-Item -ItemType Directory -Force $work,$addonParent,$temp | Out-Null
if (!$VerifyOnly -and !(Test-Path -LiteralPath (Join-Path $addonParent 'mpfb'))) {
    if (!(Test-Path -LiteralPath $sourceAddon)) { throw 'Verified local MPFB 2.0.17 is required.' }
    Copy-Item -LiteralPath $sourceAddon -Destination $addonParent -Recurse
}
$previous = @{}
foreach ($key in 'BLENDER_USER_RESOURCES','TEMP','TMP') {
    $previous[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
}
try {
    $env:BLENDER_USER_RESOURCES = $profile
    $env:TEMP = $temp
    $env:TMP = $temp
    $stages = @()
    if (!$VerifyOnly) { $stages += 'build_modular_clothing.py' }
    $stages += 'verify_modular_clothing.py'
    foreach ($stage in $stages) {
        $log = Join-Path $work ($stage + '.log')
        $extra = @()
        if ($Render -and $stage -eq 'verify_modular_clothing.py') { $extra = @('--','--render') }
        & $blender --background --factory-startup --disable-autoexec --offline-mode --threads 2 `
            --python-exit-code 1 --python (Join-Path $PSScriptRoot $stage) @extra *> $log
        if ($LASTEXITCODE -ne 0) {
            Get-Content -LiteralPath $log -Tail 40
            throw "$stage failed ($LASTEXITCODE). See $log"
        }
        Write-Host "$stage completed: $log"
    }
}
finally {
    foreach ($key in $previous.Keys) {
        [Environment]::SetEnvironmentVariable($key, $previous[$key], 'Process')
    }
}
