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
- [x] 2.4 PIE on the Estate (2026-09-29). An old save loads with the field present. On a fresh game: barrel cleared by hand ("Clear away", +seed, kindling, scrap), nettles pulled ("Pull"), medium stump in 5 axe swings, tilling refused beside weeds and allowed after pulling them, and spoiled ground named ("Clear the thin bramble here first."). The new flower and nettle meshes are in use.
- [x] 2.5 The real EstateDebris crate, barrel and heap checked in PIE from the Farm Agent's branch; EstateRubbish (SM_RottenPlanks, SM_RubbishHeapSmall, SM_ScrapHeap) loads quietly when it lands

## 3. Follow-ups

- [ ] 3.1 Physically blocking collision for stumps, boulders and barrels, once the strike approach handles convex hulls
- [x] 3.2 Dedicated primrose, bluebell and wild daffodil meshes (primrose.py, bluebell.py, wild_daffodil.py), with wild garlic from wild_garlic.py
