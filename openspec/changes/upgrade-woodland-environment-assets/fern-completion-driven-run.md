# Explicit completion-driven fern continuation

Jenny's21:19 Arizona authorization removes overall guessed duration limits
and permits diagnosed corrective attempts, with environment acceptance before
inventory redesign. Parent owns `Automation\run.json`, its single check-in
and awake holder. Historical deadline is not rewritten or silently renewed.
The worker verified38 run/awake checks without creating an awake hold.

The previous renderer passed native entry after35minutes39seconds but reached
its immutable capture watchdog with no PNG. Its failure and missing exit
snapshot remain in `fern-render-long-01`; no live process was patched.
Only after observed death and unchanged-marker release did source change.

## Narrow implementation

`CompletionDriven` selects native mode Render and explicit guard profile
`RenderCompletionDriven` with zero soft/hard timer values, meaning **no time
timer is created**, not immediate expiry. Every other deadline profile
retains its exact prior values. The profile can be selected once before
resume only, requires this run's explicit continuation and live
`completionPolicy=until-complete`, and rejects non-render modes.

The native child receives `HOMESTEAD_PROBE_COMPLETION_POLICY=until-complete`
and no `HOMESTEAD_PROBE_DEADLINE`. The rebuilt commandlet explicitly branches
around time comparisons only in that mode; it does not use a far-future
date. Stop markers, genuine failures, shader/material readiness and exit
Python state checks remain. Normal bounded native paths retain their
90/180/510-second loop and absolute deadline comparisons.

Existing `LogFernSpike` adds elapsed phase markers for asset reload/inventory,
transient scene setup, asset compilation, material update, shader readiness,
each capture/flush/readback and PNG encode/write. These identify observed
boundaries, not guessed job progress, throughput or an ETA.

Manual pause/stop or changed policy triggers cooperative stop first, then
the existing exact owned-job cancellation if needed. Supervisor failure
closes its noninherited job handle and kills the root. Root-only pre-start
containment, inherited read-only marker lifetime, no helpers, detached stdio,
offscreen RHI, Python execution-disabled checks, local-only cache and
TraceControl-only endpoint observations are unchanged. The job remains
neither a filesystem nor network/broker sandbox; sampling limits are retained.

## Outputs and provenance

Continuation uses fresh current-run `fern-render-02` output/reservation.
All ten packages are hash verified; no reimport or source/transform/material
convention changes. The only mutable old-run path remains the explicitly
admitted `fern-import-02\DDC`; all1388 other old-run files are pinned.
The previous run's cache grew from102 to1577 files before capture timeout,
which is retained data, not a visual-success claim.

Genuine targeted builds use the already reviewed two compile actions,
standalone resource conversion, library/DLL linking and project metadata
route. No new export/helper is needed. New products and supervisor sources
have separate receipts; historical build and failed-render receipts are
never rewritten. Raw prior output remains available.

Progress remains4/29 until actual evidence satisfies specific tasks. No
world upgrade, packaged4K/performance result, player/save change or preview
promotion is claimed by this control transition.

## Actual completion-driven result and readiness correction

Current-run `fern-render-02` reached Main after50.1298243seconds and completed
the synchronous asset-compilation phase after1656.004seconds in Render. It
then exited7 with an actual cooperative shader-readiness failure, not a
timeout:1733.5203548seconds total,878 samples, maximum gap29082.3499ms.
Entry/exit Python checks passed, root death and unchanged marker release
were observed, cleanup errors were empty, and no PNG was produced.
`fern-render-completion-01` preserves the raw evidence and receipt; the
successful ten-package import and candidate cache remain intact.

The installed UE5.8 `GetMaterialResource` signature takes `EShaderPlatform`;
the existing `GMaxRHIShaderPlatform` argument is correct. The actual ordering
defect was the preview's unnecessary `PostEditChange` on an unchanged loaded
material **after** global compilation finished. Engine `Material.cpp`
(`PostEditChangePropertyInternal`,5350-5435) regenerates the shader-map key
and calls `CacheResourceShadersForRendering` with precompile mode `None`.
The log records the fern key changing from
`a5dd9577dc09687ad63e15b21fdecd01449e5dc8` to
`74c7ca7bed920c9ed5708f5270dd6b10f7612865` immediately before failure.
The old loop waited for submitted work but did not submit this new map.
Unrelated debug-material/Niagara compiler diagnostics are retained, not
asserted to be the fern's errors.

The correction leaves the loaded material unchanged and follows installed
`MaterialEditingLibrary.cpp:2116-2128`: submit resource compile jobs when its
map is incomplete, then `FinishCompilation`, retaining complete-map,
direct-error and global remaining-job gates. Actual resource/map states,
shader platform/feature level and direct fern compile errors are exported
before and after readiness. Synchronous compilation is still not claimed
instantly cancellable. No fallback, reimport or plugin disabling is added.

Only `FernSpike.cpp` is newly compiled. The existing genuine commandlet
object, import library and converted resource are reused; DLL/PDB and
metadata are freshly produced through the established guarded route.
Native03 receipt pins the resulting six products. Supervisor05 references
unchanged, hash-verified guard/argument/run-control fixtures and fresh19
policy negatives. Prior build-monitor qualifications remain explicit.

The distinct corrective reservation/output is `fern-render-03`; previous
results/reservations are pinned, not reset. It uses the same explicitly
authorized completion-driven controls and retained local cache. Actual
ValidateOnly and real capture must pass before claiming images or changing
the4/29 gate state.

## Real scene allocation and failed-frame evidence

Corrective `fern-render-03` verified the material fix: actual resource/map
present and complete, compilation finished, zero global jobs and zero
direct fern errors before/after. It then failed the unchanged content gate
after front readback (exit7,53.1234297seconds, no hard termination). The
zero-byte placeholder is quarantined, not an image. Python exit and marker
cleanup passed. `fern-render-readiness-01` retains this actual failure.

The next concrete defect is scene allocation, not material readiness.
`LaunchEngineLoop.cpp:3992` copies commandlet `IsClient` into `GIsClient`.
Our settings-oriented constructor always set it false.
`RendererScene.cpp:7123` creates a real `FScene` only when `GIsClient`,
`CanEverRender` and non-null RHI all hold; otherwise it returns the dummy
`FNULLSceneInterface`. Real D3D readback alone was insufficient proof.

Native04 sets the commandlet client role only for existing `FernMode=Render`
plus both existing offscreen/commandlet-rendering flags. Settings/Import
remain nonclient. This is renderer participation, not network-client
permission. Existing no-window/offscreen, no-network/helper, job/marker and
privacy restrictions remain unchanged. A dummy renderer scene is rejected;
actual fern registration/render-state/proxy and capture registration/
visibility are exported and required.

The installed `FPreviewScene::AddComponent` registers components with its
world; `USceneCaptureComponent2D::CaptureScene` pushes end-of-frame updates,
submits its scene render builder and executes it. The probe retains that
supported path plus an explicit render-command flush and successful native
readback. Camera position/rotation/FOV/near clip/look-at and transformed fern
bounds are recorded, without changing mesh transforms or source scale.

Coordinator additionally authorized separately named
`diagnostic-front-failed.png` / `diagnostic-back-failed.png` if successful
readback fails the green-pixel gate. These encode **unaltered readback RGBA**,
remain failed evidence and never satisfy acceptance. Pixel count, channel
min/max including alpha, formats and actual call/flush/readback results are
recorded before validation. PNG encoding/writing uses a fresh same-directory
temporary file and rename; no zero-byte final placeholder. Only the exact
named partial images may be quarantined. Accepted-view RGB remains native,
with the previously documented opaque-alpha encoding.

Native04 rebuilds both changed probe sources and genuine DLL/PDB/metadata,
reusing only the unchanged library/resource conversion. Supervisor06 has
26 negative image/asset-policy cases and explicit unchanged production
mode/token/deadline AST evidence, alongside unchanged guard fixtures.
Fresh reservation `fern-render-04` pins all prior failures and uses the same
retained cache. No actual image or broader environment acceptance is yet
claimed; progress remains4/29 before real verification.

## Actual images and registration-order correction

Attempt04 produced two genuine1280x720 PNGs on D3D12/RTX5080 with actual
scene/proxy/readback proof and9127/36063 green pixels. They remain failed
policy evidence in `fern-render-scene-01` (and raw `discarded-images`),
not accepted captures: component ScreenPercentage was true. The installed
`USceneCaptureComponent::OnRegister` calls `UpdateShowFlags`, which copies
archetype flags over setters made before registration. Motion blur was
false. The standalone renderer independently uses resolution fraction1,
but that does not retroactively pass the component-state contract.

The narrow native05 correction moves both existing flag setters immediately
after `Scene.AddComponent` registration. No image/content gate changes.
Fresh attempt05 preserves attempt04 and uses refreshed genuine native/
supervisor pins. Parent's latest preference favors integrated playable
milestones and disposable test saves; this is a local render correctness
fix, not a new infrastructure or save-preservation workstream.

## Verified result

`fern-render-05` passed the complete admitted preview/settings/stop contract:
cooperative native exit0 in65.723035seconds, no hard termination, no cleanup
errors, observed owned-process death before unchanged marker release.
Both native1280x720 PNGs were decoded and inspected. Actual D3D12/RTX5080
scene/proxy/material readiness, capture call/flush and921600-pixel readback
are exported; green counts are9127/36063 and both component
ScreenPercentage/MotionBlur flags are false. Python entry/exit remained
unavailable/uninitialized with direct CPython state0. Ten packages,
protected originals and1388 other old-run files stayed unchanged.

The supervisor recorded only5 samples with a maximum33146.0063ms gap.
Root-only pre-start job accounting is separate from finite endpoint/process
sampling; neither is represented as a general network/filesystem sandbox.
Native05/supervisor07 receipts and `fern-render-success-01` preserve the
actual result, images and previous failure chain.

Both views show actual cutout fern geometry without a checker/default
material. The front has very dark undersides, and surface detail looks soft
in the no-sky DefaultLit test setup. This is a renderer/import-pipeline
milestone, not polished world-art approval. No new cosmetic iteration,
package promotion or broad gate completion is inferred. Progress stays4/29.
