# Design

## Context

See `proposal.md` and both capability specs. Inventory currently uses 112-wide, 144-high cells with a 48-pixel icon followed by item name and `Carried/Chest` quantity text. A three-button mode strip switches among Carried, Nearby chest, and Wearing. Controller builds one row model from either pack, nearest chest, or equipped wearables and enables transfer actions whenever any chest is in range.

Interacting with a chest currently forces the global Inventory page into Nearby chest mode. Existing Simulation already provides exact container layouts, stable group IDs, chest IDs, atomic transfer/split/merge/reorder operations, capacity checks, expected revisions, and saves. The paper-doll plan makes the Wearing mode redundant. The gap is presentation and access topology, not storage authority.

## Goals / Non-Goals

**Goals:**

- Make pack scanning denser and quieter through icon+count cells.
- Remove proximity-dependent chest state from the general Inventory page.
- Bind one storage session to one deliberately interacted chest.
- Reuse current stack/group and transactional authority without inventing drag-and-drop.

**Non-Goals:**

- Changing capacity/stack rules, crafting from storage, linked chests, sort/filter/search, quick-transfer gestures, chest naming, or a generic container framework.

## Decisions

### 1. Inventory becomes one carried-pack snapshot

Remove `MenuInventoryViewIndex`, the Carried/Nearby/Wearing strip, and nearest-chest branching from ordinary Inventory. Page 0 rows come only from container 0 (pack), while equipped wearables are reached through the paper-doll slots planned by `refine-equipment-preview-and-idle`. Carried wearables remain pack tiles.

Remove pack-row `CanStore/CanTake` solely based on proximity. Inventory details/actions stay focused on the selected carried subject: food use, equip, dye, split, merge, reorder, and inspection as applicable.

**Alternative considered:** leave Nearby chest hidden until in range. Rejected because storage would still be conceptually inside Inventory and dependent on ambient proximity rather than intentional chest interaction.

### 2. Use smaller fixed icon/count tiles

Target approximately 68-76 virtual pixels square with a 44-52 pixel icon, small consistent padding, and a high-contrast quantity badge in the lower-right. Always render the number, including one. Remove label/location text and reduce minimum height. Responsive column count derives from available grid width rather than the current broad page constant; supported 720p still meets pointer target and count readability.

Selection uses the existing Gold outline/backing without covering icon/count. Hover/focus updates the existing details pane with name/location/description/actions. Preserve stable row/group key and desired-column logic through refresh.

**Alternative considered:** aggregate every item kind into one tile. Rejected because current split/reorder/group identity is authoritative and duplicate stacks must remain independently actionable.

### 3. Add a dedicated exact-chest storage mode

Controller stores `TOptional<int32> ActiveChestId` only while a storage overlay is open. A valid focused chest interaction resolves the exact ID once, verifies reach/playable state, and opens storage. E/A and mouse secondary interaction share this path; normal Inventory never sets it. The storage mode is not a field-book tab.

Build two independent snapshots from `GetLayout(ActiveChestId)` and `GetLayout(0)`, presented side-by-side at ordinary 16:9 sizes with a top/bottom responsive fallback if needed. Concise `Chest` and `Pack` labels identify state, not controls. Both grids reuse compact cells and one details/actions area.

**Alternative considered:** keep using whichever chest is nearest on each refresh. Rejected because movement/state changes could silently transfer against a different chest.

### 4. Preserve explicit transactional actions

Selection does not move an item. Transfer opens the existing amount confirmation/stepper and carries active chest ID, group ID, direction, amount, and expected revision. On confirm, revalidate the exact chest still exists/is the active storage owner and run the current transaction once. Both snapshots refresh atomically.

No drag/drop, shift-click, double-click, or implicit whole-stack behavior enters the first slice. These interactions require separate user decisions and tests.

### 5. Integrate input ownership and quiet menus

Mouse secondary interaction opens a focused chest only during gameplay; the menu input preprocessor owns it after storage opens. Hotbar left-click tool use remains separate. Directional navigation treats Chest grid, Pack grid, Details, Actions, and dialogs as explicit regions. Remove the inventory-mode region entirely.

Back closes storage directly to gameplay. Load/recovery/new world/session invalidation clears `ActiveChestId` before destroying UI. Full-screen menus pause under current policy.

### 6. Coordinate sibling UI changes

This change depends on shared cell/paper-doll geometry from `refine-equipment-preview-and-idle` and the quantity stepper from `improve-menu-directional-navigation`; it can define row/snapshot interfaces independently but final menu integration is serialized. Existing icons require no asset work.

## Risks / Trade-offs

- **[Smaller cells hurt pointer/count readability]** -> keep bounded minimum hit target, 720p/4K capture/geometry checks and high-contrast count backing.
- **[Duplicate stacks look identical]** -> stable focus/details identify stack quantity/group while tiles remain deliberately minimal.
- **[Storage opens wrong nearby chest]** -> bind exact focused ID at interaction and never recompute nearest owner during the session.
- **[Removing Wearing hides equipped-item actions]** -> paper-doll slots own equip inspection/unequip; block integration until that route passes.
- **[Mouse secondary conflicts with camera/tool input]** -> admit only a valid focused chest interaction before generic world handling and test press/release/click-through.
- **[Two grids become crowded at 720p]** -> responsive side-by-side/top-bottom layout with shared compact cells and scroll, not smaller unreadable icons.

## Migration Plan

1. Record current large cells, mode strip, nearest-chest behavior, stack/group and transfer evidence.
2. Deliver compact pack-only Inventory cells while preserving details/actions.
3. Remove Nearby/Wearing views after paper-doll inspection/unequip is integrated.
4. Add exact-chest interaction/storage snapshots and transactional transfer.
5. Run empty/full/duplicate-stack/multiple-chest/input/save/load/720p/4K/full-loop acceptance.
6. Build one immutable Shipping candidate and retain the current selected build as rollback until explicit promotion.

