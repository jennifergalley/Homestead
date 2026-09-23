# Spec Delta

## Purpose

Defines a farther deterministic woodland horizon using bounded noncolliding LOD rings and incremental publication that preserves active-world authority and smooth traversal.

## ADDED Requirements

### Requirement: The visible world extends beyond the current terrain window
The game SHALL render deterministic terrain and major woodland silhouettes for at least two additional 24 m chunk rings beyond the current 5x5 visual window, while retaining the current bounded active collision window.

#### Scenario: Wide ordinary view
- **WHEN** the player uses the ordinary or wide third-person camera in open terrain
- **THEN** terrain and tree silhouettes continue beyond the former roughly 60 m visual radius without exposing an empty edge

#### Scenario: Approach distant content
- **WHEN** previously distant chunks move toward the player
- **THEN** their global terrain, tree identity, and edit state agree with the nearer representation

### Requirement: Far rendering uses bounded detail
Outer visual rings SHALL be noncolliding, non-navigating, lower-detail, and deterministically bounded. They MUST NOT duplicate authoritative resources, interaction focus, or player-created content.

#### Scenario: Inspect outer ring
- **WHEN** outer terrain and trees are loaded
- **THEN** they use admitted far LOD/cull policy with no gameplay collision, overlap, navigation, or resource interaction

#### Scenario: Cleared distant tree
- **WHEN** a permanently cleared generated tree is represented in an outer ring
- **THEN** the far visual remains absent and does not reappear during LOD transitions

### Requirement: Publication is incremental and cancellable
Increasing render distance MUST build on the deferred incremental component-streaming work so terrain, HISM, collision, and teardown publication are bounded across frames and obsolete destinations can be cancelled safely.

#### Scenario: Cross a chunk boundary
- **WHEN** ordinary controller travel changes the active chunk
- **THEN** the farther visual window updates without a visible multi-frame stall materially worse than the admitted transition budget

#### Scenario: Reverse direction during publication
- **WHEN** the player changes destination before outer publication completes
- **THEN** superseded work is cancelled or reused without orphan components, mixed world identity, or partial authoritative state

### Requirement: Farther view preserves performance and memory bounds
The accepted view-distance setting SHALL record component, instance, memory, preparation, and clean actor-cadence evidence and MUST stay within the project's selected hardware budget.

#### Scenario: Compare selected and extended views
- **WHEN** matched ordinary routes are run on the selected and candidate builds
- **THEN** the candidate documents its increased visible radius and is rejected for material sustained cadence, transition, or memory regression
