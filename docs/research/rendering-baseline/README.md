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
(dappled light on the heroine and ground), contact shadows and bounced light. The existing
auto-exposure, tuned for the old shadowless look, now clips sunlit highlights at dawn and leaves
deep shade on nearby trunks almost black. Exposure tuning is the next step (task 2.3 of
`replace-heroine-with-metahuman`). A packaged Shipping check is still pending.
