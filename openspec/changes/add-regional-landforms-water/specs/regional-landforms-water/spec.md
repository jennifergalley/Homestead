# Spec Delta

## Purpose

Defines the first playable integration of deterministic regional relief and
connected water authority into Homestead's streamed generated woodland.

## ADDED Requirements

### Requirement: Versioned regional integration

The generated world SHALL use an explicit integrated generation version when
regional relief or drainage semantics affect terrain or water. Unsupported prior
generated-world saves MUST fail before partial world materialization, and a fresh
test profile MUST be disclosed rather than silently reinterpreting saved edits.

#### Scenario: Load a pre-regional generated save

- **WHEN** a save created with the prior woodland generation version is loaded by
  the regional build
- **THEN** loading fails transactionally with the prior world and save bytes
  unchanged

### Requirement: Seam-safe regional relief influence

Loaded terrain chunks SHALL derive bounded ridge, valley, and lowland influence
from deterministic global regional descriptors. Shared chunk vertices MUST
remain identical from either side, and traversable slopes, spawn protection, and
terrain recovery limits MUST remain explicitly validated.

#### Scenario: Walk across a regional influence boundary

- **WHEN** the player crosses neighboring chunks affected by the same regional
  ridge or valley
- **THEN** terrain remains continuous and grounded without a recovery, rail,
  invisible wall, or fixed clearing

### Requirement: Connected water authority

The integrated world SHALL expose stable connected reach and lake descriptors
using canonical global identities from the regional prototype. Neighboring
loaded chunks MUST agree on downstream and outlet identities, and any
`BasinTooLarge` result MUST be reported without partial descriptor mutation.

#### Scenario: Query water across loaded chunks

- **WHEN** a regional reach or lake outlet crosses a loaded chunk boundary
- **THEN** both chunks reference the same canonical connection and downstream
  direction

### Requirement: Honest bounded rendering

The first delivery SHALL render water only where the integrated descriptor and
existing bounded mesh/collision facilities can represent it coherently. Any
descriptor without finished rendering MUST remain explicitly descriptor-only;
the build MUST NOT claim complete rivers, lakes, swimming, waterfalls, erosion,
or continent-scale hydrology.

#### Scenario: Inspect the first playable regional candidate

- **WHEN** ordinary gameplay frames and inventory evidence are reviewed
- **THEN** each visible water surface is backed by a connected descriptor and
  every unrendered regional feature is reported as deferred

### Requirement: Persistent regional world behavior

Current-version regional worlds SHALL preserve deterministic world descriptors,
tree clearing, structures, plots, inventory, and loaded-chunk regeneration
through mapped save/load and separate-process replay.

#### Scenario: Modify and revisit a regional site

- **WHEN** the player clears a tree, builds or cultivates, saves, travels across
  chunk boundaries, and reloads in another process
- **THEN** regional terrain/water identities and durable player edits return
  without handle reuse, duplicate visuals, or lost state

### Requirement: Measured playable acceptance

Promotion SHALL require portable regional tests, native descriptor inventory,
grounded traversal, ordinary visual evidence, and measured timing. Performance
evidence MUST remain qualified as instrumented/offscreen unless actual GPU or
presentation timing is measured.

#### Scenario: Promote the first regional increment

- **WHEN** the regional candidate is considered for selection
- **THEN** the exact Shipping binary, fresh profile/reset requirement, rollback
  candidate, visual limits, deferred hydrology scope, and timing methodology are
  recorded
