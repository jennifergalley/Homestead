# settings-screen-navigation Specification

## Purpose
Defines Settings as a separate vertical preference screen with direct Esc/controller access, independent from the inventory and guidebook tab set.

## Requirements

### Requirement: Settings opens directly from gameplay
Pressing Esc on keyboard/mouse or controller Menu/Start during ordinary gameplay SHALL open the Settings screen directly and pause the world. Pressing the same back action inside Settings SHALL resume gameplay.

#### Scenario: Keyboard opens Settings
- **WHEN** gameplay is active and the player presses Esc
- **THEN** Settings opens directly without first opening inventory or guidebook content

#### Scenario: Controller opens Settings
- **WHEN** gameplay is active and the player presses controller Menu/Start
- **THEN** the same Settings screen opens with controller focus and prompts

#### Scenario: Close Settings
- **WHEN** Settings is open and the player presses Esc, B, or controller Menu/Start
- **THEN** Settings closes and gameplay resumes without changing another menu page

### Requirement: Field book and Settings are separate
Inventory, Crafting, Building, Guidebook, and Appearance SHALL remain field-book destinations on their purpose-specific bindings. During ordinary gameplay, `G` SHALL open Guidebook directly. Settings MUST NOT appear as a field-book tab.

#### Scenario: Open inventory
- **WHEN** the player presses the inventory binding
- **THEN** the field book opens to inventory and does not open Settings

#### Scenario: Open guidebook
- **WHEN** the player presses `G` during ordinary gameplay
- **THEN** guidebook content opens without routing through Settings

#### Scenario: Preserve contextual menu action
- **WHEN** a native menu is already open and `G` is offered as that menu's contextual take/withdraw action
- **THEN** `G` performs the focused menu action rather than changing pages

### Requirement: Settings uses a vertical list
The Settings screen SHALL present preferences and session actions as a single vertically scrolling labeled list rather than an inventory-style multi-column grid. Focus, pointer hover, scrolling, and activation MUST remain synchronized.

#### Scenario: Navigate settings by controller or keyboard
- **WHEN** the player moves up/down through Settings
- **THEN** focus advances one visible list row at a time and scrolls the focused row into view

#### Scenario: Click a setting
- **WHEN** the player clicks a visible setting control
- **THEN** that exact control receives focus and activates or edits without a redundant action sidebar

### Requirement: Game speed uses player-facing pace labels
Settings SHALL present one `Game speed` control with `Leisurely`, `Balanced`, and `Fast` choices instead of showing raw real-minute day lengths. `Leisurely` SHALL map to the existing 120-minute day, `Balanced` to the existing 60-minute default, and `Fast` to the existing 30-minute day. Normal Settings UI MUST NOT label the setting `Day length` or display `30/60/120 minutes`.

#### Scenario: Open the default setting
- **WHEN** the current day length is 60 real minutes
- **THEN** `Game speed` shows `Balanced`

#### Scenario: Choose leisurely pace
- **WHEN** the player clicks `Leisurely` or selects it with left/right input
- **THEN** the existing 120-minute-day value becomes active without changing unrelated world/settings state

#### Scenario: Reload a saved pace
- **WHEN** a current save restores one of the supported day-length values
- **THEN** the corresponding friendly Game speed label is selected exactly

### Requirement: Save and quit are direct Settings actions
Settings SHALL contain distinct `Save` and `Quit game` rows. Activating Save SHALL write the normal manual save and remain in Settings without a confirmation dialog. Activating Quit game SHALL open exactly one quit-choice dialog containing `Save & Quit` and `Quit without Saving`; Back SHALL cancel the dialog. Neither choice SHALL open a second confirmation.

#### Scenario: Save and continue
- **WHEN** the player activates `Save`
- **THEN** a manual save is attempted, Settings remains open, and success uses concise feedback without another modal

#### Scenario: Save and quit
- **WHEN** the player opens `Quit game` and chooses `Save & Quit`
- **THEN** the game saves and exits on success without another confirmation

#### Scenario: Save and quit fails
- **WHEN** `Save & Quit` cannot write a valid save
- **THEN** the game remains open in the same quit-choice context with a readable error and no false success

#### Scenario: Quit without saving
- **WHEN** the player chooses `Quit without Saving`
- **THEN** the game exits immediately without saving and without a second confirmation

#### Scenario: Cancel quitting
- **WHEN** the quit-choice dialog is open and the player presses Back
- **THEN** the dialog closes and focus returns to `Quit game` in Settings

### Requirement: Autosave is optional and configurable
Settings SHALL contain an `Autosave` On/Off control and an `Autosave interval` choice with 5, 10, 20, and 30 real unpaused gameplay minutes. The user-level default SHALL be On with a 5-minute interval. These preferences SHALL persist independently from homestead saves.

#### Scenario: Disable autosave
- **WHEN** the player sets Autosave to Off
- **THEN** no new periodic or sleep-triggered rotating autosave is written
- **AND** existing autosave files are retained
- **AND** manual Save, Save & Quit, and eligible recovery checkpoints remain available

#### Scenario: Enable autosave
- **WHEN** the player changes Autosave from Off to On
- **THEN** a fresh full interval countdown begins without writing an immediate save

#### Scenario: Change the interval
- **WHEN** the player selects 20 minutes
- **THEN** the periodic countdown resets to 20 real unpaused gameplay minutes and the preference survives relaunch without a world save

#### Scenario: Gameplay is paused
- **WHEN** a menu, planning, failure, load/recovery, or save-in-progress state pauses eligible gameplay
- **THEN** the autosave countdown does not advance

#### Scenario: Autosave succeeds
- **WHEN** the enabled interval elapses during eligible gameplay
- **THEN** one valid save writes to the next of three rotating autosave slots and the countdown resets to the configured interval

#### Scenario: Autosave fails
- **WHEN** a rotating autosave cannot be written/read back safely
- **THEN** no success is reported, existing valid saves are retained, one clear error is shown, and retries are rate-limited rather than attempted every frame

#### Scenario: Autosave settings are invalid or read-only
- **WHEN** persisted enabled/interval data is malformed or a preference write cannot be read back exactly
- **THEN** startup uses safe On/5-minute defaults or restores the previous valid preference with an explicit error

### Requirement: Credits is removed from the game menu
The in-game Credits tab, page, focus target, navigation stop, and icon SHALL be absent. Removing the tab MUST NOT remove the packaged attribution file.

#### Scenario: Traverse every field-book tab
- **WHEN** the player cycles through all field-book tabs in either direction
- **THEN** Credits never appears and navigation wraps among the remaining destinations

#### Scenario: Inspect packaged attribution
- **WHEN** the game is packaged
- **THEN** `asset-credits.md` remains included even though no in-game Credits page exists

### Requirement: Preview metadata is confined to Settings
Preview profile/version metadata SHALL be absent from the ordinary gameplay HUD and SHALL appear only in Settings when the running build has preview metadata. A normal build without preview metadata MUST NOT show an empty or generic version row.

#### Scenario: Play an isolated preview build
- **WHEN** gameplay is visible outside Settings
- **THEN** no preview/version footer is drawn
- **AND WHEN** Settings opens
- **THEN** the preview profile/version and isolated-save status are visible there

#### Scenario: Play a normal build
- **WHEN** Settings opens without preview metadata
- **THEN** no preview/version row is shown

### Requirement: Session and recovery actions remain safe
Save, load, new woodland, resume, the single quit-choice dialog, retry, save failure, graphics failure, and restart SHALL remain available through Settings/recovery with current safety behavior. The obsolete chained unsaved-exit confirmation MUST NOT remain in the normal quit path.

#### Scenario: Open Settings from failed state
- **WHEN** the player is failed and opens Settings
- **THEN** recovery and `Quit game` remain visible and focus-safe, Save/Save & Quit cannot overwrite a usable checkpoint, and Quit without Saving remains immediate

### Requirement: Settings directional steps move one visible row
Each Up or Down step from the D-pad, left stick, or keyboard arrows on the Settings screen SHALL
move focus to the adjacent visible row in that direction, at every supported window size. Every
Settings row SHALL be reachable by directional input alone. Left and Right SHALL continue to adjust
the focused setting and MUST NOT move between rows.

#### Scenario: Step down from Save
- **WHEN** Settings is open with Save focused and the player presses D-pad Down once
- **THEN** Load latest save is focused and highlighted, not Game speed

#### Scenario: Reach every row
- **WHEN** the player presses Down repeatedly from the first row to the last
- **THEN** focus visits every Settings row exactly once, in visual order, with the list scrolling to keep the focused row visible

#### Scenario: Small editor or windowed viewport
- **WHEN** Settings is shown in a viewport about 1000 px wide, such as the default Play-In-Editor viewport
- **THEN** one Up or Down step still moves exactly one visible row
