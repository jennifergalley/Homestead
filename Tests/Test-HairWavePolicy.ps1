[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
. (Join-Path $root 'Scripts\HairWavePolicy.ps1')
$prefix='/Game/Trials/HeroineWave_20260921_01'
$names=@(Get-HairWaveNames)
$triangles=@(120440,123976,120588,124124,120452,123988)
$material="$prefix/Materials/M_Heroine_Hair_long01_Neutral.M_Heroine_Hair_long01_Neutral"
$fixture=@{
    passed=$true;cancelledAtPollingBoundary=$false;namespace=$prefix;material=$material
    texture="$prefix/Textures/T_LongWave_Neutral.T_LongWave_Neutral"
    meshes=@(for($i=0;$i -lt 6;$i++){
        $name=$names[$i]
        @{object="$prefix/Meshes/$name.$name";incumbent="/Game/SurvivalGame/Characters/Heroine/$name.$name"
            skeleton='/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton.SK_Heroine_LongWave_Skeleton'
            bones=54;referencePoseMatchesIncumbent=$true;triangles=$triangles[$i];heightCm=164
            slots=@(for($j=0;$j -lt $(if($i%2){14}else{12});$j++){
                @{index=$j;slot=$(if($j -eq 7){'M_Heroine_Hair_long01_Neutral'}else{"old$j"})
                    material=$(if($j -eq 7){$material}else{"/Game/SurvivalGame/Characters/Heroine/Materials/old$j.old$j"})}
            })
        }
    })
}
$json=$fixture|ConvertTo-Json -Depth 12
Assert-HairWaveNativeInventory ($json|ConvertFrom-Json)
$negative=0
foreach($change in @(
    {$v.passed=$false},{$v.cancelledAtPollingBoundary=$true},{$v.namespace='/Game/SurvivalGame'},
    {$v.texture='/Game/Other.Texture'},{$v.material='/Game/Other.Material'},
    {$v.meshes=$v.meshes[0..4]},{$v.meshes[1].object=$v.meshes[0].object},
    {$v.meshes[0].skeleton='/Game/Other.Skeleton'},{$v.meshes[0].bones=53},
    {$v.meshes[0].referencePoseMatchesIncumbent=$false},{$v.meshes[0].triangles=1},
    {$v.meshes[0].heightCm=1.64},{$v.meshes[0].heightCm=[double]::NaN},
    {$v.meshes[0].slots[7].slot='oldHair'},{$v.meshes[0].slots[7].material='/Game/Other.Material'},
    {$v.meshes[1].slots=$v.meshes[1].slots[0..11]}
)) {
    $v=$json|ConvertFrom-Json
    & $change
    $rejected=$false
    try{Assert-HairWaveNativeInventory $v}catch{$rejected=$true}
    if(-not $rejected){throw "Negative wave inventory admitted:$change"}
    $negative++
}
$configs=[ordered]@{}
foreach($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')){
    $configs[$name]="E:\fixture\Config\$name.ini"
}
foreach($mode in @('HairImport','HairVerify')) {
    $policy=Get-FernOperationPolicy -Mode $mode -RenderProfile CompletionDriven
    if($policy.profile -cne 'CookCompletionDriven' -or $policy.softSeconds -ne 0 -or $policy.hardSeconds -ne 0 -or $policy.ceilingSeconds -ne 0){throw 'Existing completion-driven primitive differs.'}
    $tokens=@(Get-FernProbeArguments 'E:\fixture\Game.uproject' 'E:\fixture\Output' 'E:\fixture\DDC' $configs $mode)
    foreach($required in @("-FernMode=$mode",'-nullrhi','-DisablePython','-noshaderworker')){
        if($required -cnotin $tokens){throw "Missing production token:$required"}
    }
    if('-AllowCommandletRendering' -in $tokens -or '-RenderOffScreen' -in $tokens){throw 'Unexpected rendering in hair import/verify.'}
    foreach($profile in @('Standard','LongStartup')){
        $rejected=$false
        try{$null=Get-FernOperationPolicy -Mode $mode -RenderProfile $profile}catch{$rejected=$true}
        if(-not $rejected){throw 'Hair operation admitted an unapproved timer profile.'}
        $negative++
    }
}
$pins=@(Get-HairWaveSourcePins $root)
if($pins.Count -ne 8){throw 'Exact six FBXs, neutral texture and manifest required.'}
$stems=@(Get-HairWavePackageStems)
if($stems.Count -ne 8 -or @($stems|Select-Object -Unique).Count -ne 8){throw 'Exact unique eight-package inventory required.'}
"PASS: source pins, six-mesh inventory, production token/profile checks; $negative negative cases."
