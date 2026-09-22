# Proposal

## Why

The selected woodland now streams persistent rolling terrain, but its relief and
single winding stream remain local patterns rather than coherent regional
landforms and drainage. The next useful increment should make neighboring chunks
share deterministic ridge, valley, river, and lake authority without claiming a
complete hydrology simulation or finished water art.

## What Changes

- Adopt the existing isolated regional-generation prototype as read-only
  authority behind a new explicit integrated generation version.
- Blend bounded regional ridge and valley influence into the existing generated
  chunk terrain while preserving seams, traversal limits, spawn safety, and
  persistent generated keys.
- Resolve connected regional reach/lake descriptors for loaded chunks and expose
  them to runtime validation and later rendering.
- Use the existing procedural terrain and water-ribbon facilities for the first
  visible connected-water demonstration where supported; clearly report
  descriptor-only water where rendering is deferred.
- Add portable, native inventory, traversal, save/reload, and ordinary visual
  evidence before selecting a regional candidate.
- Treat old generated-world saves as an explicit fresh-profile reset when the
  integrated generation version changes.

## Capabilities

### New Capabilities

- `regional-landforms-water`: Deterministic regional relief influence and
  connected river/lake authority integrated into the streamed playable world.

### Modified Capabilities

None.

## Impact

The change reuses the project-authored C++17 regional prototype, current
`WorldDescriptor`, generated chunk terrain, `UProceduralMeshComponent` terrain
sections, existing water ribbon/material, chunk streaming, sparse persistence,
and Shipping QA routes. The prototype and all project assets are internal or
already attributed; no new external code, accounts, licenses, dependencies, or
downloads are required.

The smallest useful in-game result is one visibly legible regional ridge/valley
influence plus a connected water descriptor crossing neighboring loaded chunks,
with safe traversal and deterministic reload. Full-round acceptance additionally
requires rendered water continuity where claimed, collision/traversal behavior,
habitat integration, persistence, performance, and ordinary daylight images.
Waterfalls, swimming, erosion, seasonal flow, flooding, a continent-scale basin
solver, and final mountain/shore art remain deferred.
