# Authoring workflow approval proposal

**Task1.3: BLOCKED / NOT APPROVED / NOT EXECUTED.** Static inspection on
2026-09-20, after accepted Shipping selection66066a1. This is not listener-free
authoring proof or permission to start import. Tasks1.1/1.2 are recorded in
`tasks.md`; all later tasks remain unchecked.

## Decision needed

The installed Editor and UnrealPak can listen on **all IPv4 interfaces** for
TraceControl. Existing inbound Allow rules include Private/Public, Any port and
Any remote address. Avoiding a new permission prompt does not remove that
exposure. No supported process/config switch inspected here disables their
compiled TraceControl implementation before bind.

The coordinator must explicitly decide whether a narrowly supervised authoring
exception for that exact listener is acceptable. Without that exception, the
current no-engine-edit/no-custom-engine/no-firewall-change constraints leave
task1.3 blocked. Do not redefine "offline-safe" as "a rule already exists."

Even with an exception, a **controlled preflight only**, not immediate asset
import, is needed to establish actual effective settings, child-process/socket
behavior and cancellation. Unattended console interruption is not graceful
in this installed source; the stop mechanism below has an unresolved packaging
decision. Neither gap is a reason to skip the gate or weaken later QA.

## Exact installed tools (read-only inventory)

All paths below are beneath
`E:\Program Files\UE_5.8\Engine\Binaries\Win64`. SHA256 values were measured
without executing these files.

| File | SHA256 |
| --- | --- |
| UnrealEditor-Cmd.exe | AE92F55952A3C9A7DEF90983FCF5AEDCE52D8EE8E4565FBD923985E9D9D3E05E |
| UnrealEditor.exe | 6F9C95C131F37CBDC384ADE02FCF6ED828BAA366B25234D0D85E212244CAC9AA |
| UnrealPak.exe | 566A036A3E221712C4A25060A8B5AEE1217D199FC41299D6B6F4DF34E47E90B0 |
| ShaderCompileWorker.exe | 0738648E0D1F2C012E14E5B644188D13B650F90C4E4911F5639BCC4C5B353285 |
| UnrealEditor-TraceLog.dll | 0759373042151249445A0260039DFA61E502DACD0E9347CDEFB26A0B6743160C |
| UnrealPak-TraceLog.dll | 142E5F7ED57BDFAAFE02F36A059FE7768304F5C8E4C1C76FC92F58BAD59539F2 |
| ShaderCompileWorker-TraceLog.dll | F5E788AA06B46149AE40742C5EB521CEF51EE9E8FA5B2C03496BF96480D37B6F |

Installed target receipts identify UnrealEditor as Development/Editor and
UnrealPak/ShaderCompileWorker as Development/Program. Editor and Pak TraceLog
DLLs contain the `Control listening on port` implementation string. The worker
DLL does not contain it; that is supporting static evidence, **not** a runtime
socket guarantee. ShaderCompiler.cpp:1687 launches workers with
`-communicatethroughfile`; ordinary non-tracing workers also receive
`-nothreading`. Do not assume these children inherit every parent option.

Proposal uses **UnrealEditor-Cmd.exe only**, not the interactive Editor window.
Shader workers are permitted children of one serial authoring operation only
after their exact identity is checked. No GUI editor, Development game,
UnrealInsights, UnrealTraceServer, Zen server, remote cook worker, asset-download
helper or account client is in the proposed runtime allowlist.

## Existing permissions are observations, not changes

Read-only `Get-NetConnectionProfile` reports active Ethernet6 as Private.
Exact-path `Get-NetFirewallApplicationFilter` / `Get-NetFirewallRule` /
port/address-filter inspection reconfirmed existing Enabled Inbound Allow
TCP and UDP rules for Editor, Editor-Cmd and UnrealPak: Private/Public,
LocalPort Any, RemoteAddress Any. No matching enabled inbound Allow rule was
returned for ShaderCompileWorker. No rule was added, edited or removed.

Before an approved attempt, recheck these exact paths, hashes, signatures/rule
state and active profile; changed identity/profile is a stop, not permission
to click Allow or broaden a rule. Check owned process identity by PID **and**
path/start time, including child identities, not a reused PID alone.

## Proposed process-local controls

`A` below means a fresh run-owned output under
`E:\Repos\SurvivalGame\Saved\Automation\20260920-182217-d1f84e39\authoring-preflight`.
Arguments are supplied as separate process arguments, not shell-concatenated
asset text. No persistent user/global config is changed.

Common Editor-Cmd policy:

```text
-notraceserver
-traceautostart=0
-unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput
-DisablePlugins=UdpMessaging,TcpMessaging
-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnabledByDefault=False
-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTransport=False
-ini:Engine:[/Script/UdpMessaging.UdpMessagingSettings]:EnableTunnel=False
-ini:Engine:[/Script/TcpMessaging.TcpMessagingSettings]:EnableTransport=False
-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=False
-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRunPipInstallOnStartup=False
-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bIsolateInterpreterEnvironment=True
-ini:EditorSettings:[/Script/UnrealEd.CrashReportsPrivacySettings]:bSendUnattendedBugReports=False
-ini:EditorSettings:[/Script/UnrealEd.AnalyticsPrivacySettings]:bSendUsageData=False
```

Add explicit `-UserDir`, `-EngineINI`, `-EditorSettingsINI`,
`-EditorPerProjectUserSettingsINI`, `-GameUserSettingsINI` and `-abslog` paths
beneath `A`, initialized only from reviewed defaults. Clear `UE_PYTHONPATH`
only in the child environment, inspect project/plugin startup Python paths,
and reject unexpected startup scripts or package-install requests. Python's
isolation setting does not remove project `Intermediate\PipInstall` site
packages: reject unreviewed prepopulated packages/startup hooks before probing.
No
`ForcePipInstallOnInit`, Messaging, TcpMessagingListen/Connect, trace destination,
Live Coding, account/marketplace, remote execution or networking arguments.

Use an explicitly **filesystem-only** DDC graph, not a reassuring graph name:

```text
-DDC=(Local=(Type=FileSystem,Path=E:\Repos\SurvivalGame\DerivedDataCache\Authoring-20260920,ReadOnly=false,Clean=false,Flush=false,DeleteUnused=false))
```

UE5.8 BaseEngine.ini:2747-2789 documents inline graph/store replacement.
`NoShared` still includes ZenLocal; `NoZenLocalFallback` still includes
ZenShared/Shared/Cloud. Neither establishes local-file-only behavior.
Verify the actual selected graph/store/path in the controlled probe before
allowing data writes beyond that disposable cache.

Source backing:

- TraceAuxiliary.cpp:2363-2379 gates separate server autolaunch with
  `notraceserver` and parses `traceautostart`; :2741-2770 shows automatic
  localhost connection is conditional on that latter value. These flags
  **do not disable the in-process listener**.
- UdpMessagingModule.cpp:311-361 checks support and transport/tunnel settings;
  TcpMessagingModule.cpp:268-292 and342-376 checks support/EnableTransport
  and explicitly requested endpoints. PluginManager.cpp:1585-1597 supports
  process-local `DisablePlugins=`.
  UdpMessagingSettings.h:34 and TcpMessaging's private Settings header:8
  both declare `config=Engine`.
- PythonScriptPluginSettings.h:48-128 defines Engine-config settings and
  interpreter isolation. PythonScriptPlugin.cpp:1361-1368 gates pip install,
  including the force-install override; PythonScriptRemoteExecution.cpp:644/679
  gates remote connections on `bRemoteExecution`.
- ConfigCacheIni.cpp:2317-2403 implements the `-ini:` syntax in supported
  non-Shipping configurations. Use one key per argument; quoted multi-key
  comma expressions have documented parsing limitations.
- EngineAnalytics.cpp:107 excludes commandlets from editor analytics runs.
  WindowsPlatformCrashContext.cpp:783-790 reads the stated privacy settings.
  Crash-report-client launch/early-failure behavior still needs explicit
  observation; no reporter or analytics child is silently admitted.

## Bounded local-file import, fresh cook and package sequence

These are proposed operations after approval, **not commands already run**.
Proposed maximums:120seconds for the settings/stop probe,10minutes per bounded
import batch,20minutes for a cook,10minutes for packing, and20minutes per
project-target build. Do not start a phase whose cap plus handoff allowance
crosses the live run deadline. Timeout handling is subject to the explicit
cancellation decision below; these are not permission for a hard kill.

1. Build only project Editor/Shipping targets as necessary; no engine rebuild,
   global macro override or installed-target naming change. Keep remote build
   execution disabled with source-confirmed UBT
   `-NoUBA -NoXGE -NoFASTBuild -NoSNDBS -NoArtifactReads -NoArtifactWrites`
   (BuildConfiguration.cs:53-86/:230-239), and monitor tool children.
   Review the actual build
   process list rather than assuming a compiler option covers every helper.
2. First run a no-import configuration/stop probe using the hashed Editor-Cmd
   and common policy. Export effective settings, enabled plugin inventory,
   DDC graph, config paths and process identity. No material, map or asset writes.
3. Only after that passes, use `-run=pythonscript` with one reviewed project-owned
   importer and an admitted local-file manifest. Import only explicitly
   receipt-verified FBX/images into a candidate namespace. No asset fetch,
   pip install, startup asset script, showcase map or unrelated character/audio
   reimport inside the editor. Do not invoke the broad existing bootstrap.
4. Fresh cook via the same hashed Editor-Cmd, `-run=cook`,
   `-targetplatform=Windows`, existing Homestead map, **`-SkipZenStore`**,
   `-CookProcessCount=1`, `-DisablePython`, and a fresh `-OutputDir=<candidate cook>`.
   CookCommandlet.cpp:335/:425-442 implements output override and SkipZenStore;
   CookOnTheFlyServer.cpp:6095-6124 parses CookProcessCount and disables
   multiprocess cooking at1.
   do not use cook-on-the-fly or a network file server. Exact final argument
   expansion/output platform layout is checked before launch.
5. UAT stages/packages the fresh cook for **Shipping**, with fresh archive/stage
   roots. `CookOutputDir`, `AdditionalCookerOptions`, `AdditionalPakOptions`
   and `AdditionalIoStoreOptions` are supported ProjectParams fields; both
   Pak and IoStore paths must receive the reviewed process policy.
   CopyBuildToStagingDirectory.Automation.cs:5373 routes IoStore to UnrealPak,
   not evidence that `-pak` alone forwards options everywhere. Reject Zen
   project-store metadata/pak streaming. Monitor hashed UnrealPak and preserve
   actual expanded child arguments. A short representative packing probe is
   required before full packing.
6. Existing `Build-Game.ps1 -ReuseCooked -ReusePakDirectory` remains appropriate
   only for unchanged cooked data. It **cannot** validate newly imported assets.
   Never reuse the accepted containers and claim they contain the new woodland.

## Expected versus unexpected network activity

Conditional exception requested: the owned Editor-Cmd or UnrealPak may have one
TraceControl TCP listener at1985, or its source-defined fallback32768-40959.
Control.cpp:57/109-123 chooses those ports; WindowsTrace.cpp:172-194 binds
address zero (all interfaces), not loopback. Multiple live tools could choose
different ports, hence serial top-level operations and owned-child accounting.

Expected otherwise: no owned UDP sockets, no accepted TCP connections, no
outbound connections, no web/Python/messaging/Zen servers. Any different
endpoint, established connection, unknown child, changed binary/config,
permission prompt, run pause/stop/deadline or timeout stops admission and
requests cancellation. An expected port alone does not identify a service:
correlate PID/path/log and preserve the bounded observations.
Editor privacy overrides are not a verified equivalent for every Program or
pre-config crash path. In particular, do not claim UnrealPak crash-reporting
is suppressed just because the Editor settings arguments were supplied.

Warm local socket providers before process start, record all owned TCP/UDP
endpoints from startup through stable operation and shutdown, and separately
record sampling gaps. This is not a packet firewall or proof that a very brief
connection cannot occur between samples. No network payloads, desktop captures,
microphone, unrelated windows or player processes are collected/manipulated.
A security prompt is a human/coordinator stop signal; do not click it or claim
unreliable automated dialog attribution as a verified safeguard.

## Cancellation gap requiring a decision

Do **not** call Ctrl-Break or unattended Ctrl-C graceful:
WindowsPlatformMisc.cpp:1127-1131 explicitly switches those cases to hard
termination. A window-close message is not a reliable commandlet stop route.

Proposed import path: a project-owned stop/deadline observer requests
`RequestExit(false)` and the reviewed Python importer checks a file marker
between bounded operations, finishing an in-flight asset save safely.
Cook exit must be tested with a disposable cook before real asset processing.
These mechanisms are not implemented or proven in this preflight.

UnrealPak does not inherit the game observer. There is currently **no verified
graceful remote cancellation path for its unattended packing operation**.
Coordinator decision is required: authorize an exact owned-PID hard-stop/
discard-partial-candidate fallback for that tool, or require a separately
verified graceful alternative before allowing it. No such exception is assumed.
If stopping a process is ever approved, affect only recorded owned identities;
never a player's process, a name-wide process group or an existing shared service.

## Shipping QA without restoring engine tracing

Keep standard Shipping engine objects and `UE_TRACE_ENABLED=0`. The accepted
startup probe proves only its own narrow save/Lit/window/socket checks.
Existing full-loop/renewal cannot currently be claimed runnable in Shipping:
controller spawning is guarded, both diagnostic actor constructors disable
ticks, some evidence uses Development logs/console or `PlayerInput::GetBind`.

Propose a project-owned explicit Shipping-QA gate with validated synthetic
save/config/user roots, simulated-input-only policy, timeout/stop marker and
file-based assertion/renderer results. Reuse existing full-loop/renewal logic,
enable its actor/tick path only in that opt-in mode, retain all gameplay/reward/
save/relaunch assertions and verify normal human launches have no actors,
input suppression or autoquit. Do not re-enable engine Trace, broad debug
bindings, logging build-environment overrides or unsupported Development checks.

Requested CVar100 and3840x2160 images are insufficient: export the actual view's
primary render fraction and Lit/lighting/complexity on the measured Shipping
frames, including after save/load. Screenshot-free CPU/render/GPU timing and
GPU-memory accounting need a Shipping-compatible project observer. Public
RHI declarations expose GPU-frame cycles, timestamp queries and texture-memory
stats, but availability/nonzero validity and scope are **not verified here**;
texture allocation is not total resident VRAM. Establish those baseline
observations before art work. Missing GPU/VRAM evidence stays unmeasured and
cannot satisfy a required budget by relabeling tick time or system RAM.

No design timing threshold, repeated-run count, two-batch visual limit,
normal-control route, old-save lifecycle, art provenance, audio preservation
or human-approval boundary is reduced to fit this workflow.

## Requested coordinator response

Approve/reject the exact known-listener authoring exception; decide the
unattended-packager cancellation requirement; and, if approved, authorize only
the bounded guard/settings/Shipping-QA preflight implementation and controlled
probes first. Until then task1.3 stays unchecked. No engine process, source asset
download, environment code/material change or additional automation has occurred.
