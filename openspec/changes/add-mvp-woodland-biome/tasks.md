## 1. Research and region

- [x] 1.1 Read the MVP woodland rules and meshes from the generated-chunk code (unchanged on `mvp-survival`) and record them in design.md
- [x] 1.2 Pick the region with the estate layout and tell the orchestrator (`mvp_woodland.json`)
- [ ] 1.3 Reference screenshots of the MVP woodland (generated woodland game in PIE on the Homestead map)

## 2. Scatter and runtime

- [ ] 2.1 `EstateSceneryKinds` rows 19+ for the MVP meshes, with MVP cull distances and shadow settings
- [ ] 2.2 `scatter.py`: keep the Cornish woods out of the region (with an ecotone) and scatter the MVP trees, young trees, underbrush, ground cover and granite by the MVP rules
- [ ] 2.3 Interactable MVP forage in the region (ids 560000+) and the MVP tree palette for them
- [ ] 2.4 Re-bake `EstateScenery.bin` and the placements `.inc`; check instance counts and load time

## 3. Verification

- [ ] 3.1 Editor build, native tests (Release)
- [ ] 3.2 PIE on the Estate: walk from the manor into the region, gather in it; side-by-side captures with 1.3
- [ ] 3.3 `[ready]` to the orchestrator with the region, captures and what to try

## 4. Ground (ground lane)

- [ ] 4.1 MVP floor zone, no blade grass, woodland footsteps and canopy kinds inside the region (`bake_ground.py`, landscape material)
