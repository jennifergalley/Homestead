# Widen the estate's beach

## Why

Jenny's playtest (2026-09-29): the estate's coast is cliff straight into the sea, with no beach to walk
along below it. The orchestrator's brief: a dry sand belt 17-38 m wide along roughly 630 m of owned
coast, keeping the cliffs, with the terrain, scenery and map bakes consistent.

## What changes

- `Scripts/Terrain/beach_belt.py` grades a belt of dry sand along the foot of the south cliffs from the
  estate's west boundary to the cove's west headland (535 m, plus 25 m tapers): a 1.7 m berm at the cliff
  foot falling to a 0.3 m swash line over 17-38 m (wider in bays, varied along the coast), then a 1 in 10
  foreshore. It only raises ground seaward of the old waterline (the cliffs are untouched), keeps clear of
  the river mouth, grades once, and keeps the work npy in step.
- The new sand paints as Beach (`weightmaps.py`); the ground and the estate map are re-baked.

## Impact

- Heightfield: 119,896 vertices raised (r16 rows 842-1425, columns 1196-1540). Needs
  `ApplyEstateHeightfield` and `ApplyEstateWeightmaps` over that rectangle and a re-built ocean shore texture
  (`bake_ocean.py`, `build_ocean.py`) in the editor, then `place_water.py`, `build_ground.py` and
  `ImportEstateMap`.
- No scenery records, placements, ids or saves change. The cove and the cove route are untouched.
- Stacks on `jennifergalley-cove-route`.
