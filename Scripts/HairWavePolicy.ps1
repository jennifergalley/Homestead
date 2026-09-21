function Get-HairWaveNames {
    @('SK_Heroine_LongWave','SK_Heroine_LongWave_Apron','SK_Heroine_Willow_LongWave',
        'SK_Heroine_Willow_LongWave_Apron','SK_Heroine_Hazel_LongWave','SK_Heroine_Hazel_LongWave_Apron')
}

function Get-HairWavePackageStems {
    @('Textures\T_LongWave_Neutral','Materials\M_Heroine_Hair_long01_Neutral') +
        @(Get-HairWaveNames | ForEach-Object { "Meshes\$_" })
}

function Get-HairWaveSourcePins([string]$Root) {
    $source=Join-Path $Root 'Assets\Characters\HairstyleRefinement'
    Assert-FernOrdinaryTree $source
    $manifestPath=Join-Path $source 'LongWave-manifest.json'
    if((Get-FileHash $manifestPath).Hash -cne 'CB6E61CD7624CC5535053BA2D503D3655A064BFA48D936AA61FED6BC8880CAC8') {
        throw 'Frozen six-wave manifest changed.'
    }
    $manifest=Get-Content $manifestPath -Raw | ConvertFrom-Json
    $names=@(Get-HairWaveNames)
    if(@($manifest.entries).Count -ne 6){throw 'Exactly six frozen wave meshes required.'}
    $pins=@(foreach($entry in $manifest.entries) {
        $name=[IO.Path]::GetFileNameWithoutExtension($entry.fbx)
        if($name -cnotin $names -or $entry.fbx -cne "Joined\$name.fbx"){throw 'Unexpected frozen wave source path.'}
        $path=Join-Path $source $entry.fbx
        if((Get-FileHash $path).Hash -cne $entry.sha256.ToUpperInvariant()){throw "Frozen wave changed:$name"}
        [ordered]@{path=$path;sha256=(Get-FileHash $path).Hash}
    })
    $texture=Join-Path $source 'Textures\T_LongWave_Neutral.png'
    if((Get-FileHash $texture).Hash -cne 'DF985D44B9B708E32C478B86F0717C775C15825971D1FB2980F600DC3A0D3A55') {
        throw 'Frozen neutral atlas changed.'
    }
    foreach($property in $manifest.protected_inputs.PSObject.Properties) {
        if((Get-FileHash (Join-Path $Root $property.Name)).Hash -cne $property.Value.ToUpperInvariant()) {
            throw "Original hair input changed:$($property.Name)"
        }
    }
    $pins+=[ordered]@{path=$texture;sha256=(Get-FileHash $texture).Hash}
    $pins+=[ordered]@{path=$manifestPath;sha256=(Get-FileHash $manifestPath).Hash}
    $pins
}

function Assert-HairWaveNativeInventory($Value) {
    $prefix='/Game/Trials/HeroineWave_20260921_01'
    $material="$prefix/Materials/M_Heroine_Hair_long01_Neutral.M_Heroine_Hair_long01_Neutral"
    $names=@(Get-HairWaveNames)
    $triangles=@(120440,123976,120588,124124,120452,123988)
    if(-not $Value.passed -or $Value.cancelledAtPollingBoundary -or $Value.namespace -cne $prefix -or
        $Value.material -cne $material -or $Value.texture -cne "$prefix/Textures/T_LongWave_Neutral.T_LongWave_Neutral" -or
        @($Value.meshes).Count -ne 6){throw 'Native wave inventory identity/approval differs.'}
    for($index=0;$index -lt 6;$index++) {
        $mesh=$Value.meshes[$index]
        $name=$names[$index]
        if($mesh.object -cne "$prefix/Meshes/$name.$name" -or
            $mesh.incumbent -cne "/Game/SurvivalGame/Characters/Heroine/$name.$name" -or
            $mesh.skeleton -cne '/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton.SK_Heroine_LongWave_Skeleton' -or
            $mesh.bones -ne 54 -or -not $mesh.referencePoseMatchesIncumbent -or
            $mesh.triangles -ne $triangles[$index] -or -not [double]::IsFinite($mesh.heightCm) -or
            $mesh.heightCm -lt 155 -or $mesh.heightCm -gt 175 -or
            @($mesh.slots).Count -ne $(if($index % 2){14}else{12}) -or
            $mesh.slots[7].index -ne 7 -or $mesh.slots[7].slot -cne 'M_Heroine_Hair_long01_Neutral' -or
            $mesh.slots[7].material -cne $material){throw "Native skeletal/slot/units inventory differs:$name"}
    }
}
