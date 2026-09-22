# Proposal

## Why

Homestead's version-three woodland now has local rolling relief, but future
mountains, valleys, rivers and lakes need a larger deterministic authority so
water crosses signed chunk and region boundaries coherently instead of appearing
from unrelated per-chunk noise. This prototype defines and tests that future
contract without delaying or modifying the current playable woodland candidate.

## What Changes

- Add an isolated pure-C++ regional generation prototype for large-scale relief
  potential, drainage ownership, connected downstream reaches and spill-defined
  lake basins.
- Define stable seed/version/region coordinates, canonical global border outlet
  identities, habitat hooks and bounded query results independent of visit order.
- Validate signed floor ownership, cross-region continuity, deterministic
  depression filling, downhill flow and multiple seeds in focused source tests.
- Keep the current woodland generator, save schema, runtime terrain, stream,
  rendering, collision and gameplay unchanged.
- Treat this branch as a design/prototype result, not a playable hydrology claim.

## Capabilities

### New Capabilities

- `regional-landforms-hydrology`: Deterministic regional relief and connected
  watershed topology suitable for later terrain, habitat, river and lake
  integration.

### Modified Capabilities

None.

## Impact

The change owns new `HomesteadRegionalGeneration.h/.cpp` and focused prototype
tests plus its OpenSpec artifacts. It reuses the project's existing C++17,
integer hashing, signed-coordinate and explicit-version patterns; no external
code, assets, license obligations, dependencies or frameworks are introduced.
The custom gap is connected regional drainage and spill handling, which the
current local rolling-noise/legacy-stream adaptation does not provide.

The smallest later in-game result would blend one generated valley and its
continuous river through neighboring woodland regions. That playable
demonstration remains main-owned and is not part of this prototype. Full-round
acceptance additionally requires water meshes, collision, traversal, habitat
placement, save behavior and actual visual play evidence. Mountains, waterfalls,
erosion, seasonal flow, flooding and a complete biome system remain deferred.
