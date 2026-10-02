# Spec Delta

## Purpose

Shows what she has just gained as a short `+N Item` line beside her, based on real stock changes only.

## ADDED Requirements

### Requirement: Only real gains show
A line SHALL appear only when her pack count and her owned count (pack, chests and set-down drops) of an item both
rise. Its amount SHALL equal the smaller rise, which is also the change in her actual stacks. Moves between the
pack, chests, the hotbar row or the ground, as well as spending, eating and pail water, SHALL show nothing.

#### Scenario: Buy five pasties
- **WHEN** she buys 5 pasties
- **THEN** one `+5 Pasty` line appears, and her pack stacks hold 5 more pasties

#### Scenario: Arrange stacks
- **WHEN** she moves a stack into the hotbar row and back, splits it, sorts the pack, or takes the pail from a chest
- **THEN** no line appears

### Requirement: No duplicate or ghost lines
Gains of an item already on screen SHALL add to its existing line. Loads, new games, the first frame and rollbacks
(where the revision goes backwards) SHALL resync counts silently.

#### Scenario: Load a save
- **WHEN** she loads a save that holds more of an item than the current session
- **THEN** no line appears

### Requirement: Non-shifting, non-blocking presentation
Lines SHALL be painted without layout or hit testing. They SHALL stay clear of the calendar, the vitals and the
hotbar, and SHALL wait while the field book, the shop or setup is open.

#### Scenario: Craft from the field book
- **WHEN** she crafts a Hatchet in the field book and closes it
- **THEN** a `+1 Hatchet` line shows beside her once the book is closed, with no success toast
