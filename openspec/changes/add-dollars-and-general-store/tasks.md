# Tasks

## 1. Money and catalogue

- [x] 1.1 Add the `HomesteadItems` catalogue table with completeness assertion. Migrate `ItemName` and the other metadata lookups to it, keeping behaviour unchanged under the existing tests
- [x] 1.2 Add money in cents, a formatting helper, and the shop records, including the `Sell`/`Buy` transactions, opening hours and the daily sell-down. Portable tests cover money, rejects, capacity, rollover and save/reload

## 2. First playable sale (first delivery)

- [x] 2.1 Add the HUD wallet readout with signed deltas at 720p and 4K. **Checked on main d11a3683** (standalone `-game` on the Estate, 1280×720 windowed and 3840×2160 fullscreen): the coin row in the `SHomesteadVitals` stack reads $10.00 at the start and shows the fading `-$1.00` delta beside $11.70 / $10.20 after buying a pasty, with no clipping and clear of the hotbar and minimap. The shop screen, greeting, Sell and Buy tabs (with From Trevennor) and the Quantity dialog also fit at both sizes. At 4K the HUD's scale caps at 1.5× (the shared `UiScale` clamp), so the coin row is smaller relative to the screen than at 720p but still legible
- [x] 2.2 Build the general-store interior and door at `GeneralStoreDoor`, with Blender counter, shelves, barrels, sacks and crates, and the closed sign
- [x] 2.3 Add `AHomesteadShopkeeper` with a labelled stand-in body, a greeting panel and an interaction that opens the shop
- [x] 2.4 Add `SHomesteadShop` Sell and Buy panes with the From {Estate} section, the Quantity dialog, total preview and pause. Verify mouse and controller parity. **Checked in PIE on the Estate (main 85c62f34):** with gamepad input (simulated key events through HomesteadPlayTools, not a physical pad) she talked with A (`[A] Talk to Mrs. Pascoe`), continued with A, moved with the D-pad, opened the Quantity dialog with A, set the max with Y, confirmed with A (sold 6 scrap iron), switched tabs with RB and LB, and left with B; the prompts switched to `[A]`/`[B]`/`[LB / RB]`. With real mouse clicks she chose Bread, pressed `+`, confirmed (bought 2 for $1.00), clicked the Sell tab and clicked Leave; the prompts switched back to keyboard text. The keyboard path is covered in 2.1 and 2.5
- [ ] 2.5 Package, carry hay and scrap from the estate to town, sell them, see them listed, buy and eat a pasty, then sleep and see the stock drain. Capture in-game views, commit and push. **Packaged Estate build (main 3b8172ba), keyboard:** she walked into the store, sold 20 hay ($1.20) and 6 scrap iron ($1.50), saw both under From Trevennor, bought a Cornish pasty ($1.00, purse $10.00 → $11.70) and ate it (Food +28, Energy +4). She slept at the manor bedroll across 6 AM, and the shelf dropped to 13 hay and 3 scrap (35%, rounded up). **Still open:** the hay and scrap came from `HomesteadGive`, and `BugItGo` teleported her to town, so the road walk carrying them is unverified

## 3. Town and shopkeeper

- [x] 3.1 Mass the town square from intact granite and slate kit variants, with varied rooflines and sign boards. Mark it as blockout in the playtest notes
- [ ] 3.2 Author Mrs. Pascoe in the in-editor MetaHuman Creator with Jenny's Epic sign-in. Assemble her with the retargeted relaxed idle and head look-at, and replace the stand-in. **Human step pending:** it needs Jenny's Epic sign-in, and the labelled stand-in keeps the store playable until then
- [ ] 3.3 Confirm the current MetaHuman and Unreal EULA terms for shipped NPC use and record them in `docs\asset-credits.md`. **Human step pending**

## 4. Acceptance

- [ ] 4.1 Run the full-loop route clear → haul by hand → sell → buy → sleep → sell-down with mouse and controller. Test closed hours and the menu and shop isolation
- [ ] 4.2 Jenny playtests the town trip. Tune the starting money, prices, sell-down rate and greeting lines from her feedback
