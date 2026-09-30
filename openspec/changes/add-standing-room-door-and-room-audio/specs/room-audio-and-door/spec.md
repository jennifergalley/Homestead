## ADDED Requirements

### Requirement: Room-aware ambience
The woodland ambience and the creek SHALL duck to 0.4 of the Ambience setting, behind a low-pass, as the
camera goes indoors. They SHALL keep the setting as their ceiling, and muted SHALL stay muted.

#### Scenario: Walking into the standing room
- **WHEN** she walks from the ruin into the roofed standing room
- **THEN** the birds and creek ease down and dull over about half a second, and the rain keeps its own indoor mix

### Requirement: Contained hearth
A roofed hearth SHALL play at 0.32 in its room. Heard from outside with a line of sight (the open door),
it SHALL keep only 0.3 of that. A closed door SHALL cut the line of sight.

#### Scenario: Standing outside the open door
- **WHEN** she stands just outside the standing room's open door
- **THEN** the crackle is faint, and it goes silent once the door shuts behind her

### Requirement: Standing-room door
The heritage stone doorway SHALL have a door leaf that opens outward, west, within 2.2 m of her and shuts
beyond 3 m. The leaf SHALL NOT collide with her or the camera. Nothing about it SHALL be saved.

#### Scenario: Going out in the morning
- **WHEN** she walks to the doorway from the bed
- **THEN** the door swings open ahead of her, she walks through without catching on it, and it closes behind her
