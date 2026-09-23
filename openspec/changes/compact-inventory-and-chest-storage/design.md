# Design

## Context

See `proposal.md` and both capability specs. Inventory currently uses 112-wide, 144-high cells with a 48-pixel icon followed by item name and `Carried/Chest` quantity text. A three-button mode strip switches among Carried, Nearby chest, and Wearing. Controller builds one row model from either pack, nearest chest, or equipped wearables and enables transfer actions whenever any chest is in range.

Interacting with a chest currently forces the global Inventory page into Nearby chest mode. Existing Simulation already provides exact container layouts, stable group IDs, chest IDs, atomic transfer/split/merge/reorder operations, capacity checks, expected revisions, and saves. The paper-doll plan makes the Wearing mode redundant. The gap is presentation and access topology, not storage authority.

## Goals / Non-Goals

**Goals:**

- Make pack scanning denser and quieter through icon+count cells.
- Remove proximity-dependent chest state from the general Inventory page.
- Bind one storage session to one deliberately interacted chest.
- Replace stack-management action buttons with direct manipulation while preserving transactional authority.
- Stack new incoming quantities automatically and provide one deterministic pack Sort.

**Non-Goals:**

- Changing capacity/stack-size rules, crafting from storage, linked chests, custom sort criteria, filter/search, shift-click quick transfer, chest sorting/naming, or a generic container framework.

## Decisions

### 1. Inventory becomes one carried-pack snapshot

Remove `MenuInventoryViewIndex`, the Carried/Nearby/Wearing strip, and nearest-chest branching from ordinary Inventory. Page 0 rows come only from container 0 (pack), while equipped wearables are reached through the paper-doll slots planned by `refine-equipment-preview-and-idle`. Carried wearables remain pack tiles.

Remove pack-row `CanStore/CanTake` solely based on proximity. Inventory details/actions stay focused on true subject actions such as food use, equip, dye, Drop, and inspection. Split, merge, reorder, and chest transfer leave the action pane entirely.

**Alternative considered:** leave Nearby chest hidden until in range. Rejected because storage would still be conceptually inside Inventory and dependent on ambient proximity rather than intentional chest interaction.

### 2. Use smaller fixed icon/count tiles

Target approximately 68-76 virtual pixels square with a 44-52 pixel icon, small consistent padding, and a high-contrast quantity badge in the lower-right. Always render the number, including one. Remove label/location text and reduce minimum height. Responsive column count derives from available grid width rather than the current broad page constant; supported 720p still meets pointer target and count readability.

Selection uses the existing Gold outline/backing without covering icon/count. Hover/focus updates the existing details pane with name/location/description/actions. Preserve stable row/group key and desired-column logic through refresh.

**Alternative considered:** aggregate every item kind into one tile. Rejected because current split/reorder/group identity is authoritative and duplicate stacks must remain independently actionable.

### 3. Add a dedicated exact-chest storage mode

Controller stores `TOptional<int32> ActiveChestId` only while a storage overlay is open. A valid focused chest interaction resolves the exact ID once, verifies reach/playable state, and opens storage. E/A and mouse secondary interaction share this path; normal Inventory never sets it. The storage mode is not a field-book tab.

Build two independent snapshots from `GetLayout(ActiveChestId)` and `GetLayout(0)`, presented side-by-side at ordinary 16:9 sizes with a top/bottom responsive fallback if needed. Concise `Chest` and `Pack` labels identify state, not controls. Both grids reuse compact cells and one details/actions area.

**Alternative considered:** keep using whichever chest is nearest on each refresh. Rejected because movement/state changes could silently transfer against a different chest.

### 4. Add pointer drag and virtual drag over the same transaction intents

Slate owns transient drag state only: source container/group or wearable ID, expected revision, pointer/key/controller owner, and visual ghost. Authority still owns every final mutation.

- pointer movement beyond a small threshold starts drag; simple click only selects;
- dropping before/after a different-item tile reorders within one container;
- dropping an ordinary stack onto the same item merges;
- dropping into the other Pack/Chest grid transfers the whole stack and auto-merges in destination;
- invalid target, capture loss, Back, page/overlay change, revision change, or release outside cancels.

Keyboard/controller virtual drag starts with Enter/A on a focused tile, lets ordinary navigation choose a target, commits with Enter/A, and cancels with Esc/B. It uses identical target classification and APIs, not action buttons.

Cross-container partial transfer is two direct steps: Ctrl+click split, then drag the new stack. Unique wearables transfer whole and never merge.

**Alternative considered:** retain action buttons as an accessible fallback. Rejected because Jenny explicitly wants spatial item manipulation; keyboard/controller virtual drag provides parity without duplicating the old UI.

### 5. Split half directly and stack incoming quantities

Ctrl+pointer click calls one atomic split-half transaction for ordinary quantity >=2. `newQuantity = floor(total/2)` and the original retains the remainder, so x5 becomes x3+x2. Place the new group immediately after the original. Ctrl+Enter and controller X provide direct parity; quantity-one/wearables no-op with concise feedback.

Centralize ordinary item insertion so gather/craft/pickup/transfer first finds the earliest compatible destination group and increments it; allocate a group only when none exists. Do not collapse all deliberately split groups on every addition—the earliest receives new quantity and later splits remain.

### 6. Add one deterministic Pack Sort

Place a small icon-only Sort button at Pack upper-left (normal Inventory and Pack pane in storage), with accessible name/tooltip but no permanent label. Sorting is one candidate transaction that:

1. collapses all ordinary duplicate item groups;
2. keeps unique wearable records separate;
3. orders Tools; crafting/building materials; food/cooking ingredients; farming supplies; clothing; Other;
4. uses enum/definition, dye, and stable ID as deterministic tie-breaks.

The first fixed category mapping is:

- Tools: Knife, Hatchet, Digging Stick, Watering Can;
- Materials: Branch, Stone, Fiber, Timber, Firewood;
- Food/cooking: Berries, Roots, Flowers, Roasted Roots, Herbed Roots;
- Farming: Seeds, Water;
- Clothing: carried wearable instances.

Sort preserves totals, capacity, identities, dyes, ownership and equipped state. Chest sorting and configurable criteria are deferred.

### 7. Integrate input ownership and quiet menus

Mouse secondary interaction opens a focused chest only during gameplay; the menu input preprocessor owns it after storage opens. Hotbar left-click tool use remains separate. Directional navigation treats Chest grid, Pack grid, Sort, Details, remaining Actions, and dialogs as explicit regions. Remove the inventory-mode region entirely.

Back closes storage directly to gameplay. Load/recovery/new world/session invalidation clears `ActiveChestId` before destroying UI. Full-screen menus pause under current policy.

### 8. Coordinate sibling UI changes

This change depends on shared cell/paper-doll geometry from `refine-equipment-preview-and-idle`; `add-droppable-inventory-items` still uses the quantity stepper, but ordinary split/merge/reorder/transfer no longer do. It can define row/snapshot/drag interfaces independently, but final menu integration is serialized. The Sort icon is original vector UI work using the existing renderer; no external asset is required.

## Risks / Trade-offs

- **[Smaller cells hurt pointer/count readability]** -> keep bounded minimum hit target, 720p/4K capture/geometry checks and high-contrast count backing.
- **[Duplicate stacks look identical]** -> stable focus/details identify stack quantity/group while tiles remain deliberately minimal.
- **[Drag commits after stale refresh/capture loss]** -> retain expected revision, classify target at drop, and cancel on rebuild/focus/capture/page changes.
- **[Ctrl+click conflicts with selection]** -> modifier path performs exactly split-half; ordinary click remains selection and quantity-one is nonmutating.
- **[Automatic stacking defeats intentional splits]** -> new quantity joins only the earliest compatible group; existing split groups remain until explicit merge/Sort.
- **[Sort surprises the player]** -> one documented fixed category order, deterministic tie-breaks, no settings dialog, and exact before/after totals in tests.
- **[Storage opens wrong nearby chest]** -> bind exact focused ID at interaction and never recompute nearest owner during the session.
- **[Removing Wearing hides equipped-item actions]** -> paper-doll slots own equip inspection/unequip; block integration until that route passes.
- **[Mouse secondary conflicts with camera/tool input]** -> admit only a valid focused chest interaction before generic world handling and test press/release/click-through.
- **[Two grids become crowded at 720p]** -> responsive side-by-side/top-bottom layout with shared compact cells and scroll, not smaller unreadable icons.

## Migration Plan

1. Record current large cells, mode strip, nearest-chest behavior, stack/group and transfer evidence.
2. Deliver compact pack-only Inventory cells plus automatic incoming stacking while preserving true item actions.
3. Remove Nearby/Wearing views after paper-doll inspection/unequip is integrated.
4. Add pointer/virtual drag, split-half, merge/reorder/cross-grid transfer, and deterministic Pack Sort.
5. Add exact-chest interaction/storage snapshots and direct manipulation.
6. Run empty/full/duplicate-stack/multiple-chest/input/save/load/720p/4K/full-loop acceptance.
7. Build one immutable Shipping candidate and retain the current selected build as rollback until explicit promotion.
