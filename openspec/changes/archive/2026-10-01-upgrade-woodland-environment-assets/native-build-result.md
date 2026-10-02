# Native build milestone, not runtime admission

2026-09-20,17:30 Arizona; run20260920-182217-d1f84e39. Original19:22 deadline
unchanged. OpenSpec is4/29, task1.3 unchecked. No Editor, real marker lock,
asset/material/map operation, cook/Pak, Shipping QA or visual upgrade occurred.

## Real products

| Product | Bytes | SHA256 |
| --- | ---: | --- |
| UnrealEditor-SurvivalGame.dll | 2093056 | BB967BC6472321597D6E13293A9FB7C89F957091BC113C619538C8868DA2178A |
| UnrealEditor-SurvivalGame.pdb | 90222592 | 698B565378F305A9343D16F92619F12CC50780DDD0BF6D10E21B56138542C367 |
| UnrealEditor-SurvivalGameEditor.dll | 138240 | A945DC9EF20A325B22419D3BF8D04BCC1DDD8A6A62CAEDAE06BC2C7F6CBCE125 |
| UnrealEditor-SurvivalGameEditor.pdb | 9457664 | DFB12BE0009BBD6CA9A842DAC30EA429BB5287ECDC055B62FFAA8A4023A2935C |
| UnrealEditor.modules | See receipt | DCB6A669262FBC473587F5C1B731F57AB7B1FCB7B088CCDAE827DFC6EE07425A |
| SurvivalGameEditor.target | See receipt | 8A39936362B32F26E506BE8FDAD9D0681C5902BB2A7B79C1ADF1C4A2661CF2C1 |

Products are in project `Binaries\Win64`, not the selected Shipping release.
Data-only inspection confirms AMD64 PE32+ DLLs, actual module names and
`ThisIsAnUnrealEngineModule` exports. The probe exports its generated
`Z_Construct_UClass_UHomesteadAuthoringProbeCommandlet` symbol.
UE5.8's IMPLEMENT_MODULE uses FModuleInitializerEntry; requiring an exported
InitializeModule function would incorrectly apply an older loading contract.
No DLL was loaded to inspect these facts.

CodeView RSDS GUID/age matches the actual MSF/PDB identity stream:
game `edb807fb-181c-4b0f-8825-84f936ad7a3f`, age2;
probe `a177f0b3-cf17-479c-be4d-278b9c6b4137`, age1.
The new data inspector passed one known matching fixture and eight malformed/
mismatched cases. Existing member/COFF tests passed three actual-data positives
and five negatives; the resource parser passed one read-only fixture and five
malformed cases. Eighteen existing endpoint/DDC policy cases also passed.
This is product validation, not proof the commandlet ran.

## Preserved sequence and monitoring limits

RC8 and import-library4/7 passed. Original DLL6 failed LNK1158 because the
cvtres application child could not execute. Its deletion of the prior runtime
Editor DLL was repaired from the verified pre-link backup; that restoration
was explicitly an old product, not a successful build.

The first separately approved converter exited0 with valid COFF but its
short-lived extra member was unidentified. Its failed admission and original
output remain unused. Six existing-fixture lifecycle scenarios then passed
with DETACHED_PROCESS: flags0x0008040C, suspended pre-start assignment,
limit1/no breakaway/KILL_ON_JOB_CLOSE and exactly the marker/NUL/file-stdio
whitelist. Twenty-four application-child attempts were denied. All five
available final job accounts contain TotalProcesses1/ActiveProcesses0; abrupt
controller death instead has independent subject-death/guard-release proof.
All six release checks and six invalid-input cases passed.

Fresh game/probe converters passed with exact approved executable/input hashes
and real read-only resource COFF. Each original DLL RSP was preserved; a fresh
derived copy changed exactly one quoted resource-input token to its verified
object. No debug/PDB/output option was removed.

Derived game link6 exited0 and produced the real DLL/PDB above. Its monitor
still **failed** querying already identified conhost19284 at shutdown
(access-denied image query). Earlier samples record exact console image and
creation identity; no unrelated/unidentified extra process was observed.
This receipt remains failed, not suppressed, rerun or retrospectively passed.
The earlier compile-action5 root-shutdown monitor failure also remains.

Derived probe link9 passed under the explicitly approved build-only detached
mode: root-only final job accounting and nine empty endpoint samples. Direct
WriteMetadata passed/exit0 in0.8611896s; sampled orchestration identified the
exact allowed conhost48316 and one empty TCP/UDP observation. The metadata
mode is not described as a leaf-job execution. Its input/tool hashes matched,
Version was null, only project manifest/receipt maps were writable, the stale
Session argument was omitted, and installed UnrealEditor.version remained
SHA2562C94D8C30DF424622504FF9CBEBB1A6DE62E3083BEE0910394DDF4BDE0A3840F.
UBT reported its already-approved user-local Trace.uba diagnostic.

Empty sampled endpoints are not continuous absence or broker isolation.
The linked outputs are usable build products, not an entirely clean monitored
build. A prior derived endpoint rollup using `@($result.samples.endpoints)`
counted a null aggregate as1; the new index enumerates each actual array.
Raw endpoint samples remain unchanged and empty.

## Persistent evidence and next boundary

`docs\research\environment-assets\guarded-link-01\receipt.json` indexes the
original success/failure/launch records, detached lifetime proof, exact
derived recipes, product identities and real metadata output hashes.
No native executable/library/raw asset is committed as evidence.

The coordinator requested one bounded read-only admission check before any
real120s settings/stop probe. The existing wrapper still needs explicit pins
to these project product identities and resolution of its CreateNoWindow
mode versus strict root-only job accounting. The build-only detached result
does not itself authorize an Editor mode change. Historical receipts remain
unchanged; selected Shipping/game/saves/configuration are protected.
