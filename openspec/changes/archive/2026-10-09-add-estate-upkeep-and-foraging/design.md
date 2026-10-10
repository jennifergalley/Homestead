## Decisions

- Rules live in `Simulation/HomesteadUpkeep.cpp` (`Simulation::UpkeepRegrowth`, called from the new-day
  hook) and `HomesteadTreeFelling.cpp` (`ClearSceneryStump`). They are native-tested.
- Regrowth is rolled with `Overgrowth::StableRoll` per node and day, so it is deterministic and needs no
  RNG state. Candidates come from the estate's own cleared nodes within `Upkeep::RegrowRadiusCm` of the
  manor, so there are no hand-placed coordinates and the rule survives the map-shrink rebake.
- Windfalls are dormant `FallenBranch` nodes (ids 584000-584179) placed under woodland trees by the
  generator; waking one is an ordinary resource edit.
- Forage placements are generated data (`HomesteadEstateWoodlandForagePlacements.inc`), clustered in
  thickets of 3-5 bushes at clearings and woodland edges.
- A cleared stump is `FelledTree.cleared`, saved as felled-section value 2; reads accept 0-2.
- The bush look is `SM_WildCurrant` (foliage) plus `SM_CurrantProduce` (berries that vanish when picked),
  replacing the bramble look for estate berry bushes only.
