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
$tree = @(@{index=0;parent=-1;childCount=1;type='Async';name='';local=$true},
    @{index=1;parent=0;childCount=1;type='';name='';local=$false},
    @{index=2;parent=1;childCount=0;type='File System';name=$cache;local=$true})
Assert-AuthoringDdc $tree $cache
$count++
foreach ($change in @(@{type='Zen'},@{local=$false},@{name='E:\Outside'},@{type='Memory';name=''},
    @{type='';name=''},@{parent=2},@{index=4},@{childCount=1},@{name='relative'})) {
    $stores=@($tree | ForEach-Object {$_.Clone()})
    foreach($key in $change.Keys){$stores[2][$key]=$change[$key]}
    Expect-Rejection { Assert-AuthoringDdc $stores $cache }
    $count++
}
foreach($change in @(@{name='unexpected'},@{local=$false},@{childCount=0},@{type='HTTP'})) {
    $stores=@($tree | ForEach-Object {$_.Clone()})
    foreach($key in $change.Keys){$stores[0][$key]=$change[$key]}
    Expect-Rejection { Assert-AuthoringDdc $stores $cache }
    $count++
}
$output='E:\DisposableProbe'
$branches=@('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input') |
    ForEach-Object {@{name=$_;logicalKey=$_;found=$true;destination="$output\Config\$_.ini"}}
Assert-AuthoringConfigBranches $branches $output
foreach($change in @(@{found=$false},@{destination='E:\Outside\Engine.ini'},@{destination='Engine'},
    @{name='Unknown'},@{logicalKey=''})) {
    $bad=@($branches | ForEach-Object {$_.Clone()})
    foreach($key in $change.Keys){$bad[0][$key]=$change[$key]}
    Expect-Rejection { Assert-AuthoringConfigBranches $bad $output }
}
Expect-Rejection { Assert-AuthoringConfigBranches @($branches | Select-Object -Skip 1) $output }
$python=@{valid=$true;librariesEnumerated=$true;moduleLoaded=$true;configured=$true;available=$false;initialized=$false
    runtimeLibraryLoaded=$true;libraries=@(@{path='E:\python311.dll';queryAvailable=$true;interpreterInitialized=0})}
Assert-AuthoringPythonState $python
Assert-AuthoringPythonState @{valid=$true;librariesEnumerated=$true;moduleLoaded=$false;configured=$false;
    available=$false;initialized=$false;runtimeLibraryLoaded=$false;libraries=@()}
foreach($change in @(@{available=$true},@{initialized=$true},@{configured=$false},@{librariesEnumerated=$false},
    @{runtimeLibraryLoaded=$false},@{libraries=@(@{path='E:\python311.dll';queryAvailable=$false;interpreterInitialized=-1})},
    @{libraries=@(@{path='E:\python311.dll';queryAvailable=$true;interpreterInitialized=1})},
    @{libraries=@(@{path='E:\python999.dll';queryAvailable=$true;interpreterInitialized=0})})) {
    $bad=$python.Clone()
    foreach($key in $change.Keys){$bad[$key]=$change[$key]}
    Expect-Rejection { Assert-AuthoringPythonState $bad }
}
'Config: one positive/six negatives; Python: two positives/eight negatives passed.'
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
