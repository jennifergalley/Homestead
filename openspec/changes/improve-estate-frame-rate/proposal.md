# Improve the Estate frame rate

## Why

Jenny wants the game to run at a steady, smooth 60 fps at 4K on her machine (RTX 5080 16 GB, Ryzen 7
3700X, 32 GB) with ray tracing on, without losing visual quality. The packaged build at `0ebd452d`
held about 57 fps at the manor at 1080p with 26-30 ms p99 "hitches", and players reported ~32 fps at
4K.

Measured first (packaged `0ebd452d`, EstateSmoke route, csvprofile + Unreal Insights, Jenny's
settings: Epic scalability, `sg.ResolutionQuality=0`, so 4K renders at the engine's default ~50% with
TSR):

- **Her monitor caps 4K at 30 Hz.** The Acer XB321HK is on HDMI 1.4, whose only 4K modes are 23-30 Hz
  (1080p gets 60 Hz). With VSync on, no game change can exceed 30 fps at 4K on that cable. Its
  DisplayPort input gives 4K 60 Hz and G-Sync. (Reported to the orchestrator for Jenny.)
- **The render thread (CPU), not the GPU, limits the manor.** 4K manor: render thread 17.5 ms median,
  GPU 10.4 ms. The biggest single item was `RayTracing_FinishGatherInstances` (3.6 ms): every
  non-Nanite estate-scenery HISM re-scans all of its instances every frame to gather ray-tracing
  instances (`FInstancedStaticMeshSceneProxy::GetDynamicRayTracingInstances`), about 13 ms of worker
  time per frame for ~340k grass, fern and flower instances in 40 estate-wide batches.
- **The woods are GPU-bound at 4K** (15.4 ms: ray-traced sun shadows through alpha-tested canopy
  6 ms, Nanite 1.8 ms, TSR 1.75 ms).
- **Game thread:** `AHomesteadWorld::Refresh` costs ~9.5 ms every 0.25 s. Hidden behind the render
  thread today; it also inflates the smoke test's `PERFORMANCE_AT` p95/p99 (the smoke actor samples
  after the controller tick), so the engine CSV's frame times are the reliable pacing numbers.

## Reuse research

- Engine facilities only: Unreal's ray-tracing primitive culling (`r.RayTracing.Culling`, 300 m) and
  primitive max draw distance already drop whole primitives cheaply; the per-instance scan is what
  costs. Splitting batches spatially lets those existing culls do the work. No new plugin or asset.
- Rejected after measurement: `r.HZBOcclusion 1` (slower: 44 fps at the manor), turning ray-traced
  shadow materials off (+10 fps in the woods but leaves would shadow as solid cards), dropping
  non-Nanite ISMs from ray tracing (their sun shadows would vanish).
- DLSS stays the documented fallback (`docs/research/rendering-baseline/README.md`).

## What changes

Delivered in batches, biggest safe wins first:

1. **Scenery cells (render thread).** Non-Nanite estate scenery is batched per kind and per 128 m cell
   (512 m for kinds drawn at any distance), with a whole-cell draw distance of the kind's cull distance
   plus one cell. Same meshes, instances, cull distances and shadows. `homestead.EstateSceneryCells 0`
   restores one batch per kind for A/B checks.
2. Next (planned): game-thread refresh gating (with the Architecture Agent's
   `improve-code-health-between-rounds` 2.2), occlusion-query count at the manor, and woods GPU cost.

## Smallest useful result and first playable demonstration

Batch 1: the manor and farm hold 60+ fps at 4K and 1080p in the packaged build; walking the estate
looks the same as before.

## Impact

- `Source/SurvivalGame/HomesteadWorld.cpp` (`BuildEstateScenery` only).
- `Scripts/Test-Game.ps1`: `-RenderScale 0` (player-default resolution), `-ExtraExecCmds`,
  `-ExtraArguments` for perf runs.
- No save, placement, content or bake change.
