# Spec Delta

## ADDED Requirements

### Requirement: The road signs offer the Map tab's walk
Each sign on the public road SHALL, when she is within reach, offer the walks it points to through the same confirm and walk transaction as the Map tab: nothing changes until she confirms, the walk passes its walking time, and she lands on the road at the far end.

#### Scenario: The manor's sign
- **WHEN** she stands at the sign by the manor and presses E, then chooses "Walk to town"
- **THEN** the clock moves on by the walk's time and she stands on the road in town

#### Scenario: Changing her mind
- **WHEN** she presses E at a sign and chooses "Stay here"
- **THEN** nothing changes and she is where she was

### Requirement: A clearly labelled stand-in until the sign mesh exists
Until the original sign mesh is imported, each sign SHALL be shown as a labelled stand-in post and board with its painted words.

#### Scenario: A player can read the temporary sign
- **WHEN** she approaches a road sign before its original mesh is imported
- **THEN** its post, board, and painted destination remain visible and legible
