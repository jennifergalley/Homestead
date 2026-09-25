# Rendering baseline upgrade (2026-09-25)

Change: `Config/DefaultEngine.ini` now targets D3D12 **SM6**, enables hardware ray tracing
(`r.RayTracing`), hardware-ray-traced Lumen and mesh distance fields, in addition to the MetaHuman
skinning settings. Before this, the game ran at feature level SM5, where the configured Virtual
Shadow Maps never activated and ray tracing was disabled.

`AHomesteadWorld::UpdateLighting` now rotates the sun and moon only when their direction changes by
more than 0.5°. Rotating a directional light invalidates every cached Virtual Shadow Map page. The
world refresh runs every 0.25 s, so continuous rotation forced a full shadow re-render every
~17 frames (paired 30-39 ms frames at 4K).

## Measurements

Route: `Scripts\Playtest-Visual.ps1 -PresentationDiagnostics` (Development `-game`, fresh
sandbox world at dawn, ordinary mapped idle/walk/turn/camera-sweep; game-thread wall intervals,
not GPU/Present timing). RTX 5080, Ryzen 7 3700X. Raw output is under `Saved\RenderingBaseline\`
(not committed); the analyzer JSON for each run is in this folder.

| Run | FPS | Median ms | p95 ms | p99 ms | Frames > 20 ms |
| --- | --- | --- | --- | --- | --- |
| 4K uncapped, before | 81.3 | 13.32 | 15.55 | 16.49 | 3 |
| 4K uncapped, after (continuous sun) | 78.0 | 10.91 | 29.25 | 38.17 | 146 |
| 4K uncapped, after (stepped sun) | **95.8** | 9.91 | 13.90 | 17.84 | 12 |
| 4K, 60 FPS cap, before | 59.8 | 16.66 | 17.55 | 17.95 | 7 |
| 4K, 60 FPS cap, after (stepped sun) | 59.7 | 16.67 | 17.60 | 17.96 | 9 |
| 720p uncapped, before | 106.0 | 10.25 | 12.85 | 13.50 | 1 |
| 720p uncapped, after (stepped sun) | 114.8 | 8.45 | 10.28 | 13.92 | 11 |
| 720p, 60 FPS cap, before | 59.8 | 16.67 | 17.58 | 18.01 | 7 |
| 720p, 60 FPS cap, after (stepped sun) | 59.7 | 16.67 | 17.56 | 17.95 | 8 |

Diagnosis runs at 4K uncapped (continuous sun): Lumen software RT, no instanced/HISM meshes in
ray tracing, and `r.RayTracing.Enable 0` all kept the spikes; `r.Shadow.Virtual.Enable 0` removed
them (median 14.71, p99 17.87). Uncached VSM costs ~25 ms per frame here (dense non-Nanite trees).
Every run logs the pre-existing "generated terrain/tree/cover inventory" FAILED line; it appears
before and after this change.

## Visual result and open items

`compare-4k-*.jpg` pairs the same route before and after. The upgrade adds real canopy shadows
(dappled light on the heroine and ground), contact shadows and bounced light. The old
auto-exposure, tuned for the shadowless look, clipped sunlit highlights at dawn and left deep
shade on nearby trunks almost black.

`compare-exposure-4k-*.jpg` shows before / upgrade with old exposure / upgrade with tuned exposure.
The tuning in `AHomesteadWorld::BuildLighting` and `UpdateLighting`:

- Local exposure: highlight contrast 0.5, shadow contrast 0.8.
- Highlight saturation 0.85.
- The dawn sun tint is reduced to (1.0, 0.9, 0.8). The sky atmosphere already reddens a low sun.
- Exposure bias is unchanged at -0.15.

Results:

- Dawn frames keep bark detail in shade, and the ground no longer blows out.
- Skin and the green dress no longer hue-clip to flat yellow.
- Pixels with a channel at 255 in frames 10/15 drop from 7.3/10.9% to 2.0/3.3%. The pre-upgrade
  baseline was 0.6/2.1%.
- Frame 3, with the camera pressed against the heroine in a 2°-elevation sun, is still bright:
  44.6% of the heroine region has a channel at 255, versus 56.4% untuned. No pixel has all three
  channels at 255.

Live PIE checks at hours 7, 9 and 12 clip ≤1.5%. Timing is unaffected: 4K uncapped with
exposure tuning is 99.3 FPS.

Rejected tuning:

- Exposure bias -0.45, AutoExposureHighPercent 95 and FilmShoulder 0.35. These made dawn darker
  and more orange without reducing clipping.
- Halving the dawn sun intensity. Auto exposure re-adapts, so it had no net effect.

Still open:

- Dusk and firelight checks.
- A packaged Shipping check with SM6/RT (task 2.2's packaged-log check and task 2.3).

## Smoothness (frame pacing), not just FPS

Jenny's bar is a smooth framerate, not just a high one. Per-pass game-thread pacing at 4K:

| Run | Walk p99 / max ms | Frames > 33 ms (all passes) | Other passes p99 |
| --- | --- | --- | --- |
| 60 FPS cap, before | 17.8 / 105 | 1 | ≤ 24.0 |
| 60 FPS cap, after (exposure-tuned) | 20.4 / 87 | 2 | ≤ 23.5 |
| Uncapped, before | 16.7 / 98 | 1 | ≤ 16.0 |
| Uncapped, after (exposure-tuned) | 30.2 / 96 | 7 | ≤ 19.2 |

- The single ~90-110 ms hitch at walk start is pre-existing; it appears before the upgrade too.
- Capped at 60, pacing matches the pre-upgrade build.
- Uncapped is less even after the upgrade. The walk pass p99 roughly doubles, and there are
  isolated 31-37 ms frames in the idle and sweep passes. The likely cause is VSM page updates as
  the camera and heroine move through dense non-Nanite canopy. That is open. Next steps: look at
  Nanite foliage or VSM cache tuning, and confirm in a packaged build with GPU/Present timing
  (these are game-thread intervals). DLSS (below) is the fallback if the GPU cost is the limit.

## MetaHuman heroine cost (2026-09-25)

Same route at 4K, run with `Playtest-Visual.ps1 -PresentationDiagnostics`. The MetaHuman runs used
`-MetaHuman`; the heroine is now the default, and `-LegacyHeroine` selects the prototype. Breakdown
from `csvprofile` with `r.GPUCsvStatsEnabled 1`, all medians.

| Run | FPS | Frame ms | Game thread | Render thread | GPU |
| --- | --- | --- | --- | --- | --- |
| Legacy heroine, uncapped | 98.5 | 9.7 | 3.5 | 9.7 | 8.3 |
| MetaHuman, uncapped | 77.5 | 13.4-13.7 | 6.0 | 13.7 | 11.0 |
| MetaHuman, hair simulation off | - | 12.4 | 5.6 | 12.4 | 10.2 |
| MetaHuman, `r.SkeletalMeshLODBias 1` | 88.5 | 10.7 (wall) | - | - | - |

- The frame is bound by the render thread.
- The GPU increase of ~2.7 ms is spread across:
  - Lumen screen probes on her (+0.37 ms).
  - Hair simulation in Niagara (+0.36 ms).
  - Strand interpolation and visibility (+0.45 ms).
  - Shadows (+0.27 ms) and subsurface (+0.14 ms).
- The game thread adds RigLogic and animation (~1 ms of parallel animation work) and groom ticks.
- `r.HairStrands.Enable 0` made pacing worse. Don't use it as a fix.
- Capped at 60, the MetaHuman runs were inconsistent: one run had 274 frames over 20 ms, a repeat
  had 22 (legacy has 10). A 256-sample Blender render from another session was running during the
  later runs, so repeat these on a quiet machine before tuning.
- Candidate fixes, tracked as OpenSpec task 4.4:
  - A LODSync minimum LOD at gameplay distance.
  - Groom LOD and simulation settings.
  - DLSS (below).

## Fallback if ray tracing gets too expensive

Jenny's back-pocket option: rather than dropping hardware ray tracing, integrate NVIDIA DLSS (DLSS 5
if a UE 5.8 plugin is available, otherwise the latest DLSS release). Its upscaling, frame generation
and ray reconstruction buy back GPU time and clean up ray-traced lighting. The RTX 5080 supports it.
It isn't needed yet: 4K uncapped runs at ~96 FPS. Revisit if the MetaHuman heroine, denser foliage
or packaged Shipping timings push 4K below the 60 FPS target.
