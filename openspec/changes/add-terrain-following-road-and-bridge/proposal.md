## Why

Jenny's 2026-09-29 playtest: the road has "artificial raised and lowered segments". reshape.py graded
it to a heavily smoothed profile clamped at 1 in 9 by one-sided passes. Down into and out of the
river valley that drew straight lines: a causeway up to 9.9 m high (chainage 411-679 m) and a
cutting up to 12.8 m deep (726-1264 m), whose banks were painted as cliff. At the crossing the river
had cut the road 1.1 m deep, with no bridge.

## What Changes

- `Scripts/Terrain/road_grade.py` re-grades only the road corridor to follow the ground. It holds a
  1 in 5 limit using the profile with the smallest worst deviation, eases the vertical curves, and
  keeps the manor and town end levels. The centreline, chainage and anchors are unchanged. Cut and
  fill against the natural ground drop to +1.6 / -1.4 m (p95 0.7 m).
- Over the river it holds a level deck 0.9 m above the water, with 1 in 12 approach ramps inside the
  existing 20 m bridge keep-out. river_channel.py re-seats the river under it.
- Kept in step: the heightfield (r16, PNG, work npy), `roadProfile` and `roadGrade`, the public road
  data, the gateway anchor height, the paint weightmaps, the ground bake and the map.
- A period wooden road bridge spans the river there, built at runtime from the public road's bridge
  data. Its deck and railings are solid, so she walks over the river and can't step off the sides.
- Editor: `ApplyEstateWeightmaps` patches the paint layers like `ApplyEstateHeightfield` patches
  heights.

## Impact

- Terrain data, `HomesteadEstatePublicRoad.inc`, `HomesteadEstate.cpp` (gateway z),
  `HomesteadEstateAuthoringLibrary`, and new bridge code.
- No save change: placements, chainage and anchors keep their positions.
