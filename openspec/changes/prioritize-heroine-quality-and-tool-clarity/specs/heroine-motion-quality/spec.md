# Spec Delta

## Purpose

Require a character-first movement improvement that is visibly natural in ordinary play while coordinating with the separate pending walk and sprint contracts already planned for Homestead.

## ADDED Requirements

### Requirement: Walking reads as intentional human movement
At the gameplay camera, the heroine SHALL show natural weight transfer, coordinated torso/hip/arm motion, planted starts and stops, and readable turns instead of one identical cycle for every direction and speed. Motion MUST remain compatible with the supported body, hair and outfit variants and MUST NOT drive player position or inventory authority.

#### Scenario: Walk, stop and turn in the woodland
- **WHEN** the player starts from rest, walks at low and full input, releases movement, and changes direction on a representative slope
- **THEN** the heroine does not visibly skate both feet, snap into idle, moonwalk through the turn, or detach from the terrain

#### Scenario: Change outfit after walking
- **WHEN** body/hair/clothing changes after a movement or work-action transition
- **THEN** the heroine resumes a valid pose without stale animation or garment separation

### Requirement: Sprint has a distinct grounded gait and safe recovery
The heroine SHALL enter a visibly different faster gait on held Shift or controller left-stick click while grounded, moving and eligible under the existing `sprint-locomotion` contract. Releasing input or losing eligibility MUST return smoothly to walking without a queued animation. Energy remains the only sprint cost and is saved through the normal world save.

#### Scenario: Compare equivalent travel
- **WHEN** ordinary walk and held sprint cover the same valid level route for the same real duration
- **THEN** sprint travels materially farther and has visibly different cadence, stride and arm motion rather than playing the walk faster

#### Scenario: Stop sprint for an interaction
- **WHEN** the heroine begins a successful Knife gather, chop, till or water action while sprint was requested
- **THEN** the action presentation takes over once, sprint stops draining Energy, and no later sprint or work gesture replays

### Requirement: Character quality is evaluated in time, not stills
Movement acceptance MUST compare chronological ordinary-play walk, sprint, stop, turn and Knife-work footage against the selected build at the same camera and route. A valid imported animation or isolated pose image alone MUST NOT count as visual acceptance.

#### Scenario: Review a replacement heroine
- **WHEN** a replacement face/body or updated garment rig is selected for trial
- **THEN** normal walking, sprinting, idle and work-action sequences are reviewed on that actual character before it can replace the selected preview
