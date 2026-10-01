# Proposal

## Why

Jenny (2026-09-29 playtest, docs/handoff/round-2.md "Owned chest names"): with several chests she can't tell them apart; she wants to name them and see the name before opening one.

## What Changes

- `Structure::customName` (trimmed UTF-8, up to 24 characters), keyed by the chest''s stable structure id. `Simulation::RenameChest` sets it; an empty name puts back "Storage chest". `Chests::DisplayName` reads it.
- Saved in an optional tagged trailing section (`chestnames`: a count, then id and hex-encoded name), written only when a chest has a name, so unnamed games save byte-for-byte as before and older saves load unchanged. Unknown ids, non-chest ids, untrimmed, empty or overlong names are refused as corrupt. No `SimulationSaveVersion` bump.
- The world prompt names the chest before she opens it; the storage view, its summary and item locations use the name.
- "Name..." beside the chest''s name (and "Name this chest..." in the item menu) opens a dialog: type on the keyboard and press Enter, or pick Save, Clear, or one of four suggestions (Pantry, Tools, Seeds and garden, Timber and stone) so a controller player can name one without typing.
## Impact

`Simulation/HomesteadSimulation.*`, new `Simulation/HomesteadChests.*`, `Tests/HomesteadChestTests.cpp`, `HomesteadControllerChests.cpp`, `HomesteadControllerFocus.cpp` (prompt), `UI/SHomesteadMenu*` (dialog, heading), `UI/HomesteadMenuInventory.cpp` (locations, summary), `UI/HomesteadNativeMenuTest.cpp`. Props owns the original Victorian trunk asset.

