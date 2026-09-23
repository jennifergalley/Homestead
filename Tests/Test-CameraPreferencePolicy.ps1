[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$character = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\HomesteadCharacter.cpp') -Raw
$controller = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\HomesteadController.cpp') -Raw
$menu = Get-Content -LiteralPath (Join-Path $root 'Source\SurvivalGame\UI\SHomesteadMenu.cpp') -Raw

if ($character -notmatch [regex]::Escape('(PC->bInvertY ? -1.0f : 1.0f)')) {
    throw 'Non-inverted camera pitch does not use the corrected sign.'
}
foreach ($required in @('LoadCameraPreferences();','PersistCameraSensitivity(','PersistCameraInversion(',
    'UpdateSinglePropertyInSection','CameraSettingsSection','CameraSensitivityKey','CameraInvertYKey')) {
    if ($controller -notmatch [regex]::Escape($required)) { throw "Missing camera preference contract: $required" }
}
$legacySensitivity = $controller -match 'Sensitivity = Save\.CameraSensitivity'
$legacyInversion = $controller -match 'bInvertY = Save\.InvertCameraY'
if ($legacySensitivity -or $legacyInversion) {
    throw 'World saves still override user-level camera preferences.'
}
foreach ($required in @('IsDirectCameraSetting','Row.Id == 3 || Row.Id == 4',
    'else if (!IsDirectCameraSetting(Row)) Actions.Add',
    'RunAction(EHomesteadItemAction::Primary)')) {
    if ($menu -notmatch [regex]::Escape($required)) { throw "Missing direct camera-row contract: $required" }
}
Write-Output 'PASS camera preference source policy: corrected pitch, isolated persistence, save independence, direct left rows.'
