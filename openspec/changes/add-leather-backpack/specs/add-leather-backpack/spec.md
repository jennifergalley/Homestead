# Spec Delta

## ADDED Requirements

### Requirement: The leather backpack doubles her pack once
The open General Store SHALL offer the leather backpack once, for 1,500 coins (tentative), as an upgrade rather than repeating goods. Buying it SHALL raise her pack from 120 to 240 and remove the offer, and it SHALL survive saving and loading. Saves from before it SHALL load with the 120 pack.

#### Scenario: Buying it
- **WHEN** she has 2,000 coins and buys the leather backpack at the counter
- **THEN** she has 500 coins, can carry 240, and the store no longer offers it

#### Scenario: Not enough money
- **WHEN** she has 1,000 coins and chooses the backpack
- **THEN** she is told it costs 1,500 coins and nothing changes

### Requirement: Hiding the backpack is a look only
Appearance SHALL let her show or hide the backpack once she owns it, saved with the game, without changing her pack's capacity.

#### Scenario: Hidden
- **WHEN** she hides the backpack with 240 things carried and saves and loads
- **THEN** it doesn't show on her back and she still carries 240
