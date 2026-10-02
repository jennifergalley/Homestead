# persistent-generated-woodland Specification

## Purpose
Defines the persisted generated-woodland contract retained from the shipped survival prototype.

## Requirements

### Requirement: Seeded continuous woodland
The game SHALL generate smooth woodland chunks from an explicit seed and
generation version, independent of load order, with seam-consistent shared
heights/normals and correct negative-coordinate ownership.

#### Scenario: Travel beyond the prototype
- **WHEN** the player crosses the old80m boundary and returns
- **THEN** safe loaded terrain and coherent woodland continue beyond it, without
  fixed rails, terrain cracks or a pre-cleared house site.

### Requirement: Authoritative remembered resources
Actionable generated resources and standing playable trees SHALL have stable
composite generated identities distinct from transient runtime handles.
Felled trees and occupied/player-cleared sites SHALL persist across unload and
save/reload; unsupported versions and exhausted persistence limits SHALL fail
explicitly and atomically without discarding player work.

#### Scenario: Fell and build at a chosen site
- **WHEN** the equipped-by-possession hatchet requirement and reach/capacity
  checks pass and the player fells a mature tree through production controls
- **THEN** the existing reward is granted once, trunk collision is removed,
  cleared space is buildable and the tree stays absent after travel and reload.

### Requirement: Bounded safe active world
The runtime SHALL bound live chunks/assets while preserving near-player
collision and durable edits outside the active window.

#### Scenario: Replace streamed chunks
- **WHEN** the active region changes or a saved distant position is loaded
- **THEN** destination terrain is ready before obsolete ground is released or
  the pawn is moved, and inactive edits/structures/plots are not evicted.

### Requirement: Honest woodland delivery
Acceptance SHALL include actual dense untouched woodland and player-cleared
site views plus travel/chop/build/save/return evidence, not only technical
generation counters.

#### Scenario: First generated-world delivery
- **WHEN** the candidate is offered for normal playtesting
- **THEN** its seed/schema/profile and supported limits are explicit, and larger
  mountains/connected rivers/lakes remain unclaimed until implemented.

