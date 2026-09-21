[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
. (Join-Path $root 'Scripts\GrassGroundPolicy.ps1')
$prefix='/Game/Trials/GrassGround_20260921_01'
$names=@('mid_b','small_b','tall_a','tiny_a')
$sizes=@(@(18.778882,20.102860,17.772157),@(16.595670,18.989212,9.989528),
    @(16.095641,15.853148,32.254639),@(6.734776,7.376876,11.245334))
$fixture=@{
    passed=$true;cancelledAtPollingBoundary=$false;namespace=$prefix;mode='GrassImport';packages=14;totalMeshTriangles=2279
    grassMaterial="$prefix/Materials/M_GrassMedium01.M_GrassMedium01"
    groundMaterial="$prefix/Materials/M_GrassGroundBlend.M_GrassGroundBlend"
    meshes=@(for($i=0;$i -lt 4;$i++){
        $name="SM_GrassMedium01_$($names[$i])"
        @{object="$prefix/Meshes/$name.$name";sourceNode="grass_medium_01_$($names[$i])_LOD0"
            sourceModelId=@(9907750,854938463,515132540,99195606)[$i]
            sourceGeometryId=@(34972056,556848324,425513347,697680708)[$i]
            sourceTriangles=@(1257,653,290,79)[$i];renderTriangles=@(1257,653,290,79)[$i]
            importedSlot='grass_medium_01';material="$prefix/Materials/M_GrassMedium01.M_GrassMedium01"
            importUniformScale=1;nanite=$false;simpleCollision=$false;boundsMinCm=@(0,0,0)
            boundsMaxCm=$sizes[$i];sourceDimensionsTimes100Cm=$sizes[$i]
            placementGroundAnchorCm=@(($sizes[$i][0]/2),($sizes[$i][1]/2),0)}
    })
    textures=@(Get-GrassTextureMaps|ForEach-Object {
        @{object="$prefix/Textures/$($_.name).$($_.name)";source=$_.file;width=$_.pixels;height=$_.pixels
            srgb=$_.srgb;compression=$_.compression;flipGreen=$false}
    })
    persistentObjectReferences=@("$prefix/Materials/M_GrassMedium01.M_GrassMedium01",
        '/Game/SurvivalGame/Textures/T_GroundColor.T_GroundColor')
}
$json=$fixture|ConvertTo-Json -Depth 12
Assert-GrassNativeInventory ($json|ConvertFrom-Json)
$verified=$json|ConvertFrom-Json
$verified.mode='GrassVerify'
Assert-GrassNativeInventory $verified
$negatives=0
foreach($change in @(
    {$v.passed=$false},{$v.cancelledAtPollingBoundary=$true},{$v.namespace='/Game/Other'},{$v.mode='Cook'},
    {$v.packages=13},{$v.totalMeshTriangles=1606633},{$v.meshes=$v.meshes[0..2]},{$v.textures=$v.textures[0..6]},
    {$v.grassMaterial='/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'},{$v.groundMaterial='/Game/Other.Other'},
    {$v.meshes[0].sourceNode='wrong'},{$v.meshes[0].sourceModelId++},{$v.meshes[0].sourceGeometryId++},
    {$v.meshes[0].sourceTriangles++},{$v.meshes[0].renderTriangles++},{$v.meshes[0].importedSlot='wrong'},
    {$v.meshes[0].object='/Game/Other.Other'},{$v.meshes[0].material='/Game/Other.Other'},
    {$v.meshes[0].importUniformScale=100},{$v.meshes[0].nanite=$true},{$v.meshes[0].simpleCollision=$true},
    {$v.meshes[0].boundsMinCm[0]=[double]::NaN},{$v.meshes[0].boundsMaxCm[0]=0},
    {$v.meshes[0].boundsMaxCm[1]*=100},{$v.meshes[0].sourceDimensionsTimes100Cm[2]*=100},
    {$v.meshes[0].placementGroundAnchorCm[2]=1},{$v.meshes[0].placementGroundAnchorCm[0]=0},
    {$v.meshes[0].boundsMinCm=@(0,0)},{$v.meshes[0].PSObject.Properties.Remove('sourceModelId')},
    {$v.textures[0].source='dry.png'},{$v.textures[0].width=2048},{$v.textures[5].height=1024},
    {$v.textures[0].srgb=$false},{$v.textures[1].srgb=$true},{$v.textures[1].compression=0},
    {$v.textures[1].flipGreen=$true},{$v.textures[0].object='/Game/Other.Other'},
    {$v.persistentObjectReferences=@('/Game/Other.Other')},{$v.persistentObjectReferences=@()}
)){
    $v=$json|ConvertFrom-Json
    & $change
    $rejected=$false
    try{Assert-GrassNativeInventory $v}catch{$rejected=$true}
    if(-not $rejected){throw "Grass negative case admitted:$change"}
    $negatives++
}
$stems=@(Get-GrassPackageStems)
if($stems.Count -ne 14 -or @($stems|Sort-Object -Unique).Count -ne 14){throw 'Exact14-package contract differs.'}
$configs=[ordered]@{}
foreach($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')){
    $configs[$name]="C:\Owned\Config\$name.ini"
}
foreach($mode in @('GrassImport','GrassVerify')){
    $p=Get-FernOperationPolicy $mode 'CompletionDriven'
    if($p.profile -cne 'CookCompletionDriven' -or $p.softSeconds -ne 0 -or $p.hardSeconds -ne 0){throw 'Grass cancellation profile differs.'}
    $tokens=@(Get-FernProbeArguments 'C:\Owned\Project.uproject' 'C:\Owned\Output' 'C:\Owned\DDC' $configs $mode)
    if("-FernMode=$mode" -cnotin $tokens -or '-nullrhi' -cnotin $tokens -or '-DisablePython' -cnotin $tokens -or
        '-noshaderworker' -cnotin $tokens -or '-RenderOffScreen' -cin $tokens){throw 'Grass production token contract differs.'}
    $rejected=$false
    try{Get-FernOperationPolicy $mode 'Standard'|Out-Null}catch{$rejected=$true}
    if(-not $rejected){throw 'Grass standard timer silently admitted.'}
}
$pins=@(Get-GrassSourcePins $root)
if($pins.Count -ne 16){throw 'Selected source/texture/read-only input pin count differs.'}
"Grass policy passed:2 valid inventories,$negatives negative inventories,14 packages,16 real input pins,2 exact production mode/token profiles. No engine launched."
