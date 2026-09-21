function Get-FernPackageStems {
    @('Meshes\SM_Fern02_a','Meshes\SM_Fern02_b','Meshes\SM_Fern02_c','Meshes\SM_Fern02_d',
        'Textures\T_Fern02_Diff','Textures\T_Fern02_NormalDX','Textures\T_Fern02_Roughness',
        'Textures\T_Fern02_AO','Textures\T_Fern02_Alpha','Materials\M_Fern02')
}

function Assert-FernOrdinaryTree([string]$Path) {
    $null=[Homestead.Authoring.LeafGuard]::InspectDirectory($Path)
    $pending=[Collections.Generic.Queue[string]]::new()
    $pending.Enqueue($Path)
    while($pending.Count) {
        foreach($item in Get-ChildItem -LiteralPath $pending.Dequeue() -Force) {
            if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse entry prohibited:$($item.FullName)"}
            if($item.PSIsContainer){$pending.Enqueue($item.FullName)}
        }
    }
}

function Assert-FernDirectoryIdentity($Expected,[string]$Path) {
    $actual=[Homestead.Authoring.LeafGuard]::InspectDirectory($Path)
    foreach($name in @('Volume','IndexHigh','IndexLow','CreationTime','Attributes')) {
        if($actual.$name -ne $Expected.$name){throw 'Reserved fern directory identity changed.'}
    }
    Assert-FernOrdinaryTree $Path
}

function Get-FernPackageFiles([string]$Directory,[switch]$Complete) {
    Assert-FernOrdinaryTree $Directory
    $stems=@(Get-FernPackageStems)
    $files=@(Get-ChildItem -LiteralPath $Directory -File -Recurse -Force)
    $records=@(foreach($file in $files) {
        $relative=[IO.Path]::GetRelativePath($Directory,$file.FullName)
        $stem=$relative.Substring(0,$relative.Length-$file.Extension.Length)
        if($stem -cnotin $stems -or $file.Extension -cnotin @('.uasset','.uexp','.ubulk')) {
            throw "Unexpected fern package output:$relative"
        }
        if($Complete -and $file.Extension -ceq '.uasset') {
            $stream=[IO.File]::OpenRead($file.FullName)
            try {$tag=[byte[]]::new(4);$stream.ReadExactly($tag)}finally{$stream.Dispose()}
            if([BitConverter]::ToUInt32($tag,0) -ne 0x9e2a83c1u){throw 'Invalid ordinary Unreal package tag.'}
        }
        [ordered]@{path=$relative;bytes=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
    })
    if($Complete) {
        foreach($stem in $stems) {
            $asset=@($records|Where-Object path -CEQ "$stem.uasset")
            if($asset.Count -ne 1 -or $asset[0].bytes -le 0){throw "Missing/nonunique fern package:$stem"}
        }
    }
    return $records
}

function Assert-FernPackagePins([object[]]$Expected,[object[]]$Actual) {
    if($Expected.Count -ne $Actual.Count){throw 'Fern package file count changed.'}
    foreach($file in $Expected) {
        $match=@($Actual|Where-Object path -CEQ $file.path)
        if($match.Count -ne 1 -or $match[0].bytes -ne $file.bytes -or $match[0].sha256 -cne $file.sha256) {
            throw "Fern package pin differs:$($file.path)"
        }
    }
}

function Assert-FernRenderImages($Value,[string]$Directory) {
    if($Value.width -ne 1280 -or $Value.height -ne 720 -or
        -not $Value.shaderMapComplete -or $Value.materialFallbackAllowed -or
        [string]::IsNullOrWhiteSpace($Value.adapter) -or [string]::IsNullOrWhiteSpace($Value.rhi) -or
        $Value.rhi -match 'Null' -or @($Value.views).Count -ne 2){throw 'Incomplete actual RHI/image/readiness evidence.'}
    $names=@('fern-a-front.png','fern-a-back.png')
    $files=@(Get-ChildItem -LiteralPath $Directory -File -Filter '*.png')
    if($files.Count -ne 2 -or @($files|Where-Object Name -CNotIn $names).Count){throw 'Unexpected preview image set.'}
    foreach($name in $names) {
        $view=@($Value.views|Where-Object image -CEQ $name)
        if($view.Count -ne 1 -or $view[0].greenPixelCount -lt 50 -or $view[0].screenPercentageShowFlag -or $view[0].motionBlur) {
            throw 'Missing actual fern-view evidence.'
        }
        $image=Get-Item -LiteralPath (Join-Path $Directory $name)
        if(($image.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $image.Length -lt 64){throw 'Invalid ordinary preview image.'}
        $stream=[IO.File]::OpenRead($image.FullName)
        try {$header=[byte[]]::new(24);$stream.ReadExactly($header)}finally{$stream.Dispose()}
        if([Convert]::ToHexString($header) -cne '89504E470D0A1A0A0000000D4948445200000500000002D0'){throw 'Native preview PNG/IHDR dimensions differ.'}
    }
}

function Assert-FernNativeInventory($Value,$SourceInventory) {
    if(-not $Value.passed -or $Value.namespace -cne '/Game/Trials/Fern02_20260920_01' -or
        @($Value.meshes).Count -ne 4 -or @($Value.textures).Count -ne 5 -or -not $Value.twoSided -or
        [Math]::Abs($Value.opacityClip-0.333) -gt 0.00001){throw 'Fern native inventory shape/material differs.'}
    $models=@($SourceInventory.models)
    $names=@('a','b','c','d')
    for($i=0;$i -lt 4;$i++) {
        $mesh=$Value.meshes[$i]
        $name="fern_02_$($names[$i])"
        $model=@($models|Where-Object name -CEQ $name)
        if($model.Count -ne 1){throw 'Source fern node mismatch.'}
        $geometry=@($SourceInventory.geometries|Where-Object id -EQ $model[0].geometryIds[0])
        if($geometry.Count -ne 1 -or $mesh.sourceModelId -ne $model[0].id -or
            $mesh.sourceGeometryId -ne $geometry[0].id -or $mesh.sourceNode -cne $name -or
            $mesh.triangles -ne $geometry[0].fanTriangleEstimate -or $mesh.importedSlot -cne 'fern_02' -or
            $mesh.object -cne "/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_$($names[$i]).SM_Fern02_$($names[$i])" -or
            $mesh.localPivotPreserved -or $mesh.importUniformScale -ne 1 -or -not $mesh.transformVertexToAbsolute -or $mesh.nanite) {
            throw "Fern mesh/source identity differs:$name"
        }
        $sizes=@(0..2|ForEach-Object {100*($geometry[0].localBounds.max[$_]-$geometry[0].localBounds.min[$_])}|Sort-Object)
        $actual=@($mesh.sizeCm|Sort-Object)
        if($actual.Count -ne 3){throw 'Missing measured fern dimensions.'}
        for($axis=0;$axis -lt 3;$axis++) {
            if(-not [double]::IsFinite([double]$actual[$axis]) -or [Math]::Abs($actual[$axis]-$sizes[$axis]) -gt 0.1) {
                throw "Fern measured unit/bounds mismatch:$name"
            }
        }
    }
    $textureNames=@('Diff','NormalDX','Roughness','AO','Alpha')
    for($i=0;$i -lt 5;$i++) {
        $texture=$Value.textures[$i]
        if($texture.object -cne "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_$($textureNames[$i]).T_Fern02_$($textureNames[$i])" -or
            $texture.width -ne 1024 -or $texture.height -ne 1024 -or $texture.flipGreen -or
            $texture.srgb -ne ($i -eq 0) -or $texture.connectedOutputIndex -ne $(if($i -lt 2){0}else{1})) {
            throw 'Fern actual map/graph inventory differs.'
        }
    }
}
