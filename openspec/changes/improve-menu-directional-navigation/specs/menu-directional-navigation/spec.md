# Spec Delta

## Purpose

Let players browse every visible menu section naturally with directional input,
without a trigger requirement or unintended gameplay and inventory actions.

## ADDED Requirements

### Requirement: Directional boundaries lead to adjacent sections
D-pad, left stick and keyboard arrows SHALL retain ordinary grid/list movement
and move to a sensible visible neighboring section at its directional boundary.
Reverse paths SHALL be available without triggers. Outer menu edges SHALL not
leak focus into the world; moving focus SHALL never activate an action.

#### Scenario: Last carried row
- **WHEN** the player presses Down on the final carried row
- **THEN** focus reaches the equipment controls below it, with Up returning to the nearest applicable carried item

#### Scenario: Sideways and upward navigation
- **WHEN** the player moves beyond an item's left/right/top boundary
- **THEN** focus can reach the visible portrait, details/actions or upper controls in that direction, without changing the active page or possessions merely by focusing them

### Requirement: Sparse and scrolled content remains navigable
Desired-column behavior SHALL survive short rows. Offscreen logical rows SHALL
scroll into view before section exit. Empty sections SHALL provide a focusable
explanation and routes out; unavailable actions SHALL be inspectable or skipped
without trapping focus. Native focus, visual highlight and semantic subject IDs
SHALL agree after refresh, removal, tab return and modal cancellation.

#### Scenario: Short final row and reverse
- **WHEN** a column-four item moves down to a two-item final row and then back up
- **THEN** the prior desired column is restored while moving farther down exits to the lower adjacent controls

#### Scenario: Empty or full inventory
- **WHEN** there are zero carried entries or enough entries to scroll
- **THEN** directional navigation neither selects nonexistent items nor skips remaining rows, and all visible sections remain reachable

### Requirement: Stick intent navigates without world or portrait side effects
The left stick SHALL feed the same directional path as the D-pad/arrows with
deadzone and repeat handling. It MUST NOT move the world character or rotate
the portrait while browsing. The existing accepted-intent classifier SHALL
remain authoritative; right-stick inspection and explicit portrait actions
SHALL retain their roles.

#### Scenario: Left stick across a boundary
- **WHEN** the left stick moves down from the final grid row
- **THEN** focus crosses the same boundary as D-pad Down, with no pawn movement, portrait rotation, item mutation or noisy-device hint change

### Requirement: Dialog navigation and quantity editing are distinct
Dialogs SHALL trap focus until explicitly closed. Quantity controls SHALL
require explicit entry into editing before directions change the draft amount;
Back SHALL leave editing before canceling the dialog. Focus movement and editor
exit SHALL not submit a transaction or activate destructive defaults.

#### Scenario: Browse a quantity dialog
- **WHEN** the player navigates from Cancel toward the amount control
- **THEN** focus changes without modifying the amount until explicit activation enters editing, and Confirm alone commits

#### Scenario: Cancel and focus return
- **WHEN** a quantity edit, exit prompt or save-failure dialog is canceled
- **THEN** focus returns to a valid initiating control or nearest surviving subject, without escaping the modal early or unpausing its parent menu

### Requirement: Existing shortcuts remain truthful and reachable
LB/RB tab shortcuts and the three-press gameplay-to-Settings-exit path SHALL
remain. Trigger/Tab region cycling MAY remain optional, but hints and guide text
SHALL describe directions as the primary way to move between sections.

#### Scenario: Quit remains discoverable
- **WHEN** the player opens Settings from gameplay, moves Right and activates
- **THEN** the cancel-default exit confirmation opens in the existing three presses
