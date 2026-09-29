# Tasks

## 1. Baseline

- [x] 1.1 Packaged 4K/1080p baseline with Jenny's settings, uncapped, CSV + Insights (see results)
- [x] 1.2 Report the monitor's 4K 30 Hz HDMI limit to the orchestrator for Jenny

## 2. Batch 1: scenery cells (render thread)

- [x] 2.1 Batch non-Nanite estate scenery per 128/512 m cell; `homestead.EstateSceneryCells` A/B switch
- [x] 2.2 Uncooked A/B at 4K and 1080p; screenshot comparison at the manor, clear-out, farm, woods, drive, store, night
- [ ] 2.3 Integrated: packaged EstateSmoke at 4K and 1080p on `main` (Integration Agent)

## 3. Next batches

- [ ] 3.1 Game thread: gate `AHomesteadWorld::Refresh` and integer signatures (with the Architecture Agent)
- [ ] 3.2 Manor occlusion queries (~1,300 per frame) and the RHI occlusion fence wait
- [ ] 3.3 Woods GPU at 4K (ray-traced shadow any-hit on canopy, Nanite, TSR) without visible change
- [ ] 3.4 Walking hitches: grass-field chunk rebuilds, scenery hide pass on clears
- [ ] 3.5 Non-RT players: VSM first-frame GPU timeout (TDR) on the dense Nanite estate

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
