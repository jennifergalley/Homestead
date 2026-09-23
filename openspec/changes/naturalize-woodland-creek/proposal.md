# Proposal

## Why

The local woodland creek is bordered by broad, uniformly tan ribbons, so its bed reads as a deliberately excavated sandy canal rather than a damp creek cutting through a forest floor. This is conspicuous in ordinary play and should be corrected before adding more environmental detail around the water.

## What Changes

- Remove the continuous tan bank overlay and expose a narrower, darker creek margin blended from the existing forest-floor materials.
- Break up the visually uniform water edge with deterministic, cross-chunk-safe width variation while preserving the authoritative stream center, interaction distance, terrain collision, and save identity.
- Reuse the existing Brown Mud Leaves and Grass Ground CC0 materials, admitted moss-rock geometry, bank grass/reeds, procedural mesh path, and current generated-cover batching; no new external asset acquisition is required.
- Keep bank vegetation restrained and irregular so it softens the transition without hiding the water or blocking movement.
- Add focused source/runtime checks and ordinary gameplay-camera comparisons covering seams, interaction, collision, and representative woodland lighting.

The smallest useful in-game result is the existing playable creek with its wide sand-colored strips removed, a muddy/leaf-litter transition visible at the banks, and unchanged refill/watering access. Full-round acceptance additionally requires deterministic edge breakup, clean chunk seams, restrained bank dressing, and visual review from ordinary traversal angles. Deferred scope includes regional hydrology expansion, waterfalls, swimming, water simulation, shoreline erosion, new water shaders, and replacement of the legacy stream authority.

## Capabilities

### New Capabilities

- `woodland-creek-presentation`: Natural woodland-creek materials, edge variation, bank dressing, and preservation of existing gameplay behavior.

### Modified Capabilities

None.

## Impact

- `Source/SurvivalGame/HomesteadWorld.cpp` and its tests: local creek procedural sections, terrain material weighting, deterministic edge shaping, and generated bank cover.
- Existing imported environment materials and meshes are reused without license changes; Brown Mud Leaves, Grass Ground, and the admitted natural dressing assets are CC0 and already credited in `docs/asset-credits.md`.
- Simulation water proximity, saves, regional descriptors, terrain collision, resource identity, and player-created structures remain behaviorally compatible.
- Implementation should follow the active work-animation milestone so shared Unreal build/package resources and final promotion evidence stay serialized.
