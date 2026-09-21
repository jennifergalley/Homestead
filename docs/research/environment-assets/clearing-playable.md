# First playable fern clearing

Run `20260921-033354-2d257ba0`; candidate
`Build\Releases\20260921-033354-2d257ba0\clearing-02`.
This is a basically usable integration increment, not the full woodland
upgrade, revised hair/UI, 4K/performance acceptance or Jenny's art approval.

## Actual game, not an isolated asset scene

The existing ordinary route passed in the genuine Shipping executable:
302 real 1280x720 frames over 43.2728 sampled seconds. It walked slowly/fully,
turned/stopped, orbited, opened the book/look screen, approached a real berry
bush, gathered it and returned from the picking action. No teleport, direct
simulation action, time edit, desktop capture or player-window input was used.
Fresh disposable QA saves are separate; no existing preview save was reset.

Full route:
`Saved\VisualPlaytests\20260921-033354-2d257ba0\clearing-01`.
Representative originals are `Frames\frame-00025.png` (clearing) and
`Frames\frame-00285.png` (after actual gathering). Exact copies are retained
in `clearing-gameplay-01\clearing.png` and `after-gather.png`.
The existing reviewer produced `review.html`, frame telemetry and limited
motion diagnostics. Eight-Hz capture/readback is not game FPS or smoothness
proof.

Inspection of those views and the moving/orbit view at frame00130 found
recognizable small fern clumps, with very bright fronds in strongly warm dawn
light. Cone/sphere trees, pointed grass and the old joined heroine hair still
dominate the composition. The new foliage is a subtle increment, not a large
woodland transformation. No normal-control obstruction was observed on this
route; this is not exhaustive placement, collision or day/night coverage.

Actual RHI was D3D12; viewport and output target1280x720, window mode2,
offscreen. Saved resolution remained1920x1080; `r.ScreenPercentage=0` retained
the normal default/automatic policy. No internal-upscaler-input-resolution,
physical scanout, visible borderless or human-controller-comfort claim follows.

## Real code, cook and stage

The exact candidate executable is
`Windows\SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe`,
SHA-256 `A61B7E85F3FC579C8555762A1971FDFB801A9BF7008A464BE246FEA0C108025A`.
`clearing-shipping-build-03` holds genuine compile, link, required manifest,
PE/PDB inspection and UBT target metadata. The narrow explicit Shipping QA
gate defaults off and enables neither debug console nor trace. Native admission
reported `shipping=1`, `trace_compiled=0`, route visual and isolated save/UserDir.

The completed `clearing-cook-02` used actual Windows Cook with `SkipZenStore`,
one process, no worker/helper, filesystem DDC and no captured cook errors.
It exited cooperatively; marker cleanup/protected-content checks passed.
Exit Python was unavailable/uninitialized, direct loaded-library query0.
These are snapshots and source/log evidence, not continuous interpreter tracing.

The QA delta changes code gates, not content/config/reflected declarations.
The matching completed cook was therefore reused rather than recooking.
`clearing-stage-02` used standard installed managed UAT loose-file staging with
actual `Pak=False`, `Build=False`, `SkipCook=True`, `SkipIoStore=True`.
Fresh runtime map/packages, all ten fern assets, registry and shader files
match the real cooked inputs; the staged executable matches the current code.
No old containers, invented metadata, broad engine copy or Pak helper was used.

The first stage's UAT itself exited0, but our postcondition wrongly required
`Metadata\ShaderLibrarySource` in the runtime package. Installed staging source
explicitly excludes cook metadata (CopyBuildToStagingDirectory lines1715-1721).
That wrapper result remains failed. A distinct corrected stage validates runtime
shader archives/global cache and reads parameters from UAT's full `Logs\Log.txt`,
not its abbreviated console output. Original Zen-cook/export/link/manifest
failures also remain preserved; see `clearing-shipping-build.md`.

## Guard result and reuse

The actual Shipping route took57.331 seconds including startup/exit. There were
185 endpoint samples, all zero TCP/UDP, maximum gap0.913 seconds. Final job:
limit1, total1, active0, flags8200. Exit0, observed death, no hard termination,
guard disposed and no cleanup errors. The existing empty global marker's
identity, bytes, timestamps/attributes and security hash were unchanged after
release. No authoring TraceControl exception applies to Shipping.

The control primitive is the already-proven pre-resume job/inherited-handle
guard, not a filesystem/network/broker sandbox. Finite observations cannot
prove no brief socket existed between samples. The completion path passed;
do not relabel this as a separate runtime cancellation stress test.

`clearing-gameplay-01\receipt.json` pins actual evidence, with raw outputs
retained in their original Saved directories. Relevant checks:52 preview/
configuration/fresh-QA guards,34 negative fern/cook policy cases, four native
manifest tests, genuine Shipping compilation and strict OpenSpec validation.

Normal Preview selection may use this qualified usable candidate while
retaining the previous offline-startup candidate for cheap rollback. Normal
launch has no QA flag, test actor, forced action or automatic quit. Continuing
priorities are the ready waves-hair comparison in game, the remaining woodland
palette and the separately implemented UI. None is claimed complete here.
