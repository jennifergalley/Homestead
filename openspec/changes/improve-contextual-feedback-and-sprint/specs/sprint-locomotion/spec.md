# Spec Delta

## Purpose

Defines responsive hold-to-sprint traversal with visible locomotion change, faster grounded movement, and a bounded cost to the existing Energy resource.

## ADDED Requirements

### Requirement: Sprint is a held input
The player SHALL sprint while holding keyboard Shift or the controller left-stick click, provided sprint conditions remain valid. Releasing the input MUST return the player to normal walking.

#### Scenario: Keyboard sprint
- **WHEN** the player holds Shift while moving on valid ground with sufficient Energy
- **THEN** sprint starts and remains active only while Shift is held

#### Scenario: Controller sprint
- **WHEN** the player holds the controller left-stick click while moving on valid ground with sufficient Energy
- **THEN** sprint behavior matches the keyboard path

#### Scenario: Release sprint
- **WHEN** the player releases the sprint input
- **THEN** movement speed and locomotion presentation return smoothly to walking

### Requirement: Sprint is faster and visibly distinct
Active sprint SHALL move the heroine materially faster than the existing 180 cm/s walk and SHALL use a dedicated authored sprint animation rather than only accelerating the walk clip.

#### Scenario: Compare travel
- **WHEN** equivalent straight grounded movement is measured while walking and sprinting
- **THEN** sprint covers more distance over the same real duration without teleporting or changing collision rules

#### Scenario: Observe sprint motion
- **WHEN** sprint is active at ordinary gameplay distance
- **THEN** forward lean, stride, arm motion, and cadence visibly differ from walking while root motion remains disabled

### Requirement: Sprint consumes existing Energy safely
Active moving sprint SHALL consume the existing Energy resource at a modest continuous rate. Holding sprint while stationary MUST NOT consume Energy, and sprint MUST stop before Energy reaches the game's failure boundary.

#### Scenario: Moving sprint cost
- **WHEN** the player sprints continuously while grounded and moving
- **THEN** Energy decreases measurably and no second stamina resource is created

#### Scenario: Stationary hold
- **WHEN** the player holds sprint without moving
- **THEN** sprint animation does not play and Energy does not decrease

#### Scenario: Low Energy
- **WHEN** Energy reaches the sprint reserve threshold
- **THEN** sprint stops, walking remains available, and sprinting alone does not trigger survival failure

### Requirement: Sprint respects gameplay state
Sprint MUST be unavailable or cancel cleanly during menus, placement planning, failed state, airborne movement, action presentation, loading, and any state that already blocks ordinary movement.

#### Scenario: Open menu while sprinting
- **WHEN** the player opens the field book during sprint
- **THEN** sprint stops, Energy drain stops, and the paused state remains exact

#### Scenario: Start an interaction
- **WHEN** an authoritative work or gather action begins
- **THEN** sprint presentation cannot overlap, queue, or grant any additional transaction

#### Scenario: Save and reload after exertion
- **WHEN** Energy was consumed by sprint and the player saves and reloads
- **THEN** the existing Energy value persists through the current save format
