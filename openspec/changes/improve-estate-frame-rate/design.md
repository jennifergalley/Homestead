# Design: improve the Estate frame rate

## How we measure

- Route: `Scripts\Test-Game.ps1 -EstateSmoke` (manor and woods timing windows, 12 s each), run by a
  perf-window wrapper so no other Unreal process or build runs. Settings copy Jenny's
  `GameUserSettings.ini` (Epic, `sg.ResolutionQuality=0`), with VSync and the frame limit off so the
  numbers show headroom: `-RenderScale 0 -ExtraExecCmds "t.MaxFPS 0, r.VSync 0, r.GPUCsvStatsEnabled 1, csvprofile start"`.
- Pacing numbers come from the engine CSV (`FrameTime`, `RenderThreadTime`, `GameThreadTime`,
  `GPUTime`, per-pass GPU stats) inside each window, not `PERFORMANCE_AT`, which samples mid-frame.
- Breakdowns: an Unreal Insights trace (`Trace.File <path> default` plus `stat namedevents`), exported
  with `UnrealInsights.exe -NoUI -AutoQuit -ExecOnAnalysisCompleteCmd="@=cmds.txt"` and
  `TimingInsights.ExportTimerStatistics` / `ExportTimingEvents` (render thread is `RenderThread 0`).
- C++ changes are compared in uncooked `-game` from the lane's editor build with a CVar A/B in the
  same binary, because lanes don't package; config-only changes are measured in the packaged build.

## Batch 1: scenery cells

Non-Nanite HISMs are always "dynamic" ray-tracing primitives (`FHierarchicalStaticMeshSceneProxy`
sets `bDynamicRayTracingInstances`), and their gather loops over every instance with a distance test
each frame. One estate-wide batch per kind meant the whole 4 km estate's grass was scanned from the
manor. Batching by cell lets:

- ray tracing drop cells more than 300 m away (`r.RayTracing.Culling.Radius`, box distance) before
  the scan (the ISM gather itself keeps only instances within 100 m, so nothing visible is lost);
- the renderer drop cells past the kind's own cull distance (`LDMaxDrawDistance` = cull + one cell,
  measured to the cell centre, so no instance that would draw is dropped).

Cell sizes: 128 m for kinds with a cull distance (45-140 m), 512 m for kinds drawn at any distance
(trees and rocks that aren't Nanite), so the far view stays a few draws per kind. Nanite kinds stay one
batch; Nanite culls instances on the GPU and its ray-tracing instances are cached. The per-batch arrays
(`EstateSceneryTransforms`, `Hidden`, clear radii) were already parallel per batch, so
`ClearEstateSceneryUnderPieces` needs no change. 378,560 instances become 2,358 batches (2,344 cells).

## Lanes and ownership

One lane (Performance Agent). Files: `HomesteadWorld.cpp` (`BuildEstateScenery` only in batch 1),
`Scripts\Test-Game.ps1` (perf switches). `AHomesteadWorld::Refresh` and the controller tick belong to
the Architecture Agent's `improve-code-health-between-rounds` step 2; refresh gating is coordinated
with it before either edits them.
