# Design

## Context

The sign is authored by `Scripts\Terrain\public_road.py` into the generated public-road include. Runtime signs are spawned by name, not serialized resource placements. The manor travel arrival is a separate anchor and should remain near the front door.

## Goals / Non-Goals

**Goals:** relocate only `ManorRoadSign` beyond the derelict farm on the roadside; orient its board/arm along the road toward town.
**Non-Goals:** road reshaping, changing travel destinations, moving the manor arrival, adding sign identities, resource placement or save/bake changes.

## Decisions

- Reuse the road generator's chainage/frame/heightfield and existing sign asset. Place the sign 26 m past the farm's projected far edge (chainage 71.41 m), on the 4.2 m verge offset between existing trees. The first 12 m candidate overlapped tree 510053; no resource was moved to clear it.
- Preserve `ManorRoadSign` name and array order, travel destinations and the separate manor arrival. Regenerate the include instead of hand-editing generated data.
- The board spans local Y while its face points local X; set yaw so the board's townward arm follows the road tangent. Verify road offset, farm clearance and heading with native geometry tests.
- Town owns generator, public-road data/header comments and focused public-road/economy tests. No map UI changes or new asset/license needs.

## Risks / Trade-offs

Sign relocation could accidentally move the walk-home arrival -> explicitly retain and test the front-door arrival.
Sampled terrain/geometry cannot establish visual quality -> Jenny checks the integrated sign; request an editor slot only if necessary to resolve geometry.
