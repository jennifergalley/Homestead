$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$compiler=Get-Content (Join-Path $PSScriptRoot 'Test-CompilerLeaf.ps1') -Raw
$metadata=Get-Content (Join-Path $root 'Scripts\Build-AuthoringProbe.ps1') -Raw
foreach($text in @($compiler,$metadata)){
    $tokens=$null;$errors=$null
    $null=[Management.Automation.Language.Parser]::ParseInput($text,[ref]$tokens,[ref]$errors)
    if($errors.Count){throw ($errors|Out-String)}
}
$compilerPrefix=$compiler.Substring(0,$compiler.IndexOf(". (Join-Path `$authorityRoot 'Scripts\CompilerLeafEvidence.ps1')"))
$metadataPrefix=$metadata.Substring(0,$metadata.IndexOf('$manifestAttempt='))
$context="`$PSScriptRoot='$($PSScriptRoot.Replace("'","''"))';"
$compilerPrefix=$compilerPrefix.Insert($compilerPrefix.IndexOf('$ErrorActionPreference'),$context)
$metadataPrefix=$metadataPrefix.Insert($metadataPrefix.IndexOf('$ErrorActionPreference'),$context)
$leaf=[scriptblock]::Create($compilerPrefix+';@{build=$shippingBuildName;link=$shippingLinkAction;compiles=$shippingCompiles}')
$meta=[scriptblock]::Create($metadataPrefix+';@{build=$shippingBuildName;link=$shippingLinkFolder}')
$grass=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate
if($grass.build -cne 'grass-shipping-build-01' -or $grass.link -ne 3 -or
    ($grass.compiles -join ',') -cne '-1,0,1,2'){throw 'Actual grass leaf selector differs'}
$actual=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate
if($actual.build -cne $grass.build -or $actual.link -cne 'link3'){throw 'Actual metadata selector differs'}
$diagnostic=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic
$diagnosticMeta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic
if($diagnostic.build -cne 'readability-shipping-build-01' -or $diagnostic.link -ne 2 -or
    ($diagnostic.compiles -join ',') -cne '-1,0,1' -or $diagnosticMeta.build -cne $diagnostic.build -or
    $diagnosticMeta.link -cne 'link2'){throw 'Actual readability diagnostic selector differs'}
$correction=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic -ReadabilityCorrection
$correctionMeta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic -ReadabilityCorrection
if($correction.build -cne 'readability-shipping-build-02' -or $correction.link -ne 2 -or
    $correctionMeta.build -cne $correction.build -or $correctionMeta.link -cne 'link2'){throw 'Actual correction selector differs'}
$lifecycle=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic -ReadabilityCorrection -ReadabilityLifecycle
$lifecycleMeta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -ReadabilityDiagnostic -ReadabilityCorrection -ReadabilityLifecycle
if($lifecycle.build -cne 'readability-shipping-build-03' -or $lifecycle.link -ne 1 -or
    ($lifecycle.compiles -join ',') -cne '-1,0' -or $lifecycleMeta.build -cne $lifecycle.build -or
    $lifecycleMeta.link -cne 'link1'){throw 'Actual lifecycle selector differs'}
$legacy=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GroveCandidate -GroveProxyCorrection
if($legacy.build -cne 'grove-shipping-build-03' -or $legacy.link -ne 1 -or
    ($legacy.compiles -join ',') -cne '-1,0'){throw 'Legacy grove selector changed'}
$navigation=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate
$navigationMeta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate
if($navigation.build -cne 'navigation-shipping-build-01' -or $navigation.link -ne 3 -or
    ($navigation.compiles -join ',') -cne '-1,0,1,2' -or $navigationMeta.build -cne $navigation.build -or
    $navigationMeta.link -cne 'link3'){throw 'Actual navigation selectors differ'}
$navigation2=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate -NavigationRevision 2
$navigation2Meta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate -NavigationRevision 2
if($navigation2.build -cne 'navigation-shipping-build-02' -or $navigation2Meta.build -cne $navigation2.build -or
    $navigation2.link -ne 3 -or $navigation2Meta.link -cne 'link3'){throw 'Second navigation selectors differ'}
$navigation4=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate -NavigationRevision 4
$navigation4Meta=& $meta -OutputDirectory 'unused' -ProjectDirectory $root -ShippingActions -WriteMetadataOnly -TreeDiagnosticCandidate -GrassGroundCandidate -NavigationCandidate -NavigationRevision 4
if($navigation4.build -cne 'navigation-shipping-build-04' -or $navigation4Meta.build -cne $navigation4.build -or
    $navigation4.link -ne 2 -or $navigation4Meta.link -cne 'link2' -or
    ($navigation4.compiles -join ',') -cne '-1,0,1'){throw 'Actual corrected analog action map differs'}
$rejected=0
foreach($flags in @(
    @{NavigationCandidate=$true},
    @{NavigationRevision=2},
    @{ShippingActions=$true;TreeDiagnosticCandidate=$true;GrassGroundCandidate=$true;ReadabilityDiagnostic=$true;NavigationCandidate=$true},
    @{ReadabilityLifecycle=$true},
    @{ReadabilityCorrection=$true},
    @{ReadabilityDiagnostic=$true},
    @{GrassGroundCandidate=$true},
    @{ShippingActions=$true;GrassGroundCandidate=$true},
    @{ShippingActions=$true;TreeDiagnosticCandidate=$true;GrassGroundCandidate=$true;GroveCandidate=$true},
    @{ShippingActions=$true;TreeDiagnosticCandidate=$true;GrassGroundCandidate=$true;TreeContactCorrection=$true}
)){
    $failed=$false
    try{$null=& $leaf -OutputDirectory 'unused' @flags}catch{$failed=$true}
    if(-not $failed){throw 'Invalid grass selector admitted'}
    $rejected++
}
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseInput($compiler,[ref]$tokens,[ref]$errors)
$checks=@($ast.FindAll({param($node)
    $node -is [Management.Automation.Language.IfStatementAst] -and
        $node.Clauses[0].Item1.Extent.Text -ceq '$GrassActions' -and
        $node.Extent.Text.Contains('Actual grass prerequisite compile missing.')
},$true))
if($checks.Count -ne 1){throw 'Missing unique grass native prerequisite guard'}
for($parent=$checks[0].Parent;$parent;$parent=$parent.Parent){
    if($parent -is [Management.Automation.Language.IfStatementAst] -and
        $parent.Clauses[0].Item1.Extent.Text -ceq '$TreeActions'){throw 'Grass prerequisite incorrectly nested under exclusive tree mode'}
}
"Passed actual Shipping/metadata selector prefixes, legacy grove selector, $rejected rejected combinations and independent grass prerequisite guard. No processes launched."
$qa=Get-Content (Join-Path $root 'Scripts\Invoke-ShippingQA.ps1') -Raw
$ast=[Management.Automation.Language.Parser]::ParseInput($qa,[ref]$tokens,[ref]$errors)
if($errors){throw 'Shipping QA syntax differs'}
$policy=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -ceq 'Assert-QACompletionRoute'},$true)
if(-not $policy){throw 'Production completion-route policy missing'}
. ([scriptblock]::Create($policy.Extent.Text))
$good='-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadEndurance -HomesteadEnduranceFresh'
Assert-QACompletionRoute $good $true 'until-complete'
Assert-QACompletionRoute '-HomesteadShippingQA -HomesteadVisualPlaytest' $false 'deadline'
Assert-QACompletionRoute '-HomesteadShippingQA -HomesteadVisualPlaytest' $true 'until-complete'
$generated='-HomesteadShippingQA -HomesteadSmokeTest -HomesteadGeneratedWoodland'
Assert-QACompletionRoute $generated $true 'until-complete'
foreach($case in @(
    @{line=$good;policy='deadline'},
    @{line=$good.Replace(' -HomesteadEnduranceFresh','');policy='until-complete'},
    @{line=$good+' -HomesteadSmokeTest';policy='until-complete'},
    @{line=$good+' -HomesteadWateringPlaytest';policy='until-complete'},
    @{line=$good+' -HomesteadEndurance';policy='until-complete'},
    @{line=$generated;policy='deadline'},
    @{line=$generated.Replace(' -HomesteadSmokeTest','');policy='until-complete'},
    @{line=$generated+' -HomesteadGeneratedWoodland';policy='until-complete'},
    @{line=$generated+' -HomesteadVisualPlaytest';policy='until-complete'},
    @{line=$generated+' -HomesteadNativeMenuTest';policy='until-complete'},
    @{line=$generated+' -HomesteadClearingTest';policy='until-complete'},
    @{line=$generated+' -HomesteadFullLoop';policy='until-complete'}
    @{line='-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadEnduranceFresh';policy='until-complete'},
    @{line='-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadPresentationDiagnostics';policy='until-complete'},
    @{line='-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadFullLoop';policy='until-complete'},
    @{line='-HomesteadShippingQA -HomesteadVisualPlaytest';policy='deadline'},
    @{line='-HomesteadShippingQA -HomesteadVisualPlaytest -HomesteadVisualPlaytest';policy='until-complete'}
)){
    $failed=$false
    try{Assert-QACompletionRoute $case.line $true $case.policy}catch{$failed=$true}
    if(-not $failed){throw 'Invalid completion-driven route admitted'}
}
Add-Type -Path (Join-Path $root 'Scripts\AuthoringLeafGuard.cs')
[Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile(0,0,'RenderCompletionDriven')
$failed=$false
try{[Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile(100,200,'RenderCompletionDriven')}catch{$failed=$true}
if(-not $failed){throw 'Completion profile admitted a synthetic deadline'}
if(-not $qa.Contains("if(`$CompletionDriven){`$guard.ArmDeadline(0,0,`$stop,'RenderCompletionDriven')}") -or
    -not $qa.Contains('else{$guard.ArmDeadline(100000,110000,$stop)}')){
    throw 'Actual completion/default guard entry points differ'
}
'Passed actual fresh Shipping completion-route policy, invalid/mixed routes and existing zero-timer guard profile.'
$smoke=Get-Content (Join-Path $root 'Scripts\Test-Game.ps1') -Raw
$null=[Management.Automation.Language.Parser]::ParseInput($smoke,[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Smoke wrapper syntax differs'}
$admission=[scriptblock]::Create($smoke.Substring(0,$smoke.IndexOf('$root = Split-Path'))+';[bool]$NativeMenu')
if(-not (& $admission -DirectionalNavigation)){throw 'Directional selector did not use the existing native menu route'}
foreach($flags in @(
    @{DirectionalNavigation=$true;NativeMenuQuit=$true},
    @{DirectionalNavigation=$true;NativeResumeFrom=''},
    @{DirectionalNavigation=$true;FullLoop=$true},
    @{DirectionalNavigation=$true;CameraLifecycle=$true},
    @{DirectionalNavigation=$true;Clearing=$true},
    @{DirectionalNavigation=$true;WithAudio=$true}
)){
    $failed=$false
    try{$null=& $admission @flags}catch{$failed=$true}
    if(-not $failed){throw 'Mixed directional smoke route admitted'}
}
'Passed production directional smoke selector and six incompatible route rejections without launching a game.'
$generatedAdmission=[scriptblock]::Create($smoke.Substring(0,$smoke.IndexOf('$root = Split-Path'))+';[bool]$RequireLit')
if(-not (& $generatedAdmission -GeneratedWoodland -ShippingQA)){throw 'Generated gameplay route must require actual Lit checks'}
foreach($flags in @(
    @{GeneratedWoodland=$true},
    @{GeneratedWoodland=$true;ShippingQA=$true;NativeMenu=$true},
    @{GeneratedWoodland=$true;ShippingQA=$true;Clearing=$true},
    @{GeneratedWoodland=$true;ShippingQA=$true;FullLoop=$true},
    @{GeneratedWoodland=$true;ShippingQA=$true;GeneratedResumeFrom=''},
    @{GeneratedWoodland=$true;ShippingQA=$true;GeneratedResumeFrom='relative'},
    @{ShippingQA=$true;GeneratedResumeFrom='E:\producer'}
)){
    $failed=$false
    try{$null=& $generatedAdmission @flags}catch{$failed=$true}
    if(-not $failed){throw 'Invalid generated smoke admission accepted'}
}
'Passed production generated-world selector and seven incompatible/invalid route rejections without a game.'
$resumePolicy=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -ceq 'Get-NativeResumeSource'},$true)
if(-not $resumePolicy){throw 'Production generated resume admission missing'}
. ([scriptblock]::Create($resumePolicy.Extent.Text))
$automation=Join-Path $root 'Saved\Automation'
$scratch=Join-Path $automation ('generated-resume-policy-'+[guid]::NewGuid().ToString('N'))
$producer=Join-Path $scratch 'producer with space'
$consumer=Join-Path $scratch 'consumer'
$names=@('generated-woodland-fixture.json','generated-woodland-fixture.sav')
$null=New-Item -ItemType Directory $producer -Force
try {
    foreach($name in $names){[IO.File]::WriteAllText((Join-Path $producer $name),'disposable path fixture, not a save')}
    $good="$generated -HomesteadGeneratedResumeFrom=`"$producer`""
    if((Get-NativeResumeSource $good $consumer $automation -Generated) -cne $producer){throw 'Generated quoted producer rejected'}
    if($null -ne (Get-NativeResumeSource $generated $consumer $automation -Generated)){throw 'Generated no-resume route changed'}
    foreach($line in @(
        "$generated -HomesteadGeneratedResumeFrom",
        "$generated -HomesteadGeneratedResumeFrom=`"`"",
        "$generated -HomesteadGeneratedResumeFrom=`"relative`"",
        "$good -HomesteadGeneratedResumeFrom=`"$producer`"",
        "$good -HomesteadNativeMenuTest",
        "$good -HomesteadNativeResumeFrom=`"$producer`"",
        $good.Replace(' -HomesteadGeneratedWoodland',''),
        "$generated -HomesteadGeneratedResumeFrom=$producer"
    )){
        $failed=$false
        try{$null=Get-NativeResumeSource $line $consumer $automation -Generated}catch{$failed=$true}
        if(-not $failed){throw "Invalid generated resume admitted:$line"}
    }
    foreach($destination in @($producer,(Join-Path $root 'outside-automation'))){
        $failed=$false
        try{$null=Get-NativeResumeSource $good $destination $automation -Generated}catch{$failed=$true}
        if(-not $failed){throw 'Invalid generated resume destination admitted'}
    }
    [IO.File]::WriteAllBytes((Join-Path $producer $names[1]),[byte[]]::new(0))
    $failed=$false
    try{$null=Get-NativeResumeSource $good $consumer $automation -Generated}catch{$failed=$true}
    if(-not $failed){throw 'Empty generated save admitted'}
    Remove-Item -LiteralPath (Join-Path $producer $names[1])
    $failed=$false
    try{$null=Get-NativeResumeSource $good $consumer $automation -Generated}catch{$failed=$true}
    if(-not $failed){throw 'Missing generated save admitted'}
} finally {
    foreach($name in $names){$path=Join-Path $producer $name;if(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path}}
    Remove-Item -LiteralPath $producer
    Remove-Item -LiteralPath $scratch
}
'Passed actual generated resume source admission and twelve malformed/mixed/path/empty/missing negatives.'
$nativeSmoke=Get-Content (Join-Path $root 'Source\SurvivalGame\HomesteadSmokeTest.cpp') -Raw
$prepareAt=$nativeSmoke.IndexOf('void AHomesteadSmokeTest::Prepare()')
$screenshotAt=$nativeSmoke.IndexOf('void AHomesteadSmokeTest::Screenshot(')
$dispatchAt=$nativeSmoke.IndexOf('PrepareGeneratedWorldChecks();')
if($prepareAt -lt 0 -or $screenshotAt -lt 0 -or $dispatchAt -le $prepareAt -or
    $nativeSmoke.Substring($screenshotAt,$prepareAt-$screenshotAt).Contains('PrepareGeneratedWorldChecks();') -or
    [regex]::Matches($nativeSmoke,'PrepareGeneratedWorldChecks\(\);').Count -ne 1){
    throw 'Generated fixture dispatch must occur once in Prepare, never in screenshot capture'
}
'Passed generated fixture dispatch placement regression.'
