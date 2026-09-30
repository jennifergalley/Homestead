## Why

Jenny's 2026-09-29 playtest: the shrubs' shadows move too much. Where it comes from:
- The estate's brambles, toyon hedge, deer brush and thimbleberry (scenery batches and pickable bushes)
  sway by world-position offset in `M_PropFoliage` (`WindStrength` 1.6, `LeafFlutter` 0.2, weighted by
  height squared).
- They cast shadows that follow the sway every frame when shadows are ray-traced.
- The camera-safe dither fades them near the camera.

The cause could be the sway, the shadows, the dither or the denoiser.

## What Changes

- Three console variables:
  - `homestead.ShrubWind` (share of the authored sway; default 0.4, 1 = as before, 0 = still);
  - `homestead.ShrubShadows` (0 = those shrubs cast no shadows);
  - `homestead.FoliageDither` (0 = camera-safe dither off, through `MPC_CameraSafeFoliage`'s radii).
- They apply to the scenery batches and the pickable bushes of those meshes. Each authored material
  gets one shared dynamic instance, and settings are re-applied only when a value changes.
- The default is the fix under test: the sway cut to 40% of what was authored, so the shadow tips move
  less than half as far. Shadows and dither are unchanged.

## Impact

- New `HomesteadWorldFoliageMotion.cpp`; small hooks in `HomesteadWorld.{h,cpp}`,
  `HomesteadWorldEstate.cpp` and `HomesteadWorldResources.cpp`. No saves or assets.
