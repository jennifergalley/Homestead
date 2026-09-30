# Spec Delta

## ADDED Requirements

### Requirement: The minimap's glyphs stay legible at every resolution
Minimap landmark badges, the north marker and her arrow SHALL be at least a fixed number of physical
pixels across at any window size, SHALL be centred on whole physical pixels, and SHALL NOT overlap
each other.

#### Scenario: 720p
- **WHEN** the game runs at 1280x720
- **THEN** a landmark in view on the minimap is at least 11 physical pixels in radius and no two badges overlap

### Requirement: A compass at the top of the HUD shows her heading and the landmarks
The HUD SHALL show a compass band at the top centre that turns with the camera, marking N, E, S and W
and the Map tab's landmarks at their bearings, without scanning the world each frame, and without
overlapping the calendar, the toast or the minimap.

#### Scenario: Facing the manor
- **WHEN** the camera looks toward the manor
- **THEN** the manor's token hangs under the middle of the compass

### Requirement: The clock is crisp
The calendar's time of day SHALL be drawn as native Slate text at its on-screen size, with no second
Canvas copy.

#### Scenario: 4K
- **WHEN** the game runs at 3840x2160
- **THEN** the time on the calendar is sharp, like the purse beneath it
