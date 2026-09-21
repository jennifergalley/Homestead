function Get-WardrobeFits([string]$Root) {
    $bodies=@('Preferred','Willow','Hazel')
    $parts=@('Base_LongWave','Base_Bob','Base_Ponytail','Tunic','Apron','Shoes','Footwraps')
    $triangles=@(@(133454,151118,143044,15374,3536,3320,11448),
        @(133442,151157,143028,15374,3536,3320,11448),@(133478,151311,143040,15374,3536,3320,11448))
    $bobTriangles=@(138104,141640,138303,141839,138285,141821)
    for($b=0;$b -lt 3;$b++) {
        $body=$bodies[$b]
        $prefix=if($b -eq 0){'SK_Heroine_'}else{"SK_Heroine_${body}_"}
        for($p=0;$p -lt 7;$p++) {
            $part=$parts[$p];$name="SK_Modular_${body}_$part"
            $source=if($p -lt 2){"Assets\Characters\HairstyleRefinement\Modular\$name.fbx"}else{"Assets\Characters\ModularClothing\$body\$name.fbx"}
            $roles=switch($p) {
                {$_ -lt 3} {
                    @('M_Heroine_Skin','M_Modular_BaseBra','M_Modular_BaseBriefs',
                        @('M_Heroine_Hair_long01_Neutral','M_Heroine_Hair_bob01_Neutral','M_Heroine_Hair_ponytail01')[$p],
                        'M_Heroine_LightEyes','M_Heroine_Eyebrows','M_Heroine_Eyelashes','M_Heroine_Teeth','M_Heroine_Tongue')
                }
                3 {@('M_Heroine_MossLinen','M_Heroine_LinenTrim','M_Heroine_ChestnutLeather','M_Heroine_Brass')}
                4 {@('M_Heroine_ApronTrim','M_Heroine_ApronLinen')}
                5 {@('M_Heroine_LeatherShoes')}
                6 {@('M_Modular_FootwrapCloth','M_Modular_FootwrapBinding')}
            }
            [pscustomobject]@{name=$name;stem="$body\$name";source=(Join-Path $Root $source)
                reference=$prefix+$(if($p -eq 2){'Ponytail'}else{'LongWave_Apron'})
                triangles=$triangles[$b][$p];fullBody=($p -lt 3);roles=@($roles);body=$body;part=$part}
        }
        for($o=0;$o -lt 2;$o++) {
            $name=$prefix+'Bob'+$(if($o){'_Apron'}else{''})
            $roles=@('M_Heroine_Brass','M_Heroine_Skin','M_Heroine_LightEyes','M_Heroine_Eyebrows',
                'M_Heroine_Eyelashes','M_Heroine_Teeth','M_Heroine_Tongue','M_Heroine_Hair_bob01_Neutral',
                'M_Heroine_LeatherShoes','M_Heroine_MossLinen','M_Heroine_LinenTrim','M_Heroine_ChestnutLeather')
            if($o){$roles[0]='M_Heroine_ApronTrim';$roles+=@('M_Heroine_Brass','M_Heroine_ApronLinen')}
            [pscustomobject]@{name=$name;stem="JoinedBob\$name";source=(Join-Path $Root "Assets\Characters\HairstyleRefinement\Joined\$name.fbx")
                reference=$name;triangles=$bobTriangles[$b*2+$o];fullBody=$true;roles=$roles;body=$body;part='JoinedBob'}
        }
    }
}

function Get-WardrobePackageStems {
    @('Textures\T_BobReuse_Neutral','Textures\T_ModularWeave','Materials\M_Heroine_Hair_bob01_Neutral',
        'Materials\M_Modular_BaseBra','Materials\M_Modular_BaseBriefs',
        'Materials\M_Modular_FootwrapCloth','Materials\M_Modular_FootwrapBinding') +
        @(Get-WardrobeFits (Split-Path $PSScriptRoot -Parent) | ForEach-Object stem)
}

function Get-WardrobeSourcePins([string]$Root) {
    $hair=Join-Path $Root 'Assets\Characters\HairstyleRefinement'
    $clothes=Join-Path $Root 'Assets\Characters\ModularClothing'
    Assert-FernOrdinaryTree $hair
    Assert-FernOrdinaryTree $clothes
    $pins=@()
    $fixed=[ordered]@{
        'Assets\Characters\HairstyleRefinement\final-bundle.json'='68E70D4F6C39495756321BDE18FA0C6F9B22C10CC777E540E3760DA90FBCAFA5'
        'Assets\Characters\HairstyleRefinement\Modular-manifest.json'='D965CB04E3590DB61311F0401365E8B5D906C9DDA809EAA21DDCCC84EC36D9F7'
        'Assets\Characters\HairstyleRefinement\Bob-manifest.json'='2F6FBB5DFE2910C004E4D62E76AD9B8A0BCB6ACC5D9D862591A85B784DB4EE6E'
        'Assets\Characters\ModularClothing\manifest.json'='BC665ADFC998898E69A44260258FDEC609350B3374D88C21C9F2293AF074DDB0'
        'Assets\Characters\HairstyleRefinement\Textures\T_BobReuse_Neutral.png'='8163B5AE9EFF12540B533E56D54283C82AC1DF601CDE2FE3DFCC01F8B7257F0E'
        'Assets\Characters\ModularClothing\Textures\T_ModularWeave.png'='5F503797B34804BFE06BB59DF5ADA2899D5160BCAB4AE15B8AA982DD24A3DA23'
    }
    foreach($name in $fixed.Keys) {
        $path=Join-Path $Root $name
        if((Get-FileHash $path).Hash -cne $fixed[$name]){throw "Canonical source pin differs:$name"}
        $pins+=@{path=$path;sha256=$fixed[$name]}
    }
    $bundle=Get-Content (Join-Path $hair 'final-bundle.json') -Raw|ConvertFrom-Json
    $bob=Get-Content (Join-Path $hair 'Bob-manifest.json') -Raw|ConvertFrom-Json
    $modular=Get-Content (Join-Path $hair 'Modular-manifest.json') -Raw|ConvertFrom-Json
    $original=Get-Content (Join-Path $clothes 'manifest.json') -Raw|ConvertFrom-Json
    foreach($fit in @(Get-WardrobeFits $Root)) {
        if($fit.part -in @('Base_LongWave','Base_Bob','JoinedBob')) {
            $relative=[IO.Path]::GetRelativePath($hair,$fit.source)
            $entries=@($bundle.entries|Where-Object fbx -CEQ $relative)
            if($entries.Count -ne 1){throw "Missing unique canonical mapping:$relative"}
            $expected=$entries[0].sha256.ToUpperInvariant()
            $manifest=if($fit.part -ceq 'JoinedBob'){$bob}else{$modular}
            $records=@($manifest.entries|Where-Object fbx -CEQ $relative)
            if($records.Count -ne 1){throw "Missing canonical material-slot record:$relative"}
            $roles=@($records[0].material_slots)
        } else {
            $entry=$original.bodies.($fit.body).exports.($fit.part)
            if($entry.fbx -cne "$($fit.body)\$($fit.name).fbx" -or $entry.triangles -ne $fit.triangles){throw 'Original modular mapping differs.'}
            $expected=$entry.sha256.ToUpperInvariant()
            $roles=@($entry.materials)
        }
        if(($roles -join '|') -cne ($fit.roles -join '|')){throw "Canonical material-slot order differs:$($fit.name)"}
        if((Get-FileHash $fit.source).Hash -cne $expected){throw "Canonical FBX differs:$($fit.name)"}
        $pins+=@{path=$fit.source;sha256=$expected}
    }
    $pins
}

function Assert-WardrobeNativeInventory($Value,[string]$Root) {
    $prefix='/Game/SurvivalGame/Characters/ModularClothing'
    $fits=@(Get-WardrobeFits $Root)
    if(-not $Value.passed -or $Value.cancelledAtPollingBoundary -or $Value.namespace -cne $prefix -or
        $Value.packages -ne 34 -or @($Value.meshes).Count -ne 27){throw 'Wardrobe inventory scope/result differs.'}
    for($i=0;$i -lt $fits.Count;$i++) {
        $fit=$fits[$i];$mesh=$Value.meshes[$i]
        $package="$prefix/$($fit.stem.Replace('\','/'))"
        $reference="/Game/SurvivalGame/Characters/Heroine/$($fit.reference).$($fit.reference)"
        if($mesh.object -cne "$package.$($fit.name)" -or $mesh.reference -cne $reference -or
            [IO.Path]::GetFullPath($mesh.source) -ine $fit.source -or
            $mesh.skeleton -cne '/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton.SK_Heroine_LongWave_Skeleton' -or
            $mesh.bones -ne 54 -or -not $mesh.referencePoseMatchesIncumbent -or $mesh.triangles -ne $fit.triangles -or
            -not [double]::IsFinite($mesh.heightCm) -or $mesh.heightCm -le 0 -or
            ($fit.fullBody -and ($mesh.heightCm -lt 155 -or $mesh.heightCm -gt 175)) -or
            (-not $fit.fullBody -and $mesh.heightCm -gt 140) -or @($mesh.slots).Count -ne $fit.roles.Count) {
            throw "Wardrobe rig/geometry/source differs:$($fit.name)"
        }
        if(@($mesh.boundsOriginCm).Count -ne 3 -or @($mesh.boundsExtentCm).Count -ne 3 -or
            @(@($mesh.boundsOriginCm)+@($mesh.boundsExtentCm)|Where-Object {-not [double]::IsFinite($_)}).Count -or
            @($mesh.boundsExtentCm|Where-Object {$_ -le 0}).Count -or
            [Math]::Abs($mesh.heightCm-2*$mesh.boundsExtentCm[2]) -gt 0.001){throw 'Invalid wardrobe bounds.'}
        $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        for($s=0;$s -lt $fit.roles.Count;$s++) {
            $slot=$mesh.slots[$s];$role=$slot.role
            if($slot.index -ne $s -or $role -cnotin $fit.roles -or -not $seen.Add($role) -or
                $slot.sourceRoleIndex -ne [Array]::IndexOf($fit.roles,$role)){throw 'Wardrobe material identity/source role differs.'}
            if($fit.part -ceq 'JoinedBob' -and $role -ceq 'M_Heroine_Hair_bob01_Neutral' -and $s -ne 7){
                throw 'Canonical joined Bob hair slot7 differs.'
            }
            $expected=if($role -eq 'M_Heroine_Hair_long01_Neutral'){
                "/Game/Trials/HeroineWave_20260921_01/Materials/$role.$role"
            }elseif($role -eq 'M_Heroine_Hair_bob01_Neutral' -or $role.StartsWith('M_Modular_')){
                "$prefix/Materials/$role.$role"
            }else{$null}
            if(($expected -and $slot.material -cne $expected) -or
                (-not $expected -and -not $slot.material.StartsWith('/Game/SurvivalGame/Characters/Heroine/Materials/',[StringComparison]::Ordinal))){
                throw 'Unapproved wardrobe material interface.'
            }
            $used=[Collections.Generic.HashSet[int]]::new()
            $triangles=0
            foreach($section in $mesh.sections){
                $index=$section.materialIndex
                if($index -ne [int]$index -or $index -lt 0 -or $index -ge $mesh.slots.Count -or
                    $section.role -cne $mesh.slots[$index].role -or $section.triangles -ne [int]$section.triangles -or
                    $section.triangles -le 0){throw 'Invalid wardrobe section material identity/triangles.'}
                $null=$used.Add($index);$triangles+=$section.triangles
            }
            if($used.Count -ne $fit.roles.Count -or $triangles -ne $fit.triangles){throw 'Incomplete wardrobe sections/triangle total.'}
        }
    }
}
