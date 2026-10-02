# safe-game-exit Specification

## Purpose
Let players leave Homestead through a discoverable in-game path without silently
losing progress, becoming trapped by save failures, or depending on Alt-F4.

## Requirements

### Requirement: Settings exposes a pinned exit
Settings SHALL display "Save and quit to desktop" without scrolling at both
supported resolutions. From ordinary play, opening Settings and reaching its
exit confirmation SHALL require no more than three discrete button/key actions.
Every menu SHALL expose a visible Settings route with controller, keyboard and
mouse access; destructive new-world controls SHALL be visually separate.

#### Scenario: Controller player wants to quit
- **WHEN** the player opens Settings from gameplay, moves to the pinned exit and activates it
- **THEN** a clearly labeled confirmation opens without traversing volume/graphics rows or using an operating-system shortcut

### Requirement: Save and quit exits only after a verified durable save
Confirming save-and-quit SHALL freeze gameplay mutations, cancel uncommitted
menu operations, save the current committed state through the existing protected
save route, and exit only after success. The confirmation SHALL default to
staying, show the active world/profile and last successful save when known,
and never equate an attempted write with saved progress.

#### Scenario: Save completes
- **WHEN** the user explicitly confirms save-and-quit and the temporary save validates, backup succeeds and replacement completes
- **THEN** the current world, wardrobe and committed settings are durably saved before desktop exit, with no second transaction triggered by a held confirm button

### Requirement: Failed saving leaves a persistent decision
A failed save SHALL keep the game open and paused and display a persistent
error with "Retry save and quit", "Return to Settings" and "Quit without saving".
No timeout, repeated input or dialog dismissal SHALL silently discard progress.
Unknown save recency SHALL be labeled unknown rather than fabricated.

#### Scenario: Disk or permission failure
- **WHEN** writing, readback validation, backup or final replacement fails
- **THEN** the error remains actionable after ordinary toast expiry, identifies that progress is not saved, and retains existing valid saves/backups

#### Scenario: Retry succeeds
- **WHEN** the player corrects the problem and explicitly retries
- **THEN** the same current committed state is saved once and the game exits only after that save succeeds

### Requirement: Unsaved exit is an explicit escape route
"Quit without saving" SHALL be available from exit confirmation and save-failure
recovery, require its own confirmation naming potential progress loss, and
default to cancel. It SHALL never overwrite saves, reset the world or silently
restore a checkpoint as a substitute for saving.

#### Scenario: Player deliberately leaves despite save failure
- **WHEN** the player selects unsaved exit and then confirms the separate warning
- **THEN** the game exits without changing save files and without claiming unsaved state was preserved

### Requirement: Recovery and planning do not trap exit access
Survival recovery SHALL expose Settings/quit independently of checkpoint retry.
It SHALL not save a failed state over a usable checkpoint. Opening Settings from
placement SHALL safely cancel uncommitted placement; nested dialogs and loading
SHALL retain coherent pause and exit ownership.

#### Scenario: All checkpoint loads fail
- **WHEN** recovery cannot read a usable checkpoint
- **THEN** the player can still open Settings and explicitly quit, with retry failure and unsaved-progress consequences visible and no automatic new-world reset

#### Scenario: Save action in failed state
- **WHEN** Settings is opened from recovery
- **THEN** save-and-quit is disabled with an explanation, "Quit to desktop" provides the separate unsaved confirmation, and returning to recovery does not invoke retry

### Requirement: Graphics and startup errors are reported honestly
Unpersisted graphics-setting errors SHALL be distinguished from world-save
errors rather than represented as a fully saved session. Existing invalid
startup/profile routing SHALL continue to fail explicitly before save access;
it SHALL not be bypassed to offer an unsafe save path.

#### Scenario: World save succeeds but graphics persistence failed
- **WHEN** quit is requested with a known outstanding graphics-save error
- **THEN** the player is told which preference was not saved and can retry, stay, or explicitly accept exiting with that preference unsaved
