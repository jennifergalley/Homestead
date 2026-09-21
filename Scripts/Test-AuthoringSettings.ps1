[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$ValidateOnly,
    [ValidateSet('Settings','Import','Render','Cook','HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify')][string]$Mode='Settings',
    [ValidateSet('Standard','LongStartup','CompletionDriven')][string]$RenderProfile='Standard',
    [ValidateSet('Clearing','HairWaves','Wardrobe','Tree')][string]$CookCandidate='Clearing',
    [switch]$TreeDiagnostic
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'AuthoringProbePolicy.ps1')
. (Join-Path $PSScriptRoot 'FernSpikePolicy.ps1')
. (Join-Path $PSScriptRoot 'HairWavePolicy.ps1')
. (Join-Path $PSScriptRoot 'WardrobePolicy.ps1')
. (Join-Path $PSScriptRoot 'TreeSpikePolicy.ps1')
Add-Type -Path (Join-Path $PSScriptRoot 'AuthoringLeafGuard.cs')
$operationPolicy=Get-FernOperationPolicy -Mode $Mode -RenderProfile $RenderProfile
$completionDriven=$RenderProfile -eq 'CompletionDriven'
$hairMode=$Mode -in @('HairImport','HairVerify')
$wardrobeMode=$Mode -in @('WardrobeImport','WardrobeVerify')
$characterMode=$hairMode -or $wardrobeMode
$treeMode=$Mode -in @('TreeImport','TreeVerify')
$treeImportName='tree-import-03'
$assetMode=$characterMode -or $treeMode
$treeCook=$CookCandidate -eq 'Tree'
if($TreeDiagnostic -and -not($Mode -eq 'TreeVerify' -or ($Mode -eq 'Cook' -and $treeCook))){
    throw 'Known tree defect admits only explicitly qualified read verification or tree cook.'
}
$treeVerifyName=if($TreeDiagnostic){'tree-diagnostic-verify-01'}else{'tree-verify-01'}
$treeCookName=if($TreeDiagnostic){'tree-diagnostic-cook-02'}else{'tree-cook-01'}
$hairCook=$CookCandidate -eq 'HairWaves'
$wardrobeCook=$CookCandidate -in @('Wardrobe','Tree')
$characterCook=$hairCook -or $wardrobeCook
if($characterCook -and $Mode -ne 'Cook'){throw 'Character cook selection requires actual Cook mode.'}
$retainedRender=$RenderProfile -ne 'Standard' -or $characterMode
$softSeconds=$operationPolicy.softSeconds
$hardSeconds=$operationPolicy.hardSeconds
$ceilingSeconds=$operationPolicy.ceilingSeconds
$root = Split-Path $PSScriptRoot -Parent
$run = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
if (-not $run.allowWork -or (($completionDriven -or $hairMode) -and $run.completionPolicy -cne 'until-complete') -or
    ($run.completionPolicy -cne 'until-complete' -and [DateTimeOffset]::UtcNow.AddSeconds($ceilingSeconds+60) -ge [DateTimeOffset]$run.deadlineUtc) -or
    $run.authoringApproval.proposalSha256 -cne 'EA25571F37A6F3109BEECCA56B54E56006D8F61F0C0B07A95BA5DE077DB0DBCC') {
    throw 'Live run/approval/deadline does not admit the conditional settings probe.'
}
$deadlineProfile=$operationPolicy.profile
[Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile($softSeconds*1000,$hardSeconds*1000,$deadlineProfile)
$wrapperHash=(Get-FileHash $PSCommandPath).Hash
$output = [IO.Path]::GetFullPath($OutputDirectory, $root)
$runRoot = Join-Path $root "Saved\Automation\$($run.id)"
$longStartup=$RenderProfile -eq 'LongStartup'
$evidenceRunRoot=if($retainedRender){Join-Path $root 'Saved\Automation\20260920-182217-d1f84e39'}else{$runRoot}
if (-not $output.StartsWith($runRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or (Test-Path -LiteralPath $output)) {
    throw 'Fresh current-run probe output required.'
}
$priorReservation=Join-Path $evidenceRunRoot 'native-settings-attempt.json'
$priorResult=Join-Path $evidenceRunRoot 'native-settings-01\probe-result.json'
$supersession=@{approval='Coordinator explicit corrected settings-only third reservation, 2026-09-20 17:57 Arizona; Python execution disabled, dependency modules permitted; no retroactive pass'
    reservationSha256='733810D16F65E4DCC00E83BF210E9E307CFD99DA64579DC12299672AE419402A'
    resultSha256='F0AE7B69A68A2DA0A2261984F52B2321FFEF6569F7B4044F206ED34768D64DF7'
    secondReservationSha256='D90A8DBF29952F7BDBF0322C85A20127EF54023F80285ABDE2B8490E459C9E8A'
    secondResultSha256='A671447286868FC3857FD39EC756DF6AB48F591A48CD45E227AC4F0B649F06F7'}
if($retainedRender) {
    $supersession['currentApproval']='Jenny explicitly authorized fresh90-minute run20260921-033354-2d257ba0 at20:33AZ; one long-startup render only. Earlier fields describe preserved historical settings attempts.'
}
if($completionDriven) {
    $supersession['currentApproval']='Jenny explicitly authorized until-complete environment then inventory work at21:19AZ. Coordinator approved diagnosed render-only corrections and separately named unaltered failed-frame diagnostics. Attempt05 corrects capture flags after registration; all prior failures, stop/pause, helpers and safety gates remain preserved.'
}
if($Mode -eq 'Cook') {
    $supersession['currentApproval']='Until-complete playable-first continuation; coordinator explicitly authorized the diagnosed second cook with supported inner Cook Main SkipZenStore. Original Zen-output/loopback-connection failure remains failed and immutable; no Zen/helper/endpoint allowance. No Python/bootstrap/reimport. Prior renderer results remain internal diagnostics, not gameplay screenshots.'
}
if($hairMode) {
    $supersession['currentApproval']='Coordinator approved frozen six-wave native integration into eight fresh trial packages and normal LongWave binding, explicitly completion-driven rather than inheriting arbitrary fern import timers. Existing stop/pause and policy-failure cancellation, retained local cache, root-only/marker/Python/endpoint controls remain; prior selected Shipping preview unchanged.'
}
if($hairCook) {
    $supersession['currentApproval']='Cook the actually imported/fresh-process-verified frozen six-wave set for ordinary Shipping game-camera acceptance. Native Mode remains installed standard Windows Cook with SkipZenStore. No UI/Bob/modular expansion, no accepted candidate overwrite; existing completion-driven controls retained.'
}
if($wardrobeMode -or $wardrobeCook) {
    $supersession['currentApproval']='Coordinator authorized canonical Bob/modular source integration and coherent native UI/wardrobe candidate after selected wave playable. Exact34 new packages in fresh ModularClothing namespace, original21 fits with six canonical base replacements and six canonical joined Bob meshes; original materials/rig reused. Completion-driven stop/pause/root-only/marker/privacy/localDDC policy unchanged. No selected playable overwrite or source reauthoring.'
    $supersession['slotCorrection']='Coordinator authorized root slot-name mapping correction after failed-before-save import01; only unchanged empty identified trial directory reused, distinct import02, no reset or deletion.'
    $supersession['wardrobeFirstReservationSha256']='501A64A7585CCCE708D01D440F6F7AE7943089918D7B1056164AC3D8B197B02B'
    $supersession['wardrobeFirstResultSha256']='0E5E622AF3835C6A7E1F988C20060FFFDDBD2AD9794C3543D0381A0673E41261'
    if((Get-FileHash (Join-Path $runRoot 'wardrobe-import-attempt-01.json')).Hash -cne $supersession.wardrobeFirstReservationSha256 -or
        (Get-FileHash (Join-Path $runRoot 'wardrobe-import-01\probe-result.json')).Hash -cne $supersession.wardrobeFirstResultSha256){
        throw 'Preserved first wardrobe failure changed.'
    }
    if($wardrobeCook){
        $supersession['currentApproval']='Coherent current native-menu/wardrobe candidate cook after actual34-package import02 and separate-process verify01. Includes retained wave/fern content; no reimport or selected playable overwrite. Existing completion-driven root-only/marker/Python/local-DDC/TraceControl policy and inner/outer SkipZenStore remain.'
    }
}
if($treeMode -or $treeCook) {
    $supersession['currentApproval']='Completion-driven environment continuation; coordinator confirmed exact frozen LOD2 tree slice after wardrobe-ui-02 delivery. Exactly17 fresh trial packages, measured trunk collision and ordinary-world acceptance, not another isolated showcase. Existing guarded native/privacy/DDC/endpoint controls retained; no original LOD0 import or selected-preview overwrite.'
    $supersession['treeCorrection']='Coordinator approved referenced bounds, reversible branch-only UV routing and selective invalid-normal engine handling; distinct import02 reuses only the unchanged empty identified failed-before-save namespace. Frozen v3 and original failure remain unchanged.'
    $supersession['treeFirstReservationSha256']='A3415A967E94AC5861CC7CE1476C81FAD92E1369D2860A6A304CD32303981E62'
    $supersession['treeFirstResultSha256']='377B6A6CDEAA110C90BF99D4A3717F0D15C35F4E4F0A474BECFE5EF8FAA0186C'
    $supersession['treeSecondReservationSha256']='3103F180B725580E429BCA8D97044FB5C497CF140D7A1F0B0305558D68884569'
    $supersession['treeSecondResultSha256']='405E75148269BC380520F26DC773B74A9773DEC790BD683F6765C60E52041B82'
    $supersession['treeNormalCorrection']='Distinct import03 distinguishes orphan entries, retains grouped repair, and permits only referenced exact-zero single-face corners to use their valid engine-computed face normal. Nonzero custom normals preserved exactly; no group-cancellation claim without measurements.'
    if($TreeDiagnostic){
        $supersession['diagnosticApproval']='Coordinator final proportional direction: use unchanged genuine receipt03 code/products, exact retained17 failed-import03 package pins, qualified read/reverify then existing cook/ordinary-game route. Exactly12 branch source-description tangent/binormal corners remain disclosed; every other geometry/normal/material/UV/identity/privacy gate unchanged. Import03 remains FAILED; no reimport, asset write, clean-pass or promotion.'
    }
}
function Assert-NamedSupersession {
    if($treeCook -and $TreeDiagnostic){
        if((Get-FileHash (Join-Path $runRoot 'tree-diagnostic-cook-01\probe-result.json')).Hash -cne 'BA11AB19CC7669CE655CD4E4776064C4C6F8F7E69C43F836A1C422FCAB9067BE' -or
            (Get-FileHash (Join-Path $runRoot 'tree-diagnostic-cook-attempt-01.json')).Hash -cne '2EB65D678AD0C8250E5D9F0E9B529C5CECF28CC730A34F4C94138FE3E4D66DF9'){
            throw 'Original omitted-tree-directory cook failure changed.'
        }
    }
    if(($treeMode -or $treeCook) -and (
        (Get-FileHash (Join-Path $runRoot 'tree-import-attempt-01.json')).Hash -cne $supersession.treeFirstReservationSha256 -or
        (Get-FileHash (Join-Path $runRoot 'tree-import-01\probe-result.json')).Hash -cne $supersession.treeFirstResultSha256 -or
        (Get-FileHash (Join-Path $runRoot 'tree-import-attempt-02.json')).Hash -cne $supersession.treeSecondReservationSha256 -or
        (Get-FileHash (Join-Path $runRoot 'tree-import-02\probe-result.json')).Hash -cne $supersession.treeSecondResultSha256)){
        throw 'Preserved first tree failure changed.'
    }
    $expectedRun=if($retainedRender){'20260921-033354-2d257ba0'}else{'20260920-182217-d1f84e39'}
    if($run.id -cne $expectedRun -or
        $output -ine (Join-Path $runRoot $(if($Mode -eq 'TreeImport'){$treeImportName}elseif($Mode -eq 'TreeVerify'){$treeVerifyName}elseif($treeCook){$treeCookName}elseif($Mode -eq 'WardrobeImport'){'wardrobe-import-02'}elseif($Mode -eq 'WardrobeVerify'){'wardrobe-verify-01'}elseif($wardrobeCook){'wardrobe-cook-01'}elseif($hairCook){'hair-cook-01'}elseif($Mode -eq 'HairImport'){'hair-import-01'}elseif($Mode -eq 'HairVerify'){'hair-verify-01'}elseif($Mode -eq 'Settings'){'native-settings-03'}elseif($Mode -eq 'Import'){'fern-import-02'}elseif($Mode -eq 'Cook'){'clearing-cook-02'}elseif($completionDriven){'fern-render-05'}else{'fern-render-01'})) -or
        (Get-FileHash $priorReservation).Hash -cne $supersession.reservationSha256 -or
        (Get-FileHash $priorResult).Hash -cne $supersession.resultSha256 -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'native-settings-attempt-02.json')).Hash -cne $supersession.secondReservationSha256 -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'native-settings-02\probe-result.json')).Hash -cne $supersession.secondResultSha256) {
        throw 'Only the explicitly authorized third attempt with both prior failures preserved is admitted.'
    }
    if($Mode -ne 'Settings' -and (
        (Get-FileHash (Join-Path $evidenceRunRoot 'native-settings-attempt-03.json')).Hash -cne 'F45A17352F06F0F029090C3ACAE9A4043F58E8EA1C0642B581B30337E96C2046' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'native-settings-03\probe-result.json')).Hash -cne 'FB6E92F924EBD7F841D52B6FF408B4ED708B49AFB3569595E32B7A8228EB5D7C')) {
        throw 'Accepted settings subgate changed.'
    }
    if($Mode -ne 'Settings' -and (
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-attempt-01.json')).Hash -cne '3D799DBE1E344B703B468A7D5016E082C3271700A6C2E50D073EDD3BBC1508C2' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-01\probe-result.json')).Hash -cne '6D6F56576EE6ECCABF0918B99D6168E7C410213387ED11EADE11FB00A49D3910')) {
        throw 'Original pre-resume fern failure changed.'
    }
    if($Mode -in @('Render','Cook') -and (
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-02\probe-result.json')).Hash -cne '31477873B75CC0BD90C7557E0A96D21BC474BE87D278D7787143D55A66720624' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-02\asset-admission.json')).Hash -cne 'C818E55DBD142CA6AA28D26CEB9BA42E6ECAEBD81F2A5FDDD04D7C64B2846B33' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-02\effective-settings.json')).Hash -cne 'C7CDD1D6653F17DE5F9B5800EA62F042059DABB95CAE549BE872F6E3D06EED2D' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-import-attempt-02.json')).Hash -cne 'B3955E859FEEE605CF10248F3B5808B55925FB3FF8BED7454C1CC1C394CADA52')) {
        throw 'Actual successful import evidence changed.'
    }
    if($retainedRender -and (
        $run.authoringApproval.baselineCheckpoint -cne '2cc51202266ab77718e9f70d539794f79e68b421' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-render-attempt-01.json')).Hash -cne 'DA6060A37CC6D29CFFD354A7A07B88873484895DBA17021DAE48A2DBFD79B283' -or
        (Get-FileHash (Join-Path $evidenceRunRoot 'fern-render-01\probe-result.json')).Hash -cne '4158EE119137E1C62AED0D6B5667D116295CD880C0D4470AA6FFC5AA5195F065')) {
        throw 'New-run authorization or immutable failed render changed.'
    }
    if($completionDriven -and (
        $run.continuationApproval.userReply -cne "Don't worry about a time limit, please just continue until the work is done. When you're done with the environment upgrade, please work on implementing the inventory redesign plan you specced out earlier autonomously, running overnight." -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-01\probe-result.json')).Hash -cne '55900C2F05D1A44B6BB28C9C127513DB1EC9874C96A664535B87EACD9CCC5109' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-02\probe-result.json')).Hash -cne 'A2E96E304CD51465C1F21A310A683E2AA1223779D6425AFCFEB6D1861BED3827' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-attempt-02.json')).Hash -cne 'D681297F480DD200D93141EF2AC747A200400A3A70316D92923B40F2376D8E6B' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-03\probe-result.json')).Hash -cne '0770F06DD440070475B97C3FEDBD6164AE01B23BC2E5FC56BADE9BEAE84D0963' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-attempt-03.json')).Hash -cne '66AC7789E75346355232E7762338E1A7BC2C8F8E6912F6548DBDE886C4EBB3E8' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-04\probe-result.json')).Hash -cne '3A4CB3A61E5F2CD3D3E83D5722413E61C19ED4FEEB70D959967808E00F2F4FFE' -or
        (Get-FileHash (Join-Path $runRoot 'fern-render-attempt-04.json')).Hash -cne '8844543BFEAF507D5EE59AF9FD9732061E1D407A6C6EDCCCA1A62864ED5165BA')) {
        throw 'Explicit completion continuation or preserved render failure changed.'
    }
    if($Mode -eq 'Cook' -and
        (Get-FileHash (Join-Path $runRoot 'fern-render-05\probe-result.json')).Hash -cne '5C87CEE4653F2DBB26CCE99D7D062A2F21078E65D6C19D8A9E68EA8AF44627AD') {
        throw 'Accepted fern pipeline result changed.'
    }
    if($Mode -eq 'Cook' -and (
        (Get-FileHash (Join-Path $runRoot 'clearing-cook-01\probe-result.json')).Hash -cne '3D96F79436A4A381B360FF260F7CDA4BD77DA09FA62B7E30377621ED09FE6885' -or
        (Get-FileHash (Join-Path $runRoot 'clearing-cook-attempt-01.json')).Hash -cne 'C982CFD4C677321FA86BCCE26CF5089B144AC9AEC3FDDD853882765A69D07287')) {
        throw 'Original failed Zen-output cook changed.'
    }
}
Assert-NamedSupersession
if($characterCook) {
    foreach($pin in @(
        @{path='clearing-cook-02\probe-result.json';sha256='42186AEDD5099F4497CB986CF440D38D9DA8F7C56AED5467B45DCB81ACF77739'},
        @{path='hair-verify-01\probe-result.json';sha256='6FA45F7737D75A9AA4C2959AA73A5DC55D4F6398E7B7FF010CBABAD9029A7F34'},
        @{path='hair-import-01\asset-admission.json';sha256='83E0A36CFCB56C1C68BB11655300D3D4158D6500E3441A9F388AA4A355F04F8B'}
    )) {
        if((Get-FileHash (Join-Path $runRoot $pin.path)).Hash -cne $pin.sha256){throw 'Accepted hair/cook prerequisite changed.'}
    }
}
$attempt = Join-Path $runRoot $(if($Mode -eq 'TreeImport'){'tree-import-attempt-03.json'}elseif($Mode -eq 'TreeVerify'){'tree-verify-attempt-01.json'}elseif($treeCook){'tree-cook-attempt-01.json'}elseif($Mode -eq 'WardrobeImport'){'wardrobe-import-attempt-02.json'}elseif($Mode -eq 'WardrobeVerify'){'wardrobe-verify-attempt-01.json'}elseif($wardrobeCook){'wardrobe-cook-attempt-01.json'}elseif($hairCook){'hair-cook-attempt-01.json'}elseif($Mode -eq 'HairImport'){'hair-import-attempt-01.json'}elseif($Mode -eq 'HairVerify'){'hair-verify-attempt-01.json'}elseif($Mode -eq 'Settings'){'native-settings-attempt-03.json'}elseif($Mode -eq 'Import'){'fern-import-attempt-02.json'}elseif($Mode -eq 'Cook'){'clearing-cook-attempt-02.json'}elseif($completionDriven){'fern-render-attempt-05.json'}else{'fern-render-attempt-01.json'})
if($TreeDiagnostic){$attempt=Join-Path $runRoot $(if($treeCook){'tree-diagnostic-cook-attempt-02.json'}else{'tree-diagnostic-verify-attempt-01.json'})}
if (Test-Path -LiteralPath $attempt) { throw 'The single native settings attempt is already reserved; no automatic retry.' }
$engine = 'E:\Program Files\UE_5.8\Engine\Binaries\Win64'
$exe = Join-Path $engine 'UnrealEditor-Cmd.exe'
$module = Join-Path $root 'Binaries\Win64\UnrealEditor-SurvivalGameEditor.dll'
$approved = [ordered]@{
    'UnrealEditor-Cmd.exe' = 'AE92F55952A3C9A7DEF90983FCF5AEDCE52D8EE8E4565FBD923985E9D9D3E05E'
    'UnrealEditor-Core.dll' = '3EAF66A3AA55FEB0BEEF9BD88411A577A0589AFADB60724F3D6651D2E46632DB'
    'UnrealEditor-TraceLog.dll' = '0759373042151249445A0260039DFA61E502DACD0E9347CDEFB26A0B6743160C'
}
foreach ($name in $approved.Keys) {
    $path = Join-Path $engine $name
    if ((Get-FileHash -LiteralPath $path).Hash -cne $approved[$name] -or
        (Get-AuthenticodeSignature -LiteralPath $path).Status -ne 'Valid') { throw "Engine identity differs:$name" }
}
$moduleHash = (Get-FileHash -LiteralPath $module).Hash
$buildReceiptPath = Join-Path $root 'docs\research\environment-assets\guarded-correction-01\receipt.json'
$buildReceiptHash = '3F4B00F44B25F3F02207A7A39A94C6F107FB9E3BB43893A3932E30E03570A53B'
if($Mode -ne 'Settings') {
    $buildReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-native-build-01\receipt.json'
    $buildReceiptHash='86DEE9CA2EA8CCC3CF6EC810F8F4967A60BBE249F3622032DEE3356D07821F6F'
}
if($completionDriven) {
    $buildReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-native-build-05\receipt.json'
    $buildReceiptHash='503386DAC6DEE1E8FA42894DEE1542CBED54EAC2AAE27A6673DA2C8D3E00FF36'
}
if($Mode -eq 'Cook') {
    $buildReceiptPath=Join-Path $root 'docs\research\environment-assets\clearing-build-02\receipt.json'
    $buildReceiptHash='C6FC72993EB2BA1BDCF5C38EDD1512312AE9CAA9504D87C8D874A50462659632'
}
if($hairMode -or $hairCook) {
    $buildReceiptPath=Join-Path $root 'docs\research\character-assets\hair-native-build-01\receipt.json'
    $buildReceiptHash='950B1A51C800A87F60B5E4B3861AA817C70E2ADE304E11580935B67EC2F8FE74'
}
if($wardrobeMode -or $wardrobeCook) {
    $buildReceiptPath=Join-Path $root 'docs\research\character-assets\wardrobe-native-build-03\receipt.json'
    $buildReceiptHash='C402E1018D27D6988F8C0ACBB5895B297EED689B48CEEF682824D61EF2DD02C3'
}
if($treeMode -or $treeCook) {
    $buildReceiptPath=Join-Path $root 'docs\research\environment-assets\tree-native-build-03\receipt.json'
    $buildReceiptHash='16BB7A09538FFC59D270794C5B522D1ACA9D3991A529FA111969D9F617FE9CFA'
}
$pythonPins = @{
    'python3.dll'='3C7ECFB999333AAF5BA9DDF4C5BFB8676B63CFCEC3DC5370CBC255A83063962F'
    'python311.dll'='3E5A5C012CDDB3D156D147ACAD59BB489C0716B87DAD274CB5BF20EEC3B68192'
}
if ((Get-FileHash $buildReceiptPath).Hash -cne $buildReceiptHash) { throw 'Accepted native build receipt differs.' }
$acceptedBuild = Get-Content $buildReceiptPath -Raw | ConvertFrom-Json
$supervisorReceipt=$null;$supervisorReceiptHash=$null
if($Mode -ne 'Settings') {
    $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-supervisor-02\receipt.json'
    $supervisorReceiptHash='7D930D038282AB18A14855EDA4E798B2847CC1DEAF7723F69B46B87099E9C761'
    if($longStartup) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-supervisor-03\receipt.json'
        $supervisorReceiptHash='B3ECE28794C53FC783E4A0BECB71CD8C5101E53F7451E447F835B326241EEDFE'
    }
    if($completionDriven) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\fern-supervisor-07\receipt.json'
        $supervisorReceiptHash='CBE90D873DA1931399402080F190F306177B2B814711673C9AEAB652CB0A12D8'
    }
    if($Mode -eq 'Cook') {
        $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\clearing-cook-policy-02\receipt.json'
        $supervisorReceiptHash='C2C06A84286CCAF19712C90DA6678D69E2D22762CDD36BF51599A7249105BD7A'
    }
    if($hairMode) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\character-assets\hair-supervisor-01\receipt.json'
        $supervisorReceiptHash='57B5B1CB196DDF7599429EFC6720A5E2092CADB1DD92BB8E2C4F56F7A3B2BDA1'
    }
    if($hairCook) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\character-assets\hair-cook-policy-01\receipt.json'
        $supervisorReceiptHash='CD7817D3AD8B87332F05CD27B6F4FDDF9682C152D1E4D6343D0CC99EE9AC3DBA'
    }
    if($wardrobeMode -or $wardrobeCook) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\character-assets\wardrobe-supervisor-04\receipt.json'
        $supervisorReceiptHash='F8AE465960EF6430D3D8372C0F7A0E3EEF70386DC4B2987CCCD259EA18550013'
    }
    if($treeMode -or $treeCook) {
        $supervisorReceiptPath=Join-Path $root 'docs\research\environment-assets\tree-supervisor-06\receipt.json'
        $supervisorReceiptHash='97DCEF0A4DA7C7E8D511BC9B8575C2D4412026B03E6DCAB7EAD5B976F59CA2B9'
    }
    if((Get-FileHash $supervisorReceiptPath).Hash -cne $supervisorReceiptHash){throw 'Supervisor revision receipt differs.'}
    $supervisorReceipt=Get-Content $supervisorReceiptPath -Raw|ConvertFrom-Json
}
$productPins = @($acceptedBuild.products)
$requiredProducts = @('UnrealEditor-SurvivalGame.dll','UnrealEditor-SurvivalGame.pdb',
    'UnrealEditor-SurvivalGameEditor.dll','UnrealEditor-SurvivalGameEditor.pdb',
    'UnrealEditor.modules','SurvivalGameEditor.target') | ForEach-Object { "Binaries\Win64\$_" }
if ($productPins.Count -ne $requiredProducts.Count -or
    @($requiredProducts | Where-Object { $_ -cnotin $productPins.path }).Count) {
    throw 'Accepted native product map differs.'
}
function Assert-AcceptedNativeProducts {
    if ((Get-FileHash $buildReceiptPath).Hash -cne $buildReceiptHash) { throw 'Accepted receipt changed.' }
    foreach ($product in $productPins) {
        $path = Join-Path $root $product.path
        if ((Get-Item $path).Length -ne $product.bytes -or (Get-FileHash $path).Hash -cne $product.sha256) {
            throw "Accepted native product differs:$($product.path)"
        }
        if($Mode -ne 'Settings') {
            foreach($sourcePin in $acceptedBuild.sources) {
                if($sourcePin.path -ceq 'Scripts\AuthoringLeafGuard.cs'){continue}
                if((Get-FileHash (Join-Path $root $sourcePin.path)).Hash -cne $sourcePin.sha256){throw "Built fern source/guard changed:$($sourcePin.path)"}
            }
            if((Get-FileHash $supervisorReceiptPath).Hash -cne $supervisorReceiptHash -or
                (Get-FileHash $PSCommandPath).Hash -cne $wrapperHash){throw 'Supervisor revision changed.'}
            foreach($pin in @($supervisorReceipt.supervisorSources)+@($supervisorReceipt.originalAttemptFiles)) {
                if((Get-FileHash (Join-Path $root $pin.path)).Hash -cne $pin.sha256){throw "Reviewed supervisor or original failed attempt changed:$($pin.path)"}
            }
        }
    }
    foreach ($name in $pythonPins.Keys) {
        if ((Get-FileHash (Join-Path (Split-Path (Split-Path $engine -Parent) -Parent) "Binaries\ThirdParty\Python3\Win64\$name")).Hash -cne $pythonPins[$name]) {
            throw 'Installed Python dependency identity differs.'
        }
    }
}
Assert-AcceptedNativeProducts
$trial=Join-Path $root $(if($treeMode){'Content\Trials\TreeSmall02_20260921_01'}elseif($wardrobeMode){'Content\SurvivalGame\Characters\ModularClothing'}elseif($hairMode){'Content\Trials\HeroineWave_20260921_01'}else{'Content\Trials\Fern02_20260920_01'})
$packageStems=if($treeMode){@(Get-TreePackageStems)}elseif($wardrobeMode){@(Get-WardrobePackageStems)}elseif($hairMode){@(Get-HairWavePackageStems)}else{@(Get-FernPackageStems)}
function Get-ProbePackageFiles { @(Get-FernPackageFiles $trial -Complete -Stems $packageStems) }
$trialIdentity=$null;$trialBefore=@();$fernResult=$null;$fernInputs=@();$fernSourceInventory=$null
$contentBefore=@();$assetAdmission=$null
if($Mode -ne 'Settings' -and -not $assetMode) {
    $sourceReceiptPath=Join-Path $root 'Assets\Environment\woodland-preparation-01\download-receipt.json'
    $inventoryPath=Join-Path $root 'Assets\Environment\woodland-preparation-01\source-inventory.json'
    if((Get-FileHash $sourceReceiptPath).Hash -cne 'AACF677F7B8CA07A5656842EA80AFEDC80845E509AFE8731F198DA973F339ECF' -or
        (Get-FileHash $inventoryPath).Hash -cne 'B38DCE70440F97817B3A8C8372D25EFC6B72BEE47E7078FC1D8ED207C1C0AAF4') {
        throw 'Reviewed source receipt/inventory changed.'
    }
    $fernInputs=@((Get-Content $sourceReceiptPath -Raw|ConvertFrom-Json).files|Where-Object asset -CEQ 'fern_02')
    $fernSourceInventory=((Get-Content $inventoryPath -Raw|ConvertFrom-Json).files|
        Where-Object { $_.asset -ceq 'fern_02' -and $_.file -ceq 'fern_02_1k.fbx' }).sourceInspection
    $sourceRoot=Join-Path $root 'Assets\Source\woodland-preparation-20260920-182217-d1f84e39\fern_02'
    if($fernInputs.Count -ne 6 -or @(Get-ChildItem $sourceRoot -Force).Count -ne 6 -or
        @($fernSourceInventory.externalTextureReferencesNotFollowed).Count -or
        @($fernSourceInventory.objectTypes.PSObject.Properties|Where-Object Name -NotIn @('Geometry','Model','Material')).Count) {
        throw 'Exact reference-free six-file fern whitelist differs.'
    }
    Assert-FernOrdinaryTree $sourceRoot
    foreach($inputFile in $fernInputs) {
        $file=Join-Path $sourceRoot $inputFile.file
        if((Get-Item $file).Length -ne $inputFile.bytes -or (Get-FileHash $file).Hash -cne $inputFile.sha256){throw 'Original fern source changed.'}
    }
    Assert-FernOrdinaryTree (Join-Path $root 'Content')
    $contentBefore=@(Get-ChildItem (Join-Path $root 'Content') -Recurse -File -Force |
        Where-Object { -not $_.FullName.StartsWith($trial+'\',[StringComparison]::OrdinalIgnoreCase) } |
        ForEach-Object { @{path=$_.FullName;sha256=(Get-FileHash $_.FullName).Hash} })
    if($Mode -eq 'Import') {
        if(Test-Path -LiteralPath $trial){throw 'Fresh fern trial namespace required; no overwrite/reuse.'}
        $failed=Get-Content (Join-Path $runRoot 'fern-import-01\probe-result.json') -Raw|ConvertFrom-Json
        $quarantine=Join-Path $runRoot 'fern-import-01\discarded-content'
        Assert-FernDirectoryIdentity $failed.trialIdentity $quarantine
        if(@(Get-ChildItem $quarantine -Force).Count){throw 'Only the verifiably empty original trial may be replaced.'}
    } else {
        $priorImport=Get-Content (Join-Path $evidenceRunRoot 'fern-import-02\probe-result.json') -Raw|ConvertFrom-Json
        $assetAdmission=Get-Content (Join-Path $evidenceRunRoot 'fern-import-02\asset-admission.json') -Raw|ConvertFrom-Json
        if($priorImport.status -cne 'passed' -or -not $priorImport.subjectExited -or $priorImport.hardTerminated){throw 'Render requires passed actual import.'}
        Assert-FernNativeInventory $assetAdmission.inventory $fernSourceInventory
        $trialIdentity=$assetAdmission.directoryIdentity
        Assert-FernDirectoryIdentity $trialIdentity $trial
        $trialBefore=@(Get-FernPackageFiles $trial -Complete)
        Assert-FernPackagePins @($assetAdmission.packages) $trialBefore
    }
}
if($assetMode) {
    $fernInputs=if($treeMode){@(Get-TreeSourcePins $root)}elseif($wardrobeMode){@(Get-WardrobeSourcePins $root)}else{@(Get-HairWaveSourcePins $root)}
    Assert-FernOrdinaryTree (Join-Path $root 'Content')
    $contentBefore=@(Get-ChildItem (Join-Path $root 'Content') -Recurse -File -Force |
        Where-Object {-not $_.FullName.StartsWith($trial+'\',[StringComparison]::OrdinalIgnoreCase)} |
        ForEach-Object {@{path=$_.FullName;sha256=(Get-FileHash $_.FullName).Hash}})
    if($Mode -in @('WardrobeImport','TreeImport')) {
        $failedPath=if($treeMode){'tree-import-02\probe-result.json'}else{'wardrobe-import-01\probe-result.json'}
        $failed=Get-Content (Join-Path $runRoot $failedPath) -Raw|ConvertFrom-Json
        if($failed.status -cne 'failed' -or $failed.exitCode -ne 7 -or -not $failed.subjectExited -or
            -not $failed.guardDisposed -or $failed.hardTerminated -or @($failed.cleanupErrors).Count){
            throw 'Only the released failed-before-save asset scope can be reused.'
        }
        $trialIdentity=$failed.trialIdentity
        Assert-FernDirectoryIdentity $trialIdentity $trial
        if(@(Get-ChildItem $trial -Force).Count){throw 'Prior asset scope must remain completely empty.'}
    } elseif($Mode -eq 'HairImport') {
        if(Test-Path -LiteralPath $trial){throw 'Fresh wave trial namespace required; no accepted asset overwrite.'}
    } elseif($TreeDiagnostic) {
        $assetAdmission=Get-TreeFailedImportDiagnosticAdmission $root
        $trialIdentity=$assetAdmission.directoryIdentity
        $trialBefore=@(Get-ProbePackageFiles)
        Assert-FernPackagePins @($assetAdmission.packages) $trialBefore
    } else {
        $importOutput=Join-Path $runRoot $(if($treeMode){$treeImportName}elseif($wardrobeMode){'wardrobe-import-02'}else{'hair-import-01'})
        $importProof=Get-Content (Join-Path $importOutput 'probe-result.json') -Raw|ConvertFrom-Json
        $assetAdmission=Get-Content (Join-Path $importOutput 'asset-admission.json') -Raw|ConvertFrom-Json
        if($importProof.status -cne 'passed' -or -not $importProof.subjectExited -or
            -not $importProof.guardDisposed -or $importProof.hardTerminated -or @($importProof.cleanupErrors).Count) {
            throw 'Persisted wave verification requires an actually passed, released import.'
        }
        if($treeMode){Assert-TreeNativeInventory $assetAdmission.inventory}
        elseif($wardrobeMode){Assert-WardrobeNativeInventory $assetAdmission.inventory $root}
        else{Assert-HairWaveNativeInventory $assetAdmission.inventory}
        $trialIdentity=$assetAdmission.directoryIdentity
        Assert-FernDirectoryIdentity $trialIdentity $trial
        $trialBefore=@(Get-ProbePackageFiles)
        Assert-FernPackagePins @($assetAdmission.packages) $trialBefore
    }
}
if($characterCook) {
    $hairAdmission=Get-Content (Join-Path $runRoot 'hair-import-01\asset-admission.json') -Raw|ConvertFrom-Json
    $hairTrial=Join-Path $root 'Content\Trials\HeroineWave_20260921_01'
    Assert-HairWaveNativeInventory $hairAdmission.inventory
    Assert-FernDirectoryIdentity $hairAdmission.directoryIdentity $hairTrial
    Assert-FernPackagePins @($hairAdmission.packages) @(Get-FernPackageFiles $hairTrial -Complete -Stems @(Get-HairWavePackageStems))
}
if($wardrobeCook){
    foreach($pin in @(
        @{path='wardrobe-import-02\asset-admission.json';sha256='6263C0C1AA3F62EF1A42BE7123AAC7ED66243BE405A49A3D599C2E3371FC4591'},
        @{path='wardrobe-verify-01\probe-result.json';sha256='4504EA31EBBA1DA6DA80A1CBFC827120DFB7CBEE410F48C7FF441FD63AA90AD3'}
    )){
        if((Get-FileHash (Join-Path $runRoot $pin.path)).Hash -cne $pin.sha256){throw 'Verified wardrobe prerequisite changed.'}
    }
    $wardrobeAdmission=Get-Content (Join-Path $runRoot 'wardrobe-import-02\asset-admission.json') -Raw|ConvertFrom-Json
    $wardrobeTrial=Join-Path $root 'Content\SurvivalGame\Characters\ModularClothing'
    Assert-WardrobeNativeInventory $wardrobeAdmission.inventory $root
    Assert-FernDirectoryIdentity $wardrobeAdmission.directoryIdentity $wardrobeTrial
    Assert-FernPackagePins @($wardrobeAdmission.packages) @(Get-FernPackageFiles $wardrobeTrial -Complete -Stems @(Get-WardrobePackageStems))
    $null=@(Get-WardrobeSourcePins $root)
}
if($treeCook -and $TreeDiagnostic) {
    $treeAdmission=Get-TreeFailedImportDiagnosticAdmission $root
    $treeVerify=Get-Content (Join-Path $runRoot "$treeVerifyName\probe-result.json") -Raw|ConvertFrom-Json
    if($treeVerify.status -cne 'passed-diagnostic-only' -or -not $treeVerify.treeDiagnostic -or
        -not $treeVerify.subjectExited -or -not $treeVerify.guardDisposed -or $treeVerify.hardTerminated -or
        @($treeVerify.cleanupErrors).Count){throw 'Diagnostic cook requires actual released qualified read verification.'}
    Assert-TreeNativeInventory $treeVerify.fernInventory -KnownTangentDiagnostic
    $null=@(Get-TreeSourcePins $root)
} elseif($treeCook) {
    $treeImport=Get-Content (Join-Path $runRoot "$treeImportName\probe-result.json") -Raw|ConvertFrom-Json
    $treeVerify=Get-Content (Join-Path $runRoot 'tree-verify-01\probe-result.json') -Raw|ConvertFrom-Json
    foreach($proof in @($treeImport,$treeVerify)) {
        if($proof.status -cne 'passed' -or -not $proof.subjectExited -or -not $proof.guardDisposed -or
            $proof.hardTerminated -or @($proof.cleanupErrors).Count){throw 'Tree cook requires actual released import and persisted verification.'}
        Assert-TreeNativeInventory $proof.fernInventory
    }
    $treeAdmission=Get-Content (Join-Path $runRoot "$treeImportName\asset-admission.json") -Raw|ConvertFrom-Json
    $treeTrial=Join-Path $root 'Content\Trials\TreeSmall02_20260921_01'
    Assert-TreeNativeInventory $treeAdmission.inventory
    Assert-FernDirectoryIdentity $treeAdmission.directoryIdentity $treeTrial
    Assert-FernPackagePins @($treeAdmission.packages) @(Get-FernPackageFiles $treeTrial -Complete -Stems @(Get-TreePackageStems))
    $null=@(Get-TreeSourcePins $root)
}
$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\research\environment-assets\authoring-preflight-01\receipt.json') -Raw | ConvertFrom-Json
$expectedRules = @($baseline.existingInboundAllowRules | Where-Object { $_.program -ieq $exe })
function Assert-ExistingNetworkPermission {
    $profiles = @(Get-NetConnectionProfile)
    if ($profiles.Count -ne $baseline.activeProfiles.Count) { throw 'Active network profile count changed.' }
    foreach ($profile in $profiles) {
        if (-not ($baseline.activeProfiles | Where-Object {
            $_.InterfaceAlias -eq $profile.InterfaceAlias -and $_.networkCategory -eq $profile.NetworkCategory.ToString()
        })) { throw 'Active network profile differs from the approved observation.' }
    }
    if ($expectedRules.Count -ne 2) { throw 'Expected exact existing Editor-Cmd TCP/UDP rules.' }
    $matchingRules = @(Get-NetFirewallApplicationFilter -Program $exe | Get-NetFirewallRule)
    if ($matchingRules.Count -ne $expectedRules.Count -or
        @($matchingRules | Where-Object Name -NotIn $expectedRules.name).Count) {
        throw 'The complete set of executable-specific authoring rules changed.'
    }
    foreach ($expected in $expectedRules) {
        $rule = Get-NetFirewallRule -Name $expected.name
        $app = $rule | Get-NetFirewallApplicationFilter
        $port = $rule | Get-NetFirewallPortFilter
        $address = $rule | Get-NetFirewallAddressFilter
        if ($app.Program -ine $exe -or $rule.Enabled.ToString() -ne 'True' -or
            $rule.Direction.ToString() -ne $expected.direction -or $rule.Action.ToString() -ne $expected.action -or
            $rule.Profile.ToString() -ne $expected.profile -or $port.Protocol.ToString() -ne $expected.protocol -or
            (@($port.LocalPort) -join ',') -ne ($expected.localPort -join ',') -or
            (@($port.RemotePort) -join ',') -ne ($expected.remotePort -join ',') -or
            (@($address.LocalAddress) -join ',') -ne ($expected.localAddress -join ',') -or
            (@($address.RemoteAddress) -join ',') -ne ($expected.remoteAddress -join ',')) {
            throw 'An existing authoring permission changed; do not click any security dialog.'
        }
    }
}
function Get-AuthoringProcesses {
    @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='UnrealEditor-Cmd.exe' OR Name='UnrealPak.exe' OR Name='ShaderCompileWorker.exe' OR Name='CrashReportClientEditor.exe' OR Name='CrashReportClient.exe' OR Name='UnrealTraceServer.exe' OR Name='zenserver.exe' OR Name='UnrealInsights.exe'" |
        Select-Object ProcessId,ParentProcessId,ExecutablePath,CreationDate)
}
Assert-ExistingNetworkPermission
if (@(Get-AuthoringProcesses).Count) { throw 'Another authoring process prevents the bounded shared-marker lock.' }
$protected = @(Get-Content -LiteralPath (Join-Path $root 'Assets\Environment\woodland-preparation-01\protected-before.json') -Raw | ConvertFrom-Json)
$before = @($protected | ForEach-Object {
    $hash = (Get-FileHash -LiteralPath (Join-Path $root $_.path)).Hash
    $selectedPreview=($assetMode -or $characterCook) -and $_.path -ceq 'Preview.json' -and
        $hash -ceq $(if($treeMode -or $treeCook){'4AC315EDC7CC6C75F11ECE46E0B0ED30E574D533A04BA432CFBEDDAA470DE116'}elseif($wardrobeMode -or $wardrobeCook){'C1262ACE336ABDEFD3A564046DD4F233A4AFDA19B9E6C2460886ACD263820904'}else{'737816A76EC8C0D8D96579FBDB30408FE4F4B81E52DF22614EC1138726F1DDFB'})
    if ($hash -cne $_.sha256 -and -not $selectedPreview -and -not(($assetMode -or $characterCook) -and $_.path -cin $acceptedBuild.sources.path) -and $_.path -notin @('SurvivalGame.uproject','Source\SurvivalGameEditor.Target.cs',
        'Scripts\Development-Run.ps1','Tests\DevelopmentRunTests.ps1',
        'Source\SurvivalGame\HomesteadWorld.cpp','Config\DefaultGame.ini')) {
        throw "Unrelated protected input changed:$($_.path)"
    }
    @{ path=$_.path;sha256=$hash }
})
$markerPath = 'C:\ProgramData\Epic\NotAllowedUnattendedBugReports'
$markerFile = Get-Item -LiteralPath $markerPath
if ($markerFile.Length -ne 0 -or ($markerFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Global marker metadata differs.' }
$configs = [ordered]@{}
foreach ($name in @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')) {
    $configs[$name] = Join-Path $output "Config\$name.ini"
}
$ddc = Join-Path $output 'DDC'
$ddcIdentity=$null
$retainedFiles=@();$ddcBefore=@()
if($Mode -in @('Render','Cook','HairImport','HairVerify','WardrobeImport','WardrobeVerify','TreeImport','TreeVerify')) {
    $ddc=Join-Path $evidenceRunRoot 'fern-import-02\DDC'
    if(-not $retainedRender){Assert-FernOrdinaryTree $ddc}
    $ddcIdentity=[Homestead.Authoring.LeafGuard]::InspectDirectory($ddc)
    $importSettings=Get-Content (Join-Path $evidenceRunRoot 'fern-import-02\effective-settings.json') -Raw|ConvertFrom-Json
    Assert-AuthoringDdc $importSettings.ddcStores $ddc
    if($retainedRender) {
        $ddcBefore=Get-FernMutableCacheObservation $ddc
        $retainedFiles=@(Get-FernRetainedFiles $evidenceRunRoot $ddc -MetadataOnly)
    }
}
$arguments=@(Get-FernProbeArguments (Join-Path $root 'SurvivalGame.uproject') $output $ddc $configs $Mode)
$environment = [Collections.Generic.Dictionary[string,string]]::new()
$environment['TEMP'] = Join-Path $output 'Temp'
$environment['TMP'] = Join-Path $output 'Temp'
$environment['UE_PYTHONPATH'] = $null
$environment['UE_PIPINSTALL_PATH'] = Join-Path $output 'PipMustRemainAbsent'
$environment['UE_SKIP_UBT_SDK_SETUP'] = '1'
$environment['UE-LocalDataCachePath'] = $ddc
$environment['HOMESTEAD_PROBE_OUTPUT'] = $output
$environment['HOMESTEAD_PROBE_COMPLETION_POLICY']=if($completionDriven){'until-complete'}else{'bounded'}
$environment['HOMESTEAD_PROBE_DEADLINE'] = if($completionDriven){$null}else{[DateTimeOffset]::UtcNow.AddSeconds($hardSeconds).ToString("yyyy-MM-ddTHH:mm:ss.fffZ")}
if(($Mode -eq 'Render' -and ('-nullrhi' -in $arguments -or '-AllowCommandletRendering' -notin $arguments -or '-RenderOffScreen' -notin $arguments)) -or
    ($Mode -ne 'Render' -and '-nullrhi' -notin $arguments) -or '-DisablePython' -notin $arguments -or
    '-noshaderworker' -notin $arguments -or $configs.Count -ne 7 -or
    ($Mode -ne 'Settings' -and "-FernMode=$Mode" -notin $arguments)){throw 'Mode-specific launch configuration differs.'}
if ($ValidateOnly) {
    [ordered]@{eligibleForGuardConstruction=$true;moduleSha256=$moduleHash;buildReceiptSha256=$buildReceiptHash
        supervisorReceiptSha256=$supervisorReceiptHash;creationFlags=0x0008040C;mode=$Mode;deadlineProfile=$deadlineProfile
        softMilliseconds=$softSeconds*1000;hardMilliseconds=$hardSeconds*1000;ceilingSeconds=$ceilingSeconds
        nativeAbsoluteDeadline=$environment['HOMESTEAD_PROBE_DEADLINE'];ddc=$ddc;arguments=$arguments
        startupSeconds=$operationPolicy.startupSeconds;captureSeconds=$operationPolicy.captureSeconds;ddcIdentity=$ddcIdentity
        retainedOldRunFiles=$retainedFiles.Count;retainedCacheFiles=$(if($retainedRender){$ddcBefore.fileCount}else{0})
        completionDriven=$completionDriven;treeDiagnostic=[bool]$TreeDiagnostic
        output=$output;globalMarkerReadLocked=$false;runtimeVerified=$false;attemptConsumed=$false}
    return
}
$null=New-Item -ItemType Directory -Path $output,(Join-Path $output 'Config'),(Join-Path $output 'Temp'),
    (Join-Path $output 'EngineUser'),(Join-Path $output 'DDC')
foreach($name in $configs.Keys){[IO.File]::WriteAllText($configs[$name],'')}
if($retainedRender) {
    @{oldRunRoot=$evidenceRunRoot;historicalFileMetadata=$retainedFiles;mutableCache=$ddc;cacheIdentity=$ddcIdentity;
        cacheBefore=$ddcBefore;permission='Only this exact retained candidate filesystem DDC is admitted old-run input/output.'} |
        ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'retained-inputs.json')
}
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$PID"
$null = Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$PID"
$guard = $null
$failure = $null
$samples = [Collections.Generic.List[object]]::new()
$jobSamples = [Collections.Generic.List[object]]::new()
$native = $null
$started = [DateTimeOffset]::UtcNow
$clock = [Diagnostics.Stopwatch]::StartNew()
$stopPath = Join-Path $output 'stop-probe.txt'
function Request-ProbeStop([string]$Reason) {
    if (-not (Test-Path -LiteralPath $stopPath)) { [IO.File]::WriteAllText($stopPath,$Reason) }
}
function Read-ProbeJob([string]$Phase, [switch]$Final) {
    $job = if ($Final) { $guard.CaptureExitedJob() } else { $guard.ObserveJobPolicy() }
    $members = @($guard.ObserveJobMembers())
    $heldRoot = $guard.ObserveHeldRoot()
    $jobSamples.Add(@{phase=$Phase;utc=[DateTimeOffset]::UtcNow.ToString('o')
        job=$job;members=$members;heldRoot=$heldRoot})
    Assert-AuthoringRootObservation $job $members $heldRoot $guard.ProcessId $guard.ImagePath $guard.ProcessCreationTime -Final:$Final
    return $heldRoot.Exited
}
try {
    $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
    if (-not $state.allowWork -or $state.id -ne $run.id -or @(Get-AuthoringProcesses).Count -or
        (($completionDriven -or $hairMode) -and $state.completionPolicy -cne 'until-complete') -or
        ($state.completionPolicy -cne 'until-complete' -and [DateTimeOffset]::UtcNow.AddSeconds($ceilingSeconds+60) -ge [DateTimeOffset]$state.deadlineUtc)) { throw 'Admission changed before guard construction.' }
    Assert-ExistingNetworkPermission
    Assert-AcceptedNativeProducts
    Assert-NamedSupersession
    if($retainedRender) {
        Assert-FernDirectoryIdentity $ddcIdentity $ddc
        if((@(Get-FernRetainedFiles $evidenceRunRoot $ddc -MetadataOnly)|ConvertTo-Json -Depth 5 -Compress) -cne
            ($retainedFiles|ConvertTo-Json -Depth 5 -Compress)){throw 'Old-run immutable files changed before launch.'}
    }
    [Homestead.Authoring.LeafGuard]::ValidateDeadlineProfile($softSeconds*1000,$hardSeconds*1000,$deadlineProfile)
    $reservation = [IO.File]::Open($attempt,'CreateNew','Write','Read')
    try {
        $bytes=[Text.Encoding]::UTF8.GetBytes((@{utc=$started.ToString('o');output=$output;runId=$run.id;supersession=$supersession
            mode=$Mode;supervisorReceiptSha256=$supervisorReceiptHash
            priorFernReservationSha256='3D799DBE1E344B703B468A7D5016E082C3271700A6C2E50D073EDD3BBC1508C2'
            priorFernResultSha256='6D6F56576EE6ECCABF0918B99D6168E7C410213387ED11EADE11FB00A49D3910'
            approval=$(if($assetMode){$supersession.currentApproval}elseif($completionDriven){'Explicit21:19 until-complete continuation; preserve bounded capture-timeout; no synthetic deadline; manual stop and genuine failures remain.'}elseif($longStartup){'Explicit fresh90-minute run20260921-033354-2d257ba0; one longer-startup render; old import02 DDC admitted input/output only; no retry.'}else{'Coordinator explicit replacement import02; original failure immutable; conditional render reads import02.'})}|ConvertTo-Json))
        $reservation.Write($bytes)
    } finally { $reservation.Dispose() }
    if($Mode -in @('WardrobeImport','TreeImport')) {
        Assert-FernDirectoryIdentity $trialIdentity $trial
        if(@(Get-ChildItem $trial -Force).Count){throw 'Prior asset scope changed before reservation.'}
    } elseif($Mode -in @('Import','HairImport')) {
        if(Test-Path -LiteralPath $trial){throw 'Trial appeared before reservation.'}
        $null=New-Item -ItemType Directory -Path $trial
        $trialIdentity=[Homestead.Authoring.LeafGuard]::InspectDirectory($trial)
    }
    $environment['HOMESTEAD_PROBE_DEADLINE']=if($completionDriven){$null}else{[DateTimeOffset]::UtcNow.AddSeconds($hardSeconds).ToString("yyyy-MM-ddTHH:mm:ss.fffZ")}
    $guard = [Homestead.Authoring.LeafGuard]::new($exe,$approved['UnrealEditor-Cmd.exe'],[string[]]$arguments,
        $root,$markerPath,(Join-Path $output 'stdout.log'),$environment,$true)
    if ($guard.CreationFlags -ne 0x0008040C -or $guard.WhitelistedHandleCount -ne 3) {
        throw 'Approved Editor detached-console/stdio creation policy differs.'
    }
    $guard.ArmDeadline($softSeconds*1000,$hardSeconds*1000,$stopPath,$deadlineProfile)
    if($guard.DeadlineProfile -cne $deadlineProfile -or $guard.SoftDeadlineMilliseconds -ne $softSeconds*1000 -or
        $guard.HardDeadlineMilliseconds -ne $hardSeconds*1000){throw 'Armed production deadline profile differs.'}
    $null = Read-ProbeJob 'suspended-before-resume'
    [ordered]@{pid=$guard.ProcessId;creationTime=$guard.ProcessCreationTime;image=$guard.ImagePath
        executableSha256=$approved['UnrealEditor-Cmd.exe'];moduleSha256=$moduleHash;arguments=$arguments
        markerBefore=$guard.MarkerBefore;job=$guard.LastVerifiedJob;explicitInheritedHandles=$guard.WhitelistedHandleCount
        expectedRules=$expectedRules;deadlineUtc=$environment['HOMESTEAD_PROBE_DEADLINE']
        creationFlags=$guard.CreationFlags;buildReceiptSha256=$buildReceiptHash;productPins=$productPins;supersession=$supersession
        buildMonitoringQualification=$acceptedBuild.qualification
        mode=$Mode;softSeconds=$softSeconds;hardSeconds=$hardSeconds;ceilingSeconds=$ceilingSeconds;trialIdentity=$trialIdentity
        deadlineProfile=$guard.DeadlineProfile;supervisorReceiptSha256=$supervisorReceiptHash;wrapperSha256=$wrapperHash
        startupSeconds=$operationPolicy.startupSeconds;captureSeconds=$operationPolicy.captureSeconds;ddc=$ddc;ddcIdentity=$ddcIdentity
        completionDriven=$completionDriven
    } | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $output 'launch.json')
    $guard.Resume()
    $readyAt = $null
    $lastPermission = 0.0
    while (-not $guard.Wait(0)) {
        Assert-FernStageBudget $operationPolicy $clock.Elapsed.TotalSeconds $readyAt
        if (Read-ProbeJob 'live') { break }
        $null = $guard.VerifyMarker()
        $children = @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$($guard.ProcessId)" |
            Select-Object ProcessId,ParentProcessId,ExecutablePath,CreationDate)
        $other = @(Get-AuthoringProcesses | Where-Object ProcessId -NE $guard.ProcessId)
        $tcp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetTCPConnection -Filter "OwningProcess=$($guard.ProcessId)" |
            Select-Object OwningProcess,LocalAddress,LocalPort,RemoteAddress,RemotePort,State)
        $udp = @(Get-CimInstance -Namespace root\StandardCimv2 -ClassName MSFT_NetUDPEndpoint -Filter "OwningProcess=$($guard.ProcessId)" |
            Select-Object OwningProcess,LocalAddress,LocalPort)
        $samples.Add(@{elapsedMs=$clock.Elapsed.TotalMilliseconds;utc=[DateTimeOffset]::UtcNow.ToString('o');tcp=$tcp;udp=$udp;children=$children;otherAuthoring=$other})
        Assert-AuthoringEndpoints $tcp $udp $guard.ProcessId
        if ($children.Count -or $other.Count) { throw 'Executed child or competing authoring process observed.' }
        $state = & (Join-Path $PSScriptRoot 'Development-Run.ps1') -Action Status
        if (-not $state.allowWork -or $state.id -ne $run.id -or
            (($completionDriven -or $hairMode) -and $state.completionPolicy -cne 'until-complete')) { throw 'Run pause/stop/deadline/policy change observed.' }
        if ($clock.Elapsed.TotalSeconds - $lastPermission -gt 3) {
            Assert-ExistingNetworkPermission
            $lastPermission = $clock.Elapsed.TotalSeconds
        }
        if (-not $native -and (Test-Path -LiteralPath (Join-Path $output 'effective-settings.json'))) {
            $native = Get-Content -LiteralPath (Join-Path $output 'effective-settings.json') -Raw | ConvertFrom-Json
            if (-not $native.valid -or $native.pid -ne $guard.ProcessId -or
                [uint64]$native.processCreationTime -ne $guard.ProcessCreationTime -or
                [IO.Path]::GetFullPath($native.executable) -ine $guard.ImagePath -or
                $native.marker.volume -ne $guard.MarkerBefore.Volume -or $native.marker.indexHigh -ne $guard.MarkerBefore.IndexHigh -or
                $native.marker.indexLow -ne $guard.MarkerBefore.IndexLow -or
                $native.cachedSendReports -or $native.cachedSendUsage -or
                $native.jobFlags -ne 8200 -or $native.jobProcessLimit -ne 1 -or $native.jobActiveProcesses -ne 1) {
                throw 'Actual native identity/config/privacy/guard evidence failed.'
            }
            Assert-AuthoringDdc $native.ddcStores $ddc
            Assert-AuthoringConfigBranches $native.configBranches $output
            Assert-AuthoringPythonState $native.pythonEntry
            if($completionDriven -and -not $native.completionDriven){throw 'Native completion policy differs.'}
            $priorPlugins = (Get-Content (Join-Path $evidenceRunRoot 'native-settings-02\effective-settings.json') -Raw | ConvertFrom-Json).enabledPlugins
            if (@($native.enabledPlugins | Where-Object { $_ -cnotin $priorPlugins }).Count) {
                throw 'An additional unreviewed plugin became enabled.'
            }
            $readyAt = $clock.Elapsed.TotalSeconds
            Assert-FernStageBudget $operationPolicy $readyAt $readyAt
            if($longStartup){$guard.ConstrainCaptureDeadline()}
            @{marker='effective-settings.json';observedElapsedSeconds=$readyAt;observedUtc=[DateTimeOffset]::UtcNow.ToString('o');
                captureTimerArmed=$guard.CaptureDeadlineArmed;softMilliseconds=$guard.CaptureSoftMilliseconds;
                hardMilliseconds=$guard.CaptureHardMilliseconds;limit='Native Main/settings export, not first instruction; observation includes sampling delay.'} |
                ConvertTo-Json | Set-Content (Join-Path $output 'entry-observed.json')
            if($Mode -ne 'Settings') {
                Assert-FernDirectoryIdentity $trialIdentity $trial
                [IO.File]::WriteAllText((Join-Path $output 'operation-admitted.txt'),$Mode)
            }
        }
        if($Mode -eq 'Cook' -and -not $fernResult -and (Test-Path (Join-Path $output 'cook-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'cook-result.json') -Raw|ConvertFrom-Json
            $additional=if($characterCook){@(Get-HairWavePackageStems|ForEach-Object {"Content\Trials\HeroineWave_20260921_01\$_.uasset"})}else{@()}
            if($wardrobeCook){$additional+=@(Get-WardrobePackageStems|ForEach-Object {"Content\SurvivalGame\Characters\ModularClothing\$_.uasset"})}
            if($treeCook){$additional+=@(Get-TreePackageStems|ForEach-Object {"Content\Trials\TreeSmall02_20260921_01\$_.uasset"})}
            Assert-HomesteadCookOutput $fernResult $output -AdditionalPackages $additional
        }
        if($hairMode -and -not $fernResult -and (Test-Path (Join-Path $output 'hair-wave-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'hair-wave-result.json') -Raw|ConvertFrom-Json
            Assert-HairWaveNativeInventory $fernResult
            $null=Get-ProbePackageFiles
        }
        if($wardrobeMode -and -not $fernResult -and (Test-Path (Join-Path $output 'wardrobe-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'wardrobe-result.json') -Raw|ConvertFrom-Json
            Assert-WardrobeNativeInventory $fernResult $root
            $null=Get-ProbePackageFiles
        }
        if($treeMode -and -not $fernResult -and (Test-Path (Join-Path $output 'tree-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'tree-result.json') -Raw|ConvertFrom-Json
            if(-not $fernResult.passed){
                $detail=if($fernResult.PSObject.Properties['failure']){$fernResult.failure}else{'See native evidence.'}
                throw "Native tree operation failed at $($fernResult.stage): $detail"
            }
            Assert-TreeNativeInventory $fernResult -KnownTangentDiagnostic:$TreeDiagnostic
            $null=Get-ProbePackageFiles
        }
        if($Mode -in @('Import','Render') -and -not $fernResult -and (Test-Path (Join-Path $output 'fern-result.json'))) {
            $fernResult=Get-Content (Join-Path $output 'fern-result.json') -Raw|ConvertFrom-Json
            if(-not $fernResult.passed) {
                $detail=if($fernResult.PSObject.Properties['failure']){$fernResult.failure}else{'See native log and readiness evidence.'}
                throw "Native fern operation failed at $($fernResult.stage): $detail"
            }
            Assert-FernNativeInventory $fernResult $fernSourceInventory
            if($Mode -eq 'Import'){$null=Get-FernPackageFiles $trial -Complete}
            else{Assert-FernRenderImages $fernResult $output}
        }
        if ($null -ne $readyAt -and (($Mode -eq 'Settings' -and $clock.Elapsed.TotalSeconds - $readyAt -ge 10 -and $samples.Count -ge 10) -or
            ($Mode -ne 'Settings' -and $fernResult))) {
            Request-ProbeStop 'complete'
        }
        Assert-FernStageBudget $operationPolicy $clock.Elapsed.TotalSeconds $readyAt
        Start-Sleep -Milliseconds 100
    }
    if (-not $native -or $guard.ExitCode -ne 0 -or $guard.HardTerminated -or $guard.DeadlineError) { throw 'Native settings probe failed or was hard-terminated.' }
    $exit = Get-Content -LiteralPath (Join-Path $output 'native-exit.json') -Raw | ConvertFrom-Json
    if (-not $exit.passed -or -not $exit.cooperative -or $exit.stopReason.Trim() -ne 'complete') { throw 'Native cooperative stop was not proved.' }
    Assert-AuthoringPythonState $exit.pythonExit
    if (Test-Path -LiteralPath $environment['UE_PIPINSTALL_PATH']) { throw 'Disabled Python unexpectedly touched the pip output.' }
    if($Mode -ne 'Settings' -and -not $fernResult){throw 'Native fern operation has no verified inventory.'}
} catch {
    $failure = $_.ToString()
} finally {
    $cleanupErrors = [Collections.Generic.List[string]]::new()
    $markerBefore=$null;$markerAfter=$null;$released=$null;$code=$null;$hard=$false;$watchdog=$null
    $subjectExited=$null;$guardDisposed=$false
    if ($guard) {
        $markerBefore = $guard.MarkerBefore
        try {
            if (-not $guard.Wait(0)) {
                try { Request-ProbeStop 'cancelled' } catch { $cleanupErrors.Add("Stop request failed:$_") }
                if (-not $guard.Wait(3000)) { $guard.HardStop(96) }
            }
        } catch {
            $cleanupErrors.Add("Owned-process cleanup failed:$_")
            try { $guard.HardStop(97) } catch { $cleanupErrors.Add("Owned-job fallback failed:$_") }
        }
        try { $subjectExited=$guard.Wait(5000) } catch { $cleanupErrors.Add("Exit observation failed:$_") }
        if ($subjectExited) {
            try { $null = Read-ProbeJob 'after-observed-death' -Final } catch { $cleanupErrors.Add("Final job evidence failed:$_") }
            try { $markerAfter=$guard.VerifyMarker() } catch { $cleanupErrors.Add("Guarded marker verification failed:$_") }
            try {
                $code=$guard.ExitCode;$hard=$guard.HardTerminated
                $watchdog=@{requested=$guard.DeadlineStopRequested;hardStop=$guard.DeadlineHardStop;error=$guard.DeadlineError}
            } catch { $cleanupErrors.Add("Exit evidence failed:$_") }
            try { $guard.Dispose();$guardDisposed=$true } catch { $cleanupErrors.Add("Guard disposal failed:$_") }
            try {
                $released=[Homestead.Authoring.LeafGuard]::InspectMarker($markerPath)
                if (($released|ConvertTo-Json -Compress) -cne ($markerBefore|ConvertTo-Json -Compress)) {
                    throw 'Post-release marker identity/metadata/ACL differs.'
                }
            } catch { $cleanupErrors.Add("Released marker verification failed:$_") }
        } else {
            $cleanupErrors.Add('Subject death was not observed; guard was not explicitly released.')
        }
    }
    foreach ($item in $before) {
        try {
            if ((Get-FileHash -LiteralPath (Join-Path $root $item.path)).Hash -cne $item.sha256) { throw "Protected file changed:$($item.path)" }
        } catch { $cleanupErrors.Add($_.ToString()) }
    }
    try { Assert-AcceptedNativeProducts } catch { $cleanupErrors.Add("Accepted build changed:$_") }
    try { Assert-NamedSupersession } catch { $cleanupErrors.Add("Original failed attempt changed:$_") }
    if($TreeDiagnostic){
        try{$null=Get-TreeFailedImportDiagnosticAdmission $root}catch{$cleanupErrors.Add("Diagnostic tree pins changed:$_")}
    }
    if($ddcIdentity) {
        try{Assert-FernDirectoryIdentity $ddcIdentity $ddc}catch{$cleanupErrors.Add("Retained cache identity changed:$_")}
    }
    if($retainedRender) {
        try {
            if((@(Get-FernRetainedFiles $evidenceRunRoot $ddc -MetadataOnly)|ConvertTo-Json -Depth 5 -Compress) -cne
                ($retainedFiles|ConvertTo-Json -Depth 5 -Compress)){throw 'Old-run metadata outside the admitted cache changed.'}
            @{cacheAfter=(Get-FernMutableCacheObservation $ddc);unchangedOldRunFileMetadata=$retainedFiles.Count;
                qualification='Historical metadata comparison is not byte-identity proof. Named historical admission receipts, actual source/tools/products and protected Content remain hash checked.'} |
                ConvertTo-Json -Depth 8 | Set-Content (Join-Path $output 'retained-output-check.json')
        } catch {$cleanupErrors.Add("Retained old-run protection failed:$_")}
    }
    if($Mode -ne 'Settings') {
        try {
            foreach($inputFile in $fernInputs) {
                $inputPath=if($assetMode){$inputFile.path}else{Join-Path $sourceRoot $inputFile.file}
                if((Get-FileHash $inputPath).Hash -cne $inputFile.sha256){throw 'Original source input changed during operation.'}
            }
            $contentAfter=@(Get-ChildItem (Join-Path $root 'Content') -Recurse -File -Force |
                Where-Object {-not $_.FullName.StartsWith($trial+'\',[StringComparison]::OrdinalIgnoreCase)})
            if($contentAfter.Count -ne $contentBefore.Count){throw 'Unexpected Content output outside the exact trial.'}
            foreach($file in $contentBefore){if((Get-FileHash $file.path).Hash -cne $file.sha256){throw "Existing content changed:$($file.path)"}}
            if($trialIdentity) {
                Assert-FernDirectoryIdentity $trialIdentity $trial
                if($Mode -in @('Render','Cook','HairVerify','WardrobeVerify','TreeVerify')){Assert-FernPackagePins $trialBefore @(Get-ProbePackageFiles)}
            }
        } catch {$cleanupErrors.Add("Fern protection check failed:$_")}
    }
    if ($cleanupErrors.Count) { $failure=(@($failure)+@($cleanupErrors) | Where-Object { $_ }) -join ' | ' }
    if($Mode -in @('Import','HairImport','WardrobeImport','TreeImport') -and $trialIdentity) {
        try {
            Assert-FernDirectoryIdentity $trialIdentity $trial
            if($failure -and ($subjectExited -or -not $guard)) {
                $partial=@(Get-ChildItem $trial -Recurse -File -Force|ForEach-Object {
                    @{path=[IO.Path]::GetRelativePath($trial,$_.FullName);bytes=$_.Length;sha256=(Get-FileHash $_.FullName).Hash}
                })
                ConvertTo-Json -InputObject @($partial) -Depth 5|Set-Content (Join-Path $output 'partial-content.json')
                if($Mode -eq 'Import') {
                    [IO.Directory]::Move($trial,(Join-Path $output 'discarded-content'))
                    Assert-FernDirectoryIdentity $trialIdentity (Join-Path $output 'discarded-content')
                }
            } elseif(-not $failure) {
                @{inventory=$fernResult;directoryIdentity=$trialIdentity;packages=@(Get-ProbePackageFiles);
                    sourceInputs=$fernInputs;sourceGraph=$fernSourceInventory}|ConvertTo-Json -Depth 20|Set-Content (Join-Path $output 'asset-admission.json')
            }
        } catch {$failure=(@($failure,"Fern disposition failed:$_")|Where-Object {$_}) -join ' | '}
    }
    if($Mode -eq 'Render' -and $failure -and ($subjectExited -or -not $guard)) {
        try {
            foreach($name in @('fern-a-front.png','fern-a-back.png','fern-a-front.png.tmp','fern-a-back.png.tmp',
                'diagnostic-front-failed.png.tmp','diagnostic-back-failed.png.tmp')) {
                $image=Join-Path $output $name
                if(Test-Path -LiteralPath $image) {
                    Assert-FernOrdinaryTree $output
                    $discard=Join-Path $output 'discarded-images'
                    if(-not(Test-Path $discard)){$null=New-Item -ItemType Directory -Path $discard}
                    [IO.File]::Move($image,(Join-Path $discard $name))
                }
            }
        } catch {$failure=(@($failure,"Partial image quarantine failed:$_")|Where-Object {$_}) -join ' | '}
    }
    $gaps = @()
    $previous = 0.0
    foreach ($sample in $samples) { $gaps += $sample.elapsedMs-$previous; $previous=$sample.elapsedMs }
    $gaps += $clock.Elapsed.TotalMilliseconds-$previous
    [ordered]@{status=$(if($failure){'failed'}elseif($TreeDiagnostic){'passed-diagnostic-only'}else{'passed'});error=$failure;cleanupErrors=$cleanupErrors
        treeDiagnostic=[bool]$TreeDiagnostic
        diagnosticQualification=$(if($TreeDiagnostic){$supersession.diagnosticApproval}else{$null})
        subjectExited=$subjectExited;guardDisposed=$guardDisposed;exitCode=$code;hardTerminated=$hard;watchdog=$watchdog
        elapsedSeconds=$clock.Elapsed.TotalSeconds;samples=$samples;maximumSampleGapMs=($gaps|Measure-Object -Maximum).Maximum
        markerBefore=$markerBefore;markerAfter=$markerAfter;markerAfterRelease=$released
        native=$native;protectedFiles=$before;executableSha256=$approved['UnrealEditor-Cmd.exe'];moduleSha256=$moduleHash
        jobSamples=$jobSamples;buildReceiptSha256=$buildReceiptHash;productPins=$productPins;supersession=$supersession
        buildMonitoringQualification=$acceptedBuild.qualification;approvedCreationFlags=0x0008040C
        mode=$Mode;fernInventory=$fernResult;trialIdentity=$trialIdentity;softSeconds=$softSeconds;hardSeconds=$hardSeconds
        supervisorReceiptSha256=$supervisorReceiptHash;wrapperSha256=$wrapperHash
        operationPolicy=$operationPolicy;ddc=$ddc;ddcIdentity=$ddcIdentity
        limits='One specifically reserved settings/import/offscreen-render operation. Sampled endpoints/processes, not continuous tracing or filesystem/network isolation. No cook/Pak/Shipping/4K/performance proof. Never click security Allow.'
    } | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $output 'probe-result.json')
}
if ($failure) { throw $failure }
if($TreeDiagnostic){"Qualified tree diagnostic completed (not clean acceptance):$output"}
else{"Native settings/stop probe passed:$output"}
