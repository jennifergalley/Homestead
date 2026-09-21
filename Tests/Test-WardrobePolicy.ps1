[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
. (Join-Path $root 'Scripts\FernSpikePolicy.ps1')
. (Join-Path $root 'Scripts\WardrobePolicy.ps1')
$prefix='/Game/SurvivalGame/Characters/ModularClothing'
$fits=@(Get-WardrobeFits $root)
$fixture=@{passed=$true;cancelledAtPollingBoundary=$false;namespace=$prefix;packages=34
    meshes=@(foreach($fit in $fits) {
        $height=if($fit.fullBody){164}else{80}
        $package="$prefix/$($fit.stem.Replace('\','/'))"
        @{object="$package.$($fit.name)";reference="/Game/SurvivalGame/Characters/Heroine/$($fit.reference).$($fit.reference)"
            source=$fit.source;skeleton='/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave_Skeleton.SK_Heroine_LongWave_Skeleton'
            bones=54;referencePoseMatchesIncumbent=$true;triangles=$fit.triangles;heightCm=$height
            boundsOriginCm=@(0,0,80);boundsExtentCm=@(20,10,($height/2))
            slots=@(for($i=0;$i -lt $fit.roles.Count;$i++) {
                $role=$fit.roles[$i]
                $folder=if($role -eq 'M_Heroine_Hair_long01_Neutral'){'/Game/Trials/HeroineWave_20260921_01/Materials'}
                    elseif($role -eq 'M_Heroine_Hair_bob01_Neutral' -or $role.StartsWith('M_Modular_')){"$prefix/Materials"}
                    else{'/Game/SurvivalGame/Characters/Heroine/Materials'}
                @{index=$i;sourceRoleIndex=$i;role=$role;material="$folder/$role.$role"}
            })
            sections=@(for($i=0;$i -lt $fit.roles.Count;$i++){
                @{materialIndex=$i;role=$fit.roles[$i];triangles=$(if($i -eq 0){$fit.triangles-$fit.roles.Count+1}else{1})}
            })
        }
    })
}
$json=$fixture|ConvertTo-Json -Depth 12
Assert-WardrobeNativeInventory ($json|ConvertFrom-Json) $root
$reordered=$json|ConvertFrom-Json
$mesh=$reordered.meshes[8]
$first=$mesh.slots[0];$mesh.slots[0]=$mesh.slots[12];$mesh.slots[12]=$first
$mesh.slots[0].index=0;$mesh.slots[12].index=12
$mesh.sections[0].materialIndex=12;$mesh.sections[12].materialIndex=0
Assert-WardrobeNativeInventory $reordered $root
$negative=0
foreach($change in @(
    {$v.passed=$false},{$v.cancelledAtPollingBoundary=$true},{$v.namespace='/Game/Other'},
    {$v.packages=33},{$v.meshes=$v.meshes[0..25]},{$v.meshes[1].object=$v.meshes[0].object},
    {$v.meshes[0].source='E:\unapproved.fbx'},{$v.meshes[0].reference='/Game/Other.Reference'},
    {$v.meshes[0].skeleton='/Game/Other.Skeleton'},{$v.meshes[0].bones=53},
    {$v.meshes[0].referencePoseMatchesIncumbent=$false},{$v.meshes[0].triangles=1},
    {$v.meshes[0].heightCm=1.64},{$v.meshes[0].heightCm=[double]::NaN},
    {$v.meshes[3].heightCm=160},{$v.meshes[3].heightCm=0},
    {$v.meshes[0].boundsExtentCm[2]=1},{$v.meshes[0].boundsExtentCm[0]=-1},
    {$v.meshes[0].boundsOriginCm[0]=[double]::PositiveInfinity},
    {$v.meshes[0].slots[0].role='wrong'},{$v.meshes[0].slots[0].index=9},
    {$v.meshes[0].slots[0].material='/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'},
    {$v.meshes[0].slots[3].material='/Game/Other.Hair'},
    {$v.meshes[1].slots[3].material='/Game/Other.Hair'},
    {$v.meshes[0].slots[1].material='/Game/Other.Bra'},
    {$v.meshes[8].slots[0].role='M_Heroine_Brass'},
    {$v.meshes[8].slots[12].role='M_Heroine_ApronTrim'},
    {$v.meshes[0].slots[0].sourceRoleIndex=8},
    {$v.meshes[0].sections[0].materialIndex=99},
    {$v.meshes[0].sections[0].role='M_Heroine_Brass'},
    {$v.meshes[0].sections[0].triangles=0},
    {$v.meshes[0].sections[0].triangles++},
    {$v.meshes[0].sections=$v.meshes[0].sections[1..8]},
    {$v.meshes[0].slots=$v.meshes[0].slots[0..7]}
)) {
    $v=$json|ConvertFrom-Json
    & $change
    $rejected=$false
    try {Assert-WardrobeNativeInventory $v $root}catch{$rejected=$true}
    if(-not $rejected){throw "Negative wardrobe inventory admitted:$change"}
    $negative++
}
$configs=[ordered]@{}
foreach($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')){
    $configs[$name]="E:\fixture\Config\$name.ini"
}
foreach($mode in @('WardrobeImport','WardrobeVerify')) {
    $policy=Get-FernOperationPolicy $mode CompletionDriven
    if($policy.profile -cne 'CookCompletionDriven' -or $policy.softSeconds -ne 0 -or $policy.hardSeconds -ne 0){throw 'Unexpected wardrobe deadline primitive.'}
    $tokens=@(Get-FernProbeArguments 'E:\fixture\Game.uproject' 'E:\fixture\Output' 'E:\fixture\DDC' $configs $mode)
    foreach($token in @("-FernMode=$mode",'-nullrhi','-DisablePython','-noshaderworker')){
        if($token -cnotin $tokens){throw "Missing production token:$token"}
    }
    if('-AllowCommandletRendering' -in $tokens -or '-RunAsCookCommandlet' -in $tokens){throw 'Wardrobe import/verify must not render/cook.'}
    foreach($profile in @('Standard','LongStartup')){
        $rejected=$false
        try{$null=Get-FernOperationPolicy $mode $profile}catch{$rejected=$true}
        if(-not $rejected){throw 'Unapproved wardrobe timer profile admitted.'}
        $negative++
    }
}
$pins=@(Get-WardrobeSourcePins $root)
$stems=@(Get-WardrobePackageStems)
if($pins.Count -ne 33 -or $fits.Count -ne 27 -or $stems.Count -ne 34 -or
    @($stems|Select-Object -Unique).Count -ne 34){throw 'Canonical input/output scope differs.'}
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Scripts\Invoke-ShippingQA.ps1'),[ref]$tokens,[ref]$errors)
if($errors){throw 'Shipping resume adapter syntax differs.'}
$policy=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -ceq 'Get-NativeResumeSource'},$true)
if(-not $policy){throw 'Production resume preflight is missing.'}
. ([scriptblock]::Create($policy.Extent.Text))
$automation=Join-Path $root 'Saved\Automation'
$scratch=Join-Path $automation ('wardrobe-resume-policy-'+[guid]::NewGuid().ToString('N'))
$producer=Join-Path $scratch 'producer with space'
$consumer=Join-Path $scratch 'consumer'
$null=New-Item -ItemType Directory $producer -Force
try {
    foreach($name in 'native-wardrobe-fixture.json','native-wardrobe-fixture.sav'){
        [IO.File]::WriteAllText((Join-Path $producer $name),'disposable preflight bytes, not a valid save')
    }
    $good="-HomesteadNativeMenuTest -HomesteadNativeResumeFrom=`"$producer`""
    if((Get-NativeResumeSource $good $consumer $automation) -cne $producer){throw 'Quoted resume producer rejected.'}
    if($null -ne (Get-NativeResumeSource '-HomesteadNativeMenuTest' $consumer $automation)){throw 'No-resume route changed.'}
    foreach($line in @('-HomesteadNativeMenuTest -HomesteadNativeResumeFrom',
        '-HomesteadNativeMenuTest -HomesteadNativeResumeFrom=""',
        '-HomesteadNativeMenuTest -HomesteadNativeResumeFrom="relative"',
        "-HomesteadNativeResumeFrom=`"$producer`"",
        "$good -HomesteadNativeQuitTest",
        "$good -HomesteadNativeResumeFrom=`"$producer`"",
        "-HomesteadNativeMenuTest -HomesteadNativeResumeFrom=$producer")){
        $rejected=$false
        try{$null=Get-NativeResumeSource $line $consumer $automation}catch{$rejected=$true}
        if(-not $rejected){throw "Invalid resume admitted:$line"}
        $negative++
    }
    foreach($destination in @($producer,(Join-Path $root 'outside-automation'))){
        $rejected=$false
        try{$null=Get-NativeResumeSource $good $destination $automation}catch{$rejected=$true}
        if(-not $rejected){throw 'Invalid resume destination admitted.'}
        $negative++
    }
} finally {
    Remove-Item -LiteralPath (Join-Path $producer 'native-wardrobe-fixture.json'),(Join-Path $producer 'native-wardrobe-fixture.sav')
    Remove-Item -LiteralPath $producer
    Remove-Item -LiteralPath $scratch
}
$menuSource=Get-Content (Join-Path $root 'Source\SurvivalGame\UI\HomesteadNativeMenuTest.cpp') -Raw
$menuBody=$menuSource.Substring($menuSource.IndexOf('void AHomesteadSmokeTest::PrepareNativeMenuChecks()'))
$quit=$menuBody.IndexOf("`n    if (FParse::Param(FCommandLine::Get(), TEXT(`"HomesteadNativeQuitTest`")))")
if($quit -lt 0){throw 'Top-level quit branch is missing.'}
foreach($step in @('Close initial guide through mapped Back','First exit-path press opens Settings')){
    $position=$menuBody.IndexOf($step)
    if($position -lt 0 -or $position -gt $quit -or [regex]::Matches($menuBody,[regex]::Escape($step)).Count -ne 1){
        throw 'Producer and quit must share separately asserted initial-state transitions.'
    }
}
$quitBody=$menuBody.Substring($quit,$menuBody.IndexOf('auto Capture =')-$quit)
if($quitBody.Contains('Tap(EKeys::Escape)') -or -not $quitBody.Contains('ReadSave(') -or
    -not $quitBody.Contains('IsEngineExitRequested()')){
    throw 'Quit must retain actual save/exit verification without repeated inline Escape taps.'
}
"PASS:27 exact fit mappings,33 pinned source files,34 packages, production mode/tokens, shared quit-entry source contract; $negative negative cases."
