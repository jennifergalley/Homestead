# compact-inventory-grid Specification

## Purpose
Makes the carried inventory fast to scan through compact icon-and-count tiles while moving names, descriptions, locations, and actions out of the grid cells.

## Requirements

### Requirement: Carried items use compact icon-and-count tiles
Inventory SHALL show each authoritative carried stack/group as a compact square tile containing only its item/wearable icon and quantity number. The quantity SHALL remain visible for every stack, including quantity one.

#### Scenario: View a mixed pack
- **WHEN** the pack contains food, materials, tools, and clothing
- **THEN** every stack appears as a small icon tile with a readable corner count and no item-name or location text inside the tile

### Requirement: Item meaning remains available outside the tile
Selecting or hovering a tile SHALL show its item name, carried location, description, and valid actions in the contextual details/action surface. Selection/focus treatment SHALL remain visible without obscuring the icon or count.

#### Scenario: Inspect an unfamiliar icon
- **WHEN** the player focuses a carried item tile
- **THEN** the details identify the item and its current actions without enlarging or relabeling the grid cell

### Requirement: Inventory is pack-only
The normal Inventory page SHALL show the carried pack plus the equipment/character composition. It MUST NOT contain `Carried`, `Nearby chest`, or `Wearing` view controls, nearby chest counts, chest contents, or proximity-enabled take/store actions.

#### Scenario: Open Inventory beside a chest
- **WHEN** the player opens Inventory while standing within chest reach
- **THEN** the page shows the same pack/equipment structure as it does away from a chest and exposes no chest storage

### Requirement: Compact grids preserve authoritative stack behavior
Split groups, duplicate item stacks, reorder identity, food use, wearable identity, capacity, stale-revision protection, save/reload, and empty/full/scrolling behavior SHALL remain authoritative even though tiles are visually smaller.

#### Scenario: Display two stacks of one item
- **WHEN** authoritative layout contains two groups of the same item
- **THEN** two separate icon/count tiles remain selectable and preserve their stable group identities

### Requirement: Pointer direct manipulation replaces stack action buttons
Inventory MUST NOT expose Split, Merge, Move earlier, or Move later action buttons. Dragging a tile to a different-item position SHALL reorder it there. Dragging an ordinary stack onto a compatible same-item stack SHALL merge them atomically. Dropping outside a valid target SHALL cancel without mutation.

#### Scenario: Reorder by dragging
- **WHEN** the player drags a Stone tile to the insertion position before Fiber
- **THEN** Stone moves to that authoritative layout position and every quantity remains unchanged

#### Scenario: Merge by dragging
- **WHEN** the player drags `Branch x2` onto compatible `Branch x3`
- **THEN** one stable Branch stack contains exactly five and the source stack is removed

#### Scenario: Cancel a drag
- **WHEN** pointer capture is lost or the tile is released outside a valid insertion/merge target
- **THEN** the layout and all quantities remain unchanged

### Requirement: Ctrl+click splits ordinary stacks
Ctrl+clicking an ordinary stack of at least two SHALL create one new adjacent stack containing half rounded down while the original keeps the remainder. Quantity-one stacks and unique wearables SHALL not split. Keyboard/controller SHALL provide an equivalent direct tile command without an action-pane button.

#### Scenario: Split an odd stack
- **WHEN** the player Ctrl+clicks `Fiber x5`
- **THEN** the original stack contains three and a new adjacent stack contains two

### Requirement: Incoming items stack automatically
When ordinary item quantity enters a container through gathering, crafting, pickup, or transfer, it SHALL join the earliest compatible existing stack in that destination. A new stack SHALL be allocated only when no compatible stack exists. Existing intentionally split stacks MUST NOT all collapse merely because new quantity arrives.

#### Scenario: Gather into a split inventory
- **WHEN** the pack has `Fiber x3` followed by `Fiber x2` and gathering adds five Fiber
- **THEN** the earliest stack becomes `Fiber x8`, the second remains `Fiber x2`, and no third stack is created

### Requirement: Pack sorting is deterministic and direct
An icon-only Sort control SHALL appear at the upper-left of the Pack. Activating it SHALL atomically merge all compatible ordinary stacks, then order tools, crafting/building materials, food/cooking ingredients, farming supplies, clothing, and other items, with stable item/definition identity tie-breaks. Sort MUST preserve every quantity, wearable identity/dye, capacity, and equipped state. No criteria dialog is required.

#### Scenario: Sort a mixed pack
- **WHEN** the player activates Sort with duplicate materials and mixed categories
- **THEN** compatible duplicates collapse, tools precede materials, materials precede food, food precedes farming supplies, clothing follows, and totals/identities remain exact

### Requirement: Grid density remains usable
Tile size, count contrast, focus, drag ghost/insertion/merge targets, Sort control, hit targets, columns, spacing, and scrolling SHALL remain usable by pointer, keyboard, and controller at supported 720p and 4K layouts without persistent navigation instructions. Keyboard/controller SHALL support virtual drag by picking up a tile, moving focus, confirming a valid drop, and canceling without mutation.

#### Scenario: Fill the pack at 720p
- **WHEN** enough stacks require scrolling
- **THEN** tiles remain distinguishable and reachable, counts remain readable, and the selected tile scrolls into view

#### Scenario: Reorder with controller
- **WHEN** the player picks up a tile with A, moves to another position, and presses A again
- **THEN** the same reorder/merge authority applies as pointer drag while B cancels safely

### Requirement: Meal icons are ink glyphs
Every prepared meal SHALL draw as a flat ink glyph of the finished dish, in the same style as the tool, fish and crop glyphs, filling its cell wherever item icons appear (craft grid, hotbar, pack, chest and shop). Meals SHALL NOT show photographic tiles.

#### Scenario: Meals in the craft grid
- **WHEN** the player opens the craft grid with meal recipes listed
- **THEN** each meal shows its own dish glyph with no grey photo tile, distinguishable at hotbar size
