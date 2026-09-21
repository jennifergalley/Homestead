function Get-GrassTextureMaps {
    foreach($group in @('GrassMedium01','GrassGround')) {
        $prefix=if($group -ceq 'GrassMedium01'){'grass_medium_01'}else{'grass_ground'}
        $size=if($group -ceq 'GrassMedium01'){'1k'}else{'2k'}
        foreach($map in @('Diff','NormalDX','Roughness') + $(if($size -ceq '1k'){@('AO','Alpha')}else{@()})) {
            $suffix=switch($map){Diff{'diff'} NormalDX{'nor_dx'} Roughness{'rough'} AO{'ao'} Alpha{'alpha'}}
            [pscustomobject]@{asset=$prefix;name="T_${group}_$map";file="${prefix}_${suffix}_$size.png"
                pixels=$(if($size -ceq '1k'){1024}else{2048});srgb=($map -ceq 'Diff')
                compression=$(if($map -ceq 'Diff'){0}elseif($map -ceq 'NormalDX'){1}else{2})}
        }
    }
}

function Get-GrassPackageStems {
    @('mid_b','small_b','tall_a','tiny_a'|ForEach-Object {"Meshes\SM_GrassMedium01_$_"}) +
        @('Materials\M_GrassMedium01','Materials\M_GrassGroundBlend') +
        @(Get-GrassTextureMaps|ForEach-Object {"Textures\$($_.name)"})
}

function Get-GrassSourcePins([string]$Root) {
    $pins=@(
        @{path='Assets\Environment\woodland-preparation-01\download-receipt.json';sha256='AACF677F7B8CA07A5656842EA80AFEDC80845E509AFE8731F198DA973F339ECF'},
        @{path='Assets\Environment\woodland-preparation-01\source-inventory.json';sha256='B38DCE70440F97817B3A8C8372D25EFC6B72BEE47E7078FC1D8ED207C1C0AAF4'},
        @{path='Assets\Environment\GrassMedium01Prepared\v1\GrassMedium01_Selected.fbx';sha256='4D2E305A8166532EF5A3AB326E6FDF5F1E0F809D21C677A4FD7D3E14A623C142'},
        @{path='Assets\Environment\GrassMedium01Prepared\v1\provenance.json';sha256='55FB1E545A837AF1AFD4B07FD641339AC22DDAE2B5B8AFC2966CAC691F9E61F8'},
        @{path='Saved\Automation\20260921-033354-2d257ba0\grass-source-prep-01\result.json';sha256='97ADC0852D853E12B3B877C20EE4970410113E868277B1D9BD79ABE45E258441'},
        @{path='Content\SurvivalGame\Textures\T_GroundColor.uasset';sha256='C25CCD49354E72B9485E4FD3089151933614D0AAB2F7F063C8ED5F75DB60ACE2'},
        @{path='Content\SurvivalGame\Textures\T_GroundNormal.uasset';sha256='A9F1D44B03AAF9FEF3ED158E8A7BAF524E6F5D232EA1A94F5EDEE7C3ACAAE5B0'},
        @{path='Content\SurvivalGame\Textures\T_GroundRoughness.uasset';sha256='A66084D29605F8B4E5114B729F7B9C0B9B9F339D4C5BEEBE6550D62492D3EA57'}
    )
    Assert-FernOrdinaryTree (Join-Path $Root 'Assets\Environment\GrassMedium01Prepared\v1')
    foreach($pin in $pins) {
        $pin.path=Join-Path $Root $pin.path
        if((Get-FileHash $pin.path).Hash -cne $pin.sha256){throw "Grass source/proof/read-only input differs:$($pin.path)"}
    }
    $receipt=Get-Content $pins[0].path -Raw|ConvertFrom-Json
    $source=Join-Path $Root 'Assets\Source\woodland-preparation-20260920-182217-d1f84e39'
    foreach($map in @(Get-GrassTextureMaps)) {
        $entries=@($receipt.files|Where-Object {$_.asset -ceq $map.asset -and $_.file -ceq $map.file})
        if($entries.Count -ne 1){throw 'Missing unique grass/ground source map.'}
        $file=Join-Path $source "$($map.asset)\$($map.file)"
        if((Get-Item $file).Length -ne $entries[0].bytes -or
            (Get-FileHash $file).Hash -cne $entries[0].sha256.ToUpperInvariant()){throw "Original map differs:$($map.file)"}
        $pins+=@{path=$file;sha256=$entries[0].sha256.ToUpperInvariant()}
    }
    $pins
}

function Assert-GrassNativeInventory($Value) {
    $prefix='/Game/Trials/GrassGround_20260921_01'
    if(-not $Value.passed -or $Value.cancelledAtPollingBoundary -or $Value.namespace -cne $prefix -or
        $Value.mode -cnotin @('GrassImport','GrassVerify') -or $Value.packages -ne 14 -or
        @($Value.meshes).Count -ne 4 -or @($Value.textures).Count -ne 8 -or $Value.totalMeshTriangles -ne 2279 -or
        $Value.grassMaterial -cne "$prefix/Materials/M_GrassMedium01.M_GrassMedium01" -or
        $Value.groundMaterial -cne "$prefix/Materials/M_GrassGroundBlend.M_GrassGroundBlend"){
        throw 'Grass/ground inventory scope/material/geometry differs.'
    }
    $names=@('mid_b','small_b','tall_a','tiny_a')
    $triangles=@(1257,653,290,79)
    $models=@(9907750,854938463,515132540,99195606)
    $geometry=@(34972056,556848324,425513347,697680708)
    $sizes=@(@(18.778882,20.102860,17.772157),@(16.595670,18.989212,9.989528),
        @(16.095641,15.853148,32.254639),@(6.734776,7.376876,11.245334))
    for($i=0;$i -lt 4;$i++){
        $mesh=$Value.meshes[$i];$name="SM_GrassMedium01_$($names[$i])"
        if($mesh.object -cne "$prefix/Meshes/$name.$name" -or
            $mesh.sourceNode -cne "grass_medium_01_$($names[$i])_LOD0" -or
            $mesh.sourceModelId -ne $models[$i] -or $mesh.sourceGeometryId -ne $geometry[$i] -or
            $mesh.sourceTriangles -ne $triangles[$i] -or $mesh.renderTriangles -ne $triangles[$i] -or
            $mesh.importedSlot -cne 'grass_medium_01' -or $mesh.material -cne $Value.grassMaterial -or
            $mesh.importUniformScale -ne 1 -or $mesh.nanite -or $mesh.simpleCollision){throw "Grass clump differs:$name"}
        foreach($field in @('boundsMinCm','boundsMaxCm','sourceDimensionsTimes100Cm','placementGroundAnchorCm')) {
            if(@($mesh.$field).Count -ne 3 -or @($mesh.$field|Where-Object {-not [double]::IsFinite($_)}).Count){
                throw "Malformed/nonfinite grass vector:$field"
            }
        }
        $actual=@(for($axis=0;$axis -lt 3;$axis++){
            if($mesh.boundsMinCm[$axis] -ge $mesh.boundsMaxCm[$axis]){throw 'Inverted/empty clump bounds.'}
            $anchor=if($axis -eq 2){$mesh.boundsMinCm[$axis]}else{($mesh.boundsMinCm[$axis]+$mesh.boundsMaxCm[$axis])/2}
            if([Math]::Abs($mesh.placementGroundAnchorCm[$axis]-$anchor) -gt 0.01 -or
                [Math]::Abs($mesh.sourceDimensionsTimes100Cm[$axis]-$sizes[$i][$axis]) -gt 0.0001){throw 'Grass anchor/source frame differs.'}
            $mesh.boundsMaxCm[$axis]-$mesh.boundsMinCm[$axis]
        })
        $actual=@($actual|Sort-Object)
        $expected=@($sizes[$i]|Sort-Object)
        for($axis=0;$axis -lt 3;$axis++){
            if([Math]::Abs($actual[$axis]-$expected[$axis]) -gt 0.1){throw 'Grass imported units/dimensions differ.'}
        }
    }
    $maps=@(Get-GrassTextureMaps)
    for($i=0;$i -lt 8;$i++){
        $texture=$Value.textures[$i];$map=$maps[$i]
        if($texture.object -cne "$prefix/Textures/$($map.name).$($map.name)" -or $texture.source -cne $map.file -or
            $texture.width -ne $map.pixels -or $texture.height -ne $map.pixels -or $texture.srgb -ne $map.srgb -or
            $texture.compression -ne $map.compression -or $texture.flipGreen){throw "Grass/ground texture differs:$($map.name)"}
    }
    $allowed=@(Get-GrassPackageStems|ForEach-Object {"$prefix/$($_.Replace('\','/'))"}) +
        @('/Game/SurvivalGame/Textures/T_GroundColor','/Game/SurvivalGame/Textures/T_GroundNormal','/Game/SurvivalGame/Textures/T_GroundRoughness')
    if(-not @($Value.persistentObjectReferences).Count){throw 'Missing grass/ground persistent reference audit.'}
    foreach($reference in $Value.persistentObjectReferences){
        if($reference.Split('.')[0] -cnotin $allowed){throw 'Grass/ground references an unexpected package.'}
    }
}
