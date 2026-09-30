# Tasks

## 1. Route (Water; source)

- [x] 1.1 Survey the manor-to-cove ground (slope, river, trees; `cove_survey.png`, `cove_lower.png` in the Water session files) and search candidate routes (`cove_route.py --search`).
- [x] 1.2 `cove_route.py`: committed turning points; paths at 1 in 7; stair legs as flights and landings within the kit; heights solved together; kerbs, rails (one side per leg, clear of the other legs, corner joins) and fingerposts; `--dry`; idempotent.
- [x] 1.3 Grade the heightfield once (3,738 vertices, r16 rows 1338-1540, columns 1457-1755), work npy in step, 327 scatter records cleared; `cove_route.json`, the layout's `coveRoute` summary and `footpaths`; `HomesteadEstateCoveRoute.inc`.
- [x] 1.4 `bake_ground.py` wears the path and keeps grass off the steps; `scatter.py` re-applies the clearing; `bake_estate_map.py` draws the dashed path.
- [x] 1.5 `Simulation/HomesteadEstateCoveRoute` and native `HomesteadCoveRouteTests` (start and end, estate, grades, flights, landings, joins, rails, kerbs, river, trees, forage on steps, fingerposts).
- [x] 1.6 Route plan with elevation/length and flight tables (design.md); plan and profile (`Saved/CoveRoute/cove_route.png`).

## 2. Editor (Water; Unreal slot)

- [ ] 2.1 `ApplyEstateHeightfield` over r16 rows 1338-1540, columns 1457-1755; `ApplyEstateWeightmaps` if needed; `build_ground.py`; `ImportEstateMap`.
- [ ] 2.2 PIE: walk the route down and back up from the front door on the graded ground, and look at it from the cove and the manor.

## 3. Kit placement (Water, after Props' `add-cove-route-kit` import)

- [ ] 3.1 Runtime builder: flights, landings, kerbs, rail bays (mirrored where flagged), pawn-only rail blockers and fingerposts from `EstateCoveRoute()`.
- [ ] 3.2 PIE: walk it down and up with the gamepad and keyboard, walk into the rails and kerbs, and check the cove and gate views.
