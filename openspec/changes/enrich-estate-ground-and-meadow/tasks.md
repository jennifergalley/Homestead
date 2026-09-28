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

## 3. Ground textures

- [x] 3.1 Finish pass in `build_landscape_material.py` with CC0 `grass_path_2` and `rocky_trail`.
- [ ] 3.2 Verify distance tiling, trodden soil, leaf litter and stony banks in PIE.

## 4. Footsteps

- [x] 4.1 Softer, low-passed steps on grass, moor and woodland floor; other ground unchanged.
- [ ] 4.2 Jenny confirms the steps in the package.
