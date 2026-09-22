function Get-WoodlandTextureMaps {
    $roles=@(
        @('shrub_04','shrub_04','Shrub04',$true),
        @('dry_branches_medium_01','dry_branches_medium_01','DryBranchesMedium01',$false),
        @('fir_sapling','fir_sapling_branches','FirSapling_Branches',$false),
        @('fir_sapling','fir_sapling_twigs','FirSapling_Twigs',$true),
        @('flower_empodium','flower_empodium','FlowerEmpodium',$true))
    foreach($role in $roles){
        foreach($channel in @('Diff','NormalDX','Roughness','AO') + $(if($role[3]){@('Alpha')}else{@()})){
            $suffix=switch($channel){Diff{'diff'} NormalDX{'nor_dx'} Roughness{'rough'} AO{'ao'} Alpha{'alpha'}}
            $extension=if($role[0] -ceq 'dry_branches_medium_01' -and $channel -ceq 'Diff'){'jpg'}else{'png'}
            [pscustomobject]@{asset=$role[0];role=$role[1];material="M_$($role[2])";masked=$role[3];
                name="T_$($role[2])_$channel";file="$($role[1])_${suffix}_1k.$extension";srgb=($channel -ceq 'Diff');
                compression=$(if($channel -ceq 'Diff'){0}elseif($channel -ceq 'NormalDX'){1}else{2})}
        }
    }
}

function Get-WoodlandMeshNames {
    @('SM_Shrub04_a','SM_Shrub04_c','SM_DryBranchesMedium01_a','SM_DryBranchesMedium01_b',
        'SM_DryBranchesMedium01_c','SM_FirSapling_a','SM_FirSapling_c','SM_FlowerEmpodium_a','SM_FlowerEmpodium_b')
}

function Get-WoodlandPackageStems {
    @(@('SM_TreeSmall02_Woodland') + @(Get-WoodlandMeshNames)|ForEach-Object {"Meshes\$_"}) +
        @(Get-WoodlandTextureMaps|Select-Object -ExpandProperty material -Unique|ForEach-Object {"Materials\$_"}) +
        @(Get-WoodlandTextureMaps|ForEach-Object {"Textures\$($_.name)"})
}

function Get-TreePalettePackageStems {
    @(@('SM_Jacaranda','SM_FirPole')|ForEach-Object {"Meshes\$_"})
    @(@('Jacaranda_Branches','Jacaranda_Trunk','Jacaranda_Leaves',
        'FirPole_Branches','FirPole_Twigs','FirPole_Dead')|ForEach-Object {"Materials\M_$_"})
    @('Jacaranda_Branches','Jacaranda_Trunk','FirPole_Branches','FirPole_Dead') |
        ForEach-Object {$role=$_;@('Diff','NormalDX','Roughness','AO')|ForEach-Object{"Textures\T_${role}_$_"}}
    @('Jacaranda_Leaves','FirPole_Twigs') |
        ForEach-Object {$role=$_;@('Diff','NormalDX','Roughness','AO','Alpha')|ForEach-Object{"Textures\T_${role}_$_"}}
}

function Get-TreePalettePackagePins([string]$Root) {
    $receipt=Join-Path $Root 'docs\research\environment-assets\tree-palette-assets-01\receipt.json'
    if((Get-FileHash $receipt).Hash -cne '7BC147268EC0FA5538024F9B4C72D769BAA7307A95336E77D4AD398B8925D2EA'){
        throw 'Qualified tree palette receipt differs.'
    }

    $value=Get-Content $receipt -Raw|ConvertFrom-Json
    if($value.status -cne 'qualified-native-inventory-passed-supervisor-shutdown-failed' -or $value.namespace -cne '/Game/Trials/TreePalette_20260921_01' -or @($value.packages).Count -ne 34){
        throw 'Qualified tree palette package scope differs.'
    }
    $expected=@(Get-TreePalettePackageStems|ForEach-Object{"Content\Trials\TreePalette_20260921_01\$_.uasset"}|Sort-Object)
    if(Compare-Object $expected @($value.packages.path|Sort-Object)){throw 'Qualified tree palette package names differ.'}
    foreach($package in $value.packages){
        $path=Join-Path $Root $package.path
        if((Get-Item $path).Length -ne $package.bytes -or (Get-FileHash $path).Hash -cne $package.sha256){
            throw "Tree palette package differs:$($package.path)"
        }
    }
    @($value.packages)
}

function Get-MatureFirPackageStems {
    @('Meshes\SM_MatureFir')
    @('MatureFir_Bark','MatureFir_Twig','MatureFir_Trunk')|ForEach-Object{"Materials\M_$_"}
    @('MatureFir_Bark','MatureFir_Trunk')|
        ForEach-Object {$role=$_;@('Diff','NormalDX','Roughness','AO')|ForEach-Object{"Textures\T_${role}_$_"}}
    @('Diff','NormalDX','Roughness','AO','Alpha')|ForEach-Object{"Textures\T_MatureFir_Twig_$_"}
}

function Get-MatureFirPackagePins([string]$Root) {
    $receipt=Join-Path $Root 'docs\research\environment-assets\mature-fir-assets-02\receipt.json'
    if((Get-FileHash $receipt).Hash -cne '8E67DA1CBDF92E7DAC04A70A26589A11B6C3801175DE913C6E644E9B0A72490C'){
        throw 'Qualified mature-fir receipt differs.'
    }
    $value=Get-Content $receipt -Raw|ConvertFrom-Json
    if($value.status -cne 'qualified-native-inventory-passed-supervisor-shutdown-failed' -or $value.namespace -cne '/Game/Trials/MatureFir_20260922_02' -or @($value.packages).Count -ne 17){
        throw 'Qualified mature-fir package scope differs.'
    }
    $expected=@(Get-MatureFirPackageStems|ForEach-Object{"Content\Trials\MatureFir_20260922_02\$_.uasset"}|Sort-Object)
    if(Compare-Object $expected @($value.packages.path|Sort-Object)){throw 'Qualified mature-fir package names differ.'}
    foreach($package in $value.packages){
        $path=Join-Path $Root $package.path
        if((Get-Item $path).Length -ne $package.bytes -or (Get-FileHash $path).Hash -cne $package.sha256){
            throw "Mature-fir package differs:$($package.path)"
        }
    }
    @($value.packages)
}

function Get-WoodlandSourcePins([string]$Root) {
    $contract='Assets\Environment\WoodlandResources\candidate01'
    $pins=@(
        @{path="$contract\asset-manifest.json";sha256='BDB728405F33D82CDE68D27E0F316C3B2988311B74A328CEFCFE46175332BAE5'},
        @{path="$contract\download-receipt.json";sha256='F8F97798328C0246099EBE90AA287515FEB925C316DF676AC2524C354005EC9F'},
        @{path="$contract\source-inventory.json";sha256='DE7F9980E188521FF5634E4AD3D744771A6A74C4F7CFDE6247D0D5862B461A77'},
        @{path="$contract\selection.json";sha256='FFE9395858EA7AA81C591CC89AABC2CC3957C6AEBA67DB101C7561F63B25CE64'},
        @{path="$contract\Prepared\provenance.json";sha256='62FCCCA01E3D80A2B5EBE04E910174E50B79A23B1D4AF3D979F05F4023739DA9'},
        @{path='Saved\Automation\20260921-033354-2d257ba0\woodland-source-prep-01\result.json';sha256='D713E6482944E7424DA770B0C5389E9EE5AA57A69C71030FD97A513E4A0C3BEE'}
    )
    foreach($pin in $pins){
        $pin.path=Join-Path $Root $pin.path
        if((Get-FileHash $pin.path).Hash -cne $pin.sha256){throw "Woodland source/proof differs:$($pin.path)"}
    }
    $preparation=Get-Content $pins[5].path -Raw|ConvertFrom-Json
    if($preparation.status -cne 'passed' -or $preparation.exitCode -ne 0 -or -not $preparation.subjectExited -or
        -not $preparation.guardDisposed -or $preparation.hardTerminated -or @($preparation.cleanupErrors).Count){
        throw 'Actual released guarded resource preparation is required.'
    }
    $source=Join-Path $Root 'Assets\Source\woodland-resources-20260921'
    $prepared=Join-Path $Root "$contract\Prepared"
    Assert-FernOrdinaryTree $source
    Assert-FernOrdinaryTree $prepared
    $receipt=Get-Content $pins[1].path -Raw|ConvertFrom-Json
    $proof=Get-Content $pins[4].path -Raw|ConvertFrom-Json
    if(@($receipt.files).Count -ne 29 -or @($proof.files).Count -ne 4){throw 'Resource source closure differs.'}
    foreach($entry in $receipt.files){
        $file=Join-Path $source "$($entry.asset)\$($entry.file)"
        if((Get-Item $file).Length -ne $entry.bytes -or (Get-FileHash $file).Hash -cne $entry.sha256.ToUpperInvariant()){
            throw "Original resource differs:$file"
        }
        $pins+=@{path=$file;sha256=$entry.sha256.ToUpperInvariant()}
    }
    foreach($entry in $proof.files){
        $file=Join-Path $prepared $entry.file
        if(-not $entry.retainedArraysAndTransformsIdentical -or
            (Get-Item $file).Length -ne $entry.bytes -or (Get-FileHash $file).Hash -cne $entry.sha256){
            throw "Prepared resource differs:$file"
        }
        $pins+=@{path=$file;sha256=$entry.sha256}
    }
    $tree=Get-TreeFailedImportDiagnosticAdmission $Root
    foreach($entry in $tree.packages){
        $pins+=@{path=(Join-Path $Root "Content\Trials\TreeSmall02_20260921_01\$($entry.path)");sha256=$entry.sha256}
    }
    $pins
}

function Assert-WoodlandNativeInventory($Value,[string]$Root) {
    $prefix='/Game/Trials/WoodlandResources_20260921_01'
    if(-not $Value.passed -or $Value.cancelledAtPollingBoundary -or $Value.namespace -cne $prefix -or
        $Value.mode -cnotin @('WoodlandImport','WoodlandVerify') -or $Value.packages -ne 38 -or
        @($Value.resources).Count -ne 9 -or @($Value.materials).Count -ne 5 -or @($Value.textures).Count -ne 23){
        throw 'Woodland native inventory scope differs.'
    }
    $canopy=$Value.canopy
    if($canopy.object -cne "$prefix/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland" -or
        $canopy.source -cne '/Game/Trials/TreeSmall02_20260921_01/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2' -or
        @($canopy.lods).Count -ne 3 -or $canopy.qualification -cnotmatch 'twelve.*import03 remains FAILED'){
        throw 'Derived canopy provenance/qualification differs.'
    }
    for($i=0;$i -lt 3;$i++){
        $lod=$canopy.lods[$i]
        if($lod.lod -ne $i -or $lod.triangles -le 0 -or $lod.triangles -gt @(231785,65000,18000)[$i] -or
            ($i -eq 0 -and $lod.triangles -ne 231785) -or [Math]::Abs($lod.screenSize-@(1,0.35,0.12)[$i]) -gt 0.00001){
            throw 'Canopy distance LOD contract differs.'
        }
    }
    $selection=Get-Content (Join-Path $Root 'Assets\Environment\WoodlandResources\candidate01\selection.json') -Raw|ConvertFrom-Json
    $source=Get-Content (Join-Path $Root 'Assets\Environment\WoodlandResources\candidate01\source-inventory.json') -Raw|ConvertFrom-Json
    $models=@($selection.assets|ForEach-Object {$_.models})
    $names=@(Get-WoodlandMeshNames)
    for($i=0;$i -lt 9;$i++){
        $mesh=$Value.resources[$i];$model=$models[$i];$name=$names[$i]
        $sapling=@($model.materials).Count -eq 2
        if($mesh.object -cne "$prefix/Meshes/$name.$name" -or $mesh.sourceNode -cne $model.name -or
            $mesh.sourceModelId -ne $model.id -or $mesh.sourceGeometryId -ne $model.geometryId -or
            $mesh.sourceTriangles -ne $model.sourceFanTriangles -or $mesh.cameraOnlyCrownCollision -ne $sapling -or
            @($mesh.slots).Count -ne @($model.materials).Count -or @($mesh.lods).Count -ne $(if($sapling){2}else{1})){
            throw "Resource identity/geometry differs:$name"
        }
        foreach($field in @('boundsMinCm','boundsMaxCm','sourceDimensionsTimes100Cm','placementGroundAnchorCm')){
            if(@($mesh.$field).Count -ne 3 -or @($mesh.$field|Where-Object {-not [double]::IsFinite($_)}).Count){
                throw 'Malformed/nonfinite resource vector.'
            }
        }
        $attributes=@($source.files|Where-Object file -Like '*.fbx'|ForEach-Object {$_.sourceInspection.roleSpecificAttributes}|Where-Object geometryId -EQ $model.geometryId)
        if($attributes.Count -ne 1){throw 'Missing exact source geometry attributes.'}
        $sizes=@(for($axis=0;$axis -lt 3;$axis++){
            $expected=100*($attributes[0].referencedLocalBounds.max[$axis]-$attributes[0].referencedLocalBounds.min[$axis])
            if([Math]::Abs($mesh.sourceDimensionsTimes100Cm[$axis]-$expected) -gt 0.0001){throw 'Source dimensions differ.'}
            $anchor=if($axis -eq 2){$mesh.boundsMinCm[$axis]}else{($mesh.boundsMinCm[$axis]+$mesh.boundsMaxCm[$axis])/2}
            if([Math]::Abs($mesh.placementGroundAnchorCm[$axis]-$anchor) -gt 0.01){throw 'Native ground anchor differs.'}
            $size=$mesh.boundsMaxCm[$axis]-$mesh.boundsMinCm[$axis]
            if($size -le 0){throw 'Empty/inverted resource bounds.'}
            $size
        })
        if([Math]::Abs($sizes[2]-$mesh.sourceDimensionsTimes100Cm[2]) -gt 0.15){throw 'Native resource height/units differ.'}
        $xy=@($sizes[0..1]|Sort-Object);$expectedXY=@($mesh.sourceDimensionsTimes100Cm[0..1]|Sort-Object)
        for($axis=0;$axis -lt 2;$axis++){if([Math]::Abs($xy[$axis]-$expectedXY[$axis]) -gt 0.15){throw 'Native XY axis-converted dimensions differ.'}}
        for($slot=0;$slot -lt @($model.materials).Count;$slot++){
            $record=$mesh.slots[$slot]
            $map=@(Get-WoodlandTextureMaps|Where-Object role -CEQ $model.materials[$slot])[0]
            $triangles=if($sapling){$model.sourceFanTrianglesByRole[$slot]}else{$model.sourceFanTriangles}
            if($record.slot -ne $slot -or $record.role -cne $model.materials[$slot] -or $record.triangles -ne $triangles -or
                $record.activeUvChannel -ne 0 -or $record.material -cne "$prefix/Materials/$($map.material).$($map.material)"){
                throw 'Resource ordered material/UV binding differs.'
            }
        }
        for($lod=0;$lod -lt @($mesh.lods).Count;$lod++){
            $record=$mesh.lods[$lod]
            if($record.lod -ne $lod -or $record.triangles -le 0 -or
                ($lod -eq 0 -and $record.triangles -ne $model.sourceFanTriangles) -or
                ($lod -gt 0 -and $record.triangles -gt [Math]::Ceiling($model.sourceFanTriangles*0.26)) -or
                ($sapling -and [Math]::Abs($record.screenSize-@(1,0.35)[$lod]) -gt 0.00001)){
                throw 'Resource native distance LOD differs.'
            }
        }
    }
    $maps=@(Get-WoodlandTextureMaps)
    for($i=0;$i -lt $maps.Count;$i++){
        $map=$maps[$i];$texture=$Value.textures[$i]
        if($texture.object -cne "$prefix/Textures/$($map.name).$($map.name)" -or
            $texture.source -cne "$($map.asset)/$($map.file)" -or $texture.width -ne 1024 -or $texture.height -ne 1024 -or
            $texture.compression -ne $map.compression -or $texture.srgb -ne $map.srgb -or $texture.flipGreen){
            throw 'Resource texture/PBR identity differs.'
        }
    }
    $materialNames=@($maps|Select-Object -ExpandProperty material -Unique)
    for($i=0;$i -lt 5;$i++){
        $material=$Value.materials[$i];$name=$materialNames[$i];$map=@($maps|Where-Object material -CEQ $name)[0]
        if($material.object -cne "$prefix/Materials/$name.$name" -or $material.sourceRole -cne $map.role -or
            $material.masked -ne $map.masked -or $material.twoSided -ne $map.masked -or
            [Math]::Abs($material.opacityClip-0.333) -gt 0.00001){throw 'Resource material role/alpha policy differs.'}
    }
    $allowed=@(Get-WoodlandPackageStems|ForEach-Object {"$prefix/$($_.Replace('\','/'))"}) +
        @(Get-TreePackageStems|ForEach-Object {"/Game/Trials/TreeSmall02_20260921_01/$($_.Replace('\','/'))"})
    if(-not @($Value.persistentObjectReferences).Count){throw 'Missing woodland reference audit.'}
    foreach($reference in $Value.persistentObjectReferences){
        if($reference.Split('.')[0] -cnotin $allowed){throw 'Unexpected woodland persistent reference.'}
    }
}
