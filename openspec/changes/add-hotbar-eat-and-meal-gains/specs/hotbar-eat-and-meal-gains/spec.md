# Spec Delta

## ADDED Requirements

### Requirement: Interact buttons eat hotbar food when nothing else is focused
With food selected on the hotbar and no focused interactable, the Interact (A / E) and Secondary (X / F) buttons SHALL eat one portion per press, and the cue SHALL show the device's Interact glyph with "Eat". A focused interactable SHALL keep its own action.

#### Scenario: Berries on open ground
- **WHEN** berries are selected and she presses A or X on open ground
- **THEN** she eats one berry per press

#### Scenario: Berries at the shopkeeper
- **WHEN** berries are selected and she faces the shopkeeper
- **THEN** the cue offers Talk and neither button eats

#### Scenario: The stack runs out
- **WHEN** the last berry is eaten and she presses A again
- **THEN** nothing is eaten and a notice says none are left

### Requirement: Meals show their actual gains on the vitals
After a meal from the hotbar, each bar that rose SHALL fill smoothly to its new value, and SHALL show "+N Food" or "+N Energy", where N is the actual change after caps, without moving any HUD element. A bar that didn't rise by at least one point SHALL show nothing.

#### Scenario: Eat when tired
- **WHEN** she eats berries at 40 energy
- **THEN** the energy bar fills to the new value and "+6 Energy" appears beside it and fades
