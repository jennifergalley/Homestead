# Spec Delta

## ADDED Requirements

### Requirement: Sprint is a toggle
L3, or a tap of Shift, SHALL turn sprint on, and the next SHALL turn it off; how long the button is held SHALL NOT matter. Shift used as a modifier with another non-movement key SHALL NOT toggle it.

#### Scenario: Toggle with L3
- **WHEN** she presses and releases L3 and walks
- **THEN** she sprints until L3 is pressed again

#### Scenario: Shift as a modifier
- **WHEN** she holds Shift and presses Q to cycle the seed pouch
- **THEN** sprint doesn't change

### Requirement: Pauses keep the toggle, resets clear it
Stopping, an action, a menu, the shop, planning or falling SHALL suspend sprint speed and keep the toggle. A load, a new game, a retry or a teleport SHALL turn it off. Menus and the shop SHALL NOT flip it.

#### Scenario: Stop and go
- **WHEN** sprint is on and she stops, then walks on
- **THEN** she sprints again without another press

### Requirement: Too tired to sprint
At or below the Energy reserve, a request SHALL be refused, or a running sprint SHALL turn itself off, with one notice, and she SHALL walk.

#### Scenario: Run until tired
- **WHEN** she sprints until Energy reaches the reserve
- **THEN** sprint turns off with one notice and she walks on
