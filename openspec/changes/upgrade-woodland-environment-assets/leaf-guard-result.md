# Direct-leaf guard implementation and disposable lifetime proof

**Partial preflight, not engine admission.** The15:02 Arizona coordinator
assignment authorized the native guard and disposable lifetime tests, then
conditionally one120-second no-asset-write Editor-Cmd settings/stop probe.
The guard mechanism is implemented and tested. The effective-settings export
and complete authoring policy/monitor wrapper are not implemented and verified,
so the conditional real probe was not started. No real global-marker lock,
Unreal execution, asset change or environment visual upgrade occurred.
Task1.3 remains unchecked; OpenSpec progress remains4/29.

## Current authorization boundary

The14:05 explicit TraceControl consent remains valid. The15:02 assignment adds
a conditional read-only lock of the existing ordinary empty
`C:\ProgramData\Epic\NotAllowedUnattendedBugReports` for that one probe, after
metadata, access, no-competing-authoring and inherited-lifetime conditions pass.
It does not authorize creating/repairing the marker, changing its ACL/attributes,
or launching first and killing an excluded helper afterward.

The direct leaf must have active-process limit1, no breakaway and
KILL_ON_JOB_CLOSE before its first loader/static execution. No executed
ShaderCompileWorker, CrashReportClientEditor, Concert or other helper is admitted.
Use the previously source-confirmed `-noshaderworker` plus
`bAllowCompilingThroughWorkers=False`, not skipped shader compilation.
The original proposal's earlier worker allowance is superseded for this probe.

All original messaging/privacy/pip/filesystem-DDC/TraceServer, exact identity,
existing-rules/profile, no-extra-network, deadline and isolation conditions
remain prerequisites. Cooperative stop comes first; the bounded owned-job hard
fallback applies only to this no-write probe. It does not approve killing
future Editor import operations. UAT/cook/Pak/import/Shipping-QA remain separate,
unpassed gates. No blanket reporter or network exception has been added.

## Implemented native primitive

`Scripts\AuthoringLeafGuard.cs` creates the exact hash-checked image suspended,
assigns an unnamed noninherited job before resume, and verifies the held
process's image/hash, native creation time, membership, actual limits and count.
Actual job evidence is exposed separately from requested parameters.

Only three handles are whitelisted: the existing read-only/share-read marker,
read-only NUL stdin, and a newly created stdout file (stderr aliases stdout).
The job handle is not inherited. The child receives the marker handle number
so it can verify the actual file identity, not reopen a guessed pathname.
An unrelated deliberately inheritable canary is excluded by the handle list.

Marker evidence contains volume/file index, size, empty-file SHA256, creation/
last-write times, attributes and owner/group/DACL digest. It is compared while
held and independently after process death/release. Access-time preservation
is not claimed. Existing stdout is never overwritten. Constructor failures
clean up any owned suspended process; disposal refuses to release a live
subject's guard. Explicit job termination and kill-on-close are hard stops.

This is a process primitive, **not a completed authoring supervisor or a
network/filesystem/broker sandbox**. Its caller must supply admission,
network/process observation, run controls and effective-setting verification.

## Actual disposable results

Final fixture run:
`Saved\Automation\20260920-182217-d1f84e39\leaf-guard-05`.
Tracked evidence/receipt:
`docs\research\environment-assets\leaf-guard-01`.

| Case | Sharing observations | Largest gap, ms | Subject/controller exit | Actual outcome |
| --- | --- | --- | --- | --- |
| Normal stop marker | 10 | 36.6024 | 0/0 | Cooperative completion |
| Disposable run paused | 12 | 16.3564 | 0/0 | Cooperative cancellation |
| Disposable deadline passed | 13 | 26.7958 | 0/0 | Cooperative cancellation |
| Subject ignores stop | 35 | 31.4841 | 92/0 | Owned-job hard cancellation after500ms |
| Controller exits without finally | 15 | 31.9809 | 0/91 | Controller failure; job-close hard termination |

The controller-failure subject's exit0 is **not** graceful success: this fixture
ignores stop and normally remains alive20seconds; the controller exits91 without
disposing its guard, and closing its last job handle terminates the subject.
The inherited read-only handle is verified in the subject's static initializer.
Write/delete/rename probes remain denied at observed live-subject points;
compatible reads and post-death release succeed. Before/after identity, bytes,
attributes, timestamps and owner/group/DACL digest match in all five cases,
including the controller-failure case.

Each static initializer also attempts normal, detached, breakaway and combined
children with handle inheritance requested:20 denied attempts total, no child
initializer witnesses. Ordinary/detached fail1816, breakaway forms fail5.
The earlier accepted fixture supplies positive ordinary/detached child-execution
controls and the inherited-host-job breakaway limitation; do not claim an
unrestricted breakaway positive control.

Six additional rejection cases passed: wrong executable hash, nonempty marker,
preexisting writer, occupied stdout, NUL argument and malformed environment key.
They leave no ready witness, release their own guards and preserve sentinel
data/existing stdout. The tests run no listener and never address the real
ProgramData marker.

Availability observations are sampled, not an atomic or continuous proof of
every instant during process rundown. The inherited kernel handle supplies the
lifetime mechanism; polling supplies bounded corroboration. Job containment
does not cover unrelated brokers or arbitrary global filesystem writes.

## Preserved unsuccessful iterations

The first native compilation failed `/W4 /WX` on a signed/unsigned comparison.
Its log is retained; an explicit DWORD conversion fixed it before executable
tests. Existing compiler setup also printed the preexisting missing-vswhere
warning; no installation or engine change was made.

The first five-case suite passed. Adding post-release/rejection coverage then
exposed a test-harness scope mistake: `Update-FixtureState` had been nested inside
another function. That run stopped during the disposable pause case, retained
its error evidence, and its exact held controller/job were cleaned up. The
function was moved to script scope; corrected and final expanded suites passed.
This was not an Unreal retry or a concealed successful run. Final source hashes
and final results, rather than earlier partial results, define this checkpoint.

## Remaining concrete blockers before the one real probe

1. Wire the primitive into the complete authoring admission/monitor wrapper:
   exact engine hashes/signatures, existing rules/profile, competing-authoring
   checks, startup-through-shutdown endpoint/root observation and measured gaps,
   fresh child-only config/UserDir/temp/DDC, run/deadline cancellation and
   explicit failed/cancelled evidence. The disposable controller proves the
   mechanism, not that missing production wiring.
2. Finish source-backed effective-settings export and startup-hook admission.
   The installed Python plugin runs every admitted `init_unreal.py` and configured
   StartupScripts before commandlet main; interpreter isolation and pip-disabled
   alone do not clear this. Its settings header confirms Engine-config startup/
   additional paths and pip/remote switches, but this checkpoint has no effective
   plugin inventory, DDC graph/path, actual privacy state or worker-state export.
   Requested arguments alone cannot substitute for those observations.
3. Only then evaluate the conditional real-marker metadata/access/competition
   prerequisites and the one120-second real probe. No real lock has been taken
   and no real-probe attempt consumed. No assumption that synthetic success
   clears the remaining conditions.

This bounded checkpoint returns the proven mechanism rather than extending the
assignment into an unreviewed supervisor framework or rushing a real launch.
The accepted Shipping preview, selected source assets, existing gameplay and
saves remain untouched. The coordinator owns the next continuation and schedule.
