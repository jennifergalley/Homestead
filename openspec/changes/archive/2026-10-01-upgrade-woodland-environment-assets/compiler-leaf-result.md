# Compiler leaf: valid object, execution admission blocked

## Latest diagnosis: exact build-only console host

The separately authorized job-member diagnosis identified the persistent extra
member, rather than inferring its identity from a count. In
`job-member-diagnostic-01`, held-job enumeration reported root12508 and
`C:\Windows\System32\conhost.exe`, PID44380, creation FILETIME
134344211072488915, actual job membership true, running/exit259.
The follow-up `job-member-diagnostic-02` includes before/live/after lists:
root only before resume, root plus console host39480 during execution, empty
after cooperative exit. Both process identities were absent afterwards.
Four attempted application children still returned the fixture's required
CreateProcess failures, with no initializer witness.

The console image matches the already approved **build-only** identity:
SHA256 `E449BCE01F275CD08F3D4E64BB73B3B43AE845A0DBDB3E6131426E66537705E5`,
Valid Microsoft Windows signature, certificate thumbprint
`BAC13DF18B37E808208A39D3A54CCE975FAC8C1D`.
It is a real running OS console host, not a provisional application child.
CREATE_NO_WINDOW did not eliminate it.

This resolves the dummy's extra-member identity and fits the existing
build-only allowance. It does **not** retroactively identify the missing member
in the earlier compiler receipt, grant an Editor console-host exception, or
prove console-host network absence. The compiler has not been rerun.
The small job-member inspector is direct QueryInformationJobObject plus
limited-query process handles, not a new supervisor/trace framework.
Future compiler checks require every counted member to have an observed
PID/image/creation identity, allowing only its root plus the exact signed
build-only console host. Four data-only negative policy cases reject unknown
images, missing identities, changed creation identity and altered job limits.

## Authorization

The16:25 Arizona continuation authorized one tiny compile-only MSVC fixture
under the pre-start leaf guard, with an owned dummy marker. It did not authorize
linking, the five project compile actions, UBT, Editor or real-marker access.

## Actual result and correction

`compiler-leaf-01` ran exactly once. The hashed compiler exited0 and produced a
fresh1818-byte AMD64 COFF object with Z7 debug sections. The external symbol
`HomesteadCompilerLeafFixture` contains `B8DF9B5713C3`: mov eax,13579BDFh; ret.
The code was parsed, never executed. Object SHA256:
`F6602A4184ADC1602147F43A63E3F8BF39CFA03628B31DAC982E7D4499505311`.
Five malformed COFF variants were rejected by the data-only parser.

**This is not a helper-free execution pass.** Post-exit job accounting reports
TotalProcesses2, ActiveProcesses0, despite the configured limit1. The dummy
validation's full sequences show ActiveProcesses2 persisting after its denied
child attempts, not merely a transient creation sample. The extra member's
identity/execution status was not captured. Console plumbing is a hypothesis,
not an established cause.

An initial worker interpretation called the live count provisional and reported
the fixture passed. Full sequence review disproved the transient explanation;
that interpretation and the overall admission claim were explicitly withdrawn
to the coordinator before any subsequent launch. The original raw result's
`status: passed` is retained as historical incomplete classification, not
current admission. The derived receipt records the blocked result.

## Exact scope and observations

Compiler:
`E:\Tools\VSBuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe`

SHA256:
`FE251EF50A1545B1B0835EE17B1E785459712B38D79E45B5C1D3D28970A36619`.
Signature was Valid. Arguments were `/nologo /c /Z7 /O2`, an explicit fresh
object path and a copied four-line project fixture. CL/_CL_ were removed only
from this child's environment; no guessed telemetry switch was used.

Before first resume: exact held image/creation identity, flags8200, limit1,
active1, three explicit inherited handles. Marker was the fresh dummy file,
not ProgramData. CREATE_NO_WINDOW was added to avoid allocating a visible
console; it is **not verified to exclude console-host processes**.

Total observed operation:0.7712303s. One sample at698.0433ms found no
root-parent child, VCTIP, TCP or UDP. Maximum sample gap:698.0433ms.
These finite observations do not identify every job member or establish
continuous network absence. No child executable identity can be inferred from
the aggregate job counters. The dummy marker's bytes/identity/ACL were unchanged.

## Retained failures and helper checks

`leaf-guard-no-window-01` failed the original strict live-count check.
`leaf-guard-no-window-diagnostic-01` reproduced flags8200/limit1/active2/total2.
No compiler was launched before those failures were examined.

`leaf-guard-no-window-02` then recorded six lifetime/stop cases and six invalid
input rejections with live accounting instead of asserting live count1.
Its fixture rejected24 pre-main child requests, wrote no forbidden witnesses,
and passed the marker lifetime/release checks. Those checks are real but
**insufficient to classify the persistent extra job member**. The worker
should not have treated this diagnostic run as complete execution admission.

Strict live-count and no-unclassified-member checks are restored by default.
Accounting-only mode is now explicitly diagnostic and cannot report an overall
pass. Compiler acceptance also rejects the actual TotalProcesses2 receipt.
Previous raw failures, accounting sequences, compiler output and diagnostics
remain unchanged; no second compiler fixture was run.

## Proposed five-action continuation: NOT AUTHORIZED

The reviewed export contains these five compile-only actions:

1. `HomesteadAuthoringProbeCommandlet.cpp.obj`
2. `Module.SurvivalGameEditor.gen.cpp.obj`
3. `PerModuleInline.gen.cpp.obj`
4. `SurvivalGameEditor.cpp.obj`
5. `Module.SurvivalGame.cpp.obj`

If the coordinator separately resolves and verifies actual job membership,
execute each exact exported compiler action in dependency order with its
original response files, working directory and produced paths. Use a separate
owned dummy marker, fresh evidence/temp directory and pre-start guard per leaf;
retain all failed outputs and never synthesize success artifacts. Do not put
UBT orchestration inside a one-process job.

Full linking remains separate: exported DLL link actions use `/DEBUG:FULL`
and may need PDB/resource helpers. A tiny `/c /Z7` result does not admit those
helpers, prove the five real translation units compile, or clear metadata,
Editor, cook, Pak, import or Shipping QA.

Actual dummy-job member enumeration is now complete, as recorded above.
Next continuation must retain these identity checks while applying the
build-only exception; the Editor console-host boundary remains separate.
No extra launch is authorized by this report. OpenSpec remains4/29;
task1.3 is unchecked.
