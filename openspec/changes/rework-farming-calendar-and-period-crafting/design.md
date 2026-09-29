# Design

## Context

- **The day clock.** `State::hour` is one running game-hour count. `DayNumber()` is
  `hour / 24 + 1`, and `SeasonName()` uses 14-day seasons. `dayMinutes` defaults to 60, and
  Settings cycles 30, 60 and 120.
- **Hunger.** `Step()` drains hunger at 2.0 an hour awake and 1.3 asleep, and it sets
  `failed = true` at 0. Nearly every transaction then refuses with "You need to recover. Load your
  recent checkpoint".
- **Sleep.** `flexible-sleep` removed the clock bedtime: energy exhaustion only makes her doze off.
- **Crops.** `HomesteadCrops` holds the crop table (grow and regrow hours, visuals, harvest
  style) and the plot status helpers. Plots grow in `Step()` from moisture and weed factors.
- **Shops.** `HomesteadShops` seeds the general store, and its stock includes the six seeds. The
  sell-down runs at the 06:00 rollover.
- **Weather.** Rain is deterministic: `IsRainDay` is every third day, 09:00–15:00.
- **Crafting.** Recipes are hand recipes: the five hafts, RoastedRoots, HerbedRoots and
  SplitFirewood. The pieces are Foundation, Wall, Doorway, Roof, Fire, Bed, Chest and Hearth.

## Goals / Non-Goals

**Goals:**

- A readable agricultural year: each season has its own crops, forage and look.
- Bring the code back in line with the pivot's cozy rules: 28-day seasons, ~30-minute days, and
  hunger that never fails her.
- A second shop that spreads buying and selling across town, ready for round 3's prices.
- The first period crafting stations, planks, fences and furniture.

**Non-Goals:**

- Quality tiers, compost, supply-sensitive prices (round 3).
- Ploughing, seed drills, irrigation.
- Oats and flower crops.
- Snow and festivals.
- Freeform décor placement.
- Cornish stone hedges.
- Changes to the rain schedule, apart from its interaction with the new day length.

## Decisions

### 1. Calendar math in one place

- A new `Homestead::Calendar` (in `HomesteadSimulation.h`, or a small `HomesteadCalendar.*`)
  derives everything from `State::hour` and the 06:00 rollover:
  - `DayIndex`: 0-based whole days since Spring 1, 1851.
  - `DayOfSeason`: 1–28.
  - `Season`: Spring, Summer, Autumn, Winter.
  - `Year`: 1851 plus whole years.
  - `Weekday`: Monday for day 0.
  - `DaysLeftInSeason`.
- `SeasonName()` and `DayNumber()` delegate to it. The rain schedule keys off `DayIndex`.
- Nothing new is stored. The calendar is a pure function of `hour`, so saves only change for
  other reasons.
- The day starts at the existing 06:00 rollover. `DayNumber` becomes the day of the season in
  the HUD. The save label becomes "Spring 12, 1851".
- **Alternative considered:** store the season and day separately. Rejected, because two sources
  of truth drift.

### 2. Day length default 30 minutes

- `State::dayMinutes` defaults to 30 for new games.
- The Settings cycle stays at 30, 60 and 120, and saves keep their own value.
- Jenny confirms whether 30 feels right at the next playtest. The playtest build notes call this
  out.

### 3. Gentle hunger replaces hunger failure

- `Step()` no longer sets `failed` for hunger.
- A new helper, `HungerState` (Fed, Hungry, Famished), with thresholds at 25 and 0 in
  `Exertion`, scales:
  - sleeping and awake energy recovery (×1.0, ×0.75, ×0.5);
  - every `Exertion::*Energy` work cost, through one `WorkCost(base)` function (×1.0, ×1.25,
    ×1.5).
- **Sleeping on an empty stomach:** she still recovers, at half the rate. Hunger clamps at 0 and
  never drops below it.
- Controller toasts fire once per downward threshold crossing: "You're getting hungry" and
  "You're famished. Everything's slower until you eat."
- The `failed` flag and the checkpoint UI stay in the code for the legacy woodland path only.
  Removing them is logged for the Architecture agent's between-rounds cleanup, not done here.
- A native test drives hunger to 0 over several days and asserts no failure, slower recovery and
  higher costs.

### 4. Crop seasons and withering

- `CropInfo` gains `unsigned seasons`, a bitmask.
- `Plant` refuses a crop out of season: "Peas grow in Spring and Summer."
- Planting in season still warns when the crop can't ripen before its last in-season day. The
  check is `CropDays(kind) > days left in its contiguous season run`: "Won't ripen before Summer
  ends."
- The plot's focus line shows the same warning while it's true.
- **Rollover:** at the first `Step` that crosses into a new season, every planted plot whose crop
  doesn't include the new season becomes `withered`. That's a new `Plot` field, saved.
  - It shows a shared dead-plant mesh (`SM_CropWithered_*` per silhouette family: root, leafy,
    stalk, vine).
  - It yields nothing.
  - The hoe clears it back to tilled soil.
  - It's recorded once in a toast: "3 plots withered with the change of season."
- Legacy Roots and Berries are wild-sown, so they grow in Spring, Summer and Autumn.

**Crop table, all times from sowing when watered (the design's source of truth):**

| Crop | Seasons | Days | Regrow days | Harvest | Sold to | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Potatoes | Spring | 6 | none | Pull | General store | Cornish earlies |
| Carrots | Spring, Summer | 5 | none | Pull | General store | |
| Broad beans | Spring | 7 | 3 | Pick | General store | |
| Peas (new) | Spring, Summer | 6 | 3 | Pick | General store | vine visual |
| Strawberries | Spring, Summer | 8 | 3 | Pick | General store | |
| Wheat (new) | Summer, Autumn | 10 | none | Cut | Tregear's | stalk visual; flour in round 9 |
| Barley (new) | Summer | 8 | none | Cut | Tregear's | stalk visual |
| Turnips | Autumn, Winter | 4 | none | Pull | General store | |
| Cabbage | Autumn, Winter | 9 | none | Cut | General store | |
| Leeks (new) | Autumn, Winter | 8 | none | Pull | General store | |
| Winter broccoli (new) | Winter | 10 | none | Cut | General store | Cornish winter cauliflower |

Prices and yields are tuned in the catalogue.

### 5. Five new crop plant sets

- Blender recipes follow `improve-crops-and-harvest`: five stages (Sprout, Young, Growing, Mature,
  Ripe) with LODs, visible produce that swells and colours, and no glint.
  - **Peas:** a twiggy pea-stick support with pods.
  - **Wheat and barley:** a small plot-sized stand of stalks that goldens. Barley has awns.
  - **Leeks:** blue-green fans with white shanks showing at ripeness.
  - **Winter broccoli:** a large leafy plant with a white curd.
- A shared withered set comes in four silhouette families.
- Harvest reuses Pull, Cut and Pick. Grain uses Cut with a sickle-like hand motion, reusing the
  cabbage cut. No new tool is added.

### 6. Tregear's, the seedsman and corn merchant

- A second `Shop` record (`ShopKind::Seedsman`):
  - Hours 08:00–17:00, closed Sundays. The general store also closes Sundays.
  - Its own stock is the in-season seeds, filtered by the calendar each day, plus the tin
    watering can.
  - It buys wheat and barley. Everything else stays with Pascoe.
- The general store loses its seed rows.
- **The tin watering can** is a new tool item. It holds twice the pail's water, fills where the
  pail fills, and uses the pail's existing animations. The pail stays usable.
- **Building:**
  - A shopfront on the town square, assembled from the intact kit with a new sign board.
  - An interior with seed drawers, sacks of grain, a scale and a counter. These are Blender
    props.
  - The door anchor `SeedsmanDoor` is added to the estate layout.
- The stand-in shopkeeper actor is reused, named Mr. Jago Tregear, with greeting lines.
- **The closed door** reads "Closed on Sundays" or "Opens at 8 AM".
- `SHomesteadShop` already renders any `Shop`. The only UI change is the shop name in the title.

### 7. Seasonal forage

- A forage-season table keyed by `ResourceKind` holds a season mask and, for blackberries, a day
  window.
- A resource outside its window is present but has nothing ready: no produce and no gather
  prompt. Its renewal clock pauses.
- **Blackberries:**
  - The 42 `BerryBramble` placements fruit from Summer 15 to Autumn 28.
  - Picking is by hand, adds `Item::Blackberries` (new, edible, sellable), and renews every 3
    days in the window.
  - The fruiting material switch already exists.
- **Field mushrooms:**
  - A new `ResourceKind` with about 40 placements in the pasture and woodland-edge parcels. The
    world lane's placement bake adds them.
  - They're available in Autumn and renew every 2 days.
- Spring flowers (primrose, bluebell, wild daffodil, wild garlic) are Spring only. Marjoram and
  the meadow herb run from Spring to Autumn.

### 8. Seasonal look via `MPC_Season`

- `AHomesteadWorld` writes the parameter collection `MPC_Season` once per in-game hour, with:
  - `SeasonBlend`: a continuous 0–4, blending across the first three days of each season;
  - `WinterBare`: 0 or 1, blended;
  - `Frost`: winter, 05:00–10:00 easing out.
- **Readers:**
  - The landscape grass layer and the PCG grass blades: hue and value tint.
  - Deciduous leaf materials (oak, beech, sycamore, hawthorn, hazel): an autumn colour ramp, then
    a winter leaf opacity-mask cut with a small ground-litter tint.
  - Holly, fir and pine: unaffected.
  - Bracken: russet in autumn, flattened brown in winter.
- It's one global parameter write, with no mesh swaps and no per-actor work. The Performance
  agent profiles winter against summer.

### 9. Period crafting

- **Stations are pieces:** Workbench and Sawhorse join the `Piece` enum and use existing
  placement.
  - A workbench recipe needs a Workbench within reach, as `stationRequired` already does for the
    cookfire.
  - Recipes: Workbench (by hand: 6 Timber, 4 Twine). Sawhorse (by hand: 4 Timber, 2 Twine).
- **Planks:** use the Sawhorse ("Saw planks": 1 Timber → 4 Planks, 1.5 energy). It adapts the
  axe-chop anim into a two-handed saw stroke, or reuses the chop as a labelled stand-in until the
  saw animation lands.
- **Fences and gates:**
  - `FenceRail` and `FenceGate` are pieces with free-standing placement. They snap end to end at
    their posts, rotate in 15° steps, and need owned land.
  - Recipes at the workbench: FenceRail 2 Planks for one 2.4 m run; FenceGate 4 Planks and 1 Scrap
    iron (the hinges).
  - The gate toggles open and closed on E/A, and its state is saved per piece. Collision blocks
    the heroine now, and livestock in round 6.
- **Furniture:**
  - Stool, Table, Chair and Shelf are pieces placed with the existing furniture offset rules on
    foundations, including the standing room.
  - Recipes at the workbench use planks, plus scrap for the shelf brackets.
  - Freeform décor comes in round 5.
- **Hearth dishes** are hearth recipes that restore hunger and energy in tiers:

  | Dish | Ingredients | Restores |
  | --- | --- | --- |
  | Roast potatoes | 3 Potato | small |
  | Pease pudding | 4 Peas | small |
  | Leek & potato soup | 2 Leek, 2 Potato | medium |
  | Vegetable stew | Potato, Carrot, Turnip, Leek | large |

  RoastedRoots and HerbedRoots stay for legacy roots.
- **Craft categories:** Craft groups recipes into tabs: Tools (the hafts), Stations, Farm (fence,
  gate, planks), Furniture and Cooking. Tabs follow the existing icon-tab and directional-focus
  rules, and LB/RB still switch the book's main tabs.

### 10. Save version

One bump at integration covers:

- the `Plot::withered` field;
- the new `Item`, `Piece`, `Recipe`, `CropKind` and `ResourceKind` values;
- gate open state;
- the second shop's records;
- the default `dayMinutes`.

Test saves reset with the existing notice, and old Estate saves move to `Retired`.

## Lanes and ownership

| Lane | Owns | Reads/shares |
| --- | --- | --- |
| **A. Calendar & hunger** | `Homestead::Calendar`; `Step()` hunger/energy changes; `WorkCost`; season rollover hook; crop season masks, plant/focus warnings, withering in `HomesteadCrops`/`Step`; HUD calendar text; hunger and season toasts | exposes `Calendar` and the rollover hook to B–E first (first increment) |
| **B. New crops** | 5 `CropKind` rows and their plant sets + withered sets (Blender); plot visuals for withered state | catalogue rows (serialized); season masks from A |
| **C. Seedsman shop** | `ShopKind::Seedsman`, Sunday closing, seed stock move, tin watering can; Tregear's building/interior/props; `SeedsmanDoor` anchor | catalogue rows (serialized); `Calendar` from A; shop UI title |
| **D. Seasonal forage & look** | forage season table, blackberry item/picking, field mushroom kind + placements + prop; `MPC_Season` and material edits | `Calendar` from A; placement bake with the world owner |
| **E. Period crafting** | Workbench/Sawhorse/Fence/Gate/furniture pieces + meshes; recipes and dishes; Craft categories in `SHomesteadMenu` | catalogue rows (serialized); `Piece`/`Recipe` enums; parcel `CanBuildAt` |

- **Serialized:** the `HomesteadItems` catalogue file. Lanes append rows in a small commit
  rebased onto `main` first. Also serialized:
  - the `SimulationSaveVersion` bump, done once at final integration;
  - `SHomesteadMenu` (lane E only this round);
  - UAT and packaging (the Integration Agent only).
- **Order:** A's `Calendar` and rollover hook land first, within a day, because B, C and D build
  on them. E is independent.

## Risks / Trade-offs

- **30-minute days feel rushed with more to do.** → Settings keep 60 and 120, and Jenny decides
  at the playtest.
- **Withering feels punitive.** → Three-day warnings, a warning when planting too late, and ripe
  crops harvestable up to the last day. The rule matches the genre convention Jenny chose Coral
  Island for.
- **Winter canopy masks on Nanite foliage.** Masked Nanite has a cost. → The Performance agent
  measures winter. If needed, the fallback is a reduced-leaf LOD in winter instead of opacity
  masking.
- **Sunday closing annoys.** → It's a data flag per shop. Jenny can veto it at the playtest.
- **Many lanes append catalogue rows.** → Small, rebased, append-only commits. The static
  completeness check catches gaps.

## Open Questions

- Day length: is 30 minutes right, or does Jenny prefer 60? Confirm at the playtest.
- Tregear's name and keeper are placeholders.
- Is Sunday closing wanted?
