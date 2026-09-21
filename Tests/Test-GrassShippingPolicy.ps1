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
$legacy=& $leaf -OutputDirectory 'unused' -ShippingActions -TreeDiagnosticCandidate -GroveCandidate -GroveProxyCorrection
if($legacy.build -cne 'grove-shipping-build-03' -or $legacy.link -ne 1 -or
    ($legacy.compiles -join ',') -cne '-1,0'){throw 'Legacy grove selector changed'}
$rejected=0
foreach($flags in @(
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
