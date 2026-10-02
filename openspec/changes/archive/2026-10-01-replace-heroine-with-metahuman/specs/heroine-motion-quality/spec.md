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

#### Scenario: Walk over uneven ground
- **WHEN** the heroine walks or sprints across the generated woodland
- **THEN** heel strikes and toe-offs stay on the terrain surface instead of dipping into it

### Requirement: Footsteps match her feet and the ground
Each footstep sound SHALL play when one of the heroine's feet visibly touches down, one sound per
touchdown at any walking or sprinting speed. The sound SHALL suit bare feet on soft woodland soil:
soft and dull, with no hard heel click, and quiet under the ambience.

#### Scenario: Walk then sprint
- **WHEN** the heroine walks and then sprints in a straight line
- **THEN** the footstep log shows alternating left and right steps at her stride cadence (about two per second walking and three sprinting), not a fixed distance interval

### Requirement: The upper body stays steady while moving
Walking and sprinting SHALL keep the heroine's torso, shoulders and head carried the way the source
motion capture carries them. Retargeting or playback MUST NOT add side-to-side sway or a rocking
yaw of the body line, and the body SHALL face the direction of travel.

#### Scenario: Walk straight ahead
- **WHEN** the heroine walks in a straight line in ordinary play
- **THEN** her head moves no more from side to side than in the source walk clip, and the recorded footage shows no wobble at the head and shoulders

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
