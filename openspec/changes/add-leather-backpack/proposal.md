# Proposal

## Why

Jenny (2026-09-29 playtest, docs/handoff/round-2.md "Leather backpack upgrade"): she runs out of pack space. A tentative one-time 1,500-coin purchase at the open General Store doubles her pack from 120 to 240; the worn rucksack shows on her back and can be hidden in Appearance without changing what she can carry.

## What Changes

- Simulation (`Simulation/HomesteadBackpack.*`): `State::leatherBackpack` and `State::backpackShown`; `PackCapacity(state)` is 120 or 240 (`MaxPackCapacity`). Every rule that meant "her pack's room" (gains, pickups, shop buys, overgrowth yields, take-down refunds, validation) reads it; load-time validation that doesn't know her state yet (world drops, saved stock before its sections are read) uses the 240 maximum, and the loaded game is then validated against its real capacity. Stacks set on the ground (drops, overgrowth leftovers, a taken-down chest's spill) never exceed her own capacity, since she picks a stack up whole.
- `Simulation::BuyBackpack(shop, player)`: at an open General Store counter, once, for `Backpack::Price` (1,500 coins, tentative). It isn't an item: it can't be sold, dropped or stored, so `ShopGoods` doesn't repeat it. `SetBackpackShown(bool)` is a look only.
- Save: optional tagged trailing section `backpack <owned> <shown>`, written only once she owns one (or has hidden it), so older games save byte-for-byte as before and older saves load with a plain pack. A 240-item pack in a save without the backpack is refused as corrupt rather than truncated. No `SimulationSaveVersion` bump.
- Shop: an "Upgrades" section heads Buy with the Leather backpack row (one only, "Carry 240", no quantity) until she owns it; too little money says so.
- Appearance: "Backpack: Shown / Hidden" appears once she owns it.
- Character: `AHomesteadCharacter::SetBackpackShown`, mirrored from the simulation each tick, (with `HasBackpackMesh()`) shows `SM_LeatherBackpack` on `spine_05` when the asset exists; Props (`jennifergalley-backpack-trunk`) adds the reference-pose pivot transform with the import, as CordBelt does. **Asset needed (Props):** an original, high-quality 1850s leather rucksack at `/Game/SurvivalGame/Environment/Props/LeatherBackpack/SM_LeatherBackpack`, authored about `spine_05` in the MetaHuman reference pose with straps over her shoulders and clear of the hair. Until it exists nothing shows and capacity is unaffected.

## Impact

`Simulation/HomesteadSimulation.*`, `HomesteadShops.cpp`, `HomesteadOvergrowth.cpp`, new `HomesteadBackpack.*`, `Tests/HomesteadEconomyTests.cpp` (new scenario), `UI/SHomesteadShop.*`, `HomesteadShopFlow.cpp`, `HomesteadControllerBook.cpp` + `UI/SHomesteadMenuPages.cpp` (Appearance row), `HomesteadCharacter.h` / `HomesteadCharacterAppearance.cpp`, `HomesteadController.cpp`, the pack capacity readouts (`HomesteadHUD.cpp`, `UI/HomesteadMenuInventory.cpp`, `HomesteadControllerLamp.cpp`, `UI/SHomesteadMenuActions.cpp`), `UI/HomesteadNativeMenuTest.cpp`. Props renames Cents to Coins separately.
