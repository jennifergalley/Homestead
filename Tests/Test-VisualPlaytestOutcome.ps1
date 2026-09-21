$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
$tokens=$null;$errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'Scripts\Playtest-Visual.ps1'),[ref]$tokens,[ref]$errors)
if($errors.Count){throw 'Visual wrapper syntax is invalid.'}
$function=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and
    $node.Name -ceq 'Assert-VisualPlaytestOutcome'},$false)
if(-not $function){throw 'Production outcome assertion is missing.'}
. ([scriptblock]::Create($function.Extent.Text))
$forage='Forage target reached=1; resources actually gathered=1'
$tree='Tree ready=1; ordinary approach=1; actual trunk blocked walking=1; ordinary retreat=1'
Assert-VisualPlaytestOutcome $forage $forage
Assert-VisualPlaytestOutcome "$forage`n$tree" $forage -RequireTree
$bad=@($forage,"$forage`n$tree`n$tree","$forage`n$tree`nFAILED readiness", $tree)
foreach($token in @('Tree ready=1','ordinary approach=1','actual trunk blocked walking=1','ordinary retreat=1')){
    $bad+="$forage`n"+$tree.Replace($token,$token.Replace('=1','=0'))
}
foreach($outcome in $bad){
    $rejected=$false
    try{Assert-VisualPlaytestOutcome $outcome $forage -RequireTree}catch{$rejected=$true}
    if(-not $rejected){throw "Invalid route outcome accepted:$outcome"}
}
foreach($case in @('01','02')){
    $actual=Get-Content (Join-Path $root "docs\research\environment-assets\tree-diagnostic-$case\ordinary.observations.txt") -Raw
    $rejected=$false
    try{Assert-VisualPlaytestOutcome $actual $forage}catch{$rejected=$true}
    if(-not $rejected){throw "Actual failed tree contact capture was accepted:$case"}
}
$actual=Get-Content (Join-Path $root 'docs\research\environment-assets\tree-diagnostic-03\ordinary.observations.txt') -Raw
Assert-VisualPlaytestOutcome $actual $forage -RequireTree
'PASS: two synthetic positives, eight negatives, two real failed scenarios rejected and corrected real scenario accepted; no game launched.'
