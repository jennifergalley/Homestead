# Authoring preflight: stopped before engine launch

**Task1.3 remains unchecked; progress4/29.** TraceControl consent is now explicit.
The current blocker is an additional startup child excluded by that consent.
No new Editor, commandlet, cook, UnrealPak, build or game process was launched.
No environment code/material/asset change or world visual improvement is claimed.

## Authorization and preserved evidence

Jenny replied "I approve - please proceed" at14:05 Arizona on2026-09-20 directly
to the bounded authoring-listener question. The coordinator resumed
`20260920-182217-d1f84e39`, with deadline unchanged at
`2026-09-21T02:22:17.1884351Z`. The live record references proposal SHA256
`EA25571F37A6F3109BEECCA56B54E56006D8F61F0C0B07A95BA5DE077DB0DBCC`.

Evidence directory: `docs\research\environment-assets\authoring-preflight-01`.
It preserves those exact original proposal bytes as `approved-proposal.md`,
separately from the current proposal's superseding status header.
`receipt.json` records actual executable/library sizes, SHA256, valid Epic
signatures, current Private profile, existing application-rule observations,
source/log identities, preserved-source verification and root preview validation.
The directory's scoped Git attributes preserve receipt/snapshot bytes on checkout.

The approved exception is the known Editor-Cmd/Pak TraceControl TCP listener
(1985 or32768-40959); only verified ShaderCompileWorker children are otherwise
admitted. It does not approve crash-report/recovery services, UDP, accepted or
outbound connections, a GUI editor, engine edits or changing firewall rules.
The12:32 exact-owned-PID hard-stop/discard-partial-Pak decision does not expand
that child/service policy.

## Concrete pre-launch conflict

The following line references are to the installed UE5.8 source, hashed in the
receipt. They are static evidence, not newly observed process behavior.

| Evidence | Finding |
| --- | --- |
| `Runtime\Core\Private\Windows\WindowsPlatformCrashContext.cpp:54-60` | Crash-monitor default is `WITH_EDITOR`; `NOINITCRASHREPORTER` defaults to0. |
| Same file:1359-1395,1727-1728 | Global crash-reporting-thread construction invokes the monitor launcher during static initialization, before process INI privacy settings. |
| Same file:568-606 | Launcher explicitly parses OS arguments before `FCommandLine` initialization. It forwards log/unattended/null-RHI options; the proposed privacy INI overrides are not a startup-child prevention gate here. |
| Same file:613-618 | Editor builds append a Concert recovery-server argument even for commandlets. This is not a claim that a new recovery socket was observed. |
| Same file:622-679 | Launch is detached; the reporter respawns itself, with the Editor reacquiring its PID through a temporary file. A root-PID-only termination policy would not account for this correctly. |
| Same file:783-790 | Privacy INI values are read later in crash-report handling, not as the global monitor-construction gate. |
| `Runtime\Launch\Private\Windows\LaunchWindows.cpp:210-235` | Unattended/error and `noexceptionhandler` handling occurs later in startup; it does not establish suppression of the preceding static constructor. |
| Existing generated Editor Core definitions:48 | `WITH_EDITOR 1`; this is an existing generated header, not a fresh engine compile. |
| Existing `Build\Logs\bootstrap.log:8,456` | Older Python commandlet logged `Started CrashReportClient (pid=2096)` with unattended/null-RHI arguments. Historical evidence only. |

Current `UnrealEditor-Core.dll` SHA256:
`3EAF66A3AA55FEB0BEEF9BD88411A577A0589AFADB60724F3D6651D2E46632DB`.
Current `CrashReportClientEditor.exe` SHA256:
`94F11E2B0DB157FDBBA604FB49EAB94EB00E80951BDB94A8288FFA34546A7B57`.
Both have valid Epic signatures. The receipt records searches of installed Core
string data, which support compiled implementation presence, not runtime flow.
The historical log did not record its executable/Core hashes; it is not promoted
to a matched new controlled probe.

No supported pre-main process switch preventing this path was established.
`-notraceserver` targets the separate Trace server, not this crash monitor.
Neither a guessed switch nor INI privacy settings justify knowingly starting an
excluded child. Renaming/removing installed files, changing global macros,
custom-building the engine, debugger/elevation workarounds or new firewall rules
were not attempted and are not proposed as implicit permission.

## Bounded same-trace continuation requested by coordinator

The coordinator requested one approximately15-minute read-only continuation,
not a launch or a new research workstream. Findings below narrow the blocker:
**monitor IPC is not a network service; the Concert argument does not establish
an active recovery server; compiled upload support is not an observed upload.**
Supplemental source/config/receipt hashes and bounded binary-data observations
are preserved in `reporter-continuation.json` beside the first receipt.

### Exact reporter and propagation

`CrashReportClientEditor-Win64-Shipping.target` identifies a **Shipping Program**,
UE5.8.2/CL56702186, shared build ID55116800, launch
`Engine\Binaries\Win64\CrashReportClientEditor.exe`. Receipt SHA256:
`8570FED22B3D0556A98D97BA3A70B26AACF1107D8555DC4D7A1666A8AAABD43F`.
This is not a Development gameplay shortcut. Its Shipping classification alone
does not disable its purpose-built HTTP/analytics/reporting facilities.

| Surface | Established static behavior | Remaining limit |
| --- | --- | --- |
| Monitor IPC | `WindowsPlatformCrashContext.cpp:441-455,522-555` creates inherited anonymous pipe handles and passes `READ`, `WRITE`, `MONITOR` PID and `ProcessGroupId`. | These handles are local IPC, not TCP listeners. No live pipe exchange was observed. |
| Explicit forwarded arguments | Same file:568-618 forwards optional `abscrashreportclientlog`, unattended/null-RHI, `CleanCrashReports`, possible low-integrity expectation, and appends Concert server name. | No forwarding of parent UserDir, INI paths/privacy overrides, NoAnalytics, DisablePlugins, DDC or trace-server flags in this constructor. |
| Environment and working directory | `HAL\PlatformProcess.h:153-165` uses `CreateProc`; `WindowsPlatformProcess.cpp:530-533,684` passes a null environment block to Windows, inheriting the parent's environment. Default working directory is inherited. | Clearing child-scoped variables can propagate to the initial reporter. No proof that every respawn retains every setting; reporter entry-point source is absent. |
| Temp/log isolation | An explicit crash-client log argument is supported; `UserTempDir` uses Windows `GetTempPath` on the ordinary-integrity path, supporting a proposed child-only TEMP/TMP root. | Neither is a UserDir/config/known-folder redirect. No claim that all reporter/session/config writes would stay in that root. |
| Installed source coverage | Runtime CrashReportCore, parent startup and recovery-plugin source are present. The Programs CrashReportClient source directory and reporter PDB are absent. | Reporter main/respawn/config selection cannot be fully traced from this installation. No account/download/source acquisition was attempted. |

### Recovery and network: distinguish evidence from assumptions

`DisasterRecoveryClient.uplugin` is disabled by default and its module type is
`EditorNoCommandlet`. Its implementation:44-48 explicitly returns false for
hosting recovery in the crash reporter (the former path is commented out).
Thus the stale-looking Concert argument/comment in Core is insufficient to
claim this commandlet starts a recovery service. The installed reporter binary
contains no exact ASCII/UTF16 identifiers for ConcertServer, ConcertSyncServer,
UnrealRecoverySvc, UdpMessaging or DisasterRecovery in the bounded data check.
This supports the distinction but is not a runtime socket proof or complete
binary control-flow analysis.

The shipped `Engine\Programs\CrashReportClient\Config\DefaultEngine.ini` contains
`bAllowToBeContacted=true`, `bSendLogFile=true`,
`CanSendWhenUIFailedToInitialize=true`, and UDP `EnableTransport=True`, TTL0.
Those are file facts, **not proof of its effective selection or loaded transport**.
The missing reporter entry point prevents verifying that selection here.
`CrashReportCoreConfig.cpp:26-38` supports a configured upload destination or a
compiled `CRC_DEFAULT_URL`; `CrashReportCoreUnattended.cpp:31-70` uploads when
the selected uploader is enabled. Binary data contains the Epic datarouter
base and public-data URLs. No connection/upload has been observed in this task.

### Supported privacy controls and why they do not yet clear the gate

- Parent `FUserSettingsContext` fields default false
  (`GenericPlatformCrashContext.h:220-225`). Later Engine-config initialization
  reads the Editor privacy settings and serializes them to the temporary context
  (`GenericPlatformCrashContext.cpp:800-821,949-983`). This is a real supported
  parent-to-monitor privacy route, not an assertion that every reporter startup
  activity waits for it.
- Early crash handling is separate: local send/analytics variables initially
  default true, then the pre-config branch consults the existing
  `NotAllowedUnattendedBugReports` marker
  (`WindowsPlatformCrashContext.cpp:759-760,793-805`).
  The static-init crash caller explicitly requests unattended reporting
  (:1538-1560), so that marker's denial makes :817-825 return without sending
  that report. **The marker exists on this machine**; only its existence was
  inspected, not its contents or any user's crash/config history.
- The marker resolves through Windows ProgramData known-folder API to
  `C:\ProgramData\Epic\NotAllowedUnattendedBugReports`, not candidate UserDir
  (`WindowsPlatformProcess.cpp:1516-1531`). Setting the parent privacy flag
  false causes an `OpenWrite` attempt on that global marker during initialization
  (`GenericPlatformCrashContext.cpp:809-816`), even when it already exists.
  This is a separate conflict with the promised no-global-config-write scope.
  No such write was made by this preflight.
- `CrashReportAnalytics.cpp:18-28` has an environment-only default configuration
  using `UE_CRC_TELEMETRY_URL/KEY`, but :55-62 permits compiled telemetry
  URL/key settings to replace it. The installed reporter lacks those two exact
  environment-name strings while containing datarouter URLs. This is not proof
  which branch executes; clearing those variables is **not a demonstrated
  universal analytics-off control** for this installed binary.
- NoAnalytics is present in reporter binary string data, and the parent's
  fallback post-crash launch path can add it (:1062-1065). The startup monitor
  constructor does not forward it. Passing it to the Editor is therefore not a
  proven monitor control. Injecting extra arguments through a log filename is
  not a supported propagation mechanism and was not attempted.
- Candidate-only parent INI privacy overrides, explicit child log and inherited
  TEMP/TMP/cleared telemetry variables are useful partial controls. They do not
  establish pre-start reporter config isolation, zero analytics, or absence of
  unreviewed known-folder writes. No exact compliant additional launch allowlist
  can yet be endorsed under the unchanged constraints.

### Detached ownership and cancellation

Core launches the initial reporter with detached/inherit-handles settings and
waits for it to respawn. The final PID is read from `ue-crc-pid-<parentPID>` under
the temp root; Core opens only query/synchronize rights
(`WindowsPlatformCrashContext.cpp:622-679`). That PID file alone is not sufficient
worker ownership evidence: any future supervisor would need pre-start process
creation coverage linking both generations, executable hash/path/start times,
matching monitor/group identity and no unrelated PID reuse.

The parent's crash-thread destructor closes its reporter handle rather than
issuing a verified cooperative child-exit request (:1420-1446). Final reporter
exit-on-parent/pipe closure is not proven without its main source or a separately
approved probe. The Pak hard-stop exception does not silently authorize terminating
an extra reporter chain. No detached process was created or killed here.

## Completed versus still missing

Completed read-only checks: all seven original tool/module hashes still match;
nine inventoried executable/library signatures are valid; the active interface
is Private; existing Editor/Editor-Cmd/Pak inbound Allow observations remain
Private/Public, Any port/address. No matching enabled inbound Allow entry for
ShaderCompileWorker was returned by that exact-path query. These facts are not
network inactivity proof or permission for the reporter. The receipt's
`permittedByOriginalProposal` labels distinguish explicitly proposed runtime
inventory entries from the GUI/newly inspected files; they are not an executable
launch policy or a claim that Core cannot load.

All234 previously protected source/asset/config/launcher hashes still match.
Real-root `Start-Preview.ps1 -ValidateOnly` still resolves `offline-startup-01`,
`jenny-review`, candidate-local UserDir and `-Res=0x0wf`, with Shipping SHA256
`CE5C9A262394E0B63E1105D71D4EBFB82CB61D4041FDC994618665F6FF29E634`.
No human game/window, accepted package, personal save, source asset or security
rule was manipulated. Visible borderless remains human-unverified.

Runtime supervisor/stop observer implementation, effective privacy/plugin/DDC
settings, startup-through-shutdown child/socket sampling, cancellation proofs,
fresh cook/Pak forwarding and opt-in Shipping QA are **not completed**.
There are zero new runtime endpoint samples. Primary render fraction, GPU timing
and resident GPU memory remain **unmeasured**, not replaced with tick/system-RAM
proxies. No helper or gameplay tests were run for this documentation-only blocked
handoff; strict OpenSpec validation remains the applicable document check.

## Decision needed before proceeding

The minimal decision is whether to keep this installed-authoring path blocked
under the current no-upload/no-unreviewed-write/no-engine-edit constraints, or
separately authorize investigation of a different controlled authoring setup.
No broader Concert or automatic-upload consent is requested: neither active
Concert nor an actual upload was demonstrated, and a simple additional reporter
allowlist does not resolve its config/analytics/write/stop gaps.
The existing TraceControl-only consent is not a decision to accept those gaps.
If no compliant workflow is available, retain the accepted Shipping preview and
report the environment upgrade blocked. Do not launch a known excluded child
merely to collect a failure, silently admit its recovery/reporting services, or
skip the settings probe and proceed to cook/Shipping QA.

Even after a resolution, task1.3 requires the actual bounded guard/settings/
child/cancellation/Shipping-QA preflight before representative asset work.
The four checked tasks and source-only/runtime distinction for2.3 are unchanged.
