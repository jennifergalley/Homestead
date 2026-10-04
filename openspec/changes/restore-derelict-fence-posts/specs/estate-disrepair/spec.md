# Spec Delta

## ADDED Requirements

### Requirement: Supporting fence posts remain visible with the fence
The broken-down farm fence SHALL render its existing supporting upright and leaning post bodies while the fence remains. Clearing nearby or subordinate pieces, tilling nearby ground, changing view distance or loading a save MUST NOT independently remove supports; support removal SHALL require removal of the whole fence.

#### Scenario: Clear and cultivate beside a standing fence bay
- **WHEN** the player clears vegetation or subordinate debris and tills beside a fence bay whose rails remain
- **THEN** its supporting post bodies remain visible and the rails do not float unsupported

#### Scenario: Return to the fence
- **WHEN** the player walks away and back or reloads a save
- **THEN** remaining supported fence bays render their post bodies consistently
