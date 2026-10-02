# Spec Delta

## Purpose

Ties storage access to one physical chest and presents that chest alongside the carried pack without exposing nearby storage through the global Inventory page.

## ADDED Requirements

### Requirement: Storage opens only from a focused chest
A dedicated storage surface SHALL open only after a valid in-world interaction with one focused chest. Merely standing near a chest or opening Inventory MUST NOT expose storage. Keyboard/mouse and controller SHALL have equivalent chest interaction paths.

#### Scenario: Interact with a chest
- **WHEN** the player focuses a reachable chest and uses E, controller A, or mouse secondary interaction
- **THEN** storage opens for that exact chest ID and pauses according to current menu policy

#### Scenario: Open Inventory near storage
- **WHEN** the player opens Inventory instead of interacting with the chest
- **THEN** no chest grid or transfer action appears

### Requirement: Storage separates chest and pack
The storage surface SHALL show the exact chest's contents and the carried pack as two clearly separated compact icon-and-count grids. Labels MAY identify the two containers as `Chest` and `Pack`, but cells SHALL not contain item names or instructional prose.

#### Scenario: Open one of two nearby chests
- **WHEN** two chests are in reach and the player interacts with the focused one
- **THEN** only that chest's authoritative contents appear beside the pack

### Requirement: Storage transactions remain atomic and exact
Whole-stack transfer, split, merge, reorder, automatic stacking, capacity rejection, wearable ownership, and expected revision SHALL continue using authoritative container transactions. Opening, closing, selecting, focusing, dragging without a valid drop, or canceling a virtual drag MUST NOT mutate either container. Storage MUST NOT expose Transfer, Split, Merge, Move earlier, or Move later action buttons.

#### Scenario: Transfer a stack by dragging
- **WHEN** the player drags one pack stack into the active Chest grid
- **THEN** that whole stack moves once, automatically joins the earliest compatible chest stack when present, and both grids refresh together

#### Scenario: Transfer part of a stack
- **WHEN** the player first Ctrl+clicks to split a pack stack and then drags the new stack into Chest
- **THEN** only that split quantity moves and no amount dialog/action button is required

#### Scenario: Capacity rejects transfer
- **WHEN** the destination cannot hold the requested quantity
- **THEN** neither container changes and the existing clear rejection is shown

### Requirement: Storage lifecycle is safe
Back SHALL close storage and return to gameplay without changing Inventory mode. Load/recovery, new world, invalid/removed chest state, or loss of valid session state SHALL close the surface safely and clear its active chest reference.

#### Scenario: Load while storage is open
- **WHEN** a load/recovery replaces world state
- **THEN** storage closes, no stale chest remains addressable, and no transaction is replayed

### Requirement: Storage is readable across inputs and resolutions
Pointer, keyboard, and controller focus/direct manipulation SHALL move predictably within and between chest grid, pack grid, details, remaining actions, Sort, and close behavior at 720p and 4K. The surface SHALL not restore generic controls legends.

#### Scenario: Cross between containers
- **WHEN** directional input leaves a grid toward the other visible container
- **THEN** focus reaches the nearest sensible tile/control without activating or transferring an item
