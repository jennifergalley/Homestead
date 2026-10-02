# Process-local containment feasibility

**Synthetic mechanisms passed; no Unreal launch is authorized or verified.**
This is the coordinator's bounded2026-09-20 14:41 Arizona feasibility assignment,
not a general sandbox, completed task1.3 or environment implementation.
OpenSpec remains4/29. Windows Sandbox was reported disabled by the coordinator;
no feature enable/install/reboot/VM launch was attempted.

## Measured disposable-process result

`Tests\WindowsContainmentProbe.cpp` is a standalone Windows-only native fixture,
not a production launcher or a change to existing gameplay/native test targets.
It can launch only itself. Compiled with the existing MSVC14.44 toolchain,
`/std:c++17 /EHsc /W4 /WX /MT /O2`, linking Shell32/Advapi32; no install.
The setup batch emitted a missing-vswhere warning, but compilation/link and
the final executable completed successfully. The warning remains in the log.

Final isolated run:
`Saved\Automation\20260920-182217-d1f84e39\containment-feasibility-03`.
Tracked evidence:
`docs\research\environment-assets\containment-feasibility-01`.
Native executable SHA256:
`7C14D95BD353D61A01A166F4169F69BE141D534CD0CC8CFE164DBACC03FB6A3E`.
Source SHA256:
`92BF723B2D822E7913ABE31C4C752F5CBC12569336104267C1BA86E650BD1F65`.

The controller creates an unnamed, non-inherited Job Object with
`JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`
(8200), active limit1, neither breakaway flag. It creates the subject with
`CREATE_SUSPENDED`, assigns it, queries actual limits/membership/active count1,
checks the subject's static-initializer marker is absent, then resumes its
primary thread. Assignment/verification failure never resumes the subject.
The subject attempts children both in its C++ static initializer and in main.
Each prospective child would write its witness file in its own static initializer.

| Creation mode (also uses NEW_PROCESS_GROUP) | Static attempt | Main attempt | Child witness |
| --- | --- | --- | --- |
| Ordinary | CreateProcess false,1816 (quota) | false,1816 | Absent |
| Detached | false,1816 | false,1816 | Absent |
| Explicit breakaway | false,5 (access denied) | false,5 | Absent |
| Detached plus breakaway | false,5 | false,5 | Absent |

All8 restricted attempts were denied, rather than discovered/killed after their
initializers executed. The subject's own static marker exists, proving the
subject actually ran. Positive baseline ordinary/detached children executed
and created all4 expected static/runtime witnesses.

### Important baseline limitation, preserved rather than hidden

The CLI environment already places the controller in a no-breakaway job with
flags8192. The first fixture incorrectly demanded successful baseline breakaway;
it failed with error5 before the restricted test. Its source/log/binary evidence
is retained under `containment-feasibility-01`, not overwritten or called passed.
The corrected fixture explicitly queries that inherited job and accepts only
the documented baseline restriction (membership, neither breakaway flag,
error5, absent witness). It records `baselineBreakawayUnavailable=1`.
It does **not** claim an unconstrained positive breakaway control. The added
one-process job is independently queried as8200. A second passing run tested
same-process sharing; the final third run added cross-process write denial.
This was an explained fixture correction and additional mechanism coverage,
not repeated Unreal launches or retry-until-lucky runtime acceptance.

## Disposable read-only file-sharing result

Both an empty file and a9-byte sentinel were created only in the fixture root.
Holding `GENERIC_READ`, `FILE_SHARE_READ`, `OPEN_EXISTING`:

- Allows a second reader.
- Denies a separate native process's Core-style `GENERIC_WRITE`, share0,
  `CREATE_ALWAYS` open with error32 before truncation.
- Denies local create/append combinations, including share-read/delete variants.
- Denies deletion and rename with error32.
- Preserves actual bytes, size, last-write time and DACL.
- Releases normally: write access becomes available after the handle closes.
- Fails closed when a writer already holds the file; no forced takeover.

The **real global marker was never opened or locked**. Metadata-only inspection
finds `C:\ProgramData\Epic\NotAllowedUnattendedBugReports` exists, length0,
attributes Archive/NotContentIndexed. Readability/share compatibility is not
established by metadata and must be checked only after separate lock approval.
No ACL, registry, security rule or persistent setting was changed.

## Installed Core failure paths and supported shader controls

Source references are hashed in the receipt; these are static findings, not an
Editor run:

| Source | Consequence |
| --- | --- |
| `WindowsPlatformProcess.cpp:542-570,684-688` | Failed CreateProcess logs a warning, sets output PID0 and returns an invalid handle. Quota1816 is not its out-of-memory special case. |
| `WindowsPlatformCrashContext.cpp:622-688` | Invalid reporter handle skips the entire respawn wait/PID-file path and returns invalid. The unconditional "Started CrashReportClient (pid=0)" log would not prove a child exists. |
| Same file:1390-1408 | Crash-thread construction continues after that return; registering the reporter PID is conditional on a valid handle. Later fallback child creation would still be subject to the job. |
| `WindowsPlatformFile.cpp:1638-1673` | Denied OpenWrite returns nullptr; it does not truncate successfully or raise a fatal error here. |
| `GenericPlatformCrashContext.cpp:800-821` | Privacy flags are loaded before the marker OpenWrite. Its local nullable handle is discarded, without setting privacy true on failure. False values remain false in the cached context. |
| `ShaderCompiler.cpp:813-828` | Supported `-noshaderworker`, Engine `DevOptions.Shaders.bAllowCompilingThroughWorkers=False`, or CVar `r.Shaders.AllowCompilingThroughWorkers=0` selects no external workers. |
| Same file:1035 | Remote compilation requires worker compilation enabled; disabling workers disables that route. |
| `ShaderCompilerThreadRunnable.cpp:960-980,1649-1663` | No-worker branch executes shader jobs directly through loaded DLLs; it does not skip shader compilation. |
| `ShaderCompiler.cpp:748`; `ShaderCompilerPrivate.h:33` | Retry counter initially-1. Worker recovery is not automatically enabled by explicitly starting in no-worker mode. |

Candidate process controls would include **`-noshaderworker`** plus explicit
`-ini:Engine:[DevOptions.Shaders]:bAllowCompilingThroughWorkers=False`.
The already supported asynchronous-compilation setting can separately be false
if a synchronous diagnostic needs it; threads themselves are not prohibited by
the process limit. Do not substitute a no-shader-compilation/skip-shader option.
Real Windows-target shader/material compilation, cook completion and stop
responsiveness remain untested. Any other required helper would be denied,
requiring a reported incompatibility, not temporarily raising the limit.

## Global marker lifetime and interference

If separately approved, require an existing empty ordinary file and successful
read-only/share-read open before tool startup. Validate its handle identity,
size and metadata without creating, repairing or changing it. Hold until the
owned tool has actually exited; do not release on a merely requested stop.
Abort before launch on sharing/access/reparse/identity failure.

This is a **global-file sharing constraint**, even though it is not a global
configuration/ACL edit. Another authoring process trying to update the same
marker may receive a sharing failure. Normal compatible readers remain allowed.
No guarantee of noninterference with concurrent authoring can be made; if that
tradeoff is not acceptable, do not acquire the lock.

The synthetic fixture proves normal handle-lifetime sharing semantics, not
supervisor-crash cleanup ordering. A production guard must resolve that lifetime
before claiming the lock lasts for the entire tool execution (for example,
separately test narrowly inherited read-handle lifetime). It must not advertise
KILL_ON_JOB_CLOSE as graceful cancellation. No real marker lock or such inherited
handle was tested here. Sharing does not deny all attribute/ACL operations; this
is protection against the inspected Core OpenWrite, not a filesystem sandbox.

## UAT/Pak/build orchestration is a separate boundary

The one-process job fits a **direct leaf executable**, not a shell/UAT/dotnet
orchestrator that must create children. Do not put UAT in limit1 and call its
failed packaging valid, raise the limit around startup, or attach after a child
has already begun loading.

`CopyBuildToStagingDirectory.Automation.cs:473-491` hardcodes the installed
UnrealPak path and calls RunAndLog; :5373 uses that path for IoStore too.
AdditionalPak/IoStore options supply arguments, not a pre-loader Job assignment.
Other paths include MakeBinaryConfig and optional Zen staging. No supported
pre-start containment hook covering those launches was established in this study.
Project compiler/UBT orchestration also needs legitimate compiler/linker children
and cannot inherit the same one-process policy blindly.

A possible later project-owned recipe could invoke reviewed Editor/Cook and
Pak/IoStore leaves separately through a guard, retaining exact manifests,
arguments, hashes and fresh outputs, then stage without hidden UE launches.
That requires explicit implementation/review and packing proof; it is not a
verified replacement for stock BuildCookRun. No installed UAT or engine file was
modified, and no wrapper path was substituted for an approved tool.

## Scope limits and next decision

The mechanism is feasible for the **specific direct CreateProcess startup path**.
It is not a network filter, per-process filesystem isolation, or containment of
arbitrary brokered process creation (Microsoft explicitly excludes WMI-created
processes from automatic job association). Parent endpoint monitoring, approved
TraceControl exception, plugin/Python/DDC/privacy controls, run/deadline observer
and owned shutdown evidence are still required.

Recommended next scope, only after coordinator review: implement the narrow
leaf guard and close the dummy-tested marker lifetime issue; then seek approval
for one no-write Editor settings/stop probe. Keep cook/Pak/UAT, imports and
Shipping QA on their own unpassed gates. Alternatively retain the hold if the
global read-lock interference or remaining orchestration work is unacceptable.
No broad reporter/Concert/upload consent is needed for a reporter that cannot
execute, but actual Unreal compatibility has **not** yet been demonstrated.

Microsoft API references used (no third-party code/assets sent):
- https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects
- https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information
- https://learn.microsoft.com/en-us/windows/win32/procthread/process-creation-flags
- https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
