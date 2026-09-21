[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
. (Join-Path $root 'Scripts\TreeSpikePolicy.ps1')
$prefix='/Game/Trials/TreeSmall02_20260921_01'
$roles=@('tree_small_02_branches','tree_small_02_leaves','tree_small_02_trunk')
$names=@('M_TreeSmall02_Branches','M_TreeSmall02_Leaves','M_TreeSmall02_Trunk')
$fixture=@{passed=$true;cancelledAtPollingBoundary=$false;namespace=$prefix;mode='TreeImport';packages=17
    mesh="$prefix/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2";triangles=231785;uvChannels=2;nanite=$false
    importUniformScale=1;convertScene=$true;convertSceneUnit=$true;transformVertexToAbsolute=$true
    boundsMinCm=@(-131.072414,-138.302326,-2.419423);boundsMaxCm=@(161.071074,290.762329,453.984356)
    lowerTrunkMinCm=@(-15,-15,-2);lowerTrunkMaxCm=@(15,15,200);collisionCenterCm=@(0,0,99)
    collisionRadiusCm=22;collisionCylinderLengthCm=158;measuredTrunkVertexSamples=1000
    meshDescriptionBasisAfter=@(for($i=0;$i -lt 3;$i++){
        @{role=$roles[$i];invalidNormalCorners=0;invalidTangentCorners=0;invalidBinormalCorners=0}
    })
    slots=@(for($i=0;$i -lt 3;$i++){
        @{index=$i;sourceRoleIndex=$i;role=$roles[$i];triangles=@(23702,193938,14145)[$i]
            sourceActiveUvChannel=$(if($i -eq 0){1}else{0});runtimeActiveUvChannel=0
            runtimeActiveUvDegenerateTriangles=0;runtimeInactiveUvDegenerateTriangles=@(23702,193938,14145)[$i]
            nearZeroImportedNormalCorners=0
            material="$prefix/Materials/$($names[$i]).$($names[$i])"}
    })
    materials=@(for($i=0;$i -lt 3;$i++){
        @{object="$prefix/Materials/$($names[$i]).$($names[$i])";shadingModel='DefaultLit'
            twoSided=($i -eq 1);masked=($i -eq 1);opacityClip=0.5}
    })
    textures=@(Get-TreeTextureMaps|ForEach-Object {
        @{object="$prefix/Textures/$($_.name).$($_.name)";source=$_.file;width=2048;height=2048
            srgb=$_.srgb;compression=$_.compression;flipGreen=$false
            materialRole=[Array]::IndexOf(@('Branch','Leaves','Trunk'),$_.role);connectedOutputIndex=$_.outputIndex}
    })
    persistentObjectReferences=@("$prefix/Materials/M_TreeSmall02_Leaves.M_TreeSmall02_Leaves")
}
$json=$fixture|ConvertTo-Json -Depth 12
Assert-TreeNativeInventory ($json|ConvertFrom-Json)
$reordered=$json|ConvertFrom-Json
$first=$reordered.slots[0];$reordered.slots[0]=$reordered.slots[2];$reordered.slots[2]=$first
$reordered.slots[0].index=0;$reordered.slots[2].index=2
Assert-TreeNativeInventory $reordered
$diagnostic=$json|ConvertFrom-Json
$diagnostic.meshDescriptionBasisAfter[0].invalidTangentCorners=12
$diagnostic.meshDescriptionBasisAfter[0].invalidBinormalCorners=12
Assert-TreeNativeInventory $diagnostic -KnownTangentDiagnostic
$rejected=$false
try{Assert-TreeNativeInventory $diagnostic}catch{$rejected=$true}
if(-not $rejected){throw 'Known-defect diagnostic was admitted as a clean inventory.'}
$diagnosticJson=$diagnostic|ConvertTo-Json -Depth 12
$negatives=0
foreach($change in @(
    {$v.passed=$false},{$v.cancelledAtPollingBoundary=$true},{$v.namespace='/Game/Other'},
    {$v.mode='Render'},{$v.packages=18},{$v.mesh='/Game/Other.Mesh'},{$v.triangles=2062487},
    {$v.uvChannels=1},{$v.nanite=$true},{$v.importUniformScale=100},{$v.convertScene=$false},
    {$v.convertSceneUnit=$false},{$v.transformVertexToAbsolute=$false},
    {$v.boundsMinCm[0]=[double]::NaN},{$v.boundsMaxCm[2]=4.5},{$v.boundsMinCm[2]=0},
    {$v.boundsMinCm[1]=-138.367891;$v.boundsMaxCm[1]=291.184974},
    {$v.boundsMinCm[1]=-290.762329;$v.boundsMaxCm[1]=138.302326},
    {$v.lowerTrunkMinCm[2]=-20},{$v.lowerTrunkMaxCm[2]=400},{$v.collisionCenterCm[0]=100},
    {$v.collisionRadiusCm=100},{$v.collisionRadiusCm=[double]::NaN},
    {$v.collisionCylinderLengthCm=0},{$v.collisionCylinderLengthCm++},{$v.measuredTrunkVertexSamples=0},
    {$v.slots=$v.slots[0..1]},{$v.slots[0].sourceRoleIndex=8},{$v.slots[0].sourceRoleIndex=1},
    {$v.slots[0].role='wrong'},{$v.slots[0].triangles++},{$v.slots[0].index=2},
    {$v.slots[0].sourceActiveUvChannel=0},{$v.slots[1].sourceActiveUvChannel=1},
    {$v.slots[0].runtimeActiveUvChannel=1},{$v.slots[0].runtimeActiveUvDegenerateTriangles=23702},
    {$v.slots[1].runtimeInactiveUvDegenerateTriangles=0},{$v.slots[0].nearZeroImportedNormalCorners=338},
    {$v.slots[0].PSObject.Properties.Remove('sourceActiveUvChannel')},
    {$v.meshDescriptionBasisAfter=@()},{$v.meshDescriptionBasisAfter[0].role='wrong'},
    {$v.meshDescriptionBasisAfter[1].invalidNormalCorners=1},{$v.meshDescriptionBasisAfter[0].invalidTangentCorners=1},
    {$v.meshDescriptionBasisAfter[0].invalidBinormalCorners=1},
    {$v.slots[0].material='/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'},
    {$v.materials[1].masked=$false},{$v.materials[1].twoSided=$false},{$v.materials[1].opacityClip=0.333},
    {$v.materials[0].twoSided=$true},{$v.materials[2].shadingModel='Unlit'},
    {$v.materials=$v.materials[0..1]},{$v.textures=$v.textures[0..11]},
    {$v.textures[0].object='/Game/Other.Texture'},{$v.textures[0].source='wrong.png'},
    {$v.textures[0].width=1024},{$v.textures[0].height=1024},{$v.textures[0].srgb=$false},
    {$v.textures[1].srgb=$true},{$v.textures[1].compression=0},{$v.textures[1].flipGreen=$true},
    {$v.textures[8].materialRole=0},{$v.textures[8].connectedOutputIndex=0},
    {$v.persistentObjectReferences=@('/Game/Other.Texture')}
)) {
    $v=$json|ConvertFrom-Json
    & $change
    $rejected=$false
    try{Assert-TreeNativeInventory $v}catch{$rejected=$true}
    if(-not $rejected){throw "Invalid tree inventory admitted:$change"}
    $v=$diagnosticJson|ConvertFrom-Json
    & $change
    $rejected=$false
    try{Assert-TreeNativeInventory $v -KnownTangentDiagnostic}catch{$rejected=$true}
    if(-not $rejected){throw "Diagnostic weakened another tree gate:$change"}
    $negatives++
}
$configs=[ordered]@{}
foreach($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')){
    $configs[$name]="E:\fixture\Config\$name.ini"
}
foreach($mode in @('TreeImport','TreeVerify')) {
    $policy=Get-FernOperationPolicy $mode CompletionDriven
    if($policy.profile -cne 'CookCompletionDriven' -or $policy.softSeconds -ne 0 -or $policy.hardSeconds -ne 0){
        throw 'Tree mode changed existing completion-driven primitive.'
    }
    $tokens=@(Get-FernProbeArguments 'E:\fixture\Game.uproject' 'E:\fixture\Output' 'E:\fixture\DDC' $configs $mode)
    foreach($token in @("-FernMode=$mode",'-nullrhi','-DisablePython','-noshaderworker')){
        if($token -cnotin $tokens){throw "Missing production tree token:$token"}
    }
    if('-AllowCommandletRendering' -in $tokens -or '-RunAsCookCommandlet' -in $tokens){throw 'Tree asset modes must not render/cook.'}
    foreach($profile in @('Standard','LongStartup')) {
        $rejected=$false
        try{$null=Get-FernOperationPolicy $mode $profile}catch{$rejected=$true}
        if(-not $rejected){throw 'Unexpected tree timeout profile admitted.'}
        $negatives++
    }
}
$pins=@(Get-TreeSourcePins $root)
$retainedDiagnostic=Get-TreeFailedImportDiagnosticAdmission $root
if(@($retainedDiagnostic.packages).Count -ne 17){throw 'Retained diagnostic package scope differs.'}
$cookConfig=Get-Content (Join-Path $root 'Config\DefaultGame.ini')
if(@($cookConfig|Where-Object {$_ -ceq '+DirectoriesToAlwaysCook=(Path="/Game/Trials/TreeSmall02_20260921_01")'}).Count -ne 1 -or
    'bCookAll=True' -cin $cookConfig){throw 'Exact runtime-loaded tree cook directory is missing or broadened.'}
$stems=@(Get-TreePackageStems)
if($pins.Count -ne 19 -or $stems.Count -ne 17 -or @($stems|Select-Object -Unique).Count -ne 17){
    throw 'Exact tree source/output scope differs.'
}
"PASS tree policy:$negatives rejected mutations/profiles; inventory mutations also rejected under exact12-corner diagnostic; diagnostic rejected as clean; reordered-role control,19 source pins,17 stems."
