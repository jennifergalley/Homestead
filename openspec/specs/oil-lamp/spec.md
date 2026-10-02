# oil-lamp Specification

## Purpose
Give her a first light she can carry and set down, fuelled by oil bought in town.

## Requirements

### Requirement: She starts with a lamp and oil
A new game SHALL start with a full oil lamp pinned to the hotbar and three oil flasks in her pack.
A save made before the lamp existed SHALL receive the same kit once when loaded, if her pack has
room.

#### Scenario: New game
- **WHEN** she starts a new game
- **THEN** her pack holds 1 Oil lamp (full) and 3 Oil flasks, and the lamp is on the hotbar

### Requirement: The lamp burns oil only while lit
The lamp SHALL hold at most six game hours of oil. It SHALL burn oil at one hour per game hour
while lit: in her hand as the selected hotbar tool while she is awake, or set down on the ground.
It SHALL go out at empty.

#### Scenario: Held at night
- **WHEN** she holds the lit lamp for two game hours
- **THEN** it has two hours less oil

#### Scenario: Put away
- **WHEN** another hotbar slot is selected
- **THEN** the lamp is out and its oil doesn't change

### Requirement: Flasks refill the lamp and are sold in town
An oil flask SHALL fill the lamp, and the general store SHALL sell oil flasks.

#### Scenario: Refill
- **WHEN** she refills a half-empty lamp from a flask
- **THEN** the lamp is full and she has one flask fewer

### Requirement: The lamp can be set down and picked up
She SHALL be able to set the lamp on dry ground within reach, where it SHALL keep burning oil and
lighting the area. She SHALL be able to pick it up again. A set-down lamp SHALL be saved.

#### Scenario: Set down and saved
- **WHEN** she sets the lamp down, saves and loads
- **THEN** the lamp is on the ground where she left it, with the oil it had, still burning
