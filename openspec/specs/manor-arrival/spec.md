# manor-arrival Specification

## Purpose
Arrive at the ruined family manor, sleep in its one standing room, and name the heroine,
family and estate.

## Requirements

### Requirement: New game names the heroine, family and estate
The New game flow SHALL include appearance selection and a Names step for first name, family surname and estate name. Each name SHALL be non-empty and at most 24 characters. The names SHALL be saved and shown in the save list.

#### Scenario: Name a new game
- **WHEN** the player enters "Clara", "Pendarves" and "Trevennor" and begins
- **THEN** the save list later shows "Clara Pendarves — Trevennor", and the boundary toast names Trevennor

#### Scenario: Controller-only setup
- **WHEN** the player completes the New game flow using only a controller
- **THEN** every field can be edited and confirmed without a mouse or keyboard

### Requirement: She arrives in the standing room
A new game SHALL spawn the heroine inside the manor's standing corner room. The room SHALL have a working bed (sleep and save), a hearth (cooking) and a chest seeded with the pail and branches.

#### Scenario: First sleep
- **WHEN** the heroine sleeps in the standing-room bed
- **THEN** the game advances to the next morning and saves

#### Scenario: Cook at the hearth
- **WHEN** the heroine cooks a known recipe at the hearth with its ingredients
- **THEN** the ingredients are consumed once and the meal is produced

### Requirement: The ruin marks the manor's footprint
The ruined manor SHALL stand on its footprint, visible from the road and the cove. Building over the footprint SHALL be rejected with a clear reason until dismantling becomes available.

#### Scenario: Build on the ruin
- **WHEN** the heroine previews a foundation inside the ruin's footprint
- **THEN** the placement is invalid with the reason "The old manor stands here"

### Requirement: Salvage piles provide the first tool heads
Salvage piles in and around the ruin SHALL provide at least one rusted head for each starter tool, as well as scrap and stone. A billhook head SHALL be within a short walk of the standing-room door.

#### Scenario: First salvage
- **WHEN** the heroine searches the salvage pile nearest the door
- **THEN** she receives a rusted billhook head, and that pile does not yield it again

### Requirement: Arrival is marked
A new game SHALL show a brief non-blocking title card with the estate name and the season and year, and SHALL add an arrival note to the field-book journal.

#### Scenario: Title card
- **WHEN** a new game begins
- **THEN** the title card appears and fades, and input is available again within about one second

### Requirement: Bed sleep fully restores energy
Any successful bed sleep SHALL leave energy at 100.

#### Scenario: Short or dawn-limited sleep
- **WHEN** she sleeps for any length in a reachable bed
- **THEN** her energy is 100 on waking

### Requirement: Evening bed sleep lasts until dawn
Bed sleep beginning at or after the earlier of 6 PM and sunset SHALL end at the earlier of 6 AM and the following sunrise, regardless of when energy fills. Sleep after midnight but before dawn SHALL end that morning. Ordinary daytime energy-restoring sleep SHALL remain available. Time-dependent world consequences and sleep saving SHALL remain active.

#### Scenario: Evening sleep with energy remaining
- **WHEN** the heroine sleeps in a reachable bed after the evening threshold with partially depleted or full energy
- **THEN** she wakes at the following dawn, not when energy first fills

#### Scenario: Ordinary daytime rest
- **WHEN** the heroine sleeps during daytime with depleted energy
- **THEN** she takes the ordinary energy-restoring rest rather than skipping to tomorrow

### Requirement: Names avoid Poldark references
Player-facing people, business and estate names SHALL NOT reference Poldark.

#### Scenario: Store and clerk
- **WHEN** she visits the general store
- **THEN** it is Trethewey's and the clerk is Mr. Josiah Trethewey
