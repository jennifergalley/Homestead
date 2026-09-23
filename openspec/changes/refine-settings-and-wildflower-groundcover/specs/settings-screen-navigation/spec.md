# Spec Delta

## Purpose

Defines Settings as a separate vertical preference screen with direct Esc/controller access, independent from the inventory and guidebook tab set.

## ADDED Requirements

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
Save, load, new woodland, resume, save-and-quit, quit-without-saving, retry, save failure, graphics failure, restart, and unsaved confirmations SHALL remain available through Settings/recovery with current safety behavior.

#### Scenario: Open Settings from failed state
- **WHEN** the player is failed and opens Settings
- **THEN** recovery and quit paths remain visible, focus-safe, and unable to overwrite a usable checkpoint
