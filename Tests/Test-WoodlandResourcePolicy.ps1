$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
. (Join-Path $root 'Scripts\TreeSpikePolicy.ps1')
. (Join-Path $root 'Scripts\WoodlandResourcePolicy.ps1')
function Reject([scriptblock]$Action){
    try{& $Action}catch{return}
    throw 'Invalid synthetic woodland fixture was accepted.'
}
$stems=@(Get-WoodlandPackageStems)
$maps=@(Get-WoodlandTextureMaps)
if($stems.Count -ne 38 -or @($stems|Select-Object -Unique).Count -ne 38 -or $maps.Count -ne 23){
    throw 'Exact resource package/map closure differs.'
}
$configs=[ordered]@{}
foreach($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')){
    $configs[$name]="E:\synthetic\Config\$name.ini"
}
foreach($mode in @('WoodlandImport','WoodlandVerify')){
    Reject {Get-FernOperationPolicy $mode Standard}
    $policy=Get-FernOperationPolicy $mode CompletionDriven
    $tokens=@(Get-FernProbeArguments 'E:\synthetic\Game.uproject' 'E:\synthetic\Output' 'E:\synthetic\DDC' $configs $mode)
    if($policy.profile -cne 'CookCompletionDriven' -or $policy.hardSeconds -ne 0 -or
        "-FernMode=$mode" -cnotin $tokens -or '-nullrhi' -cnotin $tokens -or '-DisablePython' -cnotin $tokens -or
        '-noshaderworker' -cnotin $tokens -or '-AllowCommandletRendering' -cin $tokens){throw 'Production woodland mode tokens/policy differ.'}
}
Reject {Get-FernProbeArguments 'E:\p' 'E:\o' 'E:\d' $configs 'WoodlandAnything'}
if((Get-FernOperationPolicy Settings Standard).hardSeconds -ne 110){throw 'Existing settings duration changed.'}
$selection=Get-Content (Join-Path $root 'Assets\Environment\WoodlandResources\candidate01\selection.json') -Raw|ConvertFrom-Json
$inventory=Get-Content (Join-Path $root 'Assets\Environment\WoodlandResources\candidate01\source-inventory.json') -Raw|ConvertFrom-Json
$models=@($selection.assets|ForEach-Object {$_.models})
$names=@(Get-WoodlandMeshNames)
$prefix='/Game/Trials/WoodlandResources_20260921_01'
$resources=@(for($i=0;$i -lt 9;$i++){
    $model=$models[$i];$name=$names[$i]
    $attributes=@($inventory.files|Where-Object file -Like '*.fbx'|ForEach-Object {$_.sourceInspection.roleSpecificAttributes}|Where-Object geometryId -EQ $model.geometryId)[0]
    $size=@(0..2|ForEach-Object {100*($attributes.referencedLocalBounds.max[$_]-$attributes.referencedLocalBounds.min[$_])})
    $sapling=@($model.materials).Count -eq 2
    $slots=@(for($s=0;$s -lt @($model.materials).Count;$s++){
        $map=@($maps|Where-Object role -CEQ $model.materials[$s])[0]
        @{slot=$s;role=$model.materials[$s];material="$prefix/Materials/$($map.material).$($map.material)";
            triangles=$(if($sapling){$model.sourceFanTrianglesByRole[$s]}else{$model.sourceFanTriangles});activeUvChannel=0}
    })
    $lods=@(@{lod=0;triangles=$model.sourceFanTriangles;screenSize=1})
    if($sapling){$lods+=@{lod=1;triangles=[Math]::Floor($model.sourceFanTriangles*0.25);screenSize=0.35}}
    @{object="$prefix/Meshes/$name.$name";sourceNode=$model.name;sourceModelId=$model.id;sourceGeometryId=$model.geometryId;
        sourceTriangles=$model.sourceFanTriangles;cameraOnlyCrownCollision=$sapling;slots=$slots;lods=$lods;
        boundsMinCm=@(0,0,0);boundsMaxCm=$size;sourceDimensionsTimes100Cm=$size;placementGroundAnchorCm=@(($size[0]/2),($size[1]/2),0)}
})
$fixture=@{passed=$true;cancelledAtPollingBoundary=$false;namespace=$prefix;mode='WoodlandImport';packages=38;resources=$resources;
    canopy=@{object="$prefix/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland";
        source='/Game/Trials/TreeSmall02_20260921_01/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2';
        qualification='Retains twelve branch corners; original import03 remains FAILED';
        lods=@(@{lod=0;triangles=231785;screenSize=1},@{lod=1;triangles=57946;screenSize=0.35},@{lod=2;triangles=13907;screenSize=0.12})};
    materials=@($maps|Select-Object material,role,masked -Unique|ForEach-Object {
        @{object="$prefix/Materials/$($_.material).$($_.material)";sourceRole=$_.role;masked=$_.masked;twoSided=$_.masked;opacityClip=0.333}});
    textures=@($maps|ForEach-Object {@{object="$prefix/Textures/$($_.name).$($_.name)";source="$($_.asset)/$($_.file)";
        width=1024;height=1024;compression=$_.compression;srgb=$_.srgb;flipGreen=$false}});
    persistentObjectReferences=@("$prefix/Materials/M_Shrub04.M_Shrub04")
}
$json=$fixture|ConvertTo-Json -Depth 20
Assert-WoodlandNativeInventory ($json|ConvertFrom-Json) $root
foreach($mutation in @(
    {$script:value.packages=39},
    {$script:value.resources[0].sourceModelId=0},
    {$script:value.resources[5].slots[0].role='fir_sapling_twigs'},
    {$script:value.resources[6].boundsMaxCm[2]*=2},
    {$script:value.resources[0].placementGroundAnchorCm[2]=10},
    {$script:value.canopy.lods[2].triangles=18001},
    {$script:value.textures[1].flipGreen=$true},
    {$script:value.materials[0].masked=$false},
    {$script:value.persistentObjectReferences=@('/Game/Unexpected.Asset')}
)){
    $script:value=$json|ConvertFrom-Json
    & $mutation
    Reject {Assert-WoodlandNativeInventory $script:value $root}
}
'Woodland offline package, production-mode token, measured-unit/material/LOD and negative inventory checks passed; synthetic data is not a native import.'
