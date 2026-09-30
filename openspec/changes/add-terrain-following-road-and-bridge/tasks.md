# Tasks

- [x] 1.1 Survey: road versus natural ground and ±15 m, before and after (`road_profile_before/after.png` in the Water session files).
- [x] 1.2 `road_grade.py`: terrain-following profile, bridge deck and ramps, exact corridor delta, river re-seat, idempotent; heightfield, PNG, npy, layout.
- [x] 1.3 `public_road.py` regenerated; gateway anchor height (layout and `HomesteadEstate.cpp`); `weightmaps.py`, `bake_ground.py`, `bake_estate_map.py`.
- [x] 1.4 Native: 1 in 5 limit, no long straight earthwork, level over the river.
- [x] 1.5 `ApplyEstateWeightmaps` editor helper (source; needs the editor build).
- [ ] 1.6 Editor: `ApplyEstateHeightfield` (r16 rows 1382-3111, columns 1513-2052), `ApplyEstateWeightmaps` over the same rectangle, save proxies; `build_ground.py`; `ImportEstateMap`. Then walk the road from the manor to the gateway in PIE, with no teleports.
- [x] 2.1 Period wooden road bridge at the crossing: `public_road.py` measures the deck (11 m x 3.6 m, 0.9 m over the water), records `roadBridge` and clears the scatter under it; `bake_ground.py` cuts grass there; `HomesteadWorldRoadBridge.cpp` builds oak stringers, planks, kerbs, posted two-rail railings and granite abutments, with an invisible walk slab and pawn-only rail walls; native geometry tests. (Unreal code uncompiled; elevation sketch `road_bridge_elevation.png`.)
- [ ] 2.2 PIE: cross the bridge both ways, try the railings, and look at it from both banks and from the water.
