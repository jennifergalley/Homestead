# Tasks

- [x] 1.1 `town_layout.py`: square, buildings, lanes, store slot, curved `townStreet`; self-checks; layout, store anchors and test data (plan: `town_layout_plan.png` in the Water session files).
- [x] 1.2 Store anchors in `HomesteadEstate.cpp`; native checks (square, lanes/no bunching, street clearance, store door/counter, clear walk).
- [x] 1.3 `town_massing.py` reads the layout, deletes stale blockouts, loads World Partition actors first.
- [x] 1.4 `weightmaps.py`, `bake_ground.py`, `bake_estate_map.py` know the street and square; re-baked (map crop `town_map_crop.png`).
- [ ] 2.1 Editor: `town_massing.py`; `ApplyEstateWeightmaps` over the town (r16 rows 3080-3220, columns 1420-1560); `build_ground.py`; `ImportEstateMap`; save the external actors.
- [ ] 2.2 PIE, no teleports: walk from RoadTownEnd up the street into the square to the store counter in day, night and rain; check that an old save's store opens at the new counter.
- [ ] 2.3 When the seedsman branch lands, move Tregear's into the EastHouse slot.
