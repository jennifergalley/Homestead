# Native objects complete; resource/link/metadata approval proposal

## Current verified outcome

All five inspected compile actions ran with their original argument tokens,
response files, dependency lists and produced paths. All exited0 and their
15 real outputs have fresh timestamps and measured hashes. The five objects
are AMD64 bigobj COFF, not renamed or fabricated completion markers.

Actions0/1/2/3 passed the compiler supervisor. Action5's compiler exited0, but
its original monitor failed while querying the **already identified root**
at shutdown. Root PID48580/creation134344220853502688 had been observed
throughout; the failing image query returned access-denied after compilation.
That failed receipt remains failed. Subsequent read-only output verification
does not turn its incomplete monitoring record into a full pass.

The root observation now uses its retained launch handle and verified image/
creation identity; actual liveness still comes from a kernel wait and exit-code
query. It does not suppress access-denied for extra members. The existing dummy
fixture's exit regression confirms cached identity with actual `Exited=true`,
exit0, and an empty post-exit job list. Previously identified members that
vanish with OpenProcess error87 are explicitly retained as observation losses;
unidentified vanished members still fail admission.

The separate compiler confirmation captured actual cl48916 and signed
conhost16660, three empty IPv4/IPv6 TCP/UDP observations, and a valid1866-byte
fixture object. No VCTIP/unknown member was observed. This is finite sampling,
not packet/continuous proof. Earlier incorrect interpretations and failures
are preserved in `compiler-leaf-result.md`.

OpenSpec remains4/29. The native DLL is not linked and no Editor probe ran.

## Exact remaining tools

MSVC executables are beneath
`E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64`.
All signatures below were Valid when inspected.

| Tool | SHA256 | Role |
| --- | --- | --- |
| link.exe | A364AF801A8539E4324D9489313DBF001D959128451FC99A27B24676BBAC058F | Import libraries and two DLL/PDB links |
| cvtres.exe | B5DA94E7B9FF60B388EA9013D9E0E3D4A2A68BFF7C668C7017583459DAC4E3C5 | Possible resource-to-COFF helper; inventory is not execution approval |
| mspdbsrv.exe | 8FFBACFDBEF90A79E8441D640BC8B4C86BE02A2D9035CE87C5FA4F36DC3C70E1 | Possible PDB service; inventory is not execution approval |
| SDK26100 x64 rc.exe | 43DA1503C262C30894C851589BF0155F8365D77E63A5F7BC13982320E3A6B42D | Project resource output |
| Installed UE dotnet.exe | 0AF909A3DB0C02BD736F3008E2A9B20E7BA4E87EF27DD35619A7A9EC588191A8 | Direct WriteMetadata host only |
| Installed UnrealBuildTool.dll | A513EE9E22291D8827C5390C1ED5BBAD7AC564DA6CF82F3BD57B4E27509E7182 | Direct WriteMetadata mode only |

RC path:
`C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe`.
Dotnet path:
`E:\Program Files\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe`.
UBT path:
`E:\Program Files\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll`.
The existing exact signed conhost allowance remains **build-only**.

## Execution order and produced paths

`I` below means
`E:\Repos\SurvivalGame\Intermediate\Build\Win64\x64\UnrealEditor\Development`.
`B` means `E:\Repos\SurvivalGame\Binaries\Win64`.
The original reviewed export remains SHA256
`10BD045C4AEAD4344ECAECF061DBD65BBA94224DDA478D03F296ED564F5F10FA`.

| Action | Exact operation | Dependencies | Expected outputs |
| --- | --- | --- | --- |
| 8 | SDK rc.exe with the original exported command, working directory `E:\Program Files\UE_5.8\Engine\Source` | Original engine Default.rc2 and project icon; no compile-action prerequisite | `I\SurvivalGameEditor\Default.rc2.res` |
| 4 | link.exe `/LIB` with `I\SurvivalGameEditor\UnrealEditor-SurvivalGameEditor.lib.rsp` | 0,1,2,3 | Same-directory `UnrealEditor-SurvivalGameEditor.lib` |
| 7 | link.exe `/LIB` with `I\SurvivalGame\UnrealEditor-SurvivalGame.lib.rsp` | 5 | Same-directory `UnrealEditor-SurvivalGame.lib` |
| 6 | link.exe with `I\SurvivalGame\UnrealEditor-SurvivalGame.dll.rsp` | 5; existing declared shared-PCH object and resource input must also exist/hash-match | `B\UnrealEditor-SurvivalGame.dll` and `.pdb` |
| 9 | link.exe with `I\SurvivalGameEditor\UnrealEditor-SurvivalGameEditor.dll.rsp` | 8,0,1,2,3 | `B\UnrealEditor-SurvivalGameEditor.dll` and `.pdb` |
| 10 | Direct UBT WriteMetadata, only after all links and products are real | 6,9,4,7 | `B\SurvivalGameEditor.target`, `B\UnrealEditor.modules` |

The DLL responses use `/DEBUG:FULL`, `/NOIMPLIB`, `/NOEXP`, and
`/INCREMENTAL:NO`, and include `.res` input. The `/LIB` responses use `/DEF`
and actual compiled objects. Do not drop PDBs, alter response files, preconvert
resources, synthesize manifests or rewrite ProducedItems under this proposal.
Any tool-created ancillary `.exp`/diagnostic output must be reported, not
substituted for an expected product.

## Helper/PDB boundary and concrete decision

**Recommended decision:** authorize actions8/4/7 and then the two exact DLL
link attempts, serially, with the existing pre-start limit1/no-breakaway guard,
only the exact owned build-console exception, fresh dummy marker/temp/evidence,
live job-member identities, IPv4/IPv6 endpoint sampling, live run/deadline
checks and owned-job cancellation. Rehash original responses and prerequisites
before/after each action. Preserve existing project editor binaries before an
authorized overwrite; never touch the selected Shipping package or saves.

The debugger/PDB and resource-helper requirement is not yet verified.
The installed linker contains the diagnostic
`cannot connect to or launch MSPDBSRV.EXE`; both DLL recipes request full PDBs.
Resource conversion may need cvtres. These are specific possible failures,
not permission to let those application helpers execute.
No PDB server, converter or Editor process was present at the boundary check.
An unowned existing server appearing later is an admission change, not a free
way around the guard.

If either link needs a blocked application helper, preserve its exact
diagnostic and stop: do not raise the process limit, prelaunch a guessed server,
weaken debug/output contracts, permit VCTIP, or convert files manually.
The coordinator must then approve a specific helper route. This controlled
attempt may therefore stop honestly; current source inspection does not prove
that full PDB/resource linking is child-free.

## Metadata details and explicit argument delta

The input is
`E:\Repos\SurvivalGame\Intermediate\Build\Win64\x64\SurvivalGameEditor\Development\TargetMetadata.json`,
SHA256 `2550557136B7AE6F947A2746AFFBE1FEC6538735D296EB7E342B549120294924`.
Receipt BuildId is55116800. Its `Version` is null; its `VersionFile` points at
installed UnrealEditor.version but is not a write target in this input.
Installed `WriteMetadataMode.cs` writes that file only when both Version and
VersionFile are non-null. Recheck this condition immediately before execution.
The sole module manifest and receipt paths are project-local; load-order map
is empty. Installed UnrealEditor.version SHA is
`2C94D8C30DF424622504FF9CBEBB1A6DE62E3083BEE0910394DDF4BDE0A3840F`.

The exported metadata command carries a session identifier from the finished
UBT export process. Request explicit approval to invoke the supported mode
**without reusing or fabricating `-Session`**:
installed dotnet + installed UBT DLL + `-Mode=WriteMetadata`
`-Input=<the exact input above>` `-Version=2`, with a fresh normal log path.
This is a new real UBT invocation, not the entire build or a fake tracing
session. Previously approved ordinary per-user UBT diagnostics may occur.
DOTNET_CLI_TELEMETRY_OPTOUT remains set. Do not claim a one-process restriction
on overall UBT orchestration; this proposal covers only the metadata mode.

Approve metadata only after actual DLL/PDB/import-library validation, with
the input hash/output-map checks above and no installed-engine write.
No Editor, global marker, cook, Pak, import, Shipping-QA or environment
implementation is authorized by this resource/link/metadata proposal.
