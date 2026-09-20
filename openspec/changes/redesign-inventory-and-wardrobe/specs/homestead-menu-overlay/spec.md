# Spec Delta

## Purpose

Provide clear, readable native menus for managing possessions and the homestead
with equal controller, keyboard and mouse access while the world is paused.

## ADDED Requirements

### Requirement: Implementation follows the accepted environment baseline
The redesign MUST begin implementation only after the environment change is
completed and accepted, the applicable offline authoring workflow is approved
and proven, and a new implementation authorization is recorded.

#### Scenario: Environment work is still pending
- **WHEN** the UI proposal is artifact-complete but environment acceptance or tool approval is missing
- **THEN** it remains a plan and does not authorize code, asset authoring, engine execution or preview promotion

### Requirement: Fullscreen shell communicates screen purpose
The menu SHALL fill the viewport with a translucent world backdrop and readable
content regions, using top icon tabs with text labels and a non-color-only
current-tab indicator. Inventory, Craft, Build, Guidebook, Settings, Credits and
Appearance SHALL remain distinct; recipes and plans MUST NOT imply ownership.

#### Scenario: Player changes from Inventory to Craft
- **WHEN** the player changes tabs
- **THEN** the active icon and label identify Craft, the page says "Crafting recipes", cards show requirements and availability rather than carried counts, and the world remains paused

#### Scenario: Page names are unfamiliar
- **WHEN** any top icon is hovered or focused
- **THEN** its readable label and explanatory tooltip identify its purpose without requiring recognition of the icon

### Requirement: Grids carry truthful content and complete icons
Carried items, available chest contents, recipes and building plans SHALL use
grids with complete recognizable icons, names and relevant quantities/status.
Settings SHALL use labeled native controls arranged in sections, not fake item
cells. Guidebook and Credits SHALL support readable text and scrolling.

#### Scenario: Inventory contains no carried items
- **WHEN** the player opens an empty inventory away from storage
- **THEN** the carried region explicitly says it is empty, equipped clothing remains visible separately, and no recipe output is shown as an owned item

#### Scenario: An action is unavailable
- **WHEN** a recipe lacks materials or a chest is out of reach or full
- **THEN** the affected action shows the precise reason and remains inspectable without performing an invalid transaction

### Requirement: Details support both hover and explicit focus
The details pane SHALL present the subject's name, icon, authoritative
description, quantity/location, relevant requirements and valid actions.
Mouse hover SHALL preview details without activating or changing ownership;
keyboard/controller focus SHALL expose the same information. Information MUST
NOT invent warmth, armor, value, durability, rarity or buffs.

#### Scenario: Mouse leaves a hovered item
- **WHEN** the pointer leaves a temporary hovered item
- **THEN** details return to the stable selected/focused subject or explicit empty guidance without equipping, consuming or transferring anything

#### Scenario: Focus changes without a mouse
- **WHEN** the player navigates to an owned tunic using the D-pad or arrow keys
- **THEN** its exact dye, ownership location, occupied slots and allowed actions appear in the details pane

### Requirement: Grid and tab navigation do not compete
All menu tasks SHALL be possible with controller alone and keyboard alone.
Within menus, four-way D-pad/arrow input SHALL move within content, shoulders
and Ctrl+Tab/Ctrl+Shift+Tab SHALL change top tabs, and visible region-navigation
controls SHALL reach equipment, pack, storage, details actions and pinned
Settings actions. World input meanings outside menus SHALL remain unchanged.

#### Scenario: Grid has a partial final row
- **WHEN** focus moves down into the partial row or reaches a grid edge
- **THEN** it selects the nearest existing cell in that row or remains at the boundary, never wraps into another tab or invisible cell, and scrolls the selected cell fully into view

#### Scenario: Region is empty or controls are disabled
- **WHEN** the player navigates between regions
- **THEN** an empty region has focusable explanatory guidance, disabled items have readable reasons, and there is a reliable path back to tabs and exit

### Requirement: Actions have explicit confirmation and cancellation
Move, split, transfer, equip and unequip SHALL expose explicit controller and
keyboard buttons and mouse-click equivalents. Quantity choice SHALL show source,
destination and amount, support one/all and bounded adjustment, and commit only
on confirmation. Drag-and-drop is not required for this release.

#### Scenario: Amount selection is canceled or invalidated
- **WHEN** the player cancels, loads another save, or the selected source/destination ceases to be valid before confirmation
- **THEN** no quantity or owner changes, the draft is discarded, and focus returns to the initiating cell or nearest remaining cell with an explanation when invalidated

### Requirement: Stable input intent remains authoritative
All hints and focus treatment SHALL use the existing accepted-input intent
policy. Mouse noise, repeated/released buttons and held-stick flip-back MUST NOT
override deliberate device choice; switching device MUST NOT activate controls
or change world movement/deadzone behavior.

#### Scenario: Controller selection with idle mouse noise
- **WHEN** controller navigation is followed by subthreshold mouse samples
- **THEN** controller hints and focus remain stable while deliberate subsequent mouse or keyboard use can still switch hints

### Requirement: Menu lifecycle preserves pause and action safety
Opening menus SHALL stop movement and cancel transient action presentation
without repeating its reward. Menu, quantity and exit dialogs SHALL keep
simulation paused until their owning menu closes. Opening a menu from placement
SHALL cancel the placement preview without charging resources; closing it SHALL
not accidentally place a building or resume a canceled action.
Pause SHALL stop ambient ticking without removing the existing explicit
time/resource/need costs of a successfully confirmed gameplay operation.

#### Scenario: Cancel a nested operation
- **WHEN** Back closes a quantity dialog inside Inventory
- **THEN** only that dialog closes, focus returns to its origin, and time, needs and crop growth remain paused

#### Scenario: Leaving preview or loading
- **WHEN** the player leaves character preview, closes the menu, loads a save or enters recovery
- **THEN** preview capture/input is released, appropriate gameplay camera state is restored, stale focus and transaction drafts are cleared, and no click falls through into the world

### Requirement: Readability survives supported resolutions and feedback
The overlay SHALL remain fully operable at 1280x720 and native 3840x2160,
including complete labels, counts, details, focus indicators and footer hints.
Critical text SHALL have backed contrast independent of scenery; selection and
errors SHALL not rely on color alone. Toasts MUST NOT obscure tabs, content,
details, pinned exit controls or active dialogs.

#### Scenario: Long error over a bright world
- **WHEN** a real save/craft error is shown at either supported resolution
- **THEN** the complete message is readable in a reserved area without covering controls, clipping essential information or advancing paused simulation
