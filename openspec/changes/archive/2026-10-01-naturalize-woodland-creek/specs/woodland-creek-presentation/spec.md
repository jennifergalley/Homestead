# Spec Delta

## Purpose

Defines a natural forest-creek presentation that replaces the current canal-like sandy strips while preserving deterministic world generation and existing water gameplay.

## ADDED Requirements

### Requirement: Natural creek-bank transition
The woodland creek SHALL transition from water through a narrow damp mud or leaf-litter margin into the surrounding forest-floor material. It MUST NOT present the current uninterrupted, uniformly tan bank ribbons as the dominant creek-bed surface.

#### Scenario: Ordinary creek approach
- **WHEN** the player approaches the local creek from an ordinary gameplay camera in representative daylight
- **THEN** the visible bank reads as dark damp woodland ground with an irregular vegetated transition rather than a broad sandy canal

#### Scenario: Creek viewed along its course
- **WHEN** the player looks upstream or downstream from the bank
- **THEN** the bank material and edge breakup avoid a continuous pair of parallel tan strips

### Requirement: Deterministic seamless edges
Creek-edge shape and dressing SHALL be deterministic for a world identity and global position. Adjacent generated chunks MUST agree at their shared boundary without visible gaps, overlaps, or abrupt width changes.

#### Scenario: Rebuilding the same chunk
- **WHEN** the same world and chunk are generated again
- **THEN** the creek edge and bank dressing reproduce the same positions and appearance inputs

#### Scenario: Crossing a chunk boundary
- **WHEN** the player follows the creek across an active chunk boundary
- **THEN** the water surface and bank transition remain continuous at the boundary

### Requirement: Existing water gameplay remains authoritative
The presentation change SHALL preserve the existing simulation-defined stream center and water-access behavior. Presentation geometry, materials, and decorations MUST NOT independently grant refill, watering, movement, resource, or save-state effects.

#### Scenario: Refill at the naturalized creek
- **WHEN** the player uses the existing valid creek-refill position
- **THEN** refill eligibility and the resulting authoritative water mutation match the pre-change behavior

#### Scenario: Position outside authoritative water range
- **WHEN** the player is outside the existing stream-access range even if decorative bank elements are visible
- **THEN** the game rejects refill without starting a presentation action or changing water

### Requirement: Traversable nonblocking bank dressing
Creek-bank dressing SHALL reuse nonblocking generated cover and SHALL preserve practical foot access to the water. It MUST respect existing resource, plot, structure, home, and reserved-site exclusions.

#### Scenario: Walking to the water
- **WHEN** the player approaches the creek through a representative dressed bank
- **THEN** decorative grass, reeds, ferns, and small stones do not create hidden collision or prevent reaching a valid refill position

#### Scenario: Bank near player-created content
- **WHEN** a creek-adjacent generated area contains a plot, structure, resource site, or other reserved position
- **THEN** decorative bank dressing does not overlap or obscure that reserved position

### Requirement: Bounded generated-world cost
The naturalized creek SHALL reuse the existing procedural terrain sections, authored materials, and batched cover components. It MUST NOT create per-decoration actors or add gameplay collision to decorative creek dressing.

#### Scenario: Active-window rebuild
- **WHEN** the generated active window is rebuilt around the player
- **THEN** creek presentation is rebuilt through the existing chunk and cover lifecycle without orphan components or unbounded accumulation

#### Scenario: Representative woodland cadence
- **WHEN** the accepted creek is exercised in the same representative runtime cadence route as the selected woodland build
- **THEN** the evidence records any measurable regression and the candidate is rejected if it introduces a material sustained cadence or transition regression
