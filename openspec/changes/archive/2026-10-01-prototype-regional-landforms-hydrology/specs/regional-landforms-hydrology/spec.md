# Spec Delta

## Purpose

Defines reproducible regional relief and connected watershed topology that
future terrain, water and habitat systems can query without visit-order or
chunk-boundary discontinuities.

## ADDED Requirements

### Requirement: Deterministic signed regional ownership

The regional model SHALL derive every result from an explicit seed, generation
version and signed global coordinate. Region ownership MUST use floor division
and half-open bounds, and unsupported versions or coordinates MUST fail without
mutating caller output.

#### Scenario: Query negative regional coordinates

- **WHEN** equivalent negative-boundary points are queried in different orders
- **THEN** each point has the same owning region and byte-identical integer
  topology result

### Requirement: Continuous regional relief potential

The model SHALL expose large-scale elevation and habitat potential in global
coordinates with matching values on shared regional borders. Relief descriptors
SHALL distinguish mountain, ridge, valley and lowland potential without claiming
rendered terrain or final biome assignment.

#### Scenario: Compare neighboring border samples

- **WHEN** adjacent regions sample their shared global border
- **THEN** elevation and habitat potential agree exactly and do not depend on
  which region was generated first

### Requirement: Canonical connected drainage

Every drainage sample SHALL have one canonical global identity and at most one
downstream target. A downstream target MUST have a lower resolved water-surface
potential, except within a lake basin where samples share one spill elevation.
Cross-region flow MUST reference the same border outlet identity from either
side.

#### Scenario: Follow flow across a region border

- **WHEN** a downstream path exits one region and enters its neighbor
- **THEN** both regions name the same global outlet and the path continues
  without duplication, reversal or an uphill segment

### Requirement: Deterministic lakes and spill outlets

A closed depression SHALL resolve to a bounded lake basin with one deterministic
spill elevation and canonical outlet selected from the surrounding drainage
topology. Lake members MUST share that water-surface elevation, and downstream
flow MUST begin at the outlet rather than an unrelated local noise minimum.

#### Scenario: Resolve a depression

- **WHEN** a depression has no lower immediate neighbor
- **THEN** its lake fills to the lowest deterministic spill saddle and drains
  through the same outlet regardless of query or region load order

### Requirement: River and habitat descriptors

Connected drainage SHALL expose stable reach descriptors including upstream and
downstream identities, water-surface endpoints, width and depth classes, flow
accumulation and habitat hooks. Width and depth MUST derive from connected
upstream contribution rather than independent per-region decoration.

#### Scenario: Join tributaries

- **WHEN** two upstream paths converge
- **THEN** the downstream reach references both contributions, has no lower
  accumulation class than either tributary and retains one stable identity

### Requirement: Bounded prototype queries

Generating one regional result SHALL use a fixed documented grid and halo with
bounded memory and work independent of exploration history. The prototype MUST
report its bounds and MUST NOT allocate durable state for unedited exploration.

#### Scenario: Generate distant regions

- **WHEN** many regions are queried in arbitrary order and discarded
- **THEN** each query remains within the documented per-region element and
  memory bounds and later regeneration returns the same result

### Requirement: Honest staged integration

The prototype SHALL remain separate from the current woodland generator and
SHALL NOT claim playable mountains, rivers, lakes, water collision or habitat
integration. A playable delivery requires explicit blending, rendering,
collision, traversal and visual validation owned by the runtime integration
lane.

#### Scenario: Complete prototype validation

- **WHEN** topology and continuity tests pass
- **THEN** the result is reported as a portable regional-generation prototype,
  while in-game hydrology and landform acceptance remain incomplete
