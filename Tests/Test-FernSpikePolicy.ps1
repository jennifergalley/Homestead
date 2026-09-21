[CmdletBinding()]
param([Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if(-not $output.StartsWith((Join-Path $root 'Saved\Automation')+'\') -or (Test-Path $output)){throw 'Fresh disposable test output required.'}
$null=New-Item -ItemType Directory -Path $output
$cases=[Collections.Generic.List[string]]::new()
function Reject([string]$Name,[scriptblock]$Action) {
    $rejected=$false
    try {& $Action}catch{$rejected=$true}
    if(-not $rejected){throw "Unexpected admission:$Name"}
    $cases.Add($Name)
}
$trial=Join-Path $output 'dummy-trial'
$null=New-Item -ItemType Directory -Path $trial
$identity=[Homestead.Authoring.LeafGuard]::InspectDirectory($trial)
Assert-FernDirectoryIdentity $identity $trial
foreach($stem in Get-FernPackageStems) {
    $path=Join-Path $trial "$stem.uasset"
    $null=New-Item -ItemType Directory -Path (Split-Path $path -Parent) -Force
    [IO.File]::WriteAllBytes($path,[byte[]]@(0xc1,0x83,0x2a,0x9e,1))
}
$files=@(Get-FernPackageFiles $trial -Complete)
Assert-FernDirectoryIdentity $identity $trial
Assert-FernPackagePins $files @($files|Sort-Object path -Descending)
$moved=Join-Path $output 'dummy-discarded'
[IO.Directory]::Move($trial,$moved)
Assert-FernDirectoryIdentity $identity $moved
$null=New-Item -ItemType Directory -Path $trial
Reject 'replacement-directory' {Assert-FernDirectoryIdentity $identity $trial}
Reject 'incomplete-package-set' {Get-FernPackageFiles $trial -Complete}
[IO.File]::WriteAllText((Join-Path $trial 'unexpected.umap'),'dummy')
Reject 'unexpected-package-file' {Get-FernPackageFiles $trial}
$bad=@($files|ConvertTo-Json -Depth 5|ConvertFrom-Json)
$bad[0].sha256='0'*64
Reject 'package-content-pin' {Assert-FernPackagePins $files $bad}
$bad=@($files|ConvertTo-Json -Depth 5|ConvertFrom-Json)
$bad[0].path=$bad[1].path
Reject 'duplicate-package-path' {Assert-FernPackagePins $files $bad}
[IO.File]::WriteAllText((Join-Path $moved 'Meshes\SM_Fern02_a.uasset'),'wrong tag')
Reject 'invalid-package-tag' {Get-FernPackageFiles $moved -Complete}
$source=((Get-Content (Join-Path $root 'Assets\Environment\woodland-preparation-01\source-inventory.json') -Raw|ConvertFrom-Json).files|
    Where-Object {$_.asset -ceq 'fern_02' -and $_.file -ceq 'fern_02_1k.fbx'}).sourceInspection
$value=[ordered]@{passed=$true;namespace='/Game/Trials/Fern02_20260920_01';twoSided=$true;opacityClip=0.333;meshes=@();textures=@()}
foreach($suffix in @('a','b','c','d')) {
    $model=$source.models|Where-Object name -CEQ "fern_02_$suffix"
    $geo=$source.geometries|Where-Object id -EQ $model.geometryIds[0]
    $value.meshes+=@{sourceModelId=$model.id;sourceGeometryId=$geo.id;sourceNode=$model.name;triangles=$geo.fanTriangleEstimate;
        importedSlot='fern_02';object="/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_$suffix.SM_Fern02_$suffix";
        localPivotPreserved=$false;importUniformScale=1;transformVertexToAbsolute=$true;nanite=$false;
        sizeCm=@(0..2|ForEach-Object {100*($geo.localBounds.max[$_]-$geo.localBounds.min[$_])})}
}
$names=@('Diff','NormalDX','Roughness','AO','Alpha')
for($i=0;$i -lt 5;$i++){$value.textures+=@{object="/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_$($names[$i]).T_Fern02_$($names[$i])";
    width=1024;height=1024;flipGreen=$false;srgb=($i -eq 0);connectedOutputIndex=$(if($i -lt 2){0}else{1})}}
Assert-FernNativeInventory $value $source
foreach($name in @('wrong-node','wrong-triangles','double-scale','wrong-slot','pivot-claim','wrong-alpha-channel','map-colorspace','missing-map','wrong-namespace','failed-native')) {
    $copy=$value|ConvertTo-Json -Depth 10|ConvertFrom-Json
    switch($name) {
        'wrong-node' {$copy.meshes[0].sourceModelId=1}
        'wrong-triangles' {$copy.meshes[0].triangles=1}
        'double-scale' {$copy.meshes[0].sizeCm[0]*=100}
        'wrong-slot' {$copy.meshes[0].importedSlot='other'}
        'pivot-claim' {$copy.meshes[0].localPivotPreserved=$true}
        'wrong-alpha-channel' {$copy.textures[4].connectedOutputIndex=0}
        'map-colorspace' {$copy.textures[2].srgb=$true}
        'missing-map' {$copy.textures=@($copy.textures|Select-Object -First 4)}
        'wrong-namespace' {$copy.namespace='/Game/Accepted'}
        'failed-native' {$copy.passed=$false}
    }
    Reject $name {Assert-FernNativeInventory $copy $source}
}
$images=Join-Path $output 'dummy-image-headers'
$null=New-Item -ItemType Directory -Path $images
$render=@{width=1280;height=720;shaderMapComplete=$true;materialFallbackAllowed=$false;adapter='test-only';rhi='D3D12';views=@()}
$render.sceneState=@{isClient=$true;canEverRender=$true;worldScenePresent=$true;realRenderScene=$true;
    fernRegistered=$true;fernRenderStateCreated=$true;fernSceneProxyPresent=$true;captureRegistered=$true;captureVisible=$true}
foreach($name in @('fern-a-front.png','fern-a-back.png')) {
    $header=[Convert]::FromHexString('89504E470D0A1A0A0000000D4948445200000500000002D0')
    $bytes=[byte[]]::new(64);[Array]::Copy($header,$bytes,$header.Length)
    [IO.File]::WriteAllBytes((Join-Path $images $name),$bytes)
    $render.views+=@{image=$name;greenPixelCount=50;screenPercentageShowFlag=$false;motionBlur=$false;
        captureCallReturned=$true;renderCommandsFlushed=$true;readbackSucceeded=$true;pixelCount=1280*720}
}
Assert-FernRenderImages $render $images
foreach($field in @('realRenderScene','fernRegistered','fernSceneProxyPresent')) {
    $render.sceneState[$field]=$false
    Reject "missing-$field" {Assert-FernRenderImages $render $images}
    $render.sceneState[$field]=$true
}
foreach($field in @('captureCallReturned','renderCommandsFlushed','readbackSucceeded')) {
    $render.views[0][$field]=$false
    Reject "missing-$field" {Assert-FernRenderImages $render $images}
    $render.views[0][$field]=$true
}
$render.views[0].pixelCount=0
Reject 'wrong-readback-pixel-count' {Assert-FernRenderImages $render $images}
$render.views[0].pixelCount=1280*720
$render.materialFallbackAllowed=$true
Reject 'fallback-material' {Assert-FernRenderImages $render $images}
$render.materialFallbackAllowed=$false
$render.views[0].greenPixelCount=0
Reject 'no-fern-pixels' {Assert-FernRenderImages $render $images}
$render.views[0].greenPixelCount=50
$path=Join-Path $images 'fern-a-front.png'
$bytes=[IO.File]::ReadAllBytes($path);$bytes[18]=4;[IO.File]::WriteAllBytes($path,$bytes)
Reject 'wrong-native-image-dimensions' {Assert-FernRenderImages $render $images}
@{status='passed';negativeCases=@($cases);positiveCases=@('directory-identity-across-writes-and-move','ten-dummy-package-file-gate',
    'order-independent-package-pins','source-sized-native-inventory','image-header-and-RHI-gates');
    limit='Disposable mechanism/data tests only; dummy bytes are not real Unreal packages or decoded PNGs.'}|
    ConvertTo-Json -Depth 5|Set-Content (Join-Path $output 'result.json')
Get-Content (Join-Path $output 'result.json') -Raw
