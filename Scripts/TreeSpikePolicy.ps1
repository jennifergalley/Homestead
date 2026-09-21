function Get-TreeTextureMaps {
    foreach($role in @('Branch','Leaves','Trunk')) {
        $prefix=switch($role){Branch{'tree_small_02_branch'} Leaves{'tree_small_02_leaves'} Trunk{'tree_small_02'}}
        foreach($map in @('Diff','NormalDX','Roughness','AO') + $(if($role -ceq 'Leaves'){@('Alpha')}else{@()})) {
            $suffix=switch($map){Diff{'diff'} NormalDX{'nor_dx'} Roughness{'rough'} AO{'ao'} Alpha{'alpha'}}
            $extension=if($role -ceq 'Trunk' -and $map -ceq 'Diff'){'jpg'}else{'png'}
            [pscustomobject]@{role=$role;map=$map;name="T_TreeSmall02_${role}_$map";file="${prefix}_${suffix}_2k.$extension"
                srgb=($map -ceq 'Diff');compression=$(if($map -ceq 'Diff'){0}elseif($map -ceq 'NormalDX'){1}else{2})
                outputIndex=$(if($map -cin @('Diff','NormalDX')){0}else{1})}
        }
    }
}

function Get-TreePackageStems {
    @('Meshes\SM_TreeSmall02_LOD2','Materials\M_TreeSmall02_Branches',
        'Materials\M_TreeSmall02_Leaves','Materials\M_TreeSmall02_Trunk') +
        @(Get-TreeTextureMaps | ForEach-Object {"Textures\$($_.name)"})
}

function Get-TreeSourcePins([string]$Root) {
    $source=Join-Path $Root 'Assets\Environment\TreeSmall02Prepared\v3'
    Assert-FernOrdinaryTree $source
    $manifest=Join-Path $source 'FROZEN.json'
    $manifestHash='D967504DADAD2FAC986EC4427ABAD7F2F4C1094C45799D2F479473BBE219730D'
    if((Get-FileHash $manifest).Hash -cne $manifestHash){throw 'Frozen tree handoff changed.'}
    $frozen=Get-Content $manifest -Raw|ConvertFrom-Json
    $pins=@(@{path=$manifest;sha256=$manifestHash})
    foreach($name in @('provenance.json','materials.json','roundtrip.json','TreeSmall02_LOD2.fbx')) {
        $file=Join-Path $source $name
        $hash=$frozen.sha256.$name.ToUpperInvariant()
        if((Get-FileHash $file).Hash -cne $hash){throw "Frozen tree input differs:$name"}
        $pins+=@{path=$file;sha256=$hash}
    }
    $contract=Join-Path $source 'fbx-contract.json'
    $contractHash='4EE36B8F77002D3FD49094C114C33432784F8AB4C71CD601094D6B797DF16048'
    if((Get-FileHash $contract).Hash -cne $contractHash){throw 'Frozen FBX structure contract changed.'}
    $pins+=@{path=$contract;sha256=$contractHash}
    $provenance=Get-Content (Join-Path $source 'provenance.json') -Raw|ConvertFrom-Json
    foreach($map in @(Get-TreeTextureMaps)) {
        $entries=@($provenance.sourceFiles|Where-Object file -CEQ $map.file)
        if($entries.Count -ne 1){throw 'Missing unique tree map provenance.'}
        $file=Join-Path $source "Textures\$($map.file)"
        if((Get-Item $file).Length -ne $entries[0].bytes -or (Get-FileHash $file).Hash -cne $entries[0].sha256){
            throw "Frozen tree texture differs:$($map.file)"
        }
        $pins+=@{path=$file;sha256=$entries[0].sha256}
    }
    $pins
}

function Assert-TreeNativeInventory($Value,[switch]$KnownTangentDiagnostic) {
    $prefix='/Game/Trials/TreeSmall02_20260921_01'
    $roles=@('tree_small_02_branches','tree_small_02_leaves','tree_small_02_trunk')
    $triangles=@(23702,193938,14145)
    $materials=@('M_TreeSmall02_Branches','M_TreeSmall02_Leaves','M_TreeSmall02_Trunk')
    if(-not $Value.passed -or $Value.cancelledAtPollingBoundary -or $Value.namespace -cne $prefix -or
        $Value.mode -cnotin @('TreeImport','TreeVerify') -or $Value.packages -ne 17 -or
        $Value.mesh -cne "$prefix/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2" -or
        $Value.triangles -ne 231785 -or $Value.uvChannels -ne 2 -or $Value.nanite -or
        $Value.importUniformScale -ne 1 -or -not $Value.convertScene -or -not $Value.convertSceneUnit -or
        -not $Value.transformVertexToAbsolute -or @($Value.slots).Count -ne 3 -or
        @($Value.materials).Count -ne 3 -or @($Value.textures).Count -ne 13 -or
        @($Value.meshDescriptionBasisAfter).Count -ne 3){
        throw 'Tree inventory scope/geometry/transform result differs.'
    }
    foreach($name in @('boundsMinCm','boundsMaxCm','lowerTrunkMinCm','lowerTrunkMaxCm','collisionCenterCm')) {
        if(@($Value.$name).Count -ne 3 -or @($Value.$name|Where-Object {-not [double]::IsFinite($_)}).Count){
            throw "Nonfinite/malformed tree vector:$name"
        }
    }
    $expectedMin=@(-131.072414,-138.302326,-2.419423)
    $expectedMax=@(161.071074,290.762329,453.984356)
    for($i=0;$i -lt 3;$i++){
        if([Math]::Abs($Value.boundsMinCm[$i]-$expectedMin[$i]) -gt 0.2 -or
            [Math]::Abs($Value.boundsMaxCm[$i]-$expectedMax[$i]) -gt 0.2){throw 'Tree referenced source bounds/frame differ.'}
    }
    $radius=[double]$Value.collisionRadiusCm
    $length=[double]$Value.collisionCylinderLengthCm
    $trunkHeight=$Value.lowerTrunkMaxCm[2]-$Value.lowerTrunkMinCm[2]
    if(-not [double]::IsFinite($radius) -or -not [double]::IsFinite($length) -or $radius -lt 2 -or $radius -gt 60 -or
        $length -le 0 -or $trunkHeight -lt 150 -or $trunkHeight -gt 205 -or
        $Value.lowerTrunkMaxCm[2] -gt 200 -or $Value.measuredTrunkVertexSamples -le 0 -or
        [Math]::Abs($trunkHeight-$length-2*$radius) -gt 0.01){throw 'Tree measured trunk collision differs.'}
    for($i=0;$i -lt 3;$i++) {
        $basis=$Value.meshDescriptionBasisAfter[$i]
        $expectedTangents=if($KnownTangentDiagnostic -and $i -eq 0){12}else{0}
        if($basis.role -cne $roles[$i] -or $basis.invalidNormalCorners -ne 0 -or
            $basis.invalidTangentCorners -ne $expectedTangents -or $basis.invalidBinormalCorners -ne $expectedTangents){
            throw 'Invalid post-adaptation tree basis.'
        }

        if($Value.lowerTrunkMinCm[$i] -gt $Value.lowerTrunkMaxCm[$i] -or
            $Value.lowerTrunkMinCm[$i] -lt $Value.boundsMinCm[$i]-0.01 -or
            $Value.lowerTrunkMaxCm[$i] -gt $Value.boundsMaxCm[$i]+0.01 -or
            [Math]::Abs($Value.collisionCenterCm[$i]-($Value.lowerTrunkMinCm[$i]+$Value.lowerTrunkMaxCm[$i])/2) -gt 0.01){
            throw 'Tree collider is not centered on measured lower trunk.'
        }
    }
    $seen=[Collections.Generic.HashSet[int]]::new()
    for($i=0;$i -lt 3;$i++) {
        $slot=$Value.slots[$i];$role=$slot.sourceRoleIndex
        if($slot.index -ne $i -or $role -ne [int]$role -or $role -lt 0 -or $role -gt 2 -or -not $seen.Add($role) -or
            $slot.role -cne $roles[$role] -or $slot.triangles -ne $triangles[$role] -or
            $slot.sourceActiveUvChannel -ne $(if($role -eq 0){1}else{0}) -or
            $slot.runtimeActiveUvChannel -ne 0 -or $slot.runtimeActiveUvDegenerateTriangles -ne 0 -or
            $slot.runtimeInactiveUvDegenerateTriangles -ne $triangles[$role] -or
            $slot.nearZeroImportedNormalCorners -ne 0 -or
            $slot.material -cne "$prefix/Materials/$($materials[$role]).$($materials[$role])"){
            throw 'Tree material slot identity/triangles differ.'
        }
        $material=$Value.materials[$i]
        if($material.object -cne "$prefix/Materials/$($materials[$i]).$($materials[$i])" -or
            $material.shadingModel -cne 'DefaultLit' -or $material.twoSided -ne ($i -eq 1) -or
            $material.masked -ne ($i -eq 1) -or ($i -eq 1 -and $material.opacityClip -ne 0.5)){
            throw 'Tree authored material state differs.'
        }
    }
    $maps=@(Get-TreeTextureMaps)
    for($i=0;$i -lt $maps.Count;$i++) {
        $map=$maps[$i];$texture=$Value.textures[$i]
        if($texture.object -cne "$prefix/Textures/$($map.name).$($map.name)" -or
            $texture.source -cne $map.file -or $texture.width -ne 2048 -or $texture.height -ne 2048 -or
            $texture.srgb -ne $map.srgb -or $texture.compression -ne $map.compression -or $texture.flipGreen -or
            $texture.materialRole -ne [Array]::IndexOf(@('Branch','Leaves','Trunk'),$map.role) -or
            $texture.connectedOutputIndex -ne $map.outputIndex){throw "Tree map/graph differs:$($map.name)"}
    }
    $allowed=@(Get-TreePackageStems|ForEach-Object {"$prefix/$($_.Replace('\','/'))"})
    foreach($reference in $Value.persistentObjectReferences) {
        $package=$reference.Split('.')[0]
        if($package -cnotin $allowed){throw 'Tree references an unexpected package.'}
    }
}

function Get-TreeFailedImportDiagnosticAdmission([string]$Root) {
    $runRoot=Join-Path $Root 'Saved\Automation\20260921-033354-2d257ba0'
    foreach($pin in @(
        @{path='tree-import-03\probe-result.json';sha256='EA34CE51CE7A7711E22829894BA055BE670ACA5FD2968413A2D59BE920034A88'},
        @{path='tree-import-attempt-03.json';sha256='78C88F65117184869C43D58B53E66EB2B93F9138382CAD55CDFF63B760DC1557'},
        @{path='tree-import-03\partial-content.json';sha256='B978609E071F42B450C6CADDFBE019C7B2A80B30E0703D60D0BEA10767E77F5B'}
    )){
        if((Get-FileHash (Join-Path $runRoot $pin.path)).Hash -cne $pin.sha256){throw 'Original failed tree evidence changed.'}
    }
    $failed=Get-Content (Join-Path $runRoot 'tree-import-03\probe-result.json') -Raw|ConvertFrom-Json
    if($failed.status -cne 'failed' -or $failed.error -cne 'Invalid post-adaptation tree basis.' -or
        $failed.exitCode -ne 7 -or -not $failed.subjectExited -or -not $failed.guardDisposed -or
        $failed.hardTerminated -or @($failed.cleanupErrors).Count){throw 'Unexpected failed tree disposition.'}
    Assert-TreeNativeInventory $failed.fernInventory -KnownTangentDiagnostic
    $trial=Join-Path $Root 'Content\Trials\TreeSmall02_20260921_01'
    Assert-FernDirectoryIdentity $failed.trialIdentity $trial
    $packages=@(Get-Content (Join-Path $runRoot 'tree-import-03\partial-content.json') -Raw|ConvertFrom-Json)
    Assert-FernPackagePins $packages @(Get-FernPackageFiles $trial -Complete -Stems @(Get-TreePackageStems))
    @{inventory=$failed.fernInventory;directoryIdentity=$failed.trialIdentity;packages=$packages
        qualification='Diagnostic only: exactly12 branch source-description tangent/binormal corners; all other native conditions strict. Original import03 remains FAILED; not a rendered-GPU damage claim or promotion.'}
}
