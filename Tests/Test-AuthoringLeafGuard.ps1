[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Root,
    [Parameter(Mandatory)][string]$Subject,
    [ValidateSet('Observer','Controller')][string]$Role = 'Observer',
    [ValidateSet('normal','pause','deadline','timeout','controller-failure','watchdog')][string]$Case = 'normal',
    [switch]$ObserveAccountingOnly,
    [switch]$DetachedConsole,
    [ValidateSet('normal','pause','deadline','timeout','controller-failure','watchdog')]
    [string[]]$Scenarios = @('normal','pause','deadline','timeout','controller-failure','watchdog')
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
function Write-NewJson([string]$Path, $Value) {
    $stream = [IO.File]::Open($Path, 'CreateNew', 'Write', 'Read')
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes(($Value | ConvertTo-Json -Depth 12))
        $stream.Write($bytes)
    } finally { $stream.Dispose() }
}
function Assert([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Assert-WriteAllowed([string]$Path) {
    $handle = [IO.File]::Open($Path, 'Open', 'Write', 'ReadWrite')
    $handle.Dispose()
}
function Get-DeniedOperation([string]$Path, [string]$Operation) {
    try {
        switch ($Operation) {
            'write' { Assert-WriteAllowed $Path }
            'delete' { [IO.File]::Delete($Path) }
            'rename' { [IO.File]::Move($Path, "$Path.renamed") }
        }
    } catch [IO.IOException] {
        $code = $_.Exception.HResult -band 0xffff
        if ($code -eq 32) { return $true }
        throw
    }
    return $false
}
function Update-FixtureState([string]$Directory, $Value) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $lock = $null
    while (-not $lock) {
        try { $lock = [IO.File]::Open((Join-Path $Directory 'run.lock'), 'OpenOrCreate', 'ReadWrite', 'None') }
        catch [IO.IOException] {
            if (($_.Exception.HResult -band 0xffff) -ne 32 -or $timer.Elapsed.TotalSeconds -ge 1) { throw }
            Start-Sleep -Milliseconds 5
        }
    }
    try {
        [IO.File]::WriteAllText((Join-Path $Directory 'run.json.tmp'), ($Value | ConvertTo-Json))
        [IO.File]::Move((Join-Path $Directory 'run.json.tmp'), (Join-Path $Directory 'run.json'), $true)
    } finally { $lock.Dispose() }
}
$live = & (Join-Path $repo 'Scripts\Development-Run.ps1') -Action Status
Assert $live.allowWork 'Live run does not permit this synthetic operation.'
Assert ([IO.Path]::IsPathFullyQualified($Root) -and [IO.Path]::IsPathFullyQualified($Subject)) 'Absolute fixture paths required.'
Add-Type -Path (Join-Path $repo 'Scripts\AuthoringLeafGuard.cs')

if ($Role -eq 'Controller') {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class GuardFixtureNative {
    [StructLayout(LayoutKind.Sequential)] public struct Info {
        public uint Attributes, CreationLow, CreationHigh, AccessLow, AccessHigh, WriteLow, WriteHigh;
        public uint Volume, SizeHigh, SizeLow, Links, IndexHigh, IndexLow;
    }
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool SetHandleInformation(IntPtr h, uint mask, uint flags);
    [DllImport("kernel32.dll", SetLastError=true)] static extern bool GetFileInformationByHandle(IntPtr h, out Info info);
    public static Info InheritableCanary(IntPtr h) {
        Info info;
        if (!SetHandleInformation(h, 1, 1) || !GetFileInformationByHandle(h, out info))
            throw new Win32Exception(Marshal.GetLastWin32Error());
        return info;
    }
}
'@
    $guard = $null
    $canary = [IO.File]::Open((Join-Path $Root 'canary'), 'CreateNew', 'ReadWrite', 'ReadWrite')
    try {
        $handle = $canary.SafeFileHandle.DangerousGetHandle()
        $identity = [GuardFixtureNative]::InheritableCanary($handle)
        $environment = [Collections.Generic.Dictionary[string,string]]::new()
        $environment['HOMESTEAD_GUARD_TEST_ROOT'] = $Root
        $environment['HOMESTEAD_GUARD_TEST_CASE'] = if ($Case -in @('timeout','controller-failure','watchdog')) { 'ignore-stop' } else { 'normal' }
        $environment['HOMESTEAD_TEST_CANARY_HANDLE'] = $handle.ToInt64().ToString()
        $environment['HOMESTEAD_TEST_CANARY_HIGH'] = $identity.IndexHigh.ToString()
        $environment['HOMESTEAD_TEST_CANARY_LOW'] = $identity.IndexLow.ToString()
        $guard = [Homestead.Authoring.LeafGuard]::new($Subject, (Get-FileHash -LiteralPath $Subject).Hash,
            [string[]]@(), $Root, (Join-Path $Root 'marker'), (Join-Path $Root 'subject.log'), $environment, $null, [bool]$DetachedConsole)
        Write-NewJson (Join-Path $Root 'launch.json') ([ordered]@{
            pid = $guard.ProcessId; image = $guard.ImagePath; creationTime = $guard.ProcessCreationTime
            marker = $guard.MarkerBefore; job = $guard.LastVerifiedJob; explicitHandles = $guard.WhitelistedHandleCount
            jobMembers = $guard.ObserveJobMembers()
            creationFlags = $guard.CreationFlags
            resumed = $guard.Resumed
        })
        if ($Case -eq 'watchdog') { $guard.ArmDeadline(300, 800, (Join-Path $Root 'stop.txt')) }
        $guard.Resume()
        if ($Case -eq 'watchdog') { [Threading.Thread]::Sleep(1200) }
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $stopRequested = $false
        $reason = ''
        $jobSamples = [Collections.Generic.List[object]]::new()
        $memberSamples = [Collections.Generic.List[object]]::new()
        while (-not $guard.Wait(0)) {
            Assert ($timer.Elapsed.TotalSeconds -lt 15) 'Controller test ceiling.'
            $jobSamples.Add($guard.ObserveJobPolicy())
            $memberSamples.Add(@{elapsedMs=$timer.Elapsed.TotalMilliseconds;members=$guard.ObserveJobMembers()})
            if (-not $ObserveAccountingOnly) { $guard.VerifyJob(1) }
            $null = $guard.VerifyMarker()
            $state = & (Join-Path $repo 'Scripts\Development-Run.ps1') -Action Status -StateDirectory (Join-Path $Root 'state')
            $actual = & (Join-Path $repo 'Scripts\Development-Run.ps1') -Action Status
            if (-not $actual.allowWork) { $reason = 'live-run-stop' }
            elseif (-not $state.allowWork) { $reason = if ($state.deadlinePassed) { 'deadline' } else { 'pause' } }
            elseif (Test-Path -LiteralPath (Join-Path $Root 'request-stop')) { $reason = $Case }
            if ($reason -and -not $stopRequested) {
                Write-NewJson (Join-Path $Root 'stop.txt') @{ reason = $reason }
                $stopRequested = $true
                if ($Case -eq 'controller-failure') {
                    Write-NewJson (Join-Path $Root 'controller-exit.json') @{ outcome = 'failed-controller'; exitCode = 91; cleanup = 'kill-on-job-close, not graceful' }
                    [Environment]::Exit(91)
                }
                if (-not $guard.Wait(500)) { $guard.HardStop(92) }
            }
            Start-Sleep -Milliseconds 5
        }
        $after = $guard.VerifyMarker()
        $exitedJob = $guard.CaptureExitedJob()
        $exitedMembers = $guard.ObserveJobMembers()
        $rootAfter = $guard.ObserveHeldRoot()
        Assert ($rootAfter.Exited -and $rootAfter.IdentityFromHeldRoot -and
            $rootAfter.CreationTime -eq $guard.ProcessCreationTime -and $rootAfter.Image -eq $guard.ImagePath) 'Held-root exit observation differs.'
        if (-not $ObserveAccountingOnly) { Assert ($exitedJob.TotalProcesses -eq 1) 'Unclassified extra job member.' }
        $exitCode = $guard.ExitCode
        $hard = $guard.HardTerminated
        if ($Case -eq 'watchdog') {
            Assert ($guard.DeadlineStopRequested -and $guard.DeadlineHardStop -and -not $guard.DeadlineError) 'Independent watchdog failed.'
            $stopRequested = $true; $reason = 'watchdog'
        }
        Assert $stopRequested 'Subject exited without an observed stop request.'
        Assert ($exitCode -eq $(if ($Case -eq 'watchdog') { 95 } elseif ($hard) { 92 } else { 0 })) "Unexpected subject exit: $exitCode"
        $guard.Dispose(); $guard = $null
        Write-NewJson (Join-Path $Root 'outcome.json') ([ordered]@{
            outcome = if ($hard) { 'cancelled-hard-stop' } elseif ($reason -in @('pause','deadline','live-run-stop')) { 'cancelled-cooperative' } else { 'passed-cooperative' }
            reason = $reason; exitCode = $exitCode; hardTerminated = $hard; markerAfter = $after
            liveJobAccounting = $jobSamples; exitedJob = $exitedJob
            liveJobMembers = $memberSamples
            exitedJobMembers = $exitedMembers
            exitedRoot = $rootAfter
        })
    } catch {
        Write-NewJson (Join-Path $Root 'controller-error.json') @{ error = $_.ToString() }
        throw
    } finally {
        if ($guard) {
            if (-not $guard.Wait(0)) { $guard.HardStop(93) }
            $guard.Dispose()
        }
        $canary.Dispose()
    }
    return
}

Assert (-not (Test-Path -LiteralPath $Root)) 'Fresh fixture output required.'
$null = New-Item -ItemType Directory -Path $Root
$negative = Join-Path $Root 'rejections'
$null = New-Item -ItemType Directory -Path $negative
$rejections = [Collections.Generic.List[object]]::new()
foreach ($invalid in @('wrong-hash','nonempty-marker','existing-writer','occupied-stdout','nul-argument','invalid-environment')) {
    $directory = Join-Path $negative $invalid
    $null = New-Item -ItemType Directory -Path $directory
    $marker = Join-Path $directory 'marker'
    [IO.File]::WriteAllBytes($marker, [byte[]]@())
    $expected = (Get-FileHash -LiteralPath $Subject).Hash
    $arguments = [string[]]@()
    $environment = [Collections.Generic.Dictionary[string,string]]::new()
    $writer = $null; $unexpected = $null; $rejected = $false
    switch ($invalid) {
        'wrong-hash' { $expected = '0' * 64 }
        'nonempty-marker' { [IO.File]::WriteAllText($marker, 'sentinel') }
        'existing-writer' { $writer = [IO.File]::Open($marker, 'Open', 'Write', 'ReadWrite') }
        'occupied-stdout' { [IO.File]::WriteAllText((Join-Path $directory 'stdout'), 'do not replace') }
        'nul-argument' { $arguments = @("bad`0argument") }
        'invalid-environment' { $environment['BAD=KEY'] = 'not allowed' }
    }
    try {
        try {
            $unexpected = [Homestead.Authoring.LeafGuard]::new($Subject, $expected, $arguments,
                $directory, $marker, (Join-Path $directory 'stdout'), $environment)
        } catch { $rejected = $true; $failure = $_.Exception.ToString() }
    } finally {
        if ($unexpected) { $unexpected.HardStop(94); $unexpected.Dispose() }
        if ($writer) { $writer.Dispose() }
    }
    Assert $rejected "Invalid case admitted: $invalid"
    Assert-WriteAllowed $marker
    Assert (-not (Test-Path -LiteralPath (Join-Path $directory 'ready.json'))) 'Rejected subject executed.'
    if ($invalid -eq 'nonempty-marker') { Assert (([IO.File]::ReadAllText($marker)) -eq 'sentinel') 'Sentinel changed.' }
    if ($invalid -eq 'occupied-stdout') { Assert (([IO.File]::ReadAllText((Join-Path $directory 'stdout'))) -eq 'do not replace') 'Existing stdout overwritten.' }
    $rejections.Add(@{ case = $invalid; rejected = $true; guardReleased = $true; error = $failure })
}
$results = [Collections.Generic.List[object]]::new()
foreach ($scenario in $Scenarios) {
    $live = & (Join-Path $repo 'Scripts\Development-Run.ps1') -Action Status
    Assert $live.allowWork 'Run stopped between disposable cases.'
    $directory = Join-Path $Root $scenario
    $stateDirectory = Join-Path $directory 'state'
    $null = New-Item -ItemType Directory -Path $stateDirectory
    $marker = Join-Path $directory 'marker'
    [IO.File]::WriteAllBytes($marker, [byte[]]@())
    Assert-WriteAllowed $marker
    [IO.File]::Move($marker, "$marker.positive")
    [IO.File]::Move("$marker.positive", $marker)
    [IO.File]::Delete($marker)
    [IO.File]::WriteAllBytes($marker, [byte[]]@())
    $markerBefore = [Homestead.Authoring.LeafGuard]::InspectMarker($marker)
    $state = [ordered]@{
        id = 'disposable-fixture'; state = 'running'
        startedUtc = [DateTimeOffset]::UtcNow.ToString('o')
        updatedUtc = [DateTimeOffset]::UtcNow.ToString('o')
        deadlineUtc = [DateTimeOffset]::UtcNow.AddMinutes(1).ToString('o')
    }
    Write-NewJson (Join-Path $stateDirectory 'run.json') $state
    $start = [Diagnostics.ProcessStartInfo]::new((Get-Process -Id $PID).Path)
    $start.UseShellExecute = $false
    foreach ($argument in @('-NoProfile','-NonInteractive','-File',$PSCommandPath,'-Role','Controller','-Root',$directory,'-Subject',$Subject,'-Case',$scenario)) {
        $start.ArgumentList.Add($argument)
    }
    if ($ObserveAccountingOnly) { $start.ArgumentList.Add('-ObserveAccountingOnly') }
    if ($DetachedConsole) { $start.ArgumentList.Add('-DetachedConsole') }
    $controller = [Diagnostics.Process]::Start($start)
    $subjectProcess = $null
    $samples = 0; $maxGapMs = 0.0; $lastSample = $null; $requested = $false
    $clock = [Diagnostics.Stopwatch]::StartNew()
    try {
        while (-not (Test-Path -LiteralPath (Join-Path $directory 'ready.json'))) {
            Assert (-not $controller.HasExited) "Controller exited before $scenario readiness."
            Assert ($clock.Elapsed.TotalSeconds -lt 10) 'Readiness timeout.'
            Start-Sleep -Milliseconds 10
        }
        $launch = Get-Content -LiteralPath (Join-Path $directory 'launch.json') -Raw | ConvertFrom-Json
        $ready = Get-Content -LiteralPath (Join-Path $directory 'ready.json') -Raw | ConvertFrom-Json
        $subjectProcess = [Diagnostics.Process]::GetProcessById($launch.pid)
        $null = $subjectProcess.Handle
        Assert ($subjectProcess.StartTime.ToUniversalTime().ToFileTimeUtc() -eq $launch.creationTime) 'Held subject creation identity differs.'
        Assert ($subjectProcess.MainModule.FileName -eq $launch.image) 'Held subject path differs.'
        Assert ($ready.volume -eq $launch.marker.Volume -and $ready.indexHigh -eq $launch.marker.IndexHigh -and
            $ready.indexLow -eq $launch.marker.IndexLow) 'Inherited marker differs from supervisor identity.'
        while (-not $subjectProcess.HasExited) {
            Assert ($clock.Elapsed.TotalSeconds -lt 15) 'Observer ceiling.'
            $now = $clock.Elapsed.TotalMilliseconds
            if ($null -ne $lastSample) { $maxGapMs = [Math]::Max($maxGapMs, $now - $lastSample) }
            $lastSample = $now
            $reader = [IO.File]::Open($marker, 'Open', 'Read', 'Read')
            $reader.Dispose()
            foreach ($operation in @('write','delete','rename')) {
                $denied = Get-DeniedOperation $marker $operation
                if (-not $denied) {
                    Assert $subjectProcess.HasExited "$operation guard gap while held subject alive."
                    break
                }
            }
            $samples++
            if ($samples -ge 10 -and -not $requested) {
                if ($scenario -in @('pause','deadline')) {
                    if ($scenario -eq 'pause') { $state.state = 'paused' }
                    else { $state.deadlineUtc = [DateTimeOffset]::UtcNow.AddSeconds(-1).ToString('o') }
                    Update-FixtureState $stateDirectory $state
                } else { Write-NewJson (Join-Path $directory 'request-stop') @{ stop = $true } }
                $requested = $true
            }
            Start-Sleep -Milliseconds 1
        }
        Assert ($controller.WaitForExit(5000)) 'Controller did not release handles.'
        Assert ($controller.ExitCode -eq $(if ($scenario -eq 'controller-failure') { 91 } else { 0 })) 'Unexpected controller exit.'
        Assert-WriteAllowed $marker
        [IO.File]::Move($marker, "$marker.released")
        [IO.File]::Move("$marker.released", $marker)
        Assert ((Get-Item -LiteralPath $marker).Length -eq 0) 'Dummy marker changed.'
        $markerAfter = [Homestead.Authoring.LeafGuard]::InspectMarker($marker)
        Assert (($markerBefore | ConvertTo-Json -Compress) -ceq ($markerAfter | ConvertTo-Json -Compress)) 'Post-release marker identity/bytes/metadata/ACL changed.'
        $outcome = if ($scenario -eq 'controller-failure') {
            Get-Content -LiteralPath (Join-Path $directory 'controller-exit.json') -Raw | ConvertFrom-Json
        } else {
            Get-Content -LiteralPath (Join-Path $directory 'outcome.json') -Raw | ConvertFrom-Json
        }
        Assert (@(Get-ChildItem -LiteralPath $directory -Filter 'forbidden-*').Count -eq 0) 'Forbidden child witness exists.'
        $results.Add([pscustomobject][ordered]@{
            case = $scenario; passed = $true; observations = $samples; maximumObservationGapMs = $maxGapMs
            subjectExitCode = $subjectProcess.ExitCode; controllerExitCode = $controller.ExitCode
            ready = $ready; launch = $launch; outcome = $outcome
            observerMarkerBefore = $markerBefore; observerMarkerAfter = $markerAfter
            writeRenameDeletePositiveControls = $true; compatibleRead = $true; releaseVerified = $true
            limitation = 'Sampled availability checks are not continuous execution tracing; inherited handle identity and job-close semantics provide the lifetime mechanism.'
        })
    } catch {
        Write-NewJson (Join-Path $directory 'observer-error.json') @{ error = $_.ToString() }
        throw
    } finally {
        if (-not $controller.HasExited) {
            # This exact held controller was created above; job-close owns its only fixture subject.
            $controller.Kill(); Assert ($controller.WaitForExit(5000)) 'Owned controller hard-stop failed.'
        }
        if ($subjectProcess) {
            Assert ($subjectProcess.WaitForExit(5000)) 'Owned job did not terminate its subject.'
            $subjectProcess.Dispose()
        }
        $controller.Dispose()
    }
}
Write-NewJson (Join-Path $Root 'result.json') ([ordered]@{
    passed = (-not $ObserveAccountingOnly); diagnosticChecksPassed = $true
    observeAccountingOnly = [bool]$ObserveAccountingOnly; cases = $results; rejections = $rejections
    guardSha256 = (Get-FileHash -LiteralPath (Join-Path $repo 'Scripts\AuthoringLeafGuard.cs')).Hash
    subjectSha256 = (Get-FileHash -LiteralPath $Subject).Hash; unrealExecuted = $false; realMarkerAccessed = $false
})
$results | Select-Object case, passed, observations, maximumObservationGapMs
