# closed-shop-wait Specification

## Purpose
Allow players at a closed General Store to wait for opening while the estate's ordinary calendar and world simulation continue.

## Requirements

### Requirement: Sunday waiting reaches Monday opening
At the General Store on Sunday the player SHALL be offered a cancellable wait naming Monday morning opening. Confirming SHALL advance through the existing authoritative time logic until that opening, including calendar/season, crop, rest/energy and normal world consequences, and permit entering the store. Normal overnight waiting SHALL remain available. Invalid or out-of-reach requests SHALL not change state.

#### Scenario: Sunday wait and entry
- **WHEN** the heroine confirms waiting at the General Store door on Sunday
- **THEN** the clock reaches Monday opening, ordinary world and rest consequences have occurred, and she can enter

#### Scenario: Cancel the wait
- **WHEN** the player cancels the offered wait
- **THEN** no time passes and the store stays closed
