# Proposal

## Why

Inventory currently uses large 112x144 cards containing icon, item name, location, and quantity, then adds `Carried / Nearby chest / Wearing` as global inventory modes. Jenny wants the denser icon-and-count language used by Coral Island and Minecraft, and storage should feel attached to a physical chest rather than appearing inside the general inventory whenever one happens to be nearby.

## What Changes

- Replace large inventory cards with compact square item tiles containing only the original item/wearable icon and a clear quantity number.
- Remove item names, `Carried`, chest location, and other prose from the tile itself. Focus/hover details remain the place for item name, description, location, requirements, and available actions.
- Keep one tile per existing authoritative stack/group so split, merge, reorder, capacity, ownership, and save behavior remain truthful. Always show the quantity, including `1`, so the tile contract is consistent.
- Remove the `Carried / Nearby chest / Wearing` mode strip from Inventory. Inventory becomes the carried pack grid plus the compact paper doll planned by `refine-equipment-preview-and-idle`; equipped wearables are inspected through their body/accessory slots.
- Remove all automatic nearby-chest counts, take/store actions, and chest view routing from ordinary Inventory.
- Open a dedicated storage surface only by interacting with a focused in-world chest. E/controller A and mouse secondary click on the valid chest provide equivalent access; merely standing near storage does not expose it.
- Show the opened chest and carried pack as two clearly separated compact icon/count grids in one storage surface, using the exact targeted chest ID and existing transactional transfer/split/merge authority.
- Close storage safely on Back, load/recovery, invalid chest state, or world replacement, with no mutation from opening/closing and no transfer to a different nearby chest.
- Coordinate with the tool hotbar and compact equipment/preview plans so shared icons, counts, pointer input, quiet-menu policy, focus, and 720p/4K density stay coherent.

Reference review:

- Coral Island uses compact icon cells with corner counts, item names in hover/selection details, and a chest-specific storage view reached through world interaction ([inventory reference](https://coralisland.fandom.com/wiki/Inventory), [Steam screenshots](https://store.steampowered.com/app/1158160/Coral_Island/)).
- Minecraft uses dense icon/count cells and opens one specific chest into a combined chest/player inventory surface rather than adding all nearby storage to the normal inventory ([inventory](https://minecraft.wiki/w/Inventory), [chest](https://minecraft.wiki/w/Chest)).

Homestead adapts information hierarchy and physical-storage semantics only. It retains original icons, Pine/cream/Gold styling, layouts, typography, sounds, interactions, and assets; no reference screenshot or proprietary UI is copied into the repo.

The smallest useful in-game result replaces the carried pack cards with smaller icon+count tiles and removes the three-mode strip. The first playable demonstration opens Inventory away from and beside a chest with identical pack-only contents, then right-clicks/E-interacts with one chest to open its dedicated two-grid storage view, transfers a stack, closes, and verifies another nearby chest was untouched. Full acceptance covers food, tools, materials, split groups, garments, empty/full/scrolling grids, multiple chests, pointer/keyboard/controller navigation, save/reload, 720p/4K composition, and immutable Shipping replay.

Deferred scope includes drag-and-drop, shift-click/quick transfer, sort/filter/search, chest naming/colors, linked storage, crafting from chests, stack-size rebalance, chest upgrades, arbitrary containers, and replacing transactional quantity confirmation beyond the separate stepper plan.

## Capabilities

### New Capabilities

- `compact-inventory-grid`: Small icon-and-count carried-item cells with details/actions outside the tile and no inventory mode strip.
- `chest-storage-interaction`: Dedicated exact-chest storage surface reachable only through in-world chest interaction.

### Modified Capabilities

None.

## Impact

- `SHomesteadMenu` and inventory/menu row models: compact cells, responsive columns, simplified pack page, details/action focus, and removal of global inventory views.
- Controller chest focus/input: exact `ActiveChestId`, E/A/mouse-secondary open path, dedicated storage overlay lifecycle, and no implicit proximity access.
- `HomesteadMenuInventory`, Simulation transfer/group APIs and save tests: separate pack/chest snapshots over existing transactional authority.
- Native menu/directional/full-loop/feedback tests and UI documentation: pack-only Inventory, two-grid storage, pointer/keyboard/controller parity, and 720p/4K evidence.
- Existing icons and Unreal Slate facilities are sufficient; no external asset, package, purchase, download, account, or license is required.

