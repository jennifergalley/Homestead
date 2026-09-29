# Spec Delta

## ADDED Requirements

### Requirement: She can wait at a closed shop until it opens
At a closed shop's door, the player SHALL be offered a wait showing the opening time and its length, SHALL confirm it with a second press or cancel it, and the wait SHALL pass the time through the ordinary simulation step so that everything that happens overnight happens.

#### Scenario: Arrive in the evening
- **WHEN** she reaches the closed general store at 7 PM, asks to wait and confirms
- **THEN** 13 hours pass through the normal time step, it's 8 AM, and the store is open

#### Scenario: Too hungry to last
- **WHEN** she asks to wait but would collapse from hunger before it opens
- **THEN** the wait is refused, no time passes, and she's told to eat first

#### Scenario: Change her mind
- **WHEN** she asks to wait and then presses B, walks away, or does nothing for eight seconds
- **THEN** no time passes
