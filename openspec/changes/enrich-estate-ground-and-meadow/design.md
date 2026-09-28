# Design

## Decisions

1. **Meadow: instanced blade patches round the camera, not LandscapeGrass.** LandscapeGrass comes
   out empty in UE 5.8 PIE (see `build_landscape_material.py`), and it can't read the game's
   interactables. `UHomesteadGrassField` builds 6 m chunks of 2 x 2 m patches within 51 m of the
   camera from `EstateGround.bin`, pooling `UInstancedStaticMeshComponent`s. It re-picks chunks and
   LODs in `AHomesteadWorld::Refresh` (every 0.25 s). No shadows, distance fields or ray tracing:
   world-position-offset grass would invalidate virtual shadow pages every frame.
2. **The material does the look.** Each blade's root (UV0 fraction) and rank (UV0 whole part, full
   precision UVs) let `M_EstateGrass` sample the ground map at the root, thin blades by rank with
   distance (`GrassFade`: blades ranked below min(1, (12/d)^1.7) show), shrink them to nothing in
   cleared circles (per-instance custom data, three per patch), keep the road's wheel tracks clear
   from `T_EstateRoadSDF`, and bend them with a tiling gust texture plus a per-blade flutter. The
   heroine's position comes from `MPC_CameraSafeFoliage.HeroTargetPosition`.
3. **LODs never pop.** The patch LODs are nested by rank (all blades, then rank < 0.38, then < 0.14),
   and the component only uses a coarser LOD where the fade curve (with a 4 m margin for the refresh
   interval) already hides every blade it drops.
4. **Ground finish in the landscape material.** One Custom node after the layer blend, fed by
   `T_EstateGround` (grass density, height, dryness, wear) and `T_EstateCanopy` (tree cover, stony
   banks), both baked by `bake_ground.py`. The canopy comes from the scenery's broadleaf and fir
   records (kinds 0-1) in `EstateScenery.bin`, so re-bake after `scatter.py`.
5. **Footsteps by baked surface.** `EstateGround.bin` carries a surface byte per ~4 m cell. On
   Grass/Moor/Woodland the step plays through a transient 2D audio component with a low-pass
   (2.4-3.6 kHz) at 0.55-0.7 of today's gain.

## Risks

- GPU cost of dense WPO geometry at 4K. Mitigated by distance thinning, three LODs, no shadows and
  a 51 m radius; measured with `ProfileGPU` before and after.
- The meadow hides small things. Every `State.resources` node, world drop and plot gets a clear
  circle, and the most crowded patches are left bare.
