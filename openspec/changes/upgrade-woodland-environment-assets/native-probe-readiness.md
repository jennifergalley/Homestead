# Read-only settings-probe admission: not ready

2026-09-20,17:37 Arizona. Build checkpoint `c57b77f` is privately pushed.
No Editor was launched, real-marker handle acquired, attempt reserved, or
probe-output directory created. OpenSpec remains4/29; task1.3 is unchecked.
This is the single bounded admission review requested after the build.

## Concrete corrections before consuming the attempt

1. **Pin accepted project products.** The wrapper currently measures only
   the probe DLL hash. It must verify the accepted build receipt itself
   (SHA256 E091B1C1C59F62460C29078DBE31FF805A3F39117CF678E9BADFEF934520817D)
   and its actual game/probe DLL, module manifest and target identities before
   reservation/launch, then preserve the same pins through exit.
   These files already match the real build receipt; the wrapper enforcement
   is what is missing. The qualified game-link monitoring failure must remain
   in admission evidence, not be represented as an entirely clean build.
2. **Explicit Editor console-mode decision.** The current default constructor
   selects CREATE_NO_WINDOW (flags0x08080404). The exact hashed Editor-Cmd PE
   has subsystem3/Windows CUI. Existing same-machine fixtures show this mode
   can add conhost while wrapper/native validation requires active1.
   Do not run this known mismatch or admit a second Editor process.
   The minimal candidate is explicit DETACHED_PROCESS with the same suspended
   pre-start job and marker/NUL/file-output whitelist, flags0x0008040C.
   This remains an Editor-specific decision: the successful build-only tests
   did not change Editor permission.
3. **Fix empty process enumeration.** Actual `-ValidateOnly` reaches line73
   and throws `The property 'Count' cannot be found on this object` because
   the zero-result function pipeline emits no object under StrictMode.
   Both `(Get-AuthoringProcesses).Count` call sites need caller-side array
   normalization (or an explicitly non-enumerated array return). Zero is an
   expected safe result, not an error to suppress.
4. **Wire existing actual job observations.** The wrapper records initial
   policy and live active counts, but does not persist the guard's available
   before/live/after member lists, held-root exit observation or final total
   accounting. Use the already-tested methods; require only the exact root,
   TotalProcesses1 and final ActiveProcesses0. No new tracing framework or
   extra-member tolerance is needed. A root that exits between the loop's
   wait and active-count read must be recognized by its retained-handle wait,
   not mistaken for a live policy violation or silently ignored.

No wrapper/engine source was changed during this read-only review.

## Console/stdio source evidence

The current UnrealEditor-Cmd hash remains
AE92F55952A3C9A7DEF90983FCF5AEDCE52D8EE8E4565FBD923985E9D9D3E05E;
its actual PE optional-header Subsystem is3.
Installed `LaunchEngineLoop.cpp:3465-3472` derives GIsConsoleExecutable from
that PE subsystem, not from whether a console was attached by the launcher.
Lines3959-3976 take the console-executable branch without calling
`GLogConsole->Show(true)`. Lines1888-1895 enable the separate stdout device for
the existing `-stdout`; file stdio remains explicitly supplied.

`WindowsPlatformApplicationMisc.cpp:138-152` chooses the standard console
device absent RemoteConsoleHost/NewConsole arguments. The wrapper supplies
neither, and supplies no `-log` request. `WindowsConsoleOutputDevice.cpp:440-457`
may test parent-console attachment with AttachConsole and immediately
FreeConsole after a successful temporary attachment; this is not AllocConsole
or evidence of a newly launched console helper. Its Show path contains
AllocConsole, so do not claim detached creation universally forbids later
application-driven console allocation. Actual root-only job observations
and finite endpoint monitoring must still decide the real probe result.

Thus detached creation plus existing file stdio is the narrow supported
candidate, not a proved Unreal runtime result. No guessed console flag,
engine edit, debugger, altered job limit or conhost exception is proposed.

## Current conditions and evidence limits

| Condition | Actual status |
| --- | --- |
| Built native module and matching PDB/metadata | Ready as build products; exact identities in `native-build-result.md`; not loaded/executed |
| Live approval/deadline and pinned engine executable/Core/TraceLog | Passed the read-only wrapper checks before line73 |
| Existing executable rules and active profile | Passed unchanged-rule/profile comparison before line73; no rules modified |
| Competing authoring process check | Separate read-only enumeration returned0; wrapper's zero-result Count bug prevents its normal completion |
| Protected inputs | All234 rehashed; only the explicitly authorized Editor target edit differs from source-preparation baseline |
| Real marker metadata | Existing ordinary empty file, Archive/NotContentIndexed; last-write2026-09-20T22:32:57.5529435Z; no read-lock/share-conflict test was performed |
| Marker lifetime | Disposable six-case proof exists; actual marker identity/ACL/hash/readability/share compatibility must be captured under the approved real guard immediately before execution and compared after actual death/release |
| Startup, Python and messaging | Existing arguments disable Python plugin/interpreter/pip/remote execution and UDP/TCP plugins/settings; no broad bootstrap or startup script; effective runtime checks remain unperformed |
| TraceServer/workers | Existing -notraceserver/-traceautostart=0 and -noshaderworker/worker-false settings remain; no skip-shader flag; no real shader workload proven |
| DDC/config/privacy | Fresh local filesystem-DDC graph and isolated INI/UserDir/temp settings are implemented; native exporter checks actual paths, stores and cached privacy false; runtime values remain unknown |
| Job/root observations | Primitive and synthetic lifetime are proved; wrapper needs the existing member/final-accounting wiring above |
| Stop and hard fallback | Native stop-file return, independent100s soft/110s hard watchdog and owned-job cleanup exist; dummy normal/timeout/controller-failure cases pass; real native cooperative stop is unproved |
| Network and prompts | Only the bounded TraceControl listener is permitted; sampled root TCP/UDP/competition checks are implemented, not continuous broker isolation; security prompts still require human/coordinator stop and must never be clicked |
| Protected human release | Selected Shipping package, normal game/saves and source graphics remain untouched; no environment visual upgrade claimed |

The single attempt reservation is absent. The read-only output argument
`native-settings-readiness-only` was not created. Cook, Pak, full imports,
Shipping QA, GPU/VRAM/render/performance and art gates are still separate.
