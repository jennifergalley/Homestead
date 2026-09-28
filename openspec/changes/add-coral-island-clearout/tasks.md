## 1. Simulation

- [x] 1.1 Add the Nettles, StumpMedium, BrokenCrate, BrokenBarrel, RubbishHeap and RottenPlanks kinds, with names and table rows (tool, tier, energy, swings, yields, finds)
- [x] 1.2 Let weeds and nettles be pulled by hand, with a chance of self-sown root seed
- [x] 1.3 Give every overgrowth kind a spoil radius; Till and CheckSite refuse spoiled ground and name the obstacle; weed creep respects the radius
- [x] 1.4 Bake the clear-out field (`Scripts/Terrain/clearout.py`, ids 570000+), with zones, lanes and the manor lane's keep-clear strips, and a runtime skip near earlier placements
- [x] 1.5 Native tests: field placement (count, zones, lanes, spacing, kinds, teases) and rubbish, nettles, stump swings and spoiled ground; the manor test's lawn moved beyond the field

## 2. Unreal

- [x] 2.1 Visuals: SM_NettlePatch (new `nettle.py` recipe), SM_StumpLarge at medium scale, EstateDebris meshes with store stand-ins, fallen timbers as the plank pile, and wild garlic's clump
- [x] 2.2 Prompts "Pull" and "Clear away"; the stick, stone or root kneel per kind; the mow summary counts seed
- [x] 2.3 The clear pop: swell, shrink and sink, with chips of leaf, wood or stone
- [ ] 2.4 PIE on the Estate: walk out of the front door into the field, clear each kind, check tilling is refused beside an obstacle and allowed after clearing, and take screenshots
- [ ] 2.5 When the manor lane's EstateDebris lands on main, check the real crate, barrel and heap meshes in PIE

## 3. Follow-ups

- [ ] 3.1 Physically blocking collision for stumps, boulders and barrels, once the strike approach handles convex hulls
- [ ] 3.2 Dedicated primrose, bluebell and wild daffodil meshes (still the stand-in flowers)
