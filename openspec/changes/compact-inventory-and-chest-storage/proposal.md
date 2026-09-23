# Proposal

## Why

Inventory currently uses large 112x144 cards containing icon, item name, location, and quantity, then adds `Carried / Nearby chest / Wearing` as global inventory modes. Jenny wants the denser icon-and-count language used by Coral Island and Minecraft, and storage should feel attached to a physical chest rather than appearing inside the general inventory whenever one happens to be nearby.

## What Changes

- Replace large inventory cards with compact square item tiles containing only the original item/wearable icon and a clear quantity number.
- Remove item names, `Carried`, chest location, and other prose from the tile itself. Focus/hover details remain the place for item name, description, location, requirements, and available actions.
- Keep one tile per existing authoritative stack/group and always show quantity, including `1`.
- Remove Split, Merge, Move earlier/later, and chest Transfer buttons. Pointer drag directly reorders tiles; dropping onto a compatible same-item stack merges; dragging between Pack/Chest grids transfers the whole stack.
- Split ordinary stacks by Ctrl+clicking the tile: the new stack receives half rounded down and the original keeps the remainder. Keyboard/controller use a direct equivalent rather than an action-pane button.
- Automatically add incoming ordinary item quantities to the earliest compatible stack in the destination container instead of creating unnecessary duplicate stacks. Explicitly split stacks remain separate until merged, sorted, or new incoming quantity joins the earliest one.
- Add one compact icon-only Sort button at the upper-left of the Pack. Sorting atomically collapses compatible ordinary stacks and orders tiles by a fixed default: tools, crafting/building materials, food/cooking ingredients, farming supplies, clothing, then other; stable item/definition order breaks ties.
- Remove the `Carried / Nearby chest / Wearing` mode strip from Inventory. Inventory becomes the carried pack grid plus the compact paper doll planned by `refine-equipment-preview-and-idle`; equipped wearables are inspected through their body/accessory slots.
- Remove all automatic nearby-chest counts, take/store actions, and chest view routing from ordinary Inventory.
- Open a dedicated storage surface only by interacting with a focused in-world chest. E/controller A and mouse secondary click on the valid chest provide equivalent access; merely standing near storage does not expose it.
- Show the opened chest and carried pack as two clearly separated compact icon/count grids in one storage surface, using the exact targeted chest ID and direct drag transfer/reorder/merge authority.
- Close storage safely on Back, load/recovery, invalid chest state, or world replacement, with no mutation from opening/closing and no transfer to a different nearby chest.
- Coordinate with the tool hotbar and compact equipment/preview plans so shared icons, counts, pointer input, quiet-menu policy, focus, and 720p/4K density stay coherent.

Reference review:

- Coral Island uses compact icon cells with corner counts, item names in hover/selection details, and a chest-specific storage view reached through world interaction ([inventory reference](https://coralisland.fandom.com/wiki/Inventory), [Steam screenshots](https://store.steampowered.com/app/1158160/Coral_Island/)).
- Minecraft uses dense icon/count cells and opens one specific chest into a combined chest/player inventory surface rather than adding all nearby storage to the normal inventory ([inventory](https://minecraft.wiki/w/Inventory), [chest](https://minecraft.wiki/w/Chest)).

Homestead adapts information hierarchy and physical-storage semantics only. It retains original icons, Pine/cream/Gold styling, layouts, typography, sounds, interactions, and assets; no reference screenshot or proprietary UI is copied into the repo.

The smallest useful in-game result replaces the carried pack cards with smaller icon+count tiles, removes the three-mode strip, Ctrl+clicks one stack into two, then drags tiles to reorder and merge without action buttons. The first playable demonstration opens one chest, drags a whole stack between Chest and Pack, clicks Sort to collapse compatible stacks into the fixed default order, closes, and verifies another nearby chest was untouched. Full acceptance covers automatic stacking on acquisition, food, tools, materials, split groups, garments, drag thresholds/cancel, empty/full/scrolling grids, multiple chests, pointer/keyboard/controller direct manipulation, save/reload, 720p/4K composition, and immutable Shipping replay.

Deferred scope includes user-configurable sort criteria, filters/search, shift-click quick transfer, chest sorting/naming/colors, linked storage, crafting from chests, stack-size caps/rebalance, chest upgrades, arbitrary containers, and drag-outside-to-drop.

## Capabilities

### New Capabilities

- `compact-inventory-grid`: Small icon-and-count cells, direct drag/split/merge/reorder, automatic stacking, deterministic pack sorting, details outside the tile, and no inventory mode strip.
- `chest-storage-interaction`: Dedicated exact-chest storage with direct cross-grid drag transfer reachable only through in-world interaction.

### Modified Capabilities

None.

## Impact

- `SHomesteadMenu` and inventory/menu row models: compact cells, drag preview/drop targets, Ctrl+click split, virtual controller/keyboard drag, Sort control, simplified pack page, details focus, and removal of global inventory views/actions.
- Controller chest focus/input: exact `ActiveChestId`, E/A/mouse-secondary open path, dedicated storage overlay lifecycle, and no implicit proximity access.
- `HomesteadMenuInventory`, Simulation add/transfer/split/merge/reorder/sort APIs and save tests: automatic stacking, deterministic layout order and separate pack/chest snapshots over existing transactional authority.
- Native menu/directional/full-loop/feedback tests and UI documentation: pack-only Inventory, two-grid storage, pointer/keyboard/controller parity, and 720p/4K evidence.
- Existing icons and Unreal Slate facilities are sufficient; no external asset, package, purchase, download, account, or license is required.
