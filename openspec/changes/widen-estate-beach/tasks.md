# Tasks

## 1. Terrain (Water; source)

- [x] 1.1 Survey the owned coast (cliff straight into the sea along 965 m of shoreline; seabed -3 m at 20 m out; session file `beach_survey.png`).
- [x] 1.2 `beach_belt.py`: berm, dry width 17-38 m (bays, knots), foreshore, tapers, river clear, only raising; grade once; the work npy in step; `--dry`; widths measured along the shore normal (50 m section medians 24-36 m, overall 30 m).
- [x] 1.3 Graded (119,896 vertices, r16 rows 842-1425, columns 1196-1540); `river_channel.py` (no change); `weightmaps.py` (the belt paints Beach, median weight 1.0); `bake_ground.py`; `bake_estate_map.py`. Scenery and the cove route unchanged.

- [x] 1.4 Review fixes: the dry width is smoothed over 6 m once carried to sea (11 seams, scarps up to 0.95 m/m, gone); once graded the script neither redesigns nor re-measures (`--dry` no longer crashes or proposes a second raise); empty measurement guarded. `Tests/EstateBeachTests.py`: dry sand across every 10 m, walkable from the headland to the west boundary, no bank over 1 in 2.9 across 1 m in the sand and swash zone, the river mouth open (the first version fails the scarp check at y -745..-740).

## 2. Editor (Water; Unreal slot)

- [ ] 2.1 `ApplyEstateHeightfield` and `ApplyEstateWeightmaps` over r16 rows 842-1425, columns 1196-1540; `bake_ocean.py` + `build_ocean.py`; `place_water.py`; `build_ground.py`; `ImportEstateMap`.
- [ ] 2.2 PIE: walk the beach from the west boundary to the cove's headland at midday and dusk; the swell and swash foam run up the new sand, not through it; the cliffs above are unchanged; the river mouth is open.
