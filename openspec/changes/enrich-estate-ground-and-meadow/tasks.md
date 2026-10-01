# Tasks

## 1. Ground data

- [x] 1.1 `Scripts/Terrain/bake_ground.py`: grass density, height, dryness and wear, tree canopy and
  stony banks (T_EstateGround, T_EstateCanopy), the per-cell grass and surface file
  EstateGround.bin, gust noise and the three nested blade-patch LODs.

## 2. Meadow

- [x] 2.1 `M_EstateGrass` / `MI_EstateGrass` and imports (`Scripts/Terrain/build_ground.py`).
- [x] 2.2 `UHomesteadGrassField` round the camera, driven from `AHomesteadWorld::Refresh`.
- [ ] 2.3 Verify in PIE at eye level (pasture by the manor, the drive, wood edges), including clear
  circles round interactables and the derelict farm's fence line, and measure GPU cost before and after.
  _Look verified in PIE (eye level at the pasture, manor lawn, drive, wood edge and meadow, plus the
  heroine walking with the game camera): wind motion, grazed circles round stones and the road crown
  all check out. The GPU A/B still has to be taken with a single Unreal process on the machine._

## 3. Ground textures

- [x] 3.1 Finish pass in `build_landscape_material.py` with CC0 `grass_path_2` and `rocky_trail`.
- [x] 3.2 Verify distance tiling, trodden soil, leaf litter and stony banks in PIE.

## 4. Footsteps

- [x] 4.1 Softer, low-passed steps on grass, moor and woodland floor; other ground unchanged.
- [ ] 4.2 Jenny confirms the steps in the package.

## 5. Night and rain

- [x] 5.1 `MPC_EstateGround` (Wetness, Daylight), set every refresh by `AHomesteadWorld::UpdateLighting`:
  the ground wets through over the first half hour of the rain (spells of rain at any hour; see add-rain-weather 4.1) and
  dries over the next four hours.
- [x] 5.2 Wet response: the landscape's soil, litter and stone darken (turf less) and turn glossy,
  with standing-water sheen on trodden ground; the grass blades darken and gloss.
- [x] 5.3 No grass glow at night: blade transmission and the gust sheen scale with daylight.
- [ ] 5.4 Verify night and rain in PIE (and in the package, since agent editors run without ray tracing). _PIE checked at 22:00 and in the day-2 rain at 11:00: no grass glow at night, blades wet-dark and glossy in rain. The packaged look with ray tracing is still to check._

## 6. Open follow-ups

- [ ] 6.1 GPU cost A/B (meadow on/off) at 4K with a single Unreal process (the integration session
  takes it on the package).
- [ ] 6.2 (canopy_mask now reads every tree kind and crown radius from `SCENERY_TREES` in Scripts/Map/bake_estate_map.py.) Re-bake `bake_ground.py` + `build_ground.py` whenever `scatter.py` regenerates
  `EstateScenery.bin` (the canopy mask reads its trees) or the heightfield or layout change.
- [ ] 6.3 Jenny playtests the meadow density and height, the softer steps, and the wet and night looks;
  fold her notes into `MI_EstateGrass` (`GrassShape`, `GrassFade`, `GrassWind`) and the ground-finish parameters.
- [ ] 6.4 Decide whether LandscapeGrass is still wanted now that the meadow is runtime instances (it
  stays off; `LANDSCAPE_GRASS` in `build_landscape_material.py`).
