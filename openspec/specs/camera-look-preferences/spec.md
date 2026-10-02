# camera-look-preferences Specification

## Purpose
Defines intuitive vertical camera behavior and durable player-level look preferences that remain independent from any individual homestead save.

## Requirements

### Requirement: Non-inverted look follows physical input
With vertical inversion Off, the game SHALL move the camera upward when the player moves the mouse upward or pushes the camera stick upward. With vertical inversion On, the game SHALL reverse that vertical response. Off MUST be the default when no valid preference has been stored.

#### Scenario: Fresh mouse look
- **WHEN** no camera preference exists and the player moves the mouse upward during gameplay
- **THEN** the camera pitch moves upward and Settings reports `Invert camera Y: Off`

#### Scenario: Fresh controller look
- **WHEN** no camera preference exists and the player pushes the camera stick upward during gameplay
- **THEN** the camera pitch moves upward

#### Scenario: Inverted vertical look
- **WHEN** the player enables vertical inversion and supplies the same upward mouse or stick input
- **THEN** the camera pitch moves downward

### Requirement: Camera preferences persist immediately
Changing camera sensitivity or vertical inversion SHALL persist that preference immediately in user settings without requiring a world save, autosave, or Save and quit.

#### Scenario: Quit without saving the homestead
- **WHEN** the player changes vertical inversion, quits without saving world progress, and relaunches the same installation
- **THEN** the relaunch uses the selected inversion preference

#### Scenario: Sensitivity relaunch
- **WHEN** the player changes camera sensitivity and relaunches the game
- **THEN** the relaunch uses the selected sensitivity

### Requirement: Camera setting rows activate directly
The left-side camera sensitivity and vertical-inversion rows SHALL be the actionable controls. Clicking a camera row or pressing A/Enter while it is focused SHALL change that setting directly, and the right sidebar MUST NOT expose a separate `Change setting` action for either row.

#### Scenario: Toggle inversion from the left row
- **WHEN** the player focuses the vertical-inversion row and presses A/Enter
- **THEN** the inversion preference toggles and focus does not move to a redundant action button

#### Scenario: Click sensitivity on the left
- **WHEN** the player clicks the camera-sensitivity row
- **THEN** sensitivity advances once and the preference is persisted

#### Scenario: Inspect camera-setting details
- **WHEN** either camera-setting row is selected
- **THEN** its explanatory details remain visible without a `Change setting` button in the right sidebar

### Requirement: World saves do not own camera preferences
Loading, creating, resetting, or switching homestead saves MUST NOT overwrite valid user-level camera preferences.

#### Scenario: Load a save with legacy camera fields
- **WHEN** the player has stored camera preferences and loads a current-version world save containing different legacy camera values
- **THEN** the user-level camera preferences remain active

#### Scenario: Start a new woodland
- **WHEN** the player starts a new woodland after changing camera preferences
- **THEN** the new woodland uses the existing user-level camera preferences

### Requirement: Preference persistence is isolated and verified
Saving a camera preference SHALL change only the requested camera property in the resolved user-settings file. The game MUST verify the persisted value and, on failure, restore the prior runtime preference, preserve unrelated settings, and display a clear error.

#### Scenario: Successful inversion write
- **WHEN** the player toggles vertical inversion and the settings file is writable
- **THEN** the game verifies the stored inversion value, keeps the new runtime behavior, and leaves unrelated graphics settings unchanged

#### Scenario: Read-only settings file
- **WHEN** the player changes a camera preference while the resolved settings file is read-only
- **THEN** the game reports that the preference could not be saved, restores the previous runtime value, and leaves the file unchanged

#### Scenario: Missing or invalid stored camera value
- **WHEN** startup finds no stored value or a non-finite, out-of-range, or malformed camera preference
- **THEN** the game uses the safe default for that property without invalidating or modifying the player's world save
