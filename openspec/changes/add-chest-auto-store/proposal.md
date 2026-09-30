# Proposal

## Why

Jenny (2026-09-29 playtest, docs/handoff/round-2.md "Auto-store matching stacks"): putting away the day's gathering one stack at a time is tedious; she wants the chest to take whatever matches what it already holds.

## What Changes

- `Simulation::StoreMatching(chestId, player, revision)`: one atomic, storage-only transfer. It moves only carried goods whose item already has a stack in the **current** chest (`Chests::AutoStores`: never tools, the lamp, the pail's water or garments; equipped clothes are never carried stock). Stacks below the hotbar row go first, in pack order, then row cells left to right, so the hotbar gives up its stacks last and a row cell only changes when its own stack matches (a used-up cell empties, like eating the last of a stack). It respects the chest's capacity: a partial transfer stores what fits and keeps the rest ("Stored 3 of 6 matching items; the chest is full."), and nothing is lost. No match, a full chest, a stale revision or being out of reach changes nothing.
- The chest view: a chest button beside the chest's name ("Store matching stacks (T)"), the keyboard shortcut T, and "Store matching stacks (T)" in the item menu (the gamepad's route; Y already opens that menu, so it isn't rebound).
## Impact

`Simulation/HomesteadSimulation.*`, new `Simulation/HomesteadChests.*`, `Tests/HomesteadChestTests.cpp` (new native suite), `HomesteadControllerChests.cpp`, `UI/SHomesteadMenuPages.cpp` (button), `UI/SHomesteadMenuInput.cpp` (T), `UI/SHomesteadMenuInventory.cpp` (item-menu option), `UI/HomesteadNativeMenuTest.cpp`. No save change: it only moves stock.

