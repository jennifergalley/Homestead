## 1. Research and region

- [x] 1.1 Read the MVP woodland rules and meshes from the generated-chunk code (unchanged on `mvp-survival`) and record them in design.md
- [x] 1.2 Pick the region with the estate layout and tell the orchestrator (`mvp_woodland.json`)
- [x] 1.3 Reference screenshots of the MVP woodland (generated woodland game in PIE on the Homestead map)

## 2. Scatter and runtime

- [x] 2.1 `EstateSceneryKinds` rows 19+ for the MVP meshes, with MVP cull distances and shadow settings
- [x] 2.2 `scatter.py`: keep the Cornish woods out of the region (with an ecotone) and scatter the MVP trees, young trees, underbrush, ground cover and granite by the MVP rules
- [x] 2.3 Interactable MVP forage in the region (ids 560000+) and the MVP tree palette for them
- [x] 2.4 Re-bake `EstateScenery.bin` and the placements `.inc`; check instance counts and load time (379,066 scenery instances in 39 batches, +127k, mostly grass clumps; 454 interactables; outside the region the bake is byte-identical)

## 3. Verification

- [x] 3.1 Editor build, native tests (Release)
- [x] 3.2 PIE on the Estate: walk from the manor into the region, gather in it; side-by-side captures with 1.3 (branch pile 560343 gathered; captures in the `[ready]`)
- [ ] 3.3 `[ready]` to the orchestrator with the region, captures and what to try

## 4. Ground (ground lane)

- [ ] 4.1 MVP floor zone, no blade grass, woodland footsteps and canopy kinds inside the region (`bake_ground.py`, landscape material)

## Findings

- The small broadleaf and mature fir meshes went Nanite in `accb674a` without `bUsedWithNanite` on their materials (or on `M_CameraSafeFoliage`), which draws the Default Material in packaged builds; this change sets the flags. Jacaranda, fir pole and fir saplings were 125k-464k-triangle LOD meshes and are Nanite now; the fir sapling and flower materials lacked the ISM usage flag.
- Brambles and toyon are drawn without collision (scenery can't be cleared); clearable MVP thickets are deferred.
- Until task 4.1 lands, the region stands on the estate's bright pasture green instead of the MVP's leaf-and-needle floor: the biggest visible difference left.
