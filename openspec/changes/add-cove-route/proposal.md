# Add the cove route

## Why

Jenny can see the cove from the manor but has no way down to it on foot: the valley side below the
manor's south front falls 80 m, the lower 35 m of it at about 1 in 2.3, and walking straight at it means
sliding down an exposed dirt slope. The orchestrator asked (2026-09-30) for a complete on-foot route from
the manor to the beach, built source-first against Props' step kit (`add-cove-route-kit`), whose final
steep descent is never an exposed dirt slope.

## What changes

- `Scripts/Terrain/cove_route.py` designs the route from committed turning points: a 349 m graded earth
  path from the manor's fallen south front door down the meadow in three switchbacks (never steeper than
  1 in 7), 191 granite steps in 18 flights down the valley side with landings and rails, a bench and two
  short flights onto the valley floor, and a gentle path to the sand west of the river mouth: 509 m and
  83 m of fall in all.
- It cuts the heightfield to the design (the path's bed, and 5 cm or more under every tread and landing),
  keeps the work npy in step, clears the scatter off the route, and writes the full design to
  `Scripts/Terrain/cove_route.json` and a summary and the map's dashed footpath to `estate_layout.json`.
- `bake_ground.py` wears the path through the grass and the leaf litter and keeps grass off the steps;
  `bake_estate_map.py` draws it; `scatter.py` clears a fresh scatter off it.
- `Simulation/HomesteadEstateCoveRoute` loads the generated `HomesteadEstateCoveRoute.inc`: the centreline
  with walk and bed heights, flights, landings, kerbs, rail bays and fingerposts, at the kit's pivots.
  `HomesteadCoveRouteTests` pins the route.
- Placement of Props' pieces at runtime follows when the kit's meshes are imported.

## Impact

- Heightfield: 3,772 vertices in r16 rows 1338-1541, columns 1457-1755 (cut up to 2.5 m into the bank at
  the steps, filled up to 2.7 m on the bench). Needs `ApplyEstateHeightfield` over that rectangle,
  `build_ground.py` and `ImportEstateMap` in the editor.
- Scenery: 328 scatter records cleared. No placement ids change; no save format change.
- Stacks on the river-mouth branch (the heightfield changes are sequential).
