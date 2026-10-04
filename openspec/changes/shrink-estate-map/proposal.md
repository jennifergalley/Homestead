# Proposal

## Why

Jenny's selected feedback `backlog:jenny-muucy9hz-jv5gx7` says the map is too big and travel takes too long. Today's manor-to-store road trip is about 7.2 minutes sprinting; simply halving every distance would still take about 3.6 minutes. Jenny explicitly directed that sizing use sprint, not walking.

## What Changes

- Recommend roughly half the **linear playable-map dimensions**: 4,032 m square to about 2,016 m square (one-quarter area), not half-size characters, buildings or tools.
- Re-author the daily hub around Balance's revised **4.8 m/s sprint** targets: farm gate 5-10 s, lake 20-35 s, town/store 40-60 s, beach and mine 45-70 s. Hard caps are respectively 15/45/75/90/90 s. Walking is reference only; no movement-speed buff.
- Interpret "estate a tad smaller" separately: retain roughly 85-90% of today's home-parcel area, reshape its boundary and bring the manor/hub near its town-facing edge. Do not silently quarter the usable estate with the landscape.
- First playable stage: bring the existing town/store close along a short, graded route, with a local boundary adjustment so town stays outside the owned estate; retain its roomy square. Second stage: compact the whole map and reseat terrain, water, roads, placements, scatter and map presentation coherently.
- **BREAKING:** recommend a disclosed new-game reset at the compact-map cutover; existing world-coordinate test progress is incompatible. Version/reset decisions belong to Orchestrator/Integration, not this planning lane.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `estate-life-sim-direction`: add compact-map and cozy on-foot travel requirements while retaining the fixed authored world and physical trips to sell goods.

## Impact

Reuse the original estate, town kit, flowers and terrain/road/water/map bake pipelines already in this repo, plus Unreal's existing landscape import facilities. No new external asset/license dependency; custom work is spatial re-authoring and regeneration, not a new world-generation or travel system.

Affected surfaces: `estate_layout.json` and provisional C++ geometry, landscape/runtime heightfield and ground mesh, river/lake/ocean/outer-land bakes, bridge/cove stairs, stable placement tables, biome/flower scatter, signs/travel arrivals, shop/town actors and world-map/minimap transforms. See `design.md` for measurements, ownership, reset scope and stage sizing.

**Planning only, unscheduled:** this change authorizes no implementation, editor launch, build, package, save deletion or version bump. First delivery is the closer-town trip; full acceptance is the compact map and all five route caps. New regions, speed buffs, mounts and new gameplay content are out of scope.
