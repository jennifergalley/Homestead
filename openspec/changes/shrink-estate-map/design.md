# Design

## Context

Planning only for `backlog:jenny-muucy9hz-jv5gx7`; see proposal for motivation. Read-only measurement on 2026-10-04 uses branch `19900502`, cross-checked against today's `main` `4de52b4f`: terrain/layout/road geometry and movement speeds match. No editor or builds were used; these are source-derived travel estimates, not timed playtest claims.

`HomesteadCharacter.h`: ordinary heroine **480 cm/s sprint**, 210 cm/s walk; legacy automation 300/180 must not size this map. Sprint requires energy >=25; walking below 10 energy is 75% speed. Estimates use XY chainage, sustained input and no stops/acceleration/collision detours. Walking is reference only. Instant-travel's separate 180 cm/s quote is not a sprint measurement.

| Source geometry | Current | Recommended |
| --- | --- | --- |
| Landscape/runtime ground frame | 4,033 vertices at 1 m; X/Y -2,016..2,016 m, 4,032 m square, 16.257 km2 | About 2,016 m square, 4.064 km2; half linear size |
| Home EstateBoundary | X -760..160, Y -1,150..110 m; 920 x 1,260 m envelope; polygon 898,250 m2 | Reshape to about 763,500-808,400 m2 (85-90% area), not a quarter-sized estate |
| Local estate features | Farm 60 x 60 m; manor envelope 18 x 30 m, reservation polygon 504 m2 | Retain usable local dimensions and clearances |
| Decorative horizon only | Outer-land ring extends about 70 km beyond map edge; ocean about 60 km | Keep distant horizon treatment; these are not playable travel extents |

### Travel measurements and Balance targets

Start: `StandingRoomSpawn` (-257.5, -638) m; farm: actual `DerelictFarmGate` (-222, -657). Times are **real seconds**. Balance Agent `81a547cd` revised its walking targets to sprint (`balance.md` section 9, `6817763d`).

| Route / endpoint | Straight m; sprint / walk s | Road/path m; sprint / walk s | Target sprint s; cap |
| --- | --- | --- | --- |
| Manor -> town square | 1,810.2; 377 / 862 | 2,044.0; **426** / 973 | 40-60; 75 |
| Manor -> store door | 1,839.3; 383 / 876 | 2,072.6; **432** / 987 | 40-60; 75 |
| Manor -> lake landing | 207.4; 43 / 99 | 210.4; **44** / 100 | 20-35; 45 |
| Manor -> coast, first cove sand | 320.6; 67 / 153 | 525.4; **109** / 250 | 45-70; 90 |
| Manor -> named CoveBeach marker | 411.3; 86 / 196 | 619.6; **129** / 295 | 45-70; 90 |
| Manor -> mine site | 390.9; 81 / 186 | 437.6 estimate; **91** / 208 | 45-70; 90 |
| Farm gate -> manor | 40.3; 8 / 19 | 58.1 estimate; **12** / 28 | 5-10; 15 |

Reproduce by summing `hypot(dx,dy)`, adding the connectors below, dividing by 4.8/2.1; use closed-ring shoelace area. Sources: `Scripts\Terrain\estate_layout.json`, `cove_route.json`, `HomesteadEstate.cpp`, `HomesteadEstatePublicRoad.inc`, `HomesteadEstateTerrain.h`. Landscape span is `(vertices-1)*QuadCm/100`, not 4,033 metres.

Town: 1,940.0 m road + 45.7 m street + straight endpoint connectors. Lake: 100.3 m path + 109.6 m manor approach. Cove: 508.6 m path + 16.8 m manor connector; marker another 94.2 m across sand. Road/cove 3D lengths are 1,949.1/520.3 m: XY is optimistic on grades/stairs.

**No authored mine/farm road:** those estimates project endpoints onto the road spine plus straight off-road connectors, not proven walkable routes. The lake approach needs a gate-detour check. Farm centre is 75.2 m direct / 90.2 m road estimate (16/19 s sprint); gate acceptance does not imply identical access from every field square.

## Goals / Non-Goals

**Goals:** five sprint caps; farm/store/one-fishing-stop/home circuit about three minutes. A view, flowers or existing gathering interest roughly every 50 m.

**Non-Goals:** speed/energy/clock changes, new content/systems, elaborate save migration, or implementation now.

## Decisions

### Hybrid re-authoring, not an actor-scale switch

Use a 0.5-linear macro frame plus authored spacing: farm 24-48 m, lake 96-168 m, town/store 192-288 m, coast/mine 216-336 m; caps 72/216/360/432/432 m. Budget to the store door, not road-end marker. Retain the 60 x 45 m square, 3-6 m lanes and 5.5 m street (`town_layout.py`).

Uniform half-scale leaves town/store 213/216 s sprinting; actor scaling also shrinks doors/collision. Half **area** shortens routes only by sqrt(0.5). Instead compress macro geography, move local clusters rigidly and retain human/prop sizes, farm grid and path/bridge widths.

The manor is 409.6 m from the nearest owned boundary (85 s sprint). Town-only relocation cannot meet 75 s outside the unchanged estate. Stage 1 includes a **town-side boundary pocket connected to the exterior**, retaining farm/manor/lake ownership and 85-90% home area. Stage 2 fits that hub/edge and for-sale parcels into the smaller frame with stable identities.

### One coherent spatial regeneration

Resample to **2,017 samples at 1 m** (16 x 16 components, 126 quads each); confirm import configuration when scheduled. Keeping 4,033 samples at 0.5 m also fits but quadruples sampling density. Update hard-coded frame assumptions across terrain recipes, runtime triangle sampling, editor import, baked ground, map metadata and tests together; never shrink only the Landscape actor.

Compress macro elevation **relative to sea level** too, avoiding doubled slopes; then grade full-size manor/farm/town pads, paths, steps and bridge. Reuse `reshape`, road grading, river channel, lake basin, cove route, beach belt and editor helpers. Reseat river source/mouth, lake shore/level/depth, coast/estuary and water surfaces together; regenerate ground/weightmaps, ocean shore/depth material frames and outer-land seam.

Recompute connected roads/cove/lake paths, chainage/grades, bridge deck/ramps/keep-outs, signs and travel arrivals/discovery markers. Move town/pad/store door/counter together and remove the old town. Update instant-travel distances, preserving its time/energy rules.

Retain object IDs and parcel/landmark names, transform XY/reseat Z and keep metre JSON / centimetre C++ mirrors consistent. Report moved/added/retired IDs; never reuse them or associate old clear flags with renumbered objects. Re-bake biome/trees/forage/disrepair/flowers at natural **per-metre** density, not quadruple-density compressed instances.

Reapply farm/manor/road/water exclusions, lake-path flowers, then `estate_wildflowers.py` (lake-path bake replaces flower kinds). Retain simulation-driven new-till exclusion and actual-change gating, not clock/tick scans. Re-bake/import map art/rectangle (`Scripts\Map\bake_estate_map.py`), synchronize map transform/labels/travel points; keep the 120 m minimap crop initially.

### Stages, ownership and rough size

Stage 1: **medium**, about 1-2 focused days once scheduled: closer town/route/boundary pocket/pad, nearby scatter, signs/travel and map. Player check: timed ordinary manor -> store -> home sprint, with usable store entry.

Stage 2: **large**, another 3-5 focused days plus Integration acceptance for compact terrain, water/routes, stable-ID placements and full scatter/maps. Planning ranges, not delivery dates. Skip temporary stage 1 only if the scheduled full cut would mostly discard it.

Code owns layout mirrors/IDs/readers/scatter; Opus Art owns landscape/ground/water/map import and actual kit asset edits; Gameplay/UI owns map/shop/travel wiring; Balance owns sprint budgets. Agree one metre-space layout first, with **one bake writer** for shared generated outputs. Integration alone merges/builds/packages and selects save/layout version handling. Respect shared slots and reuse valid caches.

## Risks / Trade-offs

- Steeper terrain or tiny stairs after compression -> scale macro relief, then grade full-size paths/steps and inspect the river crossing/coast in the actual game.
- Town crowding/overlap -> reserve full square/lanes/pad and outside-estate pocket before scatter.
- Frame mismatch -> deterministic regenerate/import checks; no old water/ground over new collision.
- ID/ownership/cultivation drift -> verify identities, parcel limits and clear/till/save-load behavior.
- Estimated gate/stair times -> actual uninterrupted **sprint**, energy >=25, drives acceptance.

## Migration Plan

Report placement moves/retirements and reset needs before each cut. **Do not change `SimulationSaveVersion` or `bakeVersion` in a lane.** Recommend a disclosed fresh game: reset position, tilled/crop beds, placed structures/chests/contents, cleared resources, travel discoveries, parcel purchases, inventory/money/calendar and other game progress, not account settings. Stage 1 can retain saves only if Integration confirms shop counter re-seating and unaffected player placements suffice.

Integration chooses existing compatibility handling and notifies Jenny; no blind save deletion. On failure keep the last playable release and roll source/assets back together. Check current-layout save/load on a test save. No saves/releases change now.
