# Proposal

## Why

Jenny's backlog card `backlog:jenny-muucy9hz-jv5gx7`, revised scope (2026-10-10, verbatim):

> "The immediate area around the ruined manor - the derelict farm, the area we need to clear when the game begins, the path to the pond through the woods, the walk to the town, and how far away and how large the town is now - that's all fine.
>
> What needs to change is: The cove is too far away, the mine is too far away, the estate is too big on the map, and there's much of the world that I don't think we'll ever end up actually exploring. I'd like to have some growing room for the town and other estates nearby (neighbors), as well as perhaps a communal beach and other communal areas, but in general I think the map can shrink considerably. Perhaps as part of the shrink we could consider outlining the shape of future neighboring estates on the map at least, so we make sure we leave some room for them, as well as some future communal areas like a beach, a park, maybe a community garden, etc."

Same day she added: "that beach belt looks nice and long, but it's inaccessible from the cove by walking along the beach. Please fix that."

This replaces the earlier uniform half-scale plan. Stage 1 (the village about 300 m from the manor) shipped and Jenny now calls it fine.

## What Changes

- **Nothing moves in the manor core:** ruin, derelict farm, opening clear-out, pond and its woodland path, the road and walk to the village, and the village itself stay exactly where they are, with their placement ids.
- **The cove comes closer:** the existing river-mouth cove is extended north-west up its valley, so its sand starts about 200 m from the manor. A new cove route runs down the opening valley and down granite cliff steps: about 240 m, about 50 s sprinting (cap 60 s). Its sand rings the bay and joins the long beach belt, so she can walk from the cove onto the beach.
- **The mine comes closer:** the mine ruin moves to the clifftop just west of the cove, with a woodland path from the manor (about 185 m, about 40 s) and a clifftop link to the cove steps.
- **The estate gets smaller and skinnier:** about 25 ha of land instead of about 90 ha, its west edge pulled in (Jenny, 2026-10-10). It keeps the manor core, farm, pond and woods, the cove, the beach below it, the mine and enough woodland for regrowth. The village is outside it.
- **The world is cropped:** the map sheet and playable area shrink from 4 km square to about 1.46 x 1.52 km. The terrain beyond stays as horizon; the edge is natural (sea, dense wood, hedged fields) with an invisible stop behind it.
- **Room to grow, shown on the map:** faint dashed outlines with period names for three neighbouring estates (Penhallow, Tregarthen, Polwhele), the village green, the allotments, Chapel sands and a ring of village growth room. Two for-sale plots next to her estate (Top Field 5 ha, Carn Wood 14 ha) replace today's three large ones. Outlines are reserved space only: no neighbour gameplay.
- **Village additions (Jenny, 2026-10-10):** a footpath from the village down to the river, six labourers' cottages along the road out of the village, and the road ending at a gate and milestone for "Penvose" (no Poldark place names).
- **More choppable trees:** about 100 existing scenery trees in her woods become fellable, since the skinnier estate keeps only 66.
- **BREAKING (saves):** roughly half of the generated placements (those outside the new boundary, or in the new bay) are retired; their ids are never reused. A fresh game is recommended; Integration decides any version handling.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `estate-life-sim-direction`: compact-map and on-foot travel requirements, retargeted to the revised scope.

## Impact

Reuses the existing terrain, cove route, beach belt, ocean, ground, scatter, placement and map bake scripts and the map view's parcel drawing. Affected: `Scripts/Terrain` (heightfield, layout, cove route, placements), `Scripts/Map/bake_estate_map.py`, `HomesteadEstate.cpp` and generated `.inc` placement tables, the map view (reserved outlines), the playable edge, the Estate level's landscape, water and ground imports. No new assets, systems, speed changes or external dependencies.
