# Tasks

## 1. Money and catalogue

- [x] 1.1 Add the `HomesteadItems` catalogue table with completeness assertion. Migrate `ItemName` and the other metadata lookups to it, keeping behaviour unchanged under the existing tests
- [x] 1.2 Add money in cents, a formatting helper, and the shop records, including the `Sell`/`Buy` transactions, opening hours and the daily sell-down. Portable tests cover money, rejects, capacity, rollover and save/reload

## 2. First playable sale (first delivery)

- [ ] 2.1 Add the HUD wallet readout with signed deltas at 720p and 4K
- [x] 2.2 Build the general-store interior and door at `GeneralStoreDoor`, with Blender counter, shelves, barrels, sacks and crates, and the closed sign
- [x] 2.3 Add `AHomesteadShopkeeper` with a labelled stand-in body, a greeting panel and an interaction that opens the shop
- [ ] 2.4 Add `SHomesteadShop` Sell and Buy panes with the From {Estate} section, the Quantity dialog, total preview and pause. Verify mouse and controller parity
- [ ] 2.5 Package, carry hay and scrap from the estate to town, sell them, see them listed, buy and eat a pasty, then sleep and see the stock drain. Capture in-game views, commit and push

## 3. Town and shopkeeper

- [x] 3.1 Mass the town square from intact granite and slate kit variants, with varied rooflines and sign boards. Mark it as blockout in the playtest notes
- [ ] 3.2 Author Mrs. Pascoe in the in-editor MetaHuman Creator with Jenny's Epic sign-in. Assemble her with the retargeted relaxed idle and head look-at, and replace the stand-in. **Human step pending:** it needs Jenny's Epic sign-in, and the labelled stand-in keeps the store playable until then
- [ ] 3.3 Confirm the current MetaHuman and Unreal EULA terms for shipped NPC use and record them in `docs\asset-credits.md`. **Human step pending**

## 4. Acceptance

- [ ] 4.1 Run the full-loop route clear → haul by hand → sell → buy → sleep → sell-down with mouse and controller. Test closed hours and the menu and shop isolation
- [ ] 4.2 Jenny playtests the town trip. Tune the starting money, prices, sell-down rate and greeting lines from her feedback
