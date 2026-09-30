# Tasks

## 1. Route (Water; source)

- [x] 1.1 Survey the manor-to-cove ground (slope, river, trees; `cove_survey.png`, `cove_lower.png` in the Water session files) and search candidate routes (`cove_route.py --search`).
- [x] 1.2 `cove_route.py`: committed turning points; paths at 1 in 7; stair legs as flights and landings within the kit; heights solved together; kerbs, rails (one side per leg, clear of the other legs, corner joins) and fingerposts; `--dry`; idempotent.
- [x] 1.3 Grade the heightfield once (3,772 vertices, r16 rows 1338-1541, columns 1457-1755), work npy in step, 328 scatter records cleared; `cove_route.json`, the layout's `coveRoute` summary and `footpaths`; `HomesteadEstateCoveRoute.inc`.
- [x] 1.4 `bake_ground.py` wears the path and keeps grass off the steps; `scatter.py` re-applies the clearing; `bake_estate_map.py` draws the dashed path.
- [x] 1.5 `Simulation/HomesteadEstateCoveRoute` and native `HomesteadCoveRouteTests` (start and end, estate, grades, flights, landings, joins, rails, kerbs, river, trees, forage on steps, fingerposts).
- [x] 1.7 Review fixes: the path off the bench steps' foot leaves through two sharp corners, 3 m or more off the flights' axis, railed on that side; a path's bed has priority over another leg's; the bed interpolated between stations; the under-tread line corrected (one rise higher); a tread-clearance pass on the graded heightfield; rail sides by drop, then blocked bays. Native checks on the graded ground (burial, cross and long grade, tread clearance) and against paths beside flights.
- [x] 1.8 Second review: tread-clearance samples across each leg on its own heading, lowering only under a stair leg's footprint (the pit in front of the bench steps is gone); 2 m level step-offs where paths meet flights; nothing in any stretch's clear width. Native: graded ground 0.3 m beyond every flight's foot and head (path level -8/+2 cm; under a landing 4-40 cm), path stations from 25 cm past a flight's end.
- [x] 1.6 Route plan with elevation/length and flight tables (design.md); plan and profile (`Saved/CoveRoute/cove_route.png`).

## 2. Editor (Water; Unreal slot)

- [x] 2.1 `ApplyEstateHeightfield` over r16 rows 1338-1541, columns 1457-1755; `ApplyEstateWeightmaps` if needed; `build_ground.py`; `ImportEstateMap`. _Done 2026-09-30 in band B of the stack's pass._
- [x] 2.2 PIE: walk the route down and back up from the front door on the graded ground, and look at it from the cove and the manor. _Done: down on foot from the front door to the sand (53 points), back up (137 points at 3 m), no stalls; captures from the sand, the hairpin and the head of the cliff steps (`cove_*`). The stair legs are graded ramps until the kit is placed._

## 3. Kit placement (Water, after Props' `add-cove-route-kit` import)

- [x] 3.1 Runtime builder: flights, landings, kerbs, rail bays (mirrored where flagged), pawn-only rail blockers and fingerposts from `EstateCoveRoute()`. _Source 2026-09-30, uncompiled: `Simulation/HomesteadCoveRouteKit` lays the kit out (191 treads in three variants, landings covered by the mix of 0.75 m and 0.6 m slabs that stretches least, worst 1.07; 140 kerbs; 84 rail bays, 39 mirrored, scaled X and Z to plan length; 84 pawn-only blockers from 20 cm under the path to 120 cm over the top rail; 2 fingerposts; native checks). `HomesteadWorldCoveRoute.cpp` places it after the road bridge when every mesh loads from `/Game/SurvivalGame/Environment/Props/{CoveSteps,CoveKerb,CoveRail,Fingerpost}`, and otherwise logs what's missing and leaves the graded path. A wedge closes each of the two corner turns: apex at the inner corner of the landing's uphill end edge, +X along that edge, mirrored so it opens toward the other leg, Y scaled to the turn; `cove_route.py` now emits `corner()` lines. End posts close the 20 free rail ends. Once Props' import lands there's no editor step._
- [x] 3.3 Review fix: kerbs and level rail bays are checked against the clear width of every stretch along their whole length, not just at their pivots, when the route is designed and each time it's re-emitted. That dropped the 12 kerbs that reached into the path at the bench steps' hairpin and the first switchback (one stood 25 cm from the centreline, in front of the bottom tread). Native: kerbs, level rail bays and path blockers sampled along their length.
- [ ] 3.2 PIE: walk it down and up with the gamepad and keyboard, walk into the rails and kerbs, and check the cove and gate views.
