# Proposal

## Why

Gathering, harvesting, crafting and buying change her stacks without a clear, glanceable note of what she got. A
generic toast ("Harvested 3 berries. More will ripen...") repeats what the pack already shows and shifts attention
to the notice area. Playtest feedback asked for a short `+3 Berries` beside her instead.

## What Changes

- Ported from staged branch `origin/jennifergalley-menu-pickups-accept` (commits `ae584b5a`, `79789c9a`,
  `acd012f4`) onto the Coral actual-stack hotbar baseline. The controller split on `main` moved the hooks into
  `HomesteadControllerHotbar.cpp`, `HomesteadControllerInteraction.cpp`, `HomesteadControllerMenuBridge.cpp` and
  `HomesteadControllerBook.cpp`.
- `Simulation/HomesteadHoldings.h` (portable): `CountHoldings`/`PickupGain`. A gain raises both her pack and
  everything she owns (pack, chests, set-down drops). Chest moves, picking her own drop back up, spent ingredients,
  losses and pail water never count.
- `HomesteadControllerPickups.cpp`: on each Simulation revision, one line per gained item. Repeat gains of the same
  item add to its line instead of making a second one. At most 4 lines, each lasting 2.6 s of visible time. Loads,
  new games, the first frame and any revision that goes backwards (a rollback) resync without showing anything.
- `UI/SHomesteadPickups`: a hit-test-invisible leaf painter over the viewport. It shows brass `+N` and a cream item
  name with a dark outline and no panel, right of her projected chest. Lines stay clear of the calendar, vitals
  and hotbar and rise as they fade. Nothing is laid out, so no HUD element moves and no click is blocked.
- Lines wait while the field book, the shop or setup is open, so a crafted or bought item shows once she's back in
  the world.
- Successful harvest and craft no longer toast, and neither do pack-to-chest transfers in the menu or cross-container
  drags. Refusals, watering and garment crafting still notify.

Gains are read from simulation state, not from input, so keyboard/mouse and controller behave the same way.

## Deferred

- A `+1` line for crafted garments (they are wearable instances, not item stacks). Garment crafting keeps its toast.
- Icons in the line and per-item colour.

## Capabilities

### New Capabilities

- `pickup-gain-lines`: glanceable `+N Item` feedback beside the heroine for real gains only.
