# Tasks

- [x] 1.1 `town_layout.py`: square, buildings, lanes, store slot, curved `townStreet`; self-checks; layout, store anchors and test data (plan: `town_layout_plan.png` in the Water session files).
- [x] 1.2 Store anchors in `HomesteadEstate.cpp`; native checks (square, lanes/no bunching, street clearance, store door/counter, clear walk).
- [x] 1.3 `town_massing.py` reads the layout, deletes stale blockouts, loads World Partition actors first.
- [x] 1.4 `weightmaps.py`, `bake_ground.py`, `bake_estate_map.py` know the street and square; re-baked (map crop `town_map_crop.png`).
- [x] 2.1 Editor: `town_massing.py`; `ApplyEstateWeightmaps` over the town (r16 rows 3080-3220, columns 1420-1560); `build_ground.py`; `ImportEstateMap`; save the external actors. _Done 2026-09-30: town paint band, `town_massing.py` (15 blockouts, 3 new), `build_ground.py`, `ImportEstateMap`, external actors saved._
- [x] 2.2 PIE, no teleports: walk from RoadTownEnd up the street into the square to the store counter in day, night and rain; check that an old save's store opens at the new counter. _Done on foot from RoadTownEnd along the street into the square and into the store (20 + 2 points); at the counter the focus is "Mrs. Pascoe | General store". Captures at 11:00, 21:00 and on a rain day (`town_*`, `store_counter_day.png`). The old-save store check wasn't run._
- [ ] 2.3 When the seedsman branch lands, move Tregear's into the EastHouse slot.
