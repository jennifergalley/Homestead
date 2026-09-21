$ErrorActionPreference = 'Stop'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'Scripts\AuthoringProbePolicy.ps1')
$count = 0
function Expect-Rejection([scriptblock]$Action) {
    $failed = $false
    try { & $Action } catch { $failed = $true }
    if (-not $failed) { throw 'Invalid policy fixture was admitted.' }
}
foreach ($port in @(1985,32768,40959)) {
    $endpoint = @{ OwningProcess=42;State=2;LocalAddress='0.0.0.0';RemoteAddress='0.0.0.0';RemotePort=0;LocalPort=$port }
    Assert-AuthoringEndpoints @($endpoint) @() 42
    $count++
}
Assert-AuthoringEndpoints @() @() 42
$count++
foreach ($change in @(@{LocalPort=32767},@{LocalPort=40960},@{State=5},@{LocalAddress='::'},
    @{RemoteAddress='127.0.0.1'},@{RemotePort=1234},@{OwningProcess=43})) {
    $endpoint = @{ OwningProcess=42;State=2;LocalAddress='0.0.0.0';RemoteAddress='0.0.0.0';RemotePort=0;LocalPort=1985 }
    foreach ($key in $change.Keys) { $endpoint[$key]=$change[$key] }
    Expect-Rejection { Assert-AuthoringEndpoints @($endpoint) @() 42 }
    $count++
}
Expect-Rejection { Assert-AuthoringEndpoints @() @(@{LocalPort=1}) 42 }
Expect-Rejection { Assert-AuthoringEndpoints @(@{},@{}) @() 42 }
$count += 2
$cache = 'E:\DisposableProbe\DDC'
Assert-AuthoringDdc @(@{type='';name='';local=$false},@{type='Memory';name='';local=$true},
    @{type='File System';name=$cache;local=$true}) $cache
$count++
foreach ($stores in @(
    ,@(@{type='Zen';name=$cache;local=$true}),
    ,@(@{type='File System';name=$cache;local=$false}),
    ,@(@{type='File System';name='E:\Outside';local=$true}),
    ,@(@{type='Memory';name='';local=$true})
)) {
    Expect-Rejection { Assert-AuthoringDdc $stores $cache }
    $count++
}
"$count authoring policy cases passed; no process/network/file mutation."
Set-StrictMode -Version Latest
function Empty-ProcessFixture { @() }
if (@(Empty-ProcessFixture).Count -ne 0 -or @([pscustomobject]@{Pid=42}).Count -ne 1) {
    throw 'Process-list array normalization failed.'
}
$job=@{Flags=8200;ProcessLimit=1;TotalProcesses=1;ActiveProcesses=1;HeldProcessIsMember=$true}
$held=@{Error=$null;Pid=42;Image='E:\fixture.exe';CreationTime=123;IdentityFromHeldRoot=$true;Member=$true;Exited=$false}
Assert-AuthoringRootObservation $job @($held) $held 42 'E:\fixture.exe' 123
$held.Exited=$true
Assert-AuthoringRootObservation $job @() $held 42 'E:\fixture.exe' 123
$job.ActiveProcesses=0
Assert-AuthoringRootObservation $job @() $held 42 'E:\fixture.exe' 123 -Final
foreach ($invalid in @('extra-total','unknown-member','member-error','changed-creation','missing-live-root','final-live')) {
    $badJob=$job.Clone();$badRoot=$held.Clone();$members=@()
    switch ($invalid) {
        'extra-total' {$badJob.TotalProcesses=2}
        'unknown-member' {$other=$held.Clone();$other.Pid=43;$members=@($other)}
        'member-error' {$other=$held.Clone();$other.Error='Access denied';$members=@($other)}
        'changed-creation' {$badRoot.CreationTime=124}
        'missing-live-root' {$badRoot.Exited=$false}
        'final-live' {$badJob.ActiveProcesses=1}
    }
    Expect-Rejection { Assert-AuthoringRootObservation $badJob $members $badRoot 42 'E:\fixture.exe' 123 -Final }
}
'Empty/single enumeration, three lifecycle observations and six invalid job/exit cases passed.'
