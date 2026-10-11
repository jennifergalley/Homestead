# Spec Delta

## ADDED Requirements

### Requirement: Beach finds refresh every morning
Shell scatters, driftwood, beach pebbles, mussel beds and laver on the beach SHALL be gatherable by hand once a day and SHALL be ready again at the next 06:00 rollover.

#### Scenario: Picked in the evening, back at dawn
- **WHEN** she gathers a mussel bed at 21:00
- **THEN** it yields 3 mussels, offers nothing more that night, and is ready again at 06:00

### Requirement: The beach campfire burns driftwood
The beach campfire SHALL light only with 3 driftwood, SHALL burn for 4 game hours as a cookfire with a warm flickering light, and SHALL then go out until lit again; a lit fire survives save and load.

#### Scenario: Lighting the fire
- **WHEN** she presses E at the unlit campfire carrying 3 driftwood
- **THEN** the driftwood is spent, the fire burns, and she can cook beside it until it burns out
