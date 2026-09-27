# General store and town square playtesting

Round 1, lane `add-dollars-and-general-store`. Covers her purse, the general store in town, the
shopkeeper (a stand-in body for now) and the town-square blockout on the Estate map.

## What to try

1. Start a new estate game. The purse beside the meters reads **$10.00**.
2. Walk to the town square, about 1.7 km east of the house along the road. The general store is on
   the north side, with a green **GENERAL STORE** board over the door.
3. Climb the steps and go inside. At the counter the prompt reads **Talk to Mrs. Pascoe**. Press
   E, A or click to see her greeting, then **Continue** to open the shop. The game pauses while it's
   open.
4. **Sell** tab: pick an item, set the quantity (the Quantity dialog is the field book's), check
   the total and the purse-after figure, then confirm. The purse shows a green `+` delta.
5. **Buy** tab: what you sold is now listed under **From Trevennor** at the top. Buy a Cornish
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

- Hay and scrap are sold once the clearing lane lands those items. Until then, sell stone,
  firewood, meadow herbs or branches.
- Store and town surfaces are 2 m tiling textures baked in Blender (`store_surfaces.py`), so the
  repeat can show on broad walls.
