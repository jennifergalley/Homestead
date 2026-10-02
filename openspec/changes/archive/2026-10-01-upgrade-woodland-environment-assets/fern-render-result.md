# Fern offscreen attempt: cold shader startup exceeded its bound

2026-09-20,19:07 Arizona. The one authorized real-RHI attempt failed.
**No preview PNG exists.** The completed ten import02 packages remain
byte-identical; the existing game, character, landscape, music and saves
were not replaced. Progress remains4/29.

## Actual failure and cancellation

`fern-render-attempt-01.json` reserved the single operation. Source-supported
`-RenderOffScreen` and `-AllowCommandletRendering` replaced NullRHI while
all root-only/no-shader-worker/privacy/Python/DDC controls remained in the
command line and isolated files. Exact480000/510000ms deadline profile was
armed before resume, separately regression-tested, with540s supervisor budget.

The engine initialized D3D12/SM5, selected NVIDIA GeForce RTX5080 with
driver610.88, and began cold engine/global shader work against the exact
candidate filesystem DDC. Logs report missing shader maps for the standard
WorldGrid, deferred-decal, light-function and post-process engine materials.
Two FTSRRejectShadingCS permutations took33.388s and32.424s respectively.
Shader startup did not finish before the soft deadline. These observations
identify the blocking stage, not a complete remaining-job count or ETA.

Commandlet Main was never reached: no actual entry settings/Python snapshot,
fern reload/readiness result, SceneCapture or image was produced.
The successful import's settings proof must not be relabeled as a render
settings pass. The engine's WorldGrid startup compilation is not evidence
that a fern was rendered with checker material; no fern view was reached.

The watchdog wrote its cooperative stop request. Startup did not reach a
polling boundary that honored it, so the supervisor used the approved exact
owned-job hard fallback after timeout. Exit96 is **hard cancellation, not
graceful completion**. Full supervisor elapsed494.502389s, within540s.
The independent510s hard timer was not the mechanism that fired.

- Held root PID34992, creation134344294777039572, exact pinned Editor-Cmd.
- Final job flags8200, limit1, TotalProcesses1, ActiveProcesses0, empty members.
- Subject death observed; guard disposed; cleanupErrors empty.
- Existing global marker identity/bytes/timestamps/attributes/ACL unchanged
  after release; protected originals, native products and ten packages passed
  final checks.
- 251 process/endpoint samples, maximum gap9289.3027ms. Only admitted
  TraceControl was observed; no sampled UDP, connections or executed helper.
  Finite sampling is not continuous endpoint/broker history.
- The log's "1 local workers" describes a compilation slot, not an observed
  ShaderCompileWorker child. No helper exception was added.
- No partial PNG was created, so there was no image to quarantine.
  Completed import packages were not moved/deleted. Failed logs and
  candidate-owned shader/cache scratch remain retained, not promoted.

## Evidence limits and handoff

Adapter name, driver and feature level are actual RHI startup observations.
The logged15979MB dedicated-memory figure is hardware capacity, not measured
VRAM usage; texture-pool sizing is not usage either. There is no GPU frame
time, primary screen-fraction measurement, visual quality result, human
comfort assessment, packaged4K result or performance acceptance.

Receipt and raw launch/result/log evidence are under
`docs\research\environment-assets\fern-render-01`. First pre-resume import
failure and successful replacement import are preserved separately.
No automatic retry, worker/helper relaxation, shader skipping, longer
deadline, cook/Pak/Shipping launch or main-world integration followed.
The bounded slice returns a verified isolated import and this concrete
cold-startup/render blocker for coordinator review.

## Stopped-run checkpoint

Parent stopped runtime authorization at19:07; only evidence/checkpoint work
continued. Read-only final verification matched all ten packages, six
CC0 originals, six native products and234 operation-admitted protected
file pins. This is the render's admitted baseline, which includes previously
approved target/project/run-control changes, not the older pre-authoring
baseline. No source gate was weakened to hide that baseline distinction.

`Resolve-Preview.ps1` still resolves `offline-startup-01` Shipping, executable
SHA256 `CE5C9A262394E0B63E1105D71D4EBFB82CB61D4041FDC994618665F6FF29E634`,
profile `jenny-review`. No game was launched to perform this read-only check.
The original `Build\Windows` selection and normal saves were not changed.
No owned authoring process remains; no worker schedule was created.

| Evidence | Receipt SHA256 |
| --- | --- |
| `docs\research\environment-assets\fern-native-build-01\receipt.json` | `86DEE9CA2EA8CCC3CF6EC810F8F4967A60BBE249F3622032DEE3356D07821F6F` |
| `docs\research\environment-assets\fern-supervisor-02\receipt.json` | `7D930D038282AB18A14855EDA4E798B2847CC1DEAF7723F69B46B87099E9C761` |
| `docs\research\environment-assets\fern-import-01\receipt.json` | `EEF2F75CEE7DED1F4E0DADB503EE99DE5F501F4B78FD4995F89423C89C696C62` |
| `docs\research\environment-assets\fern-import-02\receipt.json` | `D65E8D93FF728A31AEA354979E22482723B583F71B8FFAEBF8287B9EB55DA66F` |
| `docs\research\environment-assets\fern-render-01\receipt.json` | `6FC9A25ACFD7C26BE240F7B97DA0457946122E55C691E5419E810B14193EEED1` |

The import02 `asset-admission.json` preserves the six original download
URLs/license/hash mapping, complete source transform graph and ten package
hashes. Existing repository `.uasset` Git LFS treatment was verified before
staging the binary candidate. No raw portrait or personal save data is added.
Strict OpenSpec validation and actual apply progress confirm4/29; the
renderer implementation is built but its SceneCapture path remains unverified.
