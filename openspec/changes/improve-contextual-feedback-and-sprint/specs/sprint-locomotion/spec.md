# Spec Delta

## Purpose

Defines responsive toggle-to-sprint traversal with visible locomotion change, faster grounded movement, and safe low-Energy admission without a sprint-specific Energy cost.

## ADDED Requirements

### Requirement: Sprint is a toggle input
The player SHALL toggle sprint with keyboard Shift or controller left-stick click, provided sprint conditions remain valid. A second toggle MUST return the player to normal walking.

#### Scenario: Keyboard sprint
- **WHEN** the player toggles Shift while moving on valid ground with sufficient Energy
- **THEN** sprint starts and remains active until a second toggle or a sprint-cancellation condition

#### Scenario: Controller sprint
- **WHEN** the player toggles the controller left-stick click while moving on valid ground with sufficient Energy
- **THEN** sprint behavior matches the keyboard path

#### Scenario: Toggle sprint off
- **WHEN** the player toggles sprint a second time
- **THEN** movement speed and locomotion presentation return smoothly to walking

### Requirement: Sprint is faster and visibly distinct
Active sprint SHALL move the heroine materially faster than the existing 180 cm/s walk and SHALL use a dedicated authored sprint animation rather than only accelerating the walk clip.

#### Scenario: Compare travel
- **WHEN** equivalent straight grounded movement is measured while walking and sprinting
- **THEN** sprint covers more distance over the same real duration without teleporting or changing collision rules

#### Scenario: Observe sprint motion
- **WHEN** sprint is active at ordinary gameplay distance
- **THEN** forward lean, stride, arm motion, and cadence visibly differ from walking while root motion remains disabled

### Requirement: Sprint respects an Energy threshold without extra cost
Active sprint SHALL NOT consume the existing Energy resource beyond its baseline awake-time drain and ordinary work costs. Sprint MUST be refused at Energy <=10 and MUST turn off when those other costs reach that threshold. A toggled sprint state while stationary MUST NOT mutate Energy, and sprint MUST NOT auto-resume when Energy later recovers.

#### Scenario: Moving sprint does not spend extra Energy
- **WHEN** the player sprints continuously while grounded and moving
- **THEN** sprint itself does not change Energy and no second stamina resource is created

#### Scenario: Stationary hold
- **WHEN** the player has sprint toggled on without moving
- **THEN** sprint animation does not play and Energy does not change

#### Scenario: Low Energy
- **WHEN** Energy is at or falls to 10 through ordinary work or time
- **THEN** sprint is refused or turns off, walking remains available, and sprint does not auto-resume after Energy recovers

### Requirement: Sprint respects gameplay state
Sprint MUST be unavailable or cancel cleanly during menus, placement planning, failed state, airborne movement, action presentation, loading, and any state that already blocks ordinary movement.

#### Scenario: Open menu while sprinting
- **WHEN** the player opens the field book during sprint
- **THEN** sprint stops, the paused state remains exact, and sprint does not auto-resume on close

#### Scenario: Start an interaction
- **WHEN** an authoritative work or gather action begins
- **THEN** sprint presentation cannot overlap, queue, or grant any additional transaction

#### Scenario: Save and reload do not resume sprint
- **WHEN** the player saves and reloads after sprinting
- **THEN** Energy retains only its baseline/work changes and sprint remains off until explicitly requested again
