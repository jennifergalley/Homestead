# Fern-first isolated import and material spike: bounded execution approved

Outcome19:07: actual replacement import passed with the ten named packages;
the single conditional render timed out in cold RHI shader startup before
commandlet entry. No image or world upgrade exists. See
`fern-import-result.md` and `fern-render-result.md`; all historical failures
remain preserved and no automatic retry is authorized.

2026-09-20,18:12 Arizona. Settings subgate sealed at11e1fc3.
Superseded at18:14 by the coordinator's explicit approval of1953fd7184c6e1acb9e25843571095afda31f827:
native implementation/targeted guarded build, ONE import and, only after its
actual inventory passes, ONE offscreen render. Exact150/180/210s import and
480/510/540s render bounds, per-operation existing-marker read protection and
exact fresh-trial quarantine are approved. No automatic retry, cook/Pak or
Shipping authorization. Historical proposed wording below is retained.
Original19:22 deadline and current Shipping/player/saves remain unchanged.

## Exact retained inputs

Root: `Assets\Source\woodland-preparation-20260920-182217-d1f84e39\fern_02`.
All six originals were read-only reverified against the existing receipt:
6823781bytes total. No acquisition or conversion is proposed.
Full hashes/URLs/CC0 credits remain in
`Assets\Environment\woodland-preparation-01\download-receipt.json`.

| File | Bytes | SHA256 prefix |
| --- | ---: | --- |
| fern_02_1k.fbx | 223596 | 6D8238B9CC1BBB5F |
| fern_02_diff_1k.png | 1986120 | F8F504BCE117647F |
| fern_02_nor_dx_1k.png | 3193783 | 864CAE047B66A8FF |
| fern_02_rough_1k.png | 374345 | 1485A265BCD40E5D |
| fern_02_ao_1k.png | 931348 | C31A5E1C543491816 |
| fern_02_alpha_1k.png | 114589 | EA3C64284C7EBDEED |

FBX7400 contains four independent root mesh nodes, each with the sole material
slot `fern_02`: a/b/c/d have source fan estimates784/2384/2248/816 triangles,
6232 aggregate, not6232 per clump. First rendered subject is the small a clump.
Source parents, geometry IDs, local transforms, unit scale1 and axis conventions
are already inventoried. Raw node scale100 and rotation approximately-90 are
not instructions to apply another guessed100x/rotation after UE conversion.
All five maps are1024x1024,16-bit PNG; diffuse/normal are RGB with no embedded
alpha. The separate grayscale alpha map spans0-65535.
Inventory reports no Texture/Video objects or external texture references.

## One implementation route, two bounded invocations

Extend the existing project-owned native commandlet with explicit fern-only
import and render modes, retaining its admission/settings/Python entry/exit
checks. Reuse the established targeted guarded compiler/link/metadata route.
No Python importer, old bootstrap, GUI Editor, UAT or whole-game rebuild.
Implement only after separate coordinator approval of this plan.

1. Import mode retains NullRHI. Use an explicit native UFbxFactory, automated
   static-mesh import, not generic extension routing through Interchange.
   Set combine=false, skeletal/animation/LOD import=false, generated collision
   off, material/texture auto-import=false, unit/axis conversion on and uniform
   import scale1. Retain four mesh identities and each original slot name;
   require exactly the four inventoried root nodes and their geometry binding.
   Preserve the source root-node graph and transforms in the trial inventory.
   Set bTransformVertexToAbsolute=true and bBakePivotInVertex=false: use the
   factory's supported global transform conversion once, explicitly record
   that bake and its source transform, and measure resulting bounds/pivots;
   do not claim local-pivot preservation merely from disabling mesh combining.
   Camera framing must not rewrite vertices to hide a transform mismatch.
2. Import the five exact PNGs individually with native texture factories.
   No FBX texture lookup, recursive directory search or missing-map substitution.
   Recheck the full hash/inventory whitelist before engine launch; reject any
   new FBX external/embedded texture or video reference instead of following it.
   Create one explicit masked/two-sided material: diffuse sRGB; DX normal
   normal-map compression with no extra green flip; roughness/AO/alpha linear.
   Connect separate alpha to OpacityMask, not diffuse alpha. First spike uses
   DefaultLit/two-sided with clip0.333; subsurface/wind/Nanite/LOD generation
   are deferred rather than invented from unavailable maps.
3. Save only the ten final trial packages after mesh/slot/texture/graph checks.
   Any factory-generated temporary names remain unsaved and are mapped to the
   exact final names before saving, without redirectors or SaveAllDirtyPackages.
   Reopen/inventory those trial assets in the subsequent render invocation;
   record actual imported triangles, sections/slots, bounds and texture formats.
4. Render mode removes NullRHI and uses the source-supported
   -AllowCommandletRendering switch (LaunchEngineLoop.cpp:2244-2247).
   Retain the current project renderer, no-worker in-process compilation,
   disabled Python, child-only SDK-validation skip and exact filesystem DDC.
   Build a transient native preview world with fern a, a neutral floor and
   explicit directional key/fill lighting; no existing map/game world is loaded
   or saved. Use a native SceneCapture2D/render target and PNG export to produce
   front and back/edge views at1280x720 after actual shader/mesh compilation
   completes. Record actual RHI/adapter, capture settings/show flags, dimensions,
   exposure and material/shader readiness; missing/default checker material
   fails the spike. These are rendered Editor previews, not packaged gameplay,
   4K/100 Batch A, GPU/VRAM performance proof or subjective artistic acceptance.

The exact factory path exists in UnrealEd Private Fbx/FbxFactory.cpp:
435-468 selects combination;605-624 recursively imports separate nodes and
registers all results. Full hierarchy for this particular file is four root
nodes, not an invented nested scene. Material slots are checked before binding.
The lower FBX SDK parser enables texture metadata at FbxMainImport.cpp:1446-1454,
so setting the UE texture-import flag alone is not a filesystem sandbox;
the verified reference-free source is an important admission condition.

## Exact trial outputs and non-overwrite rule

Fresh namespace: `/Game/Trials/Fern02_20260920_01`.
Require it and the physical directory to be absent before reservation.
The final package stems under `Content\Trials\Fern02_20260920_01` are:

- `Meshes\SM_Fern02_a`, `Meshes\SM_Fern02_b`, `Meshes\SM_Fern02_c`, `Meshes\SM_Fern02_d`.
- `Textures\T_Fern02_Diff`, `Textures\T_Fern02_NormalDX`, `Textures\T_Fern02_Roughness`, `Textures\T_Fern02_AO`, `Textures\T_Fern02_Alpha`.
- `Materials\M_Fern02`.

Only those stems with .uasset and, if actually emitted, their .uexp/.ubulk
companions are admitted. No .umap, accepted asset replacement or world integration.
Import evidence/config/TEMP/DDC live in
`Saved\Automation\20260920-182217-d1f84e39\fern-import-01`.
Render evidence and `fern-a-front.png` / `fern-a-back.png` live in the sibling
`fern-render-01`. Render may read the successful import's candidate-owned DDC;
its exact path is pinned and checked, never substituted with global/shared/Zen.
No bootstrap, other source asset, whole pack or2-million-triangle tree is used.

## Cancellation, hard-stop request and partial disposition

Proposed import budget:150s soft-stop,180s owned-job hard deadline,210s total
supervisor ceiling. Proposed render budget:480s soft,510s hard,540s ceiling.
Both also stop for live run pause/deadline; do not launch unless the whole
operation plus cleanup margin fits before19:22. These larger operation bounds
and asset-mode hard-stop permission require approval; the settings-only
hard-stop authorization does not silently carry over.

Use the same pre-start limit1/no-breakaway/KILL_ON_JOB_CLOSE job, exact inherited
marker lifetime, detached file stdio, executable/product/rule/profile checks,
TraceControl-only endpoint admission and observed-death-before-release rule.
No SDK/worker/network/DDC relaxation is needed merely to import this FBX.
Rendering necessarily needs a real RHI and commandlet rendering, not NullRHI;
cold in-process shaders may exceed its budget. Report that failure rather
than enabling helpers or silently skipping compilation.

Poll the stop/deadline marker before/after each input, package save and
compilation pump. Supply native feedback cancellation where UFbxFactory checks
FScopedSlowTask::ShouldCancel (for example500,596,614,986);
FFeedbackContext::ReceivedUserCancel is virtual atFeedbackContext.h:45.
This is cooperative at polling boundaries, not an interrupt of arbitrary
synchronous FBX parsing, texture compression, package serialization or shader
driver work. ShaderCompiler.h:1315 CancelCompilation removes outstanding jobs;
FinishCompilation/FinishAllCompilation block. StaticMeshCompiler.h:87-90
also explicitly documents blocking shutdown. Do not describe these as
instantaneous cancellation or release the marker while blocked.

For an approved hard-stop fallback: stop only the exact owned job, wait for
verified subject death, then compare protected originals and release the
marker. Retain logs, partial file inventory/hashes and failed/cancelled status.
Move the whole *explicit fresh trial directory*, after rejecting reparse points
and verifying its reserved identity, to
`Saved\Automation\20260920-182217-d1f84e39\fern-import-01\discarded-content`.
This removes partials from /Game without deleting evidence or allowing reuse.
On render failure, leave the completed ten import packages untouched; quarantine
only named partial images in fern-render-01. Never replace accepted content;
thus there is no accepted-package restoration step. Any unexpected write
outside the allowlist stops the run for review, not a broad cleanup/rollback.

## Admission requested, not executed

Request approval for this specific native implementation/build and first import
operation with the stated hard-stop/quarantine bounds; separately admit the
real-RHI render step after actual import inventory passes. No universal
cook/Pak supervisor is a prerequisite for this isolated spike. Full1.3,
remaining2.3 evidence, quality/performance gates, cooking/Pak and Shipping QA
remain incomplete, and no environment rollout is authorized.
