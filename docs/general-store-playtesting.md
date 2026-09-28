# General store and town square playtesting

Round 1, lane `add-dollars-and-general-store`. Covers her purse, the general store in town, the
shopkeeper (a stand-in body for now) and the town-square blockout on the Estate map.

## What to try

1. Start a new estate game. The purse, a coin icon in the vitals stack at the bottom left, reads **$10.00**.
2. Walk to the town square, about 1.7 km east of the house along the road. The general store is on
   the north side, with a green **GENERAL STORE** board over the door.
3. Climb the steps and go inside. At the counter the prompt reads **Talk to Mrs. Pascoe**. Press
   E, A or click to see her greeting, then **Continue** to open the shop. The game pauses while it's
   open.
4. **Sell** tab: pick an item, set the quantity (the Quantity dialog is the field book's), check
   the total and the purse-after figure, then confirm. The balance in the shop's header goes up at
   once. The coin row's green `+` delta lasts about 3 seconds after the trade; the HUD is hidden
   while the shop is open, so you see it only if you leave straight away.
5. **Buy** tab: what you sold is now listed under **From Trevennor**, below the shop's own goods
   (scroll down if it's cut off). Buy a Cornish
   pasty ($1.00), close the shop, and eat it from the hotbar or field book. Hunger and energy rise.
6. Sleep through the night. After 6 AM the store has sold part of the From Trevennor stock (about
   a third a day).
7. Visit after 6 PM. The door is shut, the closed sign hangs on it, and interacting shows
   "Closed - opens at 8 AM".

Controls in the shop: LB/RB or Tab switch tabs, the d-pad or arrows move, A or Enter selects,
B or Escape backs out. The mouse works throughout.

## Placeholders

- Mrs. Martha Pascoe, the $10.00 start and the estate name "Trevennor" are placeholders until
  Jenny renames them.
- The shopkeeper is a **stand-in body**, the heroine mesh with an apron, labelled over her head.
  Her MetaHuman (task 3.2) waits for Jenny's Epic sign-in.
- Prices are fixed. They become supply-sensitive in a later round.

## Town square blockout

The other twelve buildings around the square are **blockout massing**. They're
`AHomesteadTownBuilding` level actors in the Estate level's `Town` folder, placed by
`Content/Python/homestead_agent/town_massing.py`. They vary in walling (granite rubble, dressed
ashlar or lime-washed render with granite quoins), height (one to three storeys), roofline (ridge
along the street or gable to the street, at 32–50°), chimney stacks, door colour and position,
shopfronts and sign boards. Their doors are inert, and the sign boards are deliberately blank.
Treat their proportions and placement as a first pass for Jenny's feedback, not final art.

The southeast corner of the square is open for the road in from the estate.

## Known gaps

- Hay and scrap sell at 6 and 25 cents. The packaged Estate build (main 3b8172ba) passed steps 3-6 with
  items from `HomesteadGive`. She was teleported to town, so the road walk carrying them is still untested.
- Store and town surfaces are 2 m tiling textures baked in Blender (`store_surfaces.py`), so the
  repeat can show on broad walls.

## Shortcuts for testing

- Development builds take `EnableCheats`, then `BugItGo <x> <y> <z> <pitch> <yaw> <roll>`.
  For example, `BugItGo -54000 116500 9350 0 90 0` puts her in front of the store door.
- **`BugItGo` also turns on `Ghost`**, so she walks through walls, the counter and knee-deep into floors. Type `Walk` afterwards to get collision back before judging anything physical.
- `HomesteadGive Hay 20` and `HomesteadGive ScrapIron 6` stock her pack.
- The manor bedroll is next to (-25500, -63720). Sleeping through 6 AM runs the sell-down.
- `HomesteadOpenStore` **moves** the store to 2.5 m ahead of her. It's for maps without a town. On the Estate the store is gone from the square until the save reloads, and loading puts it back at its anchor.
