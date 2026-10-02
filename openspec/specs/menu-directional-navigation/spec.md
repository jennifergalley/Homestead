# menu-directional-navigation Specification

## Purpose
Let players browse every visible menu section naturally with directional input,
without a trigger requirement or unintended gameplay and inventory actions.

## Requirements

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

### Requirement: Dialog navigation and quantity adjustment are intuitive
Dialogs SHALL trap focus until explicitly closed. Quantity controls SHALL use a
visible minus/value/plus stepper. While that control is focused, Left/Right
SHALL adjust the draft amount and Up/Down SHALL leave the control. Back SHALL
cancel the dialog without committing. Focus movement and adjustment SHALL not
submit a transaction or activate destructive defaults.

#### Scenario: Browse a quantity dialog
- **WHEN** the player focuses the amount stepper
- **THEN** Left/Right changes only the draft amount, Up/Down reaches adjacent controls, and Confirm alone commits

#### Scenario: Cancel and focus return
- **WHEN** a quantity adjustment, exit prompt or save-failure dialog is canceled
- **THEN** focus returns to a valid initiating control or nearest surviving subject, without escaping the modal early or unpausing its parent menu

### Requirement: Menu chrome relies on intuitive affordances instead of coaching
Menus SHALL NOT display persistent generic instructions for standard navigation,
selection, activation, tabs, or Back. Headers, summaries, empty states, details,
buttons, values, requirement states, errors, and confirmations SHALL contain
only information specific to the current game state or action. The Guidebook
MUST NOT contain an entry whose purpose is to explain ordinary menu controls.

#### Scenario: Open a normal field-book page
- **WHEN** Inventory, Craft, Build, Guidebook, Appearance, or Settings is open
- **THEN** no footer or banner says how to move, select, activate, change tabs, or go back
- **AND** visible focus, layout, labels, buttons, values, and controls remain sufficient to navigate and act

#### Scenario: No transient message exists
- **WHEN** a menu is paused and there is no current toast, error, confirmation, or other state-specific message
- **THEN** the message area is empty or shows only concise state such as `Time paused`
- **AND** it does not fill the space with generic advice

#### Scenario: Inspect Guidebook topics
- **WHEN** the player browses every Guidebook entry
- **THEN** no topic explains D-pad, stick, arrow, activation, tab, or Back controls

### Requirement: Existing shortcuts remain reachable without instruction prose
LB/RB tab shortcuts SHALL remain. Trigger/Tab region cycling MAY remain optional.
Removing their on-screen instructions MUST NOT remove or remap the actual inputs.
Settings' Quit game dialog SHALL keep Save & Quit and Quit without Saving
directionally reachable, default focus SHALL avoid the no-save choice, and Back
SHALL cancel without a chained confirmation.

#### Scenario: Quit remains discoverable
- **WHEN** the player activates Quit game in Settings
- **THEN** one modal exposes Save & Quit and Quit without Saving, with Save & Quit initially focused
- **AND** activating either choice performs that choice without another prompt
