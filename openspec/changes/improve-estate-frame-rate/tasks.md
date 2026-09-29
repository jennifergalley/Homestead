# Tasks

## 1. Baseline

- [x] 1.1 Packaged 4K/1080p baseline with Jenny's settings, uncapped, CSV + Insights (see results)
- [x] 1.2 Report the monitor's 4K 30 Hz HDMI limit to the orchestrator for Jenny

## 2. Batch 1: scenery cells (render thread)

- [x] 2.1 Batch non-Nanite estate scenery per 128/512 m cell; `homestead.EstateSceneryCells` A/B switch
- [x] 2.2 Uncooked A/B at 4K and 1080p; screenshot comparison at the manor, clear-out, farm, woods, drive, store, night
- [ ] 2.3 Integrated: packaged EstateSmoke at 4K and 1080p on `main` (Integration Agent)

## 3. Next batches

- [x] 3.1 Game thread: gate `AHomesteadWorld::Refresh` on an integer key of its inputs; integer signatures (improve-code-health-between-rounds 2.2, reviewed by the Architecture Agent); PIE: plot rebuilds on time-only stage changes, clear pop plays
- [x] 3.1a Smoke `PERFORMANCE`/`PERFORMANCE_AT` sample the engine frame time (`FApp::GetDeltaTime`)
- [ ] 3.2 Manor occlusion queries (~1,300 per frame) and the RHI occlusion fence wait. Findings: they come from the ~900 per-node StaticMeshComponents (clear-out, farm, brambles), not the scenery HISMs (`foliage.MinOcclusionQueriesPerComponent 2` / `MinInstancesPerOcclusionQuery 1024` left 1,281); `r.HZBOcclusion 1` and `r.AllowOcclusionQueries 0` are slower. Remaining lever: batch per-node visuals into instanced components (larger change; the manor already renders at ~70 fps at 4K)
- [ ] 3.3 Woods GPU at 4K. Findings: ray-traced sun shadows 6.0 ms (any-hit on leaves ~40% of it; off = solid leaf shadows, rejected); camera-safe foliage `RayTracingQualitySwitch` no gain (6.02 → 6.03 ms, shelved); `r.TSR.History.ScreenPercentage 100` = woods 59.1 → 63.2 fps but slightly softer canopy (option with shots sent for Jenny, held; `compare/tsr-history`). No quality-neutral GPU win found yet
- [ ] 3.4 Walking hitches: grass-field chunk rebuilds, scenery hide pass on clears
- [ ] 3.5 Non-RT players: VSM first-frame GPU timeout (TDR) on the dense Nanite estate. The hang is Nanite `NodeAndClusterCull` in `RenderVirtualShadowMaps(Nanite)` on frame 2. Crashed: `MarkCoarsePagesDirectional 0`, `r.Nanite.Culling.TwoPass 0`, `ResolutionLodBiasDirectional 1`, and bias 3 + `Clipmap.LastLevel 18` applied from game code (once). Passed once: bias 3 + LastLevel 18 via `-DPCVars`. Not reliable yet; note Epic scalability also sets `ResolutionLodBiasDirectionalMoving -1.5`. Parked patch: `E:\CopilotScratch\a34483d7\vsm-guard-wip.patch`

## Results (EstateSmoke, CSV frame times inside the 12 s windows; uncapped)

| Build | Scene | Res | Mean fps | Median ms | p95 ms | p99 ms | Render thread ms | GPU ms |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| packaged `0ebd452d` | manor | 4K | 51.7-57.9 | 17.2-18.3 | 18.6-28.4 | 19.8-36.1 | 17.2-18.3 | 10.4 |
| packaged `0ebd452d` | woods | 4K | 59.2-59.7 | 16.7-16.9 | 17.9 | 18.3-18.4 | 16.7 | 15.4 |
| packaged `0ebd452d` | manor | 1080p | 60.0 | 16.5 | 18.3 | 20.1 | 16.5 | 7.2 |
| packaged `0ebd452d` | woods | 1080p | 69.2 | 14.3 | 16.0 | 16.8 | 14.3 | 9.8 |
| uncooked, cells off | manor | 4K | 55.9 | 17.8 | 19.1 | 19.7 | 17.8 | 10.4 |
| uncooked, **cells on** | manor | 4K | **69.1** | **14.3** | 16.1 | 17.7 | 14.3 | 10.3 |
| uncooked, cells off | woods | 4K | 59.6 | 16.7 | 17.9 | 18.4 | 16.7 | 15.4 |
| uncooked, **cells on** | woods | 4K | 59.8 | 16.7 | 17.7 | 18.1 | 16.7 | 15.4 |
| uncooked, cells off | manor | 1080p | 58.6 | 17.0 | 18.1 | 18.8 | 17.0 | 7.1 |
| uncooked, **cells on** | manor | 1080p | **69.7** | **14.2** | 16.0 | 16.7 | 14.2 | 7.0 |
| uncooked, cells off | woods | 1080p | 69.4 | 14.3 | 15.5 | 16.3 | 14.3 | 9.7 |
| uncooked, **cells on** | woods | 1080p | 71.0 | 13.6 | 17.5 | 23.9 | 13.6 | 9.8 |

The cells-on 1080p woods p99 comes from one 0.6 s burst where every thread slowed together (CPU
contention in the uncooked run), not from the scenery. The 4K woods are GPU-bound (batch 3.3).
`RayTracing_FinishGatherInstances` at the 4K manor: 4.5 ms → 0.8 ms.

### Batch 2: refresh gate (uncooked, 4K at Jenny's settings unless noted; before = cells on)

| Scene | Game thread p95 ms | Frame median / p95 / p99 ms | Mean fps |
| --- | --- | --- | --- |
| manor, before | 15.4 | 14.3 / 16.1 / 17.7 | 69.1 |
| manor, gated | **8.1** | 14.3 / **15.5** / **16.6** | 69.7 |
| woods, before | 14.9 | 16.7 / 17.7 / 18.1 | 59.8 |
| woods, gated | **8.0** | 16.9 / 17.9 / 18.6 | 59.1 (GPU-bound) |
| manor 1080p, gated | 7.9 | 14.3 / 15.3 / 15.9 | 69.9 |
| woods 1080p, gated | 7.9 | 12.8 / 14.1 / 14.7 | 77.4 |

GPU headroom (GPU ms vs 16.7): 4K at her settings (1080p internal + TSR): manor 10.3, woods 15.6.
4K native 100%: manor 24.8 ms (38 fps), woods 41.2 ms (24 fps), so native 4K isn't viable on this GPU;
her default (engine auto, 50% + TSR) is the right setting for 4K60.


## Handoff (2026-09-29, Performance Agent retired)

Future perf work: read-only assessment by the Architecture Agent, packaged profiling by the Integration
Agent. Method: `design.md` "How we measure" (perf window, `-RenderScale 0`, CSV + Insights).

- On `main`: batch 1 scenery cells (`0c12d5e9`), batch 2 refresh gate + smoke timing (`25ee51b4`, `84ee55d7`).
- On branch `jennifergalley-performance-agent` only (pushed, not merged, not measured):
  `6841f429` Blender blocks perf windows; `b1f117c1` EstateSmoke walking segments
  (`PERFORMANCE_AT walk-manor/farm/glade/drive/sprint`); `f8fcb52a` grass chunks built over frames
  (`homestead.GrassChunkBuildsPerFrame`, default 8, 0 = old). Merge only after an isolated A/B:
  `-ExtraArguments '-DPCVars=homestead.GrassChunkBuildsPerFrame=0'` vs default, walk-* p99 / max / over-20 ms,
  plus a look for grass pop.
- Shelved: branch `perf/woods-material-rtswitch-shelved` (camera-safe foliage visible to ray tracing; no GPU gain).
- Parked: `vsm-guard-wip.patch` in this folder (RT-off VSM limits; not reliable, see 3.5).
- Held for Jenny: `r.TSR.History.ScreenPercentage 100` (woods 59 → 63 fps, slightly softer canopy).
