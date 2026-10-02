# save-compatibility Specification

## Purpose
Defines the save-compatibility contract for shipped Homestead item and world-state saves.

## Requirements

### Requirement: Appending an item keeps saves loadable
A save written by one build SHALL load in a later build of the same save version that has appended
new items to the catalogue. Items the save didn't know about SHALL load as zero in the pack and in
every chest, and everything else in the save SHALL load unchanged.

#### Scenario: Load a save from before a new item
- **WHEN** a save was written by a build whose catalogue had fewer items, and is loaded by a build that appended items
- **THEN** the game loads with the same pack, chests, world and position
- **AND** the appended items are at zero

#### Scenario: Load a save from a newer build
- **WHEN** a save's stocks are wider than this build's catalogue
- **THEN** loading is refused with the "start a new game" notice, not a corruption notice
- **AND** no save file is changed

