[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet(-1,0,1,2,3,4,5,6)][int]$CompileActionId=-1,
    [ValidateSet(-1,1,2,3,4,5,6,7,8,9)][int]$ResourceLinkActionId=-1,
    [ValidateSet('','Game','Probe')][string]$ConvertResource='',
    [string]$DerivedDllResponse='',
    [switch]$DetachedConsole,
    [switch]$FernActions,
    [switch]$UiActions,
    [switch]$WardrobeActions,
    [switch]$WardrobeSlotCorrection,
    [switch]$WardrobeMenuChecks,
    [switch]$ShippingActions,
    [switch]$SplitShippingManifest,
    [switch]$EmbedShippingManifest,
    [switch]$StageCooked,
    [switch]$HairWaveCandidate,
    [switch]$WardrobeCandidate,
    [switch]$WardrobeVisualCorrection
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
$authorityRoot=$root
if($HairWaveCandidate -and -not $ShippingActions){throw 'Hair-wave candidate applies only to the established Shipping leaves.'}
if($WardrobeCandidate -and (-not $ShippingActions -or $HairWaveCandidate)){throw 'Wardrobe candidate requires exclusive Shipping selection.'}
if($WardrobeVisualCorrection -and -not $WardrobeCandidate){throw 'Visual correction requires the explicit wardrobe Shipping candidate.'}
$shippingBuildName=if($WardrobeVisualCorrection){'wardrobe-shipping-build-02'}elseif($WardrobeCandidate){'wardrobe-shipping-build-01'}elseif($HairWaveCandidate){'hair-shipping-build-01'}else{'clearing-shipping-build-03'}
$shippingLinkAction=if($WardrobeCandidate){2}else{1}
$shippingLinkFolder="link$shippingLinkAction"
$shippingCompiles=if($WardrobeCandidate){@(-1,0,1)}else{@(-1,0)}
. (Join-Path $authorityRoot 'Scripts\CompilerLeafEvidence.ps1')
if($WardrobeMenuChecks -and (-not $WardrobeActions -or $WardrobeSlotCorrection -or $CompileActionId -notin @(-1,4) -or
    $ResourceLinkActionId -notin @(-1,5,6) -or $ConvertResource)){
    throw 'Wardrobe menu checks admit only existing game compile4/library6/DLL5.'
}
if($WardrobeSlotCorrection -and (-not $WardrobeActions -or $CompileActionId -notin @(-1,0) -or
    $ResourceLinkActionId -notin @(-1,2,3) -or $ConvertResource)){
    throw 'Wardrobe slot correction admits only existing importer compile0/library3/DLL2.'
}
if($WardrobeActions -and ($FernActions -or $UiActions -or $ShippingActions -or -not $DetachedConsole -or
    $CompileActionId -notin @(-1,0,1,4) -or $ResourceLinkActionId -notin @(-1,2,3,5,6))) {
    throw 'Only exact integrated wardrobe compile/library/DLL leaves are admitted.'
}
if(-not $WardrobeActions -and ($CompileActionId -eq 4 -or $ResourceLinkActionId -eq 5)){throw 'Action requires the pinned wardrobe export.'}
if($UiActions) {
    if($FernActions -or $ShippingActions -or $CompileActionId -notin @(0,6) -or $ResourceLinkActionId -ne -1 -or
        $ConvertResource -or $DerivedDllResponse -or -not $DetachedConsole){throw 'Only the isolated UI PCH/game compile leaves are admitted.'}
    $root='E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-solid-memory'
    if((& git -C $root rev-parse HEAD) -cne 'ea8a1800b7d4313413883fffd99fe89cd8fe1175'){throw 'Frozen UI source differs.'}
    & git -C $root diff --quiet HEAD -- Source
    if($LASTEXITCODE -ne 0){throw 'Frozen UI source has uncommitted edits.'}
} elseif($CompileActionId -eq 6){throw 'Compile6 requires the pinned isolated UI plan.'}
if($ShippingActions -and ($FernActions -or $UiActions -or $CompileActionId -notin $shippingCompiles -or
    $ResourceLinkActionId -notin @(-1,$shippingLinkAction) -or $ConvertResource -eq 'Probe' -or -not $DetachedConsole -or
    ($CompileActionId -eq -1 -and $ResourceLinkActionId -eq -1 -and -not $ConvertResource -and -not $EmbedShippingManifest -and -not $StageCooked))) {
    throw 'Only the pinned Shipping compile/link/game-resource leaves are admitted.'
}
if($ResourceLinkActionId -eq 1 -and -not $ShippingActions){throw 'Link1 requires the pinned Shipping plan.'}
if($SplitShippingManifest -and (-not $ShippingActions -or $ResourceLinkActionId -ne $shippingLinkAction -or -not $DerivedDllResponse)){throw 'Split manifest requires the exact Shipping link.'}
if($EmbedShippingManifest -and (-not $ShippingActions -or $SplitShippingManifest -or $CompileActionId -ne -1 -or
    $ResourceLinkActionId -ne -1 -or $ConvertResource -or $DerivedDllResponse)){throw 'Manifest embedding must be a separate Shipping leaf.'}
if($StageCooked -and (-not $ShippingActions -or $SplitShippingManifest -or $EmbedShippingManifest -or
    $CompileActionId -ne -1 -or $ResourceLinkActionId -ne -1 -or $ConvertResource -or $DerivedDllResponse)) {
    throw 'Loose staging must be a separate Shipping leaf.'
}
$run=& (Join-Path $authorityRoot 'Scripts\Development-Run.ps1') -Action Status
if (-not $run.allowWork -or ($run.completionPolicy -ne 'until-complete' -and
    [DateTimeOffset]::UtcNow.AddMinutes(2) -ge [DateTimeOffset]$run.deadlineUtc)) { throw 'Live run does not admit fixture.' }
$output=[IO.Path]::GetFullPath($OutputDirectory,$root)
if (-not $output.StartsWith((Join-Path $root "Saved\Automation\$($run.id)")+'\',[StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $output)) { throw 'Fresh current-run fixture directory required.' }
$compiler='E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe'
$hash='FE251EF50A1545B1B0835EE17B1E785459712B38D79E45B5C1D3D28970A36619'
if($CompileActionId -ne -1 -and $ResourceLinkActionId -ne -1){throw 'Select only one reviewed action.'}
if($ConvertResource -and ($CompileActionId -ne -1 -or $ResourceLinkActionId -ne -1)){throw 'Conversion must be a separate leaf.'}
$dllActionIds=if($WardrobeActions){@(2,5)}elseif($ShippingActions){@($shippingLinkAction)}elseif($FernActions){@(2)}else{@(6,9)}
if($FernActions -and ($CompileActionId -notin @(-1,0,1) -or $ResourceLinkActionId -notin @(-1,2,3))){throw 'Only the two exported fern compiles/library/DLL are admitted.'}
if(-not $FernActions -and -not $WardrobeActions -and -not($ShippingActions -and $WardrobeCandidate) -and $ResourceLinkActionId -in @(2,3)){throw 'Fern link IDs require the pinned fern map.'}
if($DerivedDllResponse -and $ResourceLinkActionId -notin $dllActionIds){throw 'Derived response requires an approved DLL link.'}
$selectedId=if($ResourceLinkActionId -ne -1){$ResourceLinkActionId}else{$CompileActionId}
if($EmbedShippingManifest) {
    $selectedId='embed-shipping-manifest'
    $compiler='C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\mt.exe'
    $hash='1B8A272D586A9AB53AC2CCD457A88BD0210D7D7AC3DAEA5B34743CC2AFE73B26'
    if((Get-Item $compiler).Length -ne 1972584){throw 'Approved SDK manifest tool size differs.'}
}
if($StageCooked) {
    $selectedId='stage-loose-cooked'
    $compiler='E:\Program Files\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe'
    $hash='0AF909A3DB0C02BD736F3008E2A9B20E7BA4E87EF27DD35619A7A9EC588191A8'
    if(@(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealPak.exe' OR Name='zenserver.exe'").Count) {
        throw 'Staging waits for the shared native runtime slot.'
    }
}
if($ConvertResource) {
    $selectedId="convert-$ConvertResource"
    $compiler='E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cvtres.exe'
    $hash='B5DA94E7B9FF60B388EA9013D9E0E3D4A2A68BFF7C668C7017583459DAC4E3C5'
}
if($ResourceLinkActionId -ne -1) {
    $compiler=if($ResourceLinkActionId -eq 8){'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe'}else{'E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe'}
    $hash=if($ResourceLinkActionId -eq 8){'43DA1503C262C30894C851589BF0155F8365D77E63A5F7BC13982320E3A6B42D'}else{'A364AF801A8539E4324D9489313DBF001D959128451FC99A27B24676BBAC058F'}
    $busyFilter=if($ShippingActions){"Name='mspdbsrv.exe'"}else{"Name='mspdbsrv.exe' OR Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe'"}
    if(@(Get-CimInstance Win32_Process -Filter $busyFilter).Count){throw 'Unowned PDB server or conflicting Editor prevents admission.'}
    if($ShippingActions -and @(Get-CimInstance Win32_Process -Filter "Name='SurvivalGame-Win64-Shipping.exe'" |
        Where-Object ExecutablePath -IEQ (Join-Path $root 'Binaries\Win64\SurvivalGame-Win64-Shipping.exe')).Count) {
        throw 'The exact Shipping build output is running; it will not be overwritten.'
    }
}
if ((Get-FileHash $compiler).Hash -cne $hash -or (Get-AuthenticodeSignature $compiler).Status -ne 'Valid') { throw 'Compiler identity differs.' }
$console='C:\Windows\System32\conhost.exe'
$consoleHash='E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5'
if((Get-FileHash $console).Hash -cne $consoleHash -or (Get-AuthenticodeSignature $console).Status -ne 'Valid') { throw 'Build-only console identity differs.' }
if (@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'").Count) { throw 'Existing uploader prevents an unambiguous compiler fixture.' }
Add-Type -Path (Join-Path $authorityRoot 'Scripts\AuthoringLeafGuard.cs')
$null=New-Item -ItemType Directory -Path $output,(Join-Path $output 'Temp')
$source=Join-Path $output 'fixture.cpp'
$action=$null;$responses=@();$produced=@();$working=$output;$exactArguments=$null;$backups=@()
$generatedManifest=$null
$stageCandidate=$null;$stageCookPins=@();$stageSourcePins=@()
if($StageCooked) {
    . (Join-Path $authorityRoot 'Scripts\FernSpikePolicy.ps1')
    $source='E:\Program Files\UE_5.8\Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
    if((Get-FileHash $source).Hash -cne 'DBE23866969B66071B7035E8D4B262ED3B646A67883442748652BA89B98DA2FD'){throw 'Installed AutomationTool identity differs.'}
    $cookOutput=Join-Path $root ("Saved\Automation\20260921-033354-2d257ba0\"+$(if($WardrobeCandidate){'wardrobe-cook-01'}elseif($HairWaveCandidate){'hair-cook-01'}else{'clearing-cook-02'}))
    $cookProof=Get-Content (Join-Path $cookOutput 'probe-result.json') -Raw|ConvertFrom-Json
    if($cookProof.status -cne 'passed' -or -not $cookProof.subjectExited -or $cookProof.hardTerminated -or
        -not $cookProof.guardDisposed -or @($cookProof.cleanupErrors).Count -or -not $cookProof.markerAfterRelease) {
        throw 'A genuinely completed protected cook with verified cleanup is required.'
    }
    $additional=@()
    if($HairWaveCandidate -or $WardrobeCandidate) {
        . (Join-Path $authorityRoot 'Scripts\HairWavePolicy.ps1')
        $additional=@(Get-HairWavePackageStems|ForEach-Object {"Content\Trials\HeroineWave_20260921_01\$_.uasset"})
    }
    if($WardrobeCandidate){
        . (Join-Path $authorityRoot 'Scripts\WardrobePolicy.ps1')
        $additional+=@(Get-WardrobePackageStems|ForEach-Object {"Content\SurvivalGame\Characters\ModularClothing\$_.uasset"})
    }
    Assert-HomesteadCookOutput (Get-Content (Join-Path $cookOutput 'cook-result.json') -Raw|ConvertFrom-Json) $cookOutput -AdditionalPackages $additional
    $cooked=Join-Path $cookOutput 'Cooked'
    $stageCandidate=Join-Path $root ("Build\Releases\$($run.id)\"+$(if($WardrobeVisualCorrection){'wardrobe-ui-02'}elseif($WardrobeCandidate){'wardrobe-ui-01'}elseif($HairWaveCandidate){'hair-waves-01'}else{'clearing-02'}))
    if(Test-Path $stageCandidate){throw 'Fresh candidate required; no overwrite of an earlier stage.'}
    $clone=Join-Path $output 'CookInput\Windows'
    $null=New-Item -ItemType Directory -Path $clone
    $stageCookPins=@(Get-FernRetainedFiles $cooked)
    foreach($entry in Get-ChildItem $cooked -Force){Copy-Item $entry.FullName $clone -Recurse}
    foreach($pin in $stageCookPins) {
        if((Get-FileHash (Join-Path $clone $pin.path)).Hash -cne $pin.sha256){throw 'Fresh platform-layout cook copy differs.'}
    }
    $native=Get-Content (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\final-native-product.json") -Raw|ConvertFrom-Json
    $meta=Get-Content (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\metadata\result.json") -Raw|ConvertFrom-Json
    if($meta.status -cne 'passed'){throw 'Genuine Shipping metadata is missing.'}
    foreach($product in @($native.exe,$native.pdb)) {
        if((Get-FileHash (Join-Path $root $product.path)).Hash -cne $product.sha256){throw 'Verified Shipping product changed.'}
    }
    foreach($path in @($source,(Join-Path $cookOutput 'probe-result.json'),(Join-Path $cookOutput 'cook-result.json'),
        (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\final-native-product.json"),
        (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\metadata\result.json"),
        (Join-Path $root $native.exe.path),(Join-Path $root $native.pdb.path),
        (Join-Path $root 'Binaries\Win64\SurvivalGame-Win64-Shipping.target'),(Join-Path $root 'docs\asset-credits.md'),
        (Join-Path $root 'SurvivalGame.uproject')) + @(Get-ChildItem (Join-Path $root 'Config') -File -Recurse|ForEach-Object FullName)) {
        $stageSourcePins+=@{path=$path;sha256=(Get-FileHash $path).Hash}
    }
    $responses+=$stageSourcePins
    $exactArguments=(@($source,'-NoCompile','-NoP4','BuildCookRun',"-project=$(Join-Path $root 'SurvivalGame.uproject')",
        '-platform=Win64','-clientconfig=Shipping','-skipbuild','-skipcook','-stage','-skipiostore','-nodebuginfo',
        "-stagingdirectory=$stageCandidate","-CookOutputDir=$clone",'-unattended','-utf8output') |
        ForEach-Object {[Homestead.Authoring.LeafGuard]::Quote($_)}) -join ' '
    $action=[pscustomobject]@{ProducedItems=@((Join-Path $stageCandidate 'Windows\SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe'))}
    @{cookRoot=$cooked;platformLayoutCopy=$clone;files=$stageCookPins;native=$native;sourcePins=$stageSourcePins} |
        ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'stage-inputs.json')
} elseif($EmbedShippingManifest) {
    $source='E:\Program Files\UE_5.8\Engine\Build\Windows\Resources\Default-Win64.manifest'
    if((Get-FileHash $source).Hash -cne '7AD30B1F5464A075878727EA8C19B0B355D4237E6A057C45FACBE47E1764EB0E'){throw 'Engine manifest changed.'}
    $linkOutput=Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\$shippingLinkFolder"
    $linked=Get-Content (Join-Path $linkOutput 'result.json') -Raw|ConvertFrom-Json
    if($linked.status -cne 'passed' -or -not $linked.generatedManifest){throw 'Successful separate-manifest link required.'}
    foreach($product in $linked.producedItems) {
        if((Get-FileHash $product.path).Hash -cne $product.sha256){throw 'Genuine linked product changed before embedding.'}
    }
    $manifest=$linked.generatedManifest.path
    if($manifest -ine (Join-Path $linkOutput 'link-generated.manifest') -or
        (Get-FileHash $manifest).Hash -cne $linked.generatedManifest.sha256){throw 'Genuine generated manifest changed.'}
    $image=Join-Path $root 'Binaries\Win64\SurvivalGame-Win64-Shipping.exe'
    if(@(Get-CimInstance Win32_Process -Filter "Name='SurvivalGame-Win64-Shipping.exe'" |
        Where-Object ExecutablePath -IEQ $image).Count){throw 'Build output is running; manifest cannot be replaced.'}
    Copy-Item $image (Join-Path $output 'before.exe')
    foreach($path in @($source,$manifest)){$responses+=@{path=$path;sha256=(Get-FileHash $path).Hash}}
    $exactArguments=(@('-nologo','-manifest',$manifest,$source,"-outputresource:$image;#1") |
        ForEach-Object {[Homestead.Authoring.LeafGuard]::Quote($_)}) -join ' '
    $action=[pscustomobject]@{ProducedItems=@($image)}
} elseif($ConvertResource) {
    $module=if($ConvertResource -eq 'Game'){'SurvivalGame'}else{'SurvivalGameEditor'}
    $source=Join-Path $root "Intermediate\Build\Win64\x64\UnrealEditor\Development\$module\Default.rc2.res"
    $sourceHash=if($ConvertResource -eq 'Game'){'6D828337158C19C252836D9549BC41A1578DFD616844AB3D715EC95EB9C97143'}else{'12892448A0741ED905D26B26CABB30A92F0C7AB3DD0C5C4D36AD086B26D08F27'}
    if($ShippingActions) {
        $source=Join-Path $root 'Intermediate\Build\Win64\x64\SurvivalGame\Shipping\SurvivalGame-Win64-Shipping-Default.rc2.res'
        $sourceHash='E25E0FDDCC5B1B44174D3716A85C9FDF3F72C7E42C49099D4EDCAF55B37D4887'
    }
    if((Get-FileHash $source).Hash -cne $sourceHash){throw 'Approved resource input changed.'}
    $converted=Join-Path (Split-Path $output -Parent) ($ConvertResource.ToLowerInvariant()+'-resource.obj')
    if(Test-Path $converted){throw 'Converted output must be fresh.'}
    $responses+=@{path=$source;sha256=$sourceHash}
    $exactArguments=('/MACHINE:X64 /READONLY /NOLOGO '+
        [Homestead.Authoring.LeafGuard]::Quote("/OUT:$converted")+' '+[Homestead.Authoring.LeafGuard]::Quote($source))
    $action=[pscustomobject]@{ProducedItems=@($converted)}
} elseif($selectedId -eq -1) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CompilerLeafFixture.cpp') -Destination $source
} else {
    $planPath=Join-Path $root $(if($FernActions){'Saved\Automation\20260920-182217-d1f84e39\native-build-plan-04\actions.json'}else{'Saved\Automation\20260920-182217-d1f84e39\native-build-plan-02\actions.json'})
    $planHash=if($FernActions){'612841C7EF24A780D6C89A6873A0812E4E48E4207ACE86EC9343E46C3762DA36'}else{'10BD045C4AEAD4344ECAECF061DBD65BBA94224DDA478D03F296ED564F5F10FA'}
    if($UiActions) {
        $planPath=Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\ui-native-plan-01\actions.json'
        $planHash='1B4EFE6C578CC93F48D055C0991CCF4180990F17DD7DCCCFF61C03C3A2C1E7A6'
    }
    if($ShippingActions) {
        $planPath=Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\clearing-shipping-plan-02\actions.json'
        $planHash='8923CCD7EA23E7362401B865DAD8684EB877CF0F758B2DD363A037BCFD5D7F27'
    }
    if($WardrobeCandidate){
        $planPath=Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\wardrobe-shipping-plan-01\actions.json'
        $planHash='8455A3EFDC9E9F0BD7ECB69EB52B8B793C5A51677A190CF969512F00B95A6B17'
    }
    if($WardrobeVisualCorrection){
        $planPath=Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\wardrobe-shipping-plan-02\actions.json'
        $planHash='D8358D20EA7F29D79B1BBA78EF5A34C5568D9A5CF6EE4EF0022FDA8940AF0A02'
    }
    if($WardrobeActions) {
        $planPath=Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\wardrobe-native-plan-01\actions.json'
        $planHash='5A801AC790664B9C17894AB1C730EBEC1DB5E541A8343D9B01E6CF1BE7A3E0AE'
    }
    if((Get-FileHash $planPath).Hash -cne $planHash) { throw 'Reviewed action export changed.' }
    $plan=Get-Content $planPath -Raw|ConvertFrom-Json
    $action=@($plan.Actions|Where-Object Id -EQ $selectedId)[0]
    if($action.CommandPath -ine $compiler -or ($CompileActionId -ne -1 -and
        ($action.Type -ne 'Compile' -or ($action.PrerequisiteActions.Count -and -not $UiActions)))) { throw 'Unreviewed action or compile dependencies.' }
    if($UiActions -and $CompileActionId -eq 6) {
        $pch=Get-Content (Join-Path $root 'Saved\Automation\20260921-033354-2d257ba0\ui-compile-01\pch0\result.json') -Raw|ConvertFrom-Json
        if($pch.status -cne 'passed' -or $pch.compileActionId -ne 0){throw 'Verified UI PCH dependency is missing.'}
        foreach($product in $pch.producedItems) {
            if((Get-FileHash $product.path).Hash -cne $product.sha256){throw 'Verified UI PCH output changed.'}
        }
    }
    foreach($dependency in $action.PrerequisiteActions) {
        $prior=@($plan.Actions|Where-Object Id -EQ $dependency)[0]
        foreach($path in $prior.ProducedItems){if(-not(Test-Path -LiteralPath $path)){throw "Missing prerequisite:$path"}}
        if($WardrobeActions) {
            $build=if($WardrobeMenuChecks -and $dependency -eq 4){'wardrobe-native-build-03'}
                elseif($WardrobeSlotCorrection -and $dependency -eq 0){'wardrobe-native-build-02'}else{'wardrobe-native-build-01'}
            $proof=Get-Content (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$build\compile$dependency\result.json") -Raw|ConvertFrom-Json
            if($proof.status -cne 'passed' -or $proof.compileActionId -ne $dependency){throw 'Actual wardrobe prerequisite compile missing.'}
            foreach($product in $proof.producedItems) {
                if((Get-FileHash $product.path).Hash -cne $product.sha256){throw 'Wardrobe prerequisite compile output changed.'}
            }
        }
        if($WardrobeCandidate){
            $proof=Get-Content (Join-Path $root "Saved\Automation\20260921-033354-2d257ba0\$shippingBuildName\compile$dependency\result.json") -Raw|ConvertFrom-Json
            if($proof.status -cne 'passed' -or $proof.compileActionId -ne $dependency){throw 'Actual wardrobe Shipping compile prerequisite missing.'}
            foreach($product in $proof.producedItems){
                if((Get-FileHash $product.path).Hash -cne $product.sha256){throw 'Wardrobe Shipping compile output changed.'}
            }
        }
    }
    $exactArguments=$action.CommandArguments
    if($FernActions) {
        if((Get-FileHash (Join-Path $root 'Intermediate\Build\Win64\x64\UnrealEditor\Development\SurvivalGameEditor\SurvivalGameEditor.Shared.rsp')).Hash -cne '6EB0ED3808F77D6C0675B647A639DBA7734C2B23C20EDF7604687E4310EEF045' -or
            (Get-FileHash (Join-Path $root 'Intermediate\Build\Win64\x64\UnrealEditor\Development\SurvivalGameEditor\Definitions.h')).Hash -cne '116BDD78EDABBF8B31C806F1D01DE57BD7E4617728BBD757DF9C238749552573') { throw 'Fern shared response/definitions differ.' }
        if($action.CommandArguments -notmatch '@"([^"]+)"'){throw 'Missing fern response.'}
        $fernResponsePins=@{
            0='B57B76FE3163082B2462B90BBAFDDC1DEE9053927AC6E26718025D91CB890F1B'
            1='281BA939780E14EB53E20AF93D7E51E6362555982AB6B532145425C9DA24A5D0'
            2='9DE3CD1086EB718A2993E18BD2F136AD9328B5F26B577A0C4EE39F6F164FB309'
            3='8DC5BF3C775B5680C731DADC239B71678E0D95812EE65CF85A1A063665E2EF3A'
        }
        if((Get-FileHash $Matches[1].Replace('/','\')).Hash -cne $fernResponsePins[$selectedId]){throw 'Reviewed actual UBT fern response differs.'}
    }
    $pending=[Collections.Generic.Queue[string]]::new()
    if($action.CommandArguments -match '@"([^"]+)"') {
        $response=[IO.Path]::GetFullPath($Matches[1].Replace('/','\'));$pending.Enqueue($response)
    } elseif($ResourceLinkActionId -ne 8){throw 'Missing reviewed response file.'}
    $seen=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    while($pending.Count) {
        $path=$pending.Dequeue()
        if(-not $seen.Add($path)){continue}
        if($seen.Count -gt 16 -or -not $path.StartsWith((Join-Path $root 'Intermediate')+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected response-file graph.' }
        $responses+=@{path=$path;sha256=(Get-FileHash $path).Hash}
        foreach($line in Get-Content $path) {
            if($line -match '^@"([^"]+)"$'){$pending.Enqueue([IO.Path]::GetFullPath($Matches[1].Replace('/','\')))}
        }
    }
    if($CompileActionId -ne -1) {
        $first=(Get-Content $response -TotalCount 1).Trim('"').Replace('/','\')
        $source=[IO.Path]::GetFullPath($first)
    } elseif($ResourceLinkActionId -eq 8) {
        $source='E:\Program Files\UE_5.8\Engine\Build\Windows\Resources\Default.rc2'
    } else { $source=$response }
    if($DerivedDllResponse) {
        $derived=[IO.Path]::GetFullPath($DerivedDllResponse,$root)
        if(-not $derived.StartsWith((Join-Path $root "Saved\Automation\$($run.id)")+'\',[StringComparison]::OrdinalIgnoreCase) -or
            (Split-Path (Split-Path $derived -Parent) -Leaf) -notmatch '^resource-conversion-[0-9]{2}$'){throw 'Derived response is outside the approved fresh recipe.'}
        $gameLink=if($WardrobeActions){$ResourceLinkActionId -eq 5}else{$ResourceLinkActionId -eq 6}
        $which=if($ShippingActions -or $gameLink){'game'}else{'probe'}
        $module=if($gameLink){'SurvivalGame'}else{'SurvivalGameEditor'}
        $resource=Join-Path $root "Intermediate\Build\Win64\x64\UnrealEditor\Development\$module\Default.rc2.res"
        if($ShippingActions){$resource=Join-Path $root 'Intermediate\Build\Win64\x64\SurvivalGame\Shipping\SurvivalGame-Win64-Shipping-Default.rc2.res'}
        $converted=Join-Path (Split-Path $derived -Parent) "$which-resource.obj"
        $conversion=Get-Content (Join-Path (Split-Path $derived -Parent) "$which\result.json") -Raw|ConvertFrom-Json
        if($conversion.status -ne 'passed' -or (Get-FileHash $converted).Hash -cne $conversion.objectSha256){throw 'Converted resource is not a verified real product.'}
        $null=Read-ResourceCoff $converted
        $oldToken='"'+$resource.Replace('\','/')+'"'
        $newToken='"'+$converted+'"'
        $originalText=[IO.File]::ReadAllText($response)
        if([regex]::Matches($originalText,[regex]::Escape($oldToken)).Count -ne 1){throw 'Expected exactly one original resource token.'}
        $expected=$originalText.Replace($oldToken,$newToken)
        if($SplitShippingManifest) {
            $generatedManifest=Join-Path $output 'link-generated.manifest'
            $eol=if($expected.Contains("`r`n")){"`r`n"}else{"`n"}
            $embed='/MANIFEST:EMBED'
            $inputToken='/MANIFESTINPUT:"../Build/Windows/Resources/Default-Win64.manifest"'
            if([regex]::Matches($expected,[regex]::Escape($embed)).Count -ne 1 -or
                [regex]::Matches($expected,[regex]::Escape($inputToken)).Count -ne 1){throw 'Original manifest recipe differs.'}
            $expected=$expected.Replace($embed,('/MANIFEST'+$eol+'/MANIFESTFILE:"'+$generatedManifest+'"')).Replace($inputToken+$eol,'')
        }
        if([IO.File]::ReadAllText($derived) -cne $expected){throw 'Derived response changes more than the approved resource token.'}
        $responses+=@{path=$derived;sha256=(Get-FileHash $derived).Hash}
        $responses+=@{path=$converted;sha256=(Get-FileHash $converted).Hash}
        $exactArguments='@"'+$derived+'"'
    }
    $working=$action.WorkingDirectory
    foreach($path in $action.ProducedItems) {
        if(-not $path.StartsWith((Join-Path $root 'Intermediate\Build')+'\',[StringComparison]::OrdinalIgnoreCase) -and
            -not ($ResourceLinkActionId -in $dllActionIds -and $path.StartsWith((Join-Path $root 'Binaries\Win64')+'\',[StringComparison]::OrdinalIgnoreCase))) { throw 'Unexpected tool output path.' }
        if(($ResourceLinkActionId -in $dllActionIds -or $path -like '*.lib') -and (Test-Path -LiteralPath $path)) {
            $backupDirectory=Join-Path $output 'before-products'
            $null=New-Item -ItemType Directory -Path $backupDirectory -Force
            $backup=Join-Path $backupDirectory ([IO.Path]::GetFileName($path))
            $beforeHash=(Get-FileHash -LiteralPath $path).Hash
            if($path -like '*.lib'){[IO.File]::Move($path,$backup)}
            else{Copy-Item -LiteralPath $path -Destination $backup}
            if((Get-FileHash $backup).Hash -cne $beforeHash){throw 'Editor binary backup differs.'}
            $backups+=@{path=$path;backup=$backup;sha256=$beforeHash}
        }
    }
}
$marker=Join-Path $output 'dummy-marker'
[IO.File]::WriteAllBytes($marker,[byte[]]@())
$object=Join-Path $output 'fixture.obj'
$environment=[Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP']=Join-Path $output 'Temp';$environment['TMP']=$environment['TEMP']
$environment['CL']=$null;$environment['_CL_']=$null
$arguments=[string[]]@('/nologo','/c','/Z7','/O2',"/Fo$object",$source)
if($action) {
    if(-not $ConvertResource -and -not $EmbedShippingManifest -and -not $StageCooked){foreach($entry in $plan.Environment.PSObject.Properties){$environment[$entry.Name]=[string]$entry.Value}}
    $arguments=[string[]]@()
    $object=$action.ProducedItems[0]
}
if($StageCooked) {
    $environment['DOTNET_CLI_TELEMETRY_OPTOUT']='1'
    $environment['DOTNET_SKIP_FIRST_TIME_EXPERIENCE']='1'
    $environment['DOTNET_CLI_HOME']=Join-Path $output 'DotNetHome'
    $environment['uebp_LogFolder']=Join-Path $output 'Logs'
    $environment['UnrealBuildTool_SourceFileWorkingSet__Provider']='None'
    $environment['UBT_EXTRA_ARGS']=$null
    $null=New-Item -ItemType Directory -Path $environment['DOTNET_CLI_HOME'],$environment['uebp_LogFolder']
}
$samples=[Collections.Generic.List[object]]::new()
$memberSamples=[Collections.Generic.List[object]]::new()
$observedPids=[Collections.Generic.HashSet[uint32]]::new()
$verifiedPids=[Collections.Generic.HashSet[uint32]]::new()
$guard=$null;$failure=$null;$code=$null;$coff=$null;$afterJob=$null;$checks=0;$markerAfter=$null;$members=@()
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null=Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$clock=[Diagnostics.Stopwatch]::StartNew()
try {
    $state=& (Join-Path $authorityRoot 'Scripts\Development-Run.ps1') -Action Status
    if (-not $state.allowWork -or $state.id -ne $run.id) { throw 'Live admission changed.' }
    $guard=[Homestead.Authoring.LeafGuard]::new($compiler,$hash,$arguments,$working,$marker,(Join-Path $output 'compiler.log'),$environment,$exactArguments,[bool]$DetachedConsole)
    $guard.ArmDeadline($(if($action){100000}else{30000}),$(if($action){110000}else{40000}),(Join-Path $output 'watchdog-stop'))
    $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
    [ordered]@{pid=$guard.ProcessId;image=$guard.ImagePath;creationTime=$guard.ProcessCreationTime
        executableSha256=$hash;arguments=$arguments;job=$guard.LastVerifiedJob;resumed=$guard.Resumed
        marker=$guard.MarkerBefore;explicitHandles=$guard.WhitelistedHandleCount;sourceSha256=(Get-FileHash $source).Hash
        guardSha256=(Get-FileHash (Join-Path $authorityRoot 'Scripts\AuthoringLeafGuard.cs')).Hash
        allowedBuildOnlyConsole=@{path=$console;sha256=$consoleHash;signature='Valid'}
        compileActionId=$CompileActionId;workingDirectory=$working;responseFiles=$responses
        expectedProducedItems=$(if($action){$action.ProducedItems}else{@($object)})
        resourceLinkActionId=$ResourceLinkActionId;exactReviewedArguments=$exactArguments;previousProductBackups=$backups
        resourceConversion=$ConvertResource;derivedDllResponse=$DerivedDllResponse
        splitShippingManifest=[bool]$SplitShippingManifest;embedShippingManifest=[bool]$EmbedShippingManifest
        stageCooked=[bool]$StageCooked;stageCandidate=$stageCandidate
        creationFlags=$guard.CreationFlags;detachedConsole=[bool]$DetachedConsole;fernActions=[bool]$FernActions;uiActions=[bool]$UiActions;shippingActions=[bool]$ShippingActions;wardrobeActions=[bool]$WardrobeActions
    }|ConvertTo-Json -Depth 8|Set-Content (Join-Path $output 'launch.json')
    $guard.Resume()
    $null=$observedPids.Add($guard.ProcessId)
    $null=$verifiedPids.Add($guard.ProcessId)
    do {
        $memberSamples.Add([pscustomobject]@{Utc=[DateTimeOffset]::UtcNow.ToString('o');Error=$null;Members=$guard.ObserveJobMembers()})
        foreach($member in $memberSamples[-1].Members){$null=$observedPids.Add($member.Pid)}
        $ids=[uint32[]]$observedPids
        $endpoints=@([Homestead.Authoring.LeafGuard]::ObserveEndpoints([uint32[]]$ids))
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;endpoints=$endpoints;queriedPids=$ids})
        if($endpoints.Count){throw 'Unexpected owned IPv4/IPv6 TCP/UDP endpoint.'}
        foreach($member in $memberSamples[-1].Members) {
            if($member.Error -and $member.NativeError -eq 87 -and $verifiedPids.Contains($member.Pid)){continue}
            if($member.Error -or $member.Image -notin @($compiler,$console)){throw "Unexpected or unidentified live job member:$($member|ConvertTo-Json -Compress)"}
            $null=$verifiedPids.Add($member.Pid)
        }
        $state=& (Join-Path $authorityRoot 'Scripts\Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id -or $clock.Elapsed.TotalSeconds -gt $(if($action){105}else{35})) { throw 'Compiler leaf cancelled by run/deadline.' }
    } while (-not $guard.Wait(10))
    $code=$guard.ExitCode
    if (@(Get-CimInstance Win32_Process -Filter "Name='VCTIP.EXE'").Count) { throw 'Uploader appeared during compiler fixture.' }
    $afterJob=$guard.CaptureExitedJob()
    if ($code -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw "Compiler fixture failed:$code" }
    if($action) {
        foreach($path in $action.ProducedItems) {
            $file=Get-Item -LiteralPath $path
            if(-not $StageCooked -and $file.LastWriteTimeUtc.ToFileTimeUtc() -lt $guard.ProcessCreationTime) { throw "Compiler output is stale:$path" }
            if($path -like '*.obj') {
                $stream=[IO.File]::OpenRead($path)
                try { $header=[byte[]]::new(56);$read=$stream.Read($header,0,56) } finally {$stream.Dispose()}
                $normal=$read -ge 20 -and [BitConverter]::ToUInt16($header,0) -eq 0x8664
                $big=$read -eq 56 -and [BitConverter]::ToUInt16($header,0) -eq 0 -and
                    [BitConverter]::ToUInt16($header,2) -eq 0xffff -and [BitConverter]::ToUInt16($header,6) -eq 0x8664
                if(-not $normal -and -not $big){throw 'Output is not an AMD64 COFF object.'}
            }
            elseif($path -match '\.(lib|dll|exe|pdb|res)$') {
                $stream=[IO.File]::OpenRead($path)
                try {
                    $header=[byte[]]::new(64);$read=$stream.Read($header,0,64)
                    if($path -like '*.lib' -and [Text.Encoding]::ASCII.GetString($header,0,8) -cne "!<arch>`n"){throw 'Invalid import library.'}
                    if($path -like '*.pdb' -and -not [Text.Encoding]::ASCII.GetString($header,0,32).StartsWith('Microsoft C/C++ MSF 7.00')){throw 'Invalid PDB.'}
                    if($path -like '*.res' -and ($read -lt 32 -or [BitConverter]::ToUInt32($header,0) -ne 0 -or [BitConverter]::ToUInt32($header,4) -ne 32)){throw 'Invalid resource file.'}
                    if($path -match '\.(dll|exe)$') {
                        if($read -lt 64 -or [Text.Encoding]::ASCII.GetString($header,0,2) -cne 'MZ'){throw 'Invalid DLL header.'}
                        $at=[BitConverter]::ToUInt32($header,60)
                        if($at+6 -gt $stream.Length){throw 'Invalid PE offset.'}
                        $null=$stream.Seek($at,'Begin');$pe=[byte[]]::new(6);$null=$stream.Read($pe,0,6)
                        if([BitConverter]::ToUInt32($pe,0) -ne 0x4550 -or [BitConverter]::ToUInt16($pe,4) -ne 0x8664){throw 'Invalid AMD64 PE.'}
                    }
                } finally {$stream.Dispose()}
            }
            $produced+=@{path=$path;bytes=$file.Length;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o');sha256=(Get-FileHash $path).Hash}
        }
        foreach($item in $responses) {
            if((Get-FileHash $item.path).Hash -cne $item.sha256){throw 'Compiler response file changed during execution.'}
        }
        if($SplitShippingManifest) {
            $file=Get-Item $generatedManifest
            if($file.Length -eq 0 -or $file.LastWriteTimeUtc.ToFileTimeUtc() -lt $guard.ProcessCreationTime){throw 'Fresh linker-generated manifest is missing.'}
        }
        if($StageCooked) {
            $platform=Join-Path $stageCandidate 'Windows'
            Assert-FernOrdinaryTree $platform
            if((Get-FileHash $object).Hash -cne $native.exe.sha256){throw 'Staged executable differs from genuine merged Shipping code.'}
            if(@(Get-ChildItem $platform -Recurse -File|Where-Object Extension -In @('.pak','.utoc','.ucas')).Count){throw 'Unexpected containers in loose-file candidate.'}
            foreach($pin in $stageCookPins) {
                if((Get-FileHash (Join-Path $cooked $pin.path)).Hash -cne $pin.sha256){throw 'Accepted cook changed while staging.'}
                if($pin.path -notlike 'SurvivalGame\Metadata\*' -and
                    ($pin.path -match '\.(uasset|umap|uexp|ubulk|ushaderbytecode)$' -or
                    $pin.path -match '(?:^|\\)(AssetRegistry|GlobalShaderCache[^\\]*)\.bin$')) {
                    $staged=Join-Path $platform $pin.path
                    if(-not(Test-Path $staged) -or (Get-FileHash $staged).Hash -cne $pin.sha256){throw "Fresh cooked file missing/different in stage:$($pin.path)"}
                }
            }
            $log=Get-Content (Join-Path $output 'Logs\Log.txt') -Raw
            foreach($setting in @('Pak=False','SkipCook=True','Build=False','SkipIoStore=True')) {
                if($log -notmatch ('(?m)^'+[regex]::Escape($setting)+'\r?$')){throw "Actual UAT stage parameter differs:$setting"}
            }
            Copy-Item (Join-Path $root 'docs\asset-credits.md') (Join-Path $platform 'asset-credits.md')
            [ordered]@{configuration='Shipping';packageDirectory=$platform;archiveDirectory=$stageCandidate;
                executable='SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe';status='Genuine loose-file stage; runtime acceptance pending';
                cookedInput=$cooked;nativeExecutableSha256=$native.exe.sha256;containersUsed=$false;runtimeVerified=$false} |
                ConvertTo-Json|Set-Content (Join-Path $stageCandidate 'build-receipt.json')
        }
        if($ConvertResource){$coff=Read-ResourceCoff $object}
    } else {
        $bytes=[IO.File]::ReadAllBytes($object)
        $coff=Read-CompilerLeafCoff $bytes
        foreach ($case in @('truncated','wrong-machine','symbol-overflow','bad-strings','wrong-code')) {
        $bad=[byte[]]$bytes.Clone()
        switch ($case) {
            'truncated' { $bad=[byte[]]@(0,1,2) }
            'wrong-machine' { $bad[0]=0 }
            'symbol-overflow' { [Array]::Copy([BitConverter]::GetBytes([uint32]::MaxValue),0,$bad,12,4) }
            'bad-strings' {
                $at=[BitConverter]::ToUInt32($bad,8)+18*[BitConverter]::ToUInt32($bad,12)
                [Array]::Copy([BitConverter]::GetBytes([uint32]::MaxValue),0,$bad,$at,4)
            }
            'wrong-code' { $section=$coff.sections|Where-Object name -EQ $coff.function.section|Select-Object -First 1;$bad[$section.offset+$coff.function.offset]=0 }
        }
        $rejected=$false
        try { $null=Read-CompilerLeafCoff $bad } catch { $rejected=$true }
        if (-not $rejected) { throw "Malformed COFF accepted:$case" }
            $checks++
        }
    }
    $members=@($memberSamples)
    Assert-CompilerLeafAccounting $afterJob $members $guard.ProcessId $compiler
    if($UiActions) {
        if((& git -C $root rev-parse HEAD) -cne 'ea8a1800b7d4313413883fffd99fe89cd8fe1175'){throw 'Frozen UI source changed during compile.'}
        & git -C $root diff --quiet HEAD -- Source
        if($LASTEXITCODE -ne 0){throw 'UI source was edited during compile.'}
    }
    if((Get-FileHash $console).Hash -cne $consoleHash) { throw 'Console image changed.' }
} catch { $failure=$_.ToString() }
finally {
    $members=@($memberSamples)
    if ($guard) {
        try {
            if (-not $guard.Wait(0)) { $guard.HardStop(98) }
            if($null -eq $code){$code=$guard.ExitCode}
            $markerAfter=$guard.VerifyMarker()
        } catch { $failure=(@($failure,$_.ToString())|Where-Object {$_}) -join ' | ' }
        finally { $guard.Dispose() }
    }
    $previous=0.0;$gaps=@()
    foreach($sample in $samples){$gaps+=$sample.elapsedMs-$previous;$previous=$sample.elapsedMs}
    $gaps+=$clock.Elapsed.TotalMilliseconds-$previous
    [ordered]@{status=$(if($failure){'failed'}else{'passed'});error=$failure;exitCode=$code;elapsedSeconds=$clock.Elapsed.TotalSeconds
        samples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum;exitedJob=$afterJob;jobMemberSamples=$members
        coff=$coff;malformedCoffCases=$checks;markerAfter=$markerAfter;compileActionId=$CompileActionId;resourceLinkActionId=$ResourceLinkActionId;producedItems=$produced;previousProductBackups=$backups
        objectSha256=$(if(Test-Path $object){(Get-FileHash $object).Hash}else{$null})
        generatedManifest=$(if($generatedManifest -and (Test-Path $generatedManifest)){@{path=$generatedManifest;sha256=(Get-FileHash $generatedManifest).Hash}}else{$null})
        stageCandidate=$stageCandidate;stageCookFileCount=$stageCookPins.Count
        limits=$(if($StageCooked){'Standard managed UAT loose-file staging under the existing root-only guard; no compiler/cook/Pak/game or extra child admitted. No real global marker. Finite endpoint/member samples, not continuous tracing.'}else{'One explicitly selected guarded build leaf with exact approved Windows console host. No application helpers/UBT/Editor/global marker. Finite endpoint/member samples, not continuous tracing. Aggregate counters do not identify denied targets.'})
    }|ConvertTo-Json -Depth 12|Set-Content (Join-Path $output 'result.json')
}
if($failure){throw $failure}
if($action) {
    [pscustomobject]@{status='passed';actionId=$selectedId;producedFiles=$produced.Count;endpointSamples=$samples.Count;output=$output}
} else { Get-Content (Join-Path $output 'result.json') -Raw }
