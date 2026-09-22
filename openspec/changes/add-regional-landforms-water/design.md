# Design

## Context

See `proposal.md` and
`specs/regional-landforms-water/spec.md`. The selected version-four woodland
streams 24 m procedural chunks, persists sparse generated edits, and renders a
legacy local stream ribbon. The isolated regional prototype supplies 960 m
regions, 30 m drainage nodes, signed global identities, bounded spill lakes,
connected reaches, and habitat hooks, but it is not included by current runtime
or build files. Its optimized portable suite must pass before adoption.

The reusable implementation is entirely project-authored C++17 and existing
Unreal Engine functionality. Existing procedural mesh, collision, material,
chunk streaming, save wrapper, Shipping QA, and compatible caches remain the
selected integration path. No new asset, dependency, account, download, or
license is required.

## Goals / Non-Goals

**Goals:**

- Adopt one tested immutable regional API behind an explicit new integrated
  generation version.
- Add bounded regional ridge/valley influence without breaking current chunk
  seams, spawn safety, traversal, persistent keys, or selected woodland art.
- Materialize connected reach/lake descriptors for loaded chunks and render one
  coherent bounded water demonstration using existing facilities where valid.
- Produce a frequent playable Shipping increment with portable, native,
  persistence, performance, and ordinary daylight evidence.

**Non-Goals:**

- Replacing the regional prototype's bounded basin policy or hiding
  `BasinTooLarge`.
- Full terrain erosion, waterfalls, swimming, flooding, seasonal discharge,
  ocean systems, final mountains, final shore ecology, or whole-world retained
  hydrology state.
- Reworking selected tree, cover, character, controls, audio, UI, or economy
  behavior.

## Decisions

### Gate adoption on the portable regional suite

The prototype source/test lane owns
`HomesteadRegionalGeneration.h/.cpp` and its portable tests. Main will integrate
only a commit that completes `/W4 /WX` compile, link, and test under the existing
guard. Fixture errors may be corrected in tests, but timeouts or semantic
failures are not bypassed.

**Alternative:** integrate the currently compiling source before tests finish.
Rejected because native terrain would make an unverified watershed contract
part of the save/version surface.

### Add an explicit adapter instead of rewriting local generation

The existing chunk generator remains responsible for stable entity keys,
woodland density, local playable relief, and per-vertex seams. A narrow regional
adapter samples the tested authority in global coordinates and contributes a
bounded low-frequency relief offset plus loaded-chunk water descriptors.

The first blend clamps influence near protected spawn/camera segments and uses
a fixed relief budget: 90% of the prior local deviation plus regional influence.
The selected fixture MUST contribute at least 300 cm across 120 m while keeping
maximum sampled non-stream slope at or below 0.22 and q95 at or below 0.14.
This deliberately revises the prior 0.20 maximum by less than ten percent and
requires native grounded traversal plus build-pocket evidence before acceptance.
The same global sample is used for every shared vertex.

**Alternative:** replace `LandHeight` with regional elevation. Rejected because
it would invalidate established terrain metrics, visual acceptance, and every
generated key assumption at once.

### Keep descriptor identity separate from render-component indices

Runtime descriptor maps use canonical regional node/reach/lake identities.
Procedural mesh sections and collision components are derived caches rebuilt
from those identities; mutable component or section indices are never persisted.
Sparse player edits remain the only durable world deltas.

**Alternative:** serialize active water component indices. Rejected because
streaming/rebuild order would make saves unstable.

### Reuse the existing procedural terrain and water material

Main owns `HomesteadWorld` terrain/water mesh, collision, inventory, and
playtests. The first visible water uses current procedural mesh/material
facilities only when descriptor endpoints can form a continuous bounded surface.
Lake/reach descriptors that cannot yet render coherently remain visible in
inventory evidence but absent from gameplay geometry.

**Alternative:** acquire a river spline or water plugin now. Rejected because
the current blocker is tested authority/integration, not missing licensed art or
engine infrastructure.

### Stage delivery in three bounded gates

1. Portable regional authority completes with deterministic fixtures and bounds.
2. Native descriptor inventory plus regional relief influence compiles and
   traverses safely without selecting a candidate.
3. One bounded connected-water render, save/reload, performance, and ordinary
   visual route qualifies a Shipping candidate.

Generator owns portable authority. Main owns adapter, terrain/water rendering,
collision, builds, and runtime evidence. The engine/compiler slot remains
serialized; source-only lanes may work independently.

## Risks / Trade-offs

- **[Regional influence creates steep or discontinuous terrain]** -> Sample only
  global coordinates, clamp amplitude/slope, retain protected-start checks, and
  reject any recovery or seam failure.
- **[Coarse 30 m drainage looks disconnected at chunk scale]** -> Treat it as
  authority and interpolate bounded render geometry; retain the local stream
  until a continuous regional replacement is visibly accepted.
- **[A real seed exceeds the 4,096-node basin cap]** -> Preserve atomic
  `BasinTooLarge`, choose a bounded test/play region for the first increment,
  and defer hierarchical basins.
- **[Version bump discards disposable test saves]** -> Use a fresh v7 preview
  profile and retain generated-woodland-32/v6 as rollback.
- **[Water rendering expands beyond evidence]** -> Inventory distinguishes
  descriptor-only and rendered reaches/lakes; promotion language names exactly
  what is visible.
- **[Regional queries regress cadence]** -> Cache immutable loaded-region
  descriptors, rebuild only on world/region changes, and compare the same
  Shipping route against woodland32.

## Migration Plan

1. Keep generated-woodland-32 selected while regional work develops.
2. Introduce a new integrated generation version and fresh isolated test
   profile; prior generated saves reject transactionally.
3. Land portable authority, then adapter/descriptor inventory, then bounded
   rendering as separate checkpoints.
4. Promote only after exact Shipping and visual/runtime acceptance.
5. Roll back by restoring generated-woodland-32/v6 and removing the regional
   adapter from the next candidate; no personal saves are migrated or deleted.
