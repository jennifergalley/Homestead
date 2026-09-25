# Spec Delta

## Purpose

Require a character-first movement improvement that is visibly natural in ordinary play while coordinating with the separate pending walk and sprint contracts already planned for Homestead.

## ADDED Requirements

### Requirement: Locomotion is motion-matched on the MetaHuman
Walking, jogging, sprinting, starts, stops, pivots and turns SHALL be selected from a motion-capture
database by motion matching, so the heroine plants her feet on starts and stops, leans into turns,
and changes direction without sliding or popping. Player position and speed SHALL remain governed
by the game's movement rules, not by animation.

#### Scenario: Start, stop and reverse
- **WHEN** the player walks forward, stops, then pushes the stick the opposite way in ordinary play
- **THEN** recorded footage shows a planted stop, a readable pivot and acceleration away, with no visible foot sliding

#### Scenario: Circle strafe with the camera
- **WHEN** the player orbits the camera while walking
- **THEN** the heroine turns her body smoothly toward the movement direction without snapping

### Requirement: Feet and hands meet the world
The heroine's feet SHALL adapt to the generated terrain's slopes and small height changes while
standing and walking. During work actions her hands SHALL meet the tool and target: the hatchet
strikes the trunk, the watering can tips over the plot, and gathering reaches the plant.

#### Scenario: Stand on a slope
- **WHEN** the heroine stands still on a sloped bank near the creek
- **THEN** both feet rest on the ground with hips adjusted, not floating or sinking

#### Scenario: Chop a tree
- **WHEN** the player fells a focused tree
- **THEN** the swing aligns to the trunk and the hatchet visibly contacts it at impact

### Requirement: The face is alive
While idle, walking and working, the heroine SHALL blink at natural intervals, move her eyes with
small saccades, breathe, and turn her head and eyes toward nearby points of interest, such as
the focused resource or the camera in the Appearance preview. These behaviors MUST NOT interrupt
or distort body animation.

#### Scenario: Idle close-up
- **WHEN** the heroine stands idle for ten seconds in the Appearance preview
- **THEN** she blinks and shifts her gaze naturally rather than holding a fixed stare
