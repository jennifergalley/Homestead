# Design

## Crop table

`HomesteadCrops.cpp` holds one `CropInfo` row per `CropKind`, in enum order:

- display name and seed;
- produce, with a count, and an optional bonus;
- growing hours and regrow hours;
- harvest style;
- mesh stem.

Plant, HarvestCrop, the growth tick, the focus line and the plot visuals all read this table.
Nothing else switches on the crop kind.

Growing time is authored in game hours but always shown in days, rounded up
(`CropDays = ceil(growHours / 24)`). The new crops use whole days, so a crop sown and watered
at 9 AM on day 1 is ripe at 9 AM on day 1 + N. Regrowing crops (beans, strawberries, berries)
drop back to `growth = 1 - regrowHours / growHours` when picked and ripen again after
`regrowHours`.

| Crop | Days | Regrow | Seed (store price) | Harvest | Produce value | Style | Mesh |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Roots (legacy) | 2 (30 h) | - | Seeds (not sold) | 4 roots + 2 seeds | 4c each | Pull | CropCarrot |
| Berries (legacy) | 2 (42 h) | 1 (24 h) | a berry | 6 berries | 6c each | Pick | CropStrawberry |
| Turnips | 4 | - | Turnip seed, 20c | 2 turnips | 20c | Pull | CropTurnip |
| Carrots | 5 | - | Carrot seed, 25c | 3 carrots | 16c | Pull | CropCarrot |
| Potatoes | 6 | - | Seed potato, 30c | 4 potatoes | 14c | Pull (lift) | CropPotato |
| Broad beans | 7 | 3 | Broad bean seed, 45c | 5 bean pods | 8c | Pick | CropBroadBean |
| Strawberries | 8 | 3 | Strawberry runner, 60c | 4 strawberries | 12c | Pick | CropStrawberry |
| Cabbage | 9 | - | Cabbage seed, 40c | 1 cabbage | 90c | Cut | CropCabbage |

The store sells at base price × 125%, so the seed column is the shelf price. Every crop pays back
its seed on the first harvest. Short crops earn about 5c a plot per day and long ones slightly
more, so each crop is worth growing. The regrowing crops pay off only if she keeps picking.

Produce is `ItemCategory::Food`, bought by the store (`StoreBuys`), and edible raw:

- turnip, carrot and cabbage: a light snack;
- strawberries: a treat, with a small energy bump;
- broad beans and potatoes: edible raw in the game for now; cooking recipes come later.

The plural names read naturally in the store and inventory ("turnips", "bean pods").

Seasons don't gate crops yet: everything grows in any season. Season gating waits for the season
calendar to matter.

## Growth modifiers

`growth += hours × Moisture(m) × Weeds(w) / growHours`, where:

- `Moisture(m) = 1` when m ≥ 0.4. Below that it falls linearly to 0.2 in bone-dry soil. Soil
  loses 0.025 an hour in dry weather, so a full watering stays above 0.4 for 24 hours. Watering
  once a day, or rain, gives full speed. 0.4 is also where the bed turns to the wet material, so
  "looks wet" and "growing at full speed" agree.
- `Weeds(w) = 1` when w ≤ 0.5. Above that it falls linearly to 0.3 when fully weeded over. Weeds
  creep at 0.009 an hour, so a plot slows after about two and a half days unweeded. A 4-9 day
  crop needs one or two weedings.

Dry or weedy plots never die; they just ripen later. The focus line names the reason:

- "needs water, growing slowly"
- "weedy, growing slowly"
- both together.

## Stages and visuals

`StageOf(plot)` returns one of Bare, Sown, Sprout, Young, Growing, Mature or Ripe. The growth
thresholds are:

| Stage | Growth |
| --- | --- |
| Sown | below 0.06 |
| Sprout | 0.06 to 0.30 |
| Young | 0.30 to 0.55 |
| Growing | 0.55 to 0.80 |
| Mature | 0.80 to 1.0 |
| Ripe | 1.0 |

A picked regrowing plant shows Mature, not Sprout, while it ripens again.

Each crop has `SM_Crop<Name>_<Stage>` meshes for the five plant stages, made in Blender:

- one plant per plot, sized for the 1 m garden cell;
- three LODs;
- no collision.

The Sown stage keeps the existing soil mound. `AHomesteadWorld` swaps the plot's crop mesh when
the stage changes, so there is no per-frame cost. Legacy Roots use the carrot meshes and legacy
Berries use the strawberry meshes.

## Ripeness indicators

A ripe plot:

- shows its produce on the Ripe mesh (turnip and carrot shoulders, the potato haulm yellowing
  with tubers breaking the soil, the cabbage head, bean pods, red strawberries);
- adds a slow, soft glint above the plant: a small unlit additive sprite that pulses, seen from
  across the garden but not bright enough to look magical;
- when focused, reads "<Crop>: ready to harvest" with the action "Harvest".

A plot that needs water keeps the dry bed material. The focus line and the wet material share
the 0.4 threshold, so they always agree.

## Harvest animation

- **Roots and cabbage (Pull and Cut).** She kneels and reaches down with both hands. She grips,
  pulls up with a lean back, and holds the produce at chest height briefly. Then she drops it
  into her pack. This is the kneel-gather base with a pull-and-lift overlay, authored in
  `crop_harvest.py`.
- **Beans and strawberries (Pick).** She crouches at the plant, picks with one hand and drops the
  produce into her other hand.

The produce mesh shows in her hand during the lift, using the existing `HoldProduce`. It is hidden
when the montage ends or is interrupted, the same as the clearing lane's kneel props. A lab
command previews both motions.

## Save compatibility

- New CropKinds are appended after Berries. The plot's kind is saved as an int and range-checked
  on load, so older saves load unchanged.
- New Items are appended only after the v13 count-prefixed stock save is on main, which makes
  appended Items safe without a version bump.
- This change doesn't touch `SimulationSaveVersion`.
