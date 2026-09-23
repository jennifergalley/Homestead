# Spec Delta

## Purpose

Makes the carried inventory fast to scan through compact icon-and-count tiles while moving names, descriptions, locations, and actions out of the grid cells.

## ADDED Requirements

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

### Requirement: Grid density remains usable
Tile size, count contrast, focus, hit targets, columns, spacing, and scrolling SHALL remain usable by pointer, keyboard, and controller at supported 720p and 4K layouts without persistent navigation instructions.

#### Scenario: Fill the pack at 720p
- **WHEN** enough stacks require scrolling
- **THEN** tiles remain distinguishable and reachable, counts remain readable, and the selected tile scrolls into view

