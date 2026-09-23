# Inventory and storage baseline

Selected build: `crafting-04-shipping / crafting-v17`

The current native Inventory uses 112 x 144 virtual-pixel cards with a 48-pixel
icon, item name, `Carried` or chest location, and quantity text. A three-button
`Carried / Nearby chest / Wearing` strip changes the backing container.
Proximity to whichever chest is nearest supplies chest counts and enables
Transfer actions. Split, Merge, Move earlier, Move later, and Transfer are
separate action-pane operations with amount/destination dialogs.

Authoritative layout behavior before this change:

- ordinary groups retain stable group IDs and deliberate duplicate stacks;
- new quantity reconciliation already increments the earliest compatible group;
- split inserts the new group beside the source;
- merge preserves exact quantity and removes the source group;
- reorder, transfer, split, and merge reject stale revisions atomically;
- chest access is recomputed from proximity instead of retaining one interacted
  chest ID;
- full packs support up to 120 layout entries and scroll through broad cards.

Evidence:

| Capture | SHA-256 | What it records |
|---|---|---|
| `Build\Releases\20260923-074750-crafting\crafting-04-shipping\Acceptance\native-menu\native-inventory.png` | `4079972F5E8097FD6318AAFC7600411FFE9DC0E053A3B75F7A1197DF9F52EB86` | Large carried cards, location/count prose, mode strip, focus, paper-doll bar |
| `Build\Releases\20260923-074750-crafting\crafting-04-shipping\Acceptance\native-menu\native-storage-transactions.png` | `4C1F0BFDBBB70B615195660B5CE13C893FB608AAC82EB596047BA6A193837CD8` | Nearby-chest mode and action/dialog transaction result |
| `Build\Releases\20260923-074750-crafting\crafting-04-shipping\Acceptance\directional\native-navigation-scrolled.png` | `41E2382CBA43043EC802D6840FFB5964A93016452066083127DF390E0918D178` | Full scrolling/focus behavior with the broad cells |

Comparative conventions, not copied expression:

- [Coral Island inventory](https://coralisland.fandom.com/wiki/Inventory)
  keeps compact icon/count cells and moves names into selection details.
- [Minecraft inventory](https://minecraft.wiki/w/Inventory) and
  [chests](https://minecraft.wiki/w/Chest) use dense cells and bind storage to
  one deliberately opened world chest.

Homestead retains its original Pine/cream/Gold styling, vector icons,
typography, details architecture, and Simulation authority.
