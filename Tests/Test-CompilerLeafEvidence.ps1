$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $root 'Scripts\CompilerLeafEvidence.ps1')
$evidence=Join-Path $root 'docs\research\environment-assets\compiler-leaf-01'
$result=Get-Content (Join-Path $evidence 'member-diagnostic-02.json') -Raw|ConvertFrom-Json
$case=$result.cases[0]
$samples=@($case.outcome.liveJobMembers|ForEach-Object{[pscustomobject]@{Error=$null;Members=$_.members}})
Assert-CompilerLeafAccounting $case.outcome.exitedJob $samples $case.launch.pid $case.launch.image
$retired=[pscustomobject]@{Error=$null;Members=@([pscustomobject]@{
    Pid=$case.launch.pid;Error='Process disappeared after enumeration';NativeError=87
})}
Assert-CompilerLeafAccounting $case.outcome.exitedJob (@($samples)+@($retired)) $case.launch.pid $case.launch.image
$retired.Members[0].Pid=[uint32]999999
$rejected=$false
try { Assert-CompilerLeafAccounting $case.outcome.exitedJob (@($samples)+@($retired)) $case.launch.pid $case.launch.image } catch { $rejected=$true }
if(-not $rejected){throw 'An unidentified vanished member was accepted.'}
foreach($invalid in @('unknown-image','missing-member','changed-creation','bad-policy')) {
    $copy=$samples|ConvertTo-Json -Depth 8|ConvertFrom-Json
    $job=$case.outcome.exitedJob|ConvertTo-Json|ConvertFrom-Json
    switch($invalid) {
        'unknown-image' { $copy[0].Members[-1].Image='C:\unexpected.exe' }
        'missing-member' { $copy=@([pscustomobject]@{Error=$null;Members=@($copy[0].Members[0])}) }
        'changed-creation' { $copy[-1].Members[0].CreationTime=1 }
        'bad-policy' { $job.ProcessLimit=2 }
    }
    $rejected=$false
    try { Assert-CompilerLeafAccounting $job $copy $case.launch.pid $case.launch.image } catch { $rejected=$true }
    if(-not $rejected) { throw "Invalid job evidence accepted:$invalid" }
}
$coff=Read-CompilerLeafCoff ([IO.File]::ReadAllBytes((Join-Path $evidence 'fixture.coff')))
if($coff.bytes -ne 1818 -or $coff.function.machineCode -cne 'B8DF9B5713C3') { throw 'Recorded COFF proof differs.' }
'Three actual-data positive checks and five negative policy cases passed; no tool/process launched.'
