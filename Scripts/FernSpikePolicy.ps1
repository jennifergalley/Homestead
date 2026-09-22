function Get-FernOperationPolicy {
    param(
        [ValidateSet('Settings','Import','Render','Cook','HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify','GrassImport','GrassVerify','WoodlandImport','WoodlandVerify','TreePaletteImport','TreePaletteVerify','MatureFirImport','MatureFirVerify')][string]$Mode,
        [ValidateSet('Standard','LongStartup','CompletionDriven')][string]$RenderProfile='Standard'
    )
    if($RenderProfile -ne 'Standard' -and $Mode -ne 'Render' -and
        -not($Mode -in @('Cook','HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify','GrassImport','GrassVerify','WoodlandImport','WoodlandVerify','TreePaletteImport','TreePaletteVerify','MatureFirImport','MatureFirVerify') -and $RenderProfile -eq 'CompletionDriven')){throw 'Unsupported extended profile/mode.'}
    $policy=switch($Mode) {
        'Settings' {@{profile='Default';softSeconds=100;hardSeconds=110;ceilingSeconds=120;startupSeconds=0;captureSeconds=0}}
        'Import' {@{profile='Import';softSeconds=150;hardSeconds=180;ceilingSeconds=210;startupSeconds=0;captureSeconds=0}}
        {$_ -in @('HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify','GrassImport','GrassVerify','WoodlandImport','WoodlandVerify','TreePaletteImport','TreePaletteVerify','MatureFirImport','MatureFirVerify')} {
            if($RenderProfile -ne 'CompletionDriven'){throw 'Targeted asset work requires explicit completion-driven policy.'}
            @{profile='CookCompletionDriven';softSeconds=0;hardSeconds=0;ceilingSeconds=0;startupSeconds=0;captureSeconds=0}
        }
        'Cook' {
            if($RenderProfile -ne 'CompletionDriven'){throw 'Cook requires explicit completion-driven policy.'}
            @{profile='CookCompletionDriven';softSeconds=0;hardSeconds=0;ceilingSeconds=0;startupSeconds=0;captureSeconds=0}
        }
        'Render' {
            if($RenderProfile -eq 'CompletionDriven') {
                @{profile='RenderCompletionDriven';softSeconds=0;hardSeconds=0;ceilingSeconds=0;startupSeconds=0;captureSeconds=0}
            } elseif($RenderProfile -eq 'LongStartup') {
                @{profile='RenderLongStartup';softSeconds=3240;hardSeconds=3300;ceilingSeconds=3360;startupSeconds=2700;captureSeconds=600}
            } else {@{profile='Render';softSeconds=480;hardSeconds=510;ceilingSeconds=540;startupSeconds=0;captureSeconds=0}}
        }
    }
    [Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile($policy.softSeconds*1000,$policy.hardSeconds*1000,$policy.profile)
    return $policy
}

function Assert-FernStageBudget($Policy,[double]$ElapsedSeconds,$EntrySeconds) {
    if(-not [double]::IsFinite($ElapsedSeconds) -or $ElapsedSeconds -lt 0){throw 'Invalid elapsed budget.'}
    if($Policy.profile -in @('RenderCompletionDriven','CookCompletionDriven')){return}
    if($Policy.profile -eq 'RenderLongStartup') {
        if($null -eq $EntrySeconds) {
            if($ElapsedSeconds -ge $Policy.startupSeconds){throw '45-minute native entry boundary exceeded.'}
        } elseif(-not [double]::IsFinite([double]$EntrySeconds) -or $EntrySeconds -lt 0 -or
            $EntrySeconds -ge $Policy.startupSeconds -or $EntrySeconds -gt $ElapsedSeconds -or
            $ElapsedSeconds-$EntrySeconds -ge $Policy.captureSeconds) {
            throw 'Bounded capture/exit stage exceeded or invalid entry marker time.'
        }
    }
    if($ElapsedSeconds -gt $Policy.softSeconds+5){throw 'Bounded authoring operation timeout.'}
}

function Get-FernProbeArguments([string]$Project,[string]$Output,[string]$Ddc,$Configs,[string]$Mode) {
    if($Mode -cnotin @('Settings','Import','Render','Cook','HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify','GrassImport','GrassVerify','WoodlandImport','WoodlandVerify','TreePaletteImport','TreePaletteVerify','MatureFirImport','MatureFirVerify') -or $Configs.Count -ne 7){throw 'Invalid native mode/config map.'}
    $tokens=@($Project,'-run=HomesteadAuthoringProbe',"-EvidenceDirectory=$Output",
        '-notraceserver','-traceautostart=0','-unattended','-nop4','-nosplash','-stdout','-FullStdOutLogOutput',
        '-DisablePython','-DisablePlugins=PythonScriptPlugin,EditorScriptingUtilities,UdpMessaging,TcpMessaging',
        '-noshaderworker',"-UserDir=$(Join-Path $Output 'EngineUser')","-abslog=$(Join-Path $Output 'editor.log')",
        "-DDC=(Local=(Type=FileSystem,Path=$Ddc,ReadOnly=false,Clean=false,Flush=false,DeleteUnused=false))",
        '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnabledByDefault=False',
        '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTransport=False',
        '-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTunnel=False',
        '-ini:Engine:[/Script/TcpMessaging.TcpMessagingSettings]:EnableTransport=False',
        '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=False',
        '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRunPipInstallOnStartup=False',
        '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bIsolateInterpreterEnvironment=True',
        '-ini:Engine:[DevOptions.Shaders]:bAllowCompilingThroughWorkers=False',
        '-ini:Engine:[ConsoleVariables]:r.Shaders.AllowCompilingThroughWorkers=0',
        '-ini:EditorSettings:[/Script/UnrealEd.CrashReportsPrivacySettings]:bSendUnattendedBugReports=False',
        '-ini:EditorSettings:[/Script/UnrealEd.AnalyticsPrivacySettings]:bSendUsageData=False')
    foreach($name in $Configs.Keys){$tokens+="-${name}INI=$($Configs[$name])"}
    if($Mode -ne 'Settings'){$tokens+="-FernMode=$Mode"}
    if($Mode -eq 'Render'){$tokens+=@('-AllowCommandletRendering','-RenderOffScreen')}
    else{$tokens+='-nullrhi'}
    if($Mode -eq 'Cook'){$tokens+=@('-RunAsCookCommandlet','-TargetPlatform=Windows','-CookProcessCount=1','-SkipZenStore')}
    return [string[]]$tokens
}

function Assert-HomesteadCookOutput($Value,[string]$Output,[string[]]$AdditionalPackages=@()) {
    $cooked=Join-Path $Output 'Cooked'
    if(-not $Value.passed -or $Value.exitCode -ne 0 -or $Value.cancelled -or @($Value.errors).Count -or
        $Value.targetPlatform -cne 'Windows' -or -not $Value.cookByTheBook -or $Value.cookProcessCount -ne 1 -or
        -not $Value.skipZenStore -or $Value.arguments -cnotmatch '(?:^|\s)-SkipZenStore(?:\s|$)' -or
        $Value.outputDirectory.Replace('/','\').TrimEnd('\') -ine $cooked.TrimEnd('\')) {
        throw 'Native Windows cook failed or its admitted output/mode differs.'
    }
    Assert-FernOrdinaryTree $cooked
    $registries=@(Get-ChildItem $cooked -Recurse -File -Filter 'DevelopmentAssetRegistry.bin')
    if($registries.Count -ne 1){throw 'Cook requires one actual development asset registry.'}
    $gameRoot=Split-Path (Split-Path $registries[0].FullName -Parent) -Parent
    $requiredPackages=@('Metadata\CookMetadata.ucookmeta','Content\SurvivalGame\Maps\Homestead.umap') +
        @(Get-FernPackageStems|ForEach-Object {"Content\Trials\Fern02_20260920_01\$_.uasset"})
    if($AdditionalPackages){$requiredPackages+=$AdditionalPackages}
    foreach($relative in $requiredPackages) {
        $file=Join-Path $gameRoot $relative
        if(-not(Test-Path $file -PathType Leaf) -or (Get-Item $file).Length -eq 0){throw "Missing real cooked output:$relative"}
    }
    if(@(Get-ChildItem $cooked -Recurse -File -Filter 'ue.projectstore').Count){throw 'Unexpected Zen cook output; file-based cook required.'}
}

function Get-FernPackageStems {
    @('Meshes\SM_Fern02_a','Meshes\SM_Fern02_b','Meshes\SM_Fern02_c','Meshes\SM_Fern02_d',
        'Textures\T_Fern02_Diff','Textures\T_Fern02_NormalDX','Textures\T_Fern02_Roughness',
        'Textures\T_Fern02_AO','Textures\T_Fern02_Alpha','Materials\M_Fern02')
}

function Get-FernOrdinaryFiles([string]$Directory,[string]$ExceptDirectory='') {
    $null=[Homestead.Authoring.LeafGuard]::InspectDirectory($Directory)
    $Directory=[IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($Directory))
    if($ExceptDirectory){
        $ExceptDirectory=[IO.Path]::TrimEndingDirectorySeparator([IO.Path]::GetFullPath($ExceptDirectory))
        if(-not $ExceptDirectory.StartsWith($Directory+'\',[StringComparison]::OrdinalIgnoreCase)){
            throw 'Excluded cache must be a strict descendant of the observed directory.'
        }
    }
    $pending=[Collections.Generic.Queue[string]]::new()
    $pending.Enqueue($Directory)
    while($pending.Count){
        foreach($item in Get-ChildItem -LiteralPath $pending.Dequeue() -Force){
            if($item.Attributes -band ([IO.FileAttributes]::ReparsePoint -bor [IO.FileAttributes]::Device)){
                throw "Nonordinary entry prohibited:$($item.FullName)"
            }
            if($item.PSIsContainer){
                if($ExceptDirectory -and $item.FullName -ieq $ExceptDirectory){continue}
                $pending.Enqueue($item.FullName)
            }else{$item}
        }
    }
}

function Get-FernRetainedFiles([string]$Directory,[string]$ExceptDirectory='',[switch]$MetadataOnly) {
    @(Get-FernOrdinaryFiles $Directory $ExceptDirectory | Sort-Object FullName | ForEach-Object {
        $record=[ordered]@{path=[IO.Path]::GetRelativePath($Directory,$_.FullName);bytes=$_.Length;
            lastWriteUtcTicks=$_.LastWriteTimeUtc.Ticks;attributes=[int]$_.Attributes}
        if(-not $MetadataOnly){$record.sha256=(Get-FileHash $_.FullName).Hash}
        $record
    })
}

function Get-FernMutableCacheObservation([string]$Directory) {
    $identity=[Homestead.Authoring.LeafGuard]::InspectDirectory($Directory)
    $count=0;[long]$bytes=0
    Get-FernOrdinaryFiles $Directory | ForEach-Object {$count++;$bytes+=$_.Length}
    [ordered]@{directoryIdentity=$identity;fileCount=$count;bytes=$bytes;
        method='Ordinary-tree metadata only; mutable cache bytes are deliberately not hashed.'}
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

function Get-FernPackageFiles([string]$Directory,[switch]$Complete,[string[]]$Stems=@(Get-FernPackageStems)) {
    Assert-FernOrdinaryTree $Directory
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
    foreach($field in @('isClient','canEverRender','worldScenePresent','realRenderScene','fernRegistered',
        'fernRenderStateCreated','fernSceneProxyPresent','captureRegistered','captureVisible')) {
        if($Value.sceneState.$field -ne $true){throw "Missing actual renderer scene/component state:$field"}
    }
    $names=@('fern-a-front.png','fern-a-back.png')
    $files=@(Get-ChildItem -LiteralPath $Directory -File -Filter '*.png')
    if($files.Count -ne 2 -or @($files|Where-Object Name -CNotIn $names).Count){throw 'Unexpected preview image set.'}
    foreach($name in $names) {
        $view=@($Value.views|Where-Object image -CEQ $name)
        if($view.Count -ne 1 -or $view[0].greenPixelCount -lt 50 -or $view[0].screenPercentageShowFlag -or $view[0].motionBlur) {
            throw 'Missing actual fern-view evidence.'
        }
        if(-not $view[0].captureCallReturned -or -not $view[0].renderCommandsFlushed -or
            -not $view[0].readbackSucceeded -or $view[0].pixelCount -ne 1280*720) {
            throw 'Missing actual capture/flush/native readback evidence.'
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
