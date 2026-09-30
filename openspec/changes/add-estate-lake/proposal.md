# Proposal

## Why

Jenny (playtest, 2026-09-29) wants a lake on the property, "north-east of the manor", which she described
as up and to the left on her map, close enough to reach for watering the farm. The field-book map is north
up with east to the right, so up-left is north-WEST. We match what she sees on the map, and tell her
plainly about the cardinal correction.

## What Changes

- **The site (A_NW, agreed with the orchestrator):** an irregular upland pool centred (-50, -745) m,
  about 76 x 44 m and 2,650 m², its long axis running south-west to north-east along the slope.
  - It's in the meadow clearing up-left of the walled farm, about 110 m from the farm's north-west corner.
  - It sits 15 m or more clear of the MVP woodland zone, 200 m from the drive, and away from the river,
    the spring and the manor clear-out.
  - No placement id is within 3 m of it.
- **The basin** (`Scripts/Terrain/lake_basin.py`):
  - The water level is 98.95 m. The bed shelves at 1 in 4 to 1.8 m deep.
  - A wet lip rises 0.3 per metre out of the water.
  - The uphill (north-west) shore is cut into a bank up to 4 m high, where granite breaks through.
  - The downhill (south-east) shore is held by a low turfed pond bay, up to 1.3 m of fill.
  - The farm-side landing shelves at 1 in 10, so she can kneel at the water.
  - A 100 m trodden footpath runs from the farm's north fence to the landing, at up to 12% grade.
  - It writes the heightfield, PNG and npy, and `estate_layout.json` "lake" (shore, level, landing,
    path, pathProfile). Once graded it only reapplies the scenery.
- **Scenery** (`lake_features.py`, also run at the end of `scatter.py`):
  - The water, its wet lip and the path are cleared of scatter (404 records).
  - The margin is planted with tussock grass (kinds 10 and 11), with talus and boulders (5 and 6) on
    the cut bank. No new kinds are claimed.
- **Water** (`AHomesteadWaterPool`):
  - A closed shoreline spline at the water level, tagged HomesteadWater.
  - A ring-meshed Single Layer Water surface runs 60 cm under the banks.
  - `MI_EstatePond` over a still-water build of the creek graph (`M_EstatePond`, 8% drift) gives peaty,
    amber shallows and near-black depths.
  - An invisible wading limit, 2.2 m in from the shore, blocks pawns only.
  - It's not spatially loaded.
- **The pail and probes:** `WaterEdgeDistance` treats a closed shoreline's inside as in the water.
  `FreshWaterDipPoint` aims 25 cm inside the real waterline. The creek burble skips still water.
- **Ground and map:**
  - `bake_ground.py` marks the lake as Water: no blades on it, a muddy bed, lush margins, and a trodden
    path and landing.
  - `bake_estate_map.py` draws the lake with an inked shore, and draws footpaths dashed.

## Capabilities

### New Capabilities

- `estate-lake`

### Modified Capabilities

None.

## Impact

- New: `HomesteadWaterPool.{h,cpp}`, `lake_basin.py`, `lake_features.py`, `M_EstatePond`, `MI_EstatePond`, `EstateLake`.
- Changed: `HomesteadController.cpp` (water probe), `place_water.py`, `bake_ground.py`, `scatter.py`,
  `bake_estate_map.py`, `bootstrap_unreal.py` (the water graph's `flow`), the heightfield, heightmap,
  layout, scenery, ground data and map, and the landscape proxies over the lake.
- No save change and no placement ids.
