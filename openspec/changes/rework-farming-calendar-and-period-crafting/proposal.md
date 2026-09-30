# Proposal

## Why

Round 1 made the estate walkable and profitable in the smallest way: clear it, carry hay and scrap to
town, sell to Mrs. Pascoe. The farming year is still missing. Several round-2 pieces have
already landed at Jenny's request, outside this change:

- `improve-crops-and-harvest`: six period crops, growth measured in days, visible ripening, and
  the kneel-harvest.
- `add-rain-weather`: rain that waters crops.
- `flexible-sleep`: no bedtime by the clock.
- The calendar HUD: a 12-hour clock, a sun/moon dial and a weather icon.

Round 2 turns those pieces into a real agricultural year: seasons, seasonal crops, a farm supply
shop and period crafting. It also fixes three places where the code still disagrees with the
pivot design (`pivot-to-cozy-estate-life-sim`):

- **Seasons are 14 days**, not the agreed 28 (`Simulation::SeasonName`), and crops ignore them.
- **Days default to 60 real minutes**, not the agreed ~30 (`State::dayMinutes`).
- **Empty hunger still fails the game** (`Step` sets `failed`) and sends her to a checkpoint. That
  breaks the `estate-life-sim-direction` rule that nothing fails her. Jenny's 2026-09-29
  decision goes further, removing hunger entirely in favour of Coral Island's one energy bar.

## What Changes

- **Calendar:**
  - Four 28-day seasons (Spring, Summer, Autumn, Winter) and a year counter starting in 1851.
  - Seven named weekdays. Spring 1, 1851 is a Monday.
  - The HUD calendar reads "Mon, Spring 12". In a season's last three days it adds "3 days left".
  - New Estate games default to 60 real minutes. The existing Settings choices of 30, 60 or 120
    minutes stay, and existing saves retain their stored day length.
  - Sleep stays exactly as `flexible-sleep` made it. This supersedes the design's "2 AM
    pass-out" rule.
- **Gentle hunger (superseded 2026-09-29):**
  - Lane A delivered Hungry and Famished penalties that replace hunger failure (task 1.3).
  - Jenny then chose the Coral Island one-bar model, so that work is interim.
  - See **One energy bar** below and design §3a.
- **One energy bar (Jenny, 2026-09-29):**
  - Hunger is removed from gameplay and from the HUD. There's no drain, penalty, toast or
    failure on the estate.
  - Food restores energy only.
  - **Snacks** restore a little: raw forage, bread, cheese, raw produce.
  - **Meals** restore more and grant **Well fed**. Meals are the pasty, hearth dishes and, later,
    fish dishes. While Well fed, every piece of work costs 15% less energy for a few game hours.
    Better dishes last longer, and eating again refreshes the timer rather than stacking.
  - The serialized `hunger` field stays in saves, read and ignored, until the next planned save
    bump.
- **A kinder start:** the standing-room chest also holds **3 Cornish pasties and 2 loaves of
  bread**.
- **Finding the hoe blade:**
  - Salvage yields the hoe blade **second**, right after the billhook.
  - Trying to till without a hoe says to search the old manor's salvage.
  - The arrival journal note mentions that the garden tools hung in the west rooms by the
    chimney.
- **Seasonal crops:**
  - Every crop lists the seasons it grows in. Planting is refused out of season, with the
    reason.
  - Planting a crop that can't ripen before its last season ends is allowed, with a warning on
    the planting toast and the plot's focus line.
  - At a season change, crops that don't grow in the new season wither. A withered plot shows a
    dead plant that the hoe clears. Ripe produce withers too.
  - Five new period crops: **peas**, **wheat**, **barley**, **leeks** and **winter broccoli**
    (Cornwall's famous winter cauliflower). Each of the four seasons now has at least three
    crops. See the design's table.
- **Seedsman and corn merchant**, a second town shop (placeholder "Tregear's, Seedsman & Corn
  Merchant", keeper Mr. Jago Tregear with a labelled stand-in body):
  - It sells in-season seed and a tin watering can that holds twice the pail.
  - It buys grain (wheat and barley).
  - Seeds move here from Pascoe's general store, which keeps food, twine and lamp oil.
  - Both shops close on Sundays, in period fashion.
- **Seasonal foraging:**
  - Each forage kind lists its seasons.
  - The berry brambles fruit from Summer 15 to Autumn 28, so blackberry picking arrives.
  - Field mushrooms come up in autumn pasture as a new forage.
  - Spring flowers are spring-only.
- **Seasonal look:**
  - One global season parameter tints grass and deciduous leaves: fresh spring green, deep summer
    green, amber and russet in autumn.
  - Winter bares the oak, beech, sycamore, hawthorn and hazel canopies. Holly and the conifers stay
    green.
  - Winter mornings frost the grass.
  - The look blends over the first three days of each season. Snow is deferred.
- **Period crafting:**
  - A **workbench** is the station for the new recipes. Hafting stays a hand recipe.
  - A **sawhorse** with a frame saw turns timber into planks.
  - **Post-and-rail fences and a gate**: fence runs snap end to end, and the gate opens and closes.
  - **First furniture:** stool, table, chair and shelf, placed with the existing piece placement.
  - **Period hearth dishes** from her own crops: roast potatoes, leek and potato soup,
    vegetable stew and pease pudding. Better dishes restore more.
  - Craft is sorted into Tools, Stations, Farm, Furniture and Cooking categories.
- **Save:** one `SimulationSaveVersion` bump at integration covers crop seasons, withered plots,
  weekdays and the year, new items and pieces, and the second shop. Test saves reset with the
  notice.

## Reuse research

- **In the project:**
  - The crop table and visuals pipeline from `improve-crops-and-harvest`: `CropInfo`,
    five-stage Blender plants with LODs, the produce ripening material and the kneel-harvest. New
    crops are new rows plus new Blender recipes in the same style.
  - Shop records, the `SHomesteadShop` Sell/Buy screens, the "From {Estate}" stock and the
    stand-in shopkeeper actor from `add-dollars-and-general-store`. A second shop is data plus a
    building.
  - Town massing from the ruin kit's intact variants.
  - The `flexible-sleep` options, the `add-rain-weather` schedule, and the calendar HUD.
  - The snapped building pieces, their furniture offsets and placement.
  - Hafting recipes, the Craft UI with icon ingredients and the craft pose.
  - The 42 berry brambles and the forage renewal system.
  - `homestead_foliage.py` and `homestead_shrub.py` for the mushroom and fence-rail props.
- **Unreal facilities:** a Material Parameter Collection for the season tint and winter leaf
  masks, shared by the landscape grass, PCG foliage and Nanite tree materials. There's no
  per-season mesh swap for grass. Bare winter canopies use the leaf-card opacity masks on the
  existing Nanite trees.
- **Comparative references (conventions only):**
  - Coral Island and Stardew Valley: 28-day seasons, seasonal seed stock, crops withering at a
    season change, a weekday calendar, and shops closed on some days.
  - Dreamlight Valley: clear seed-season labels.
- **Period verisimilitude:**
  - West Cornwall was known for early potatoes and for winter broccoli (winter cauliflower) sent
    up-country by rail from the mid-19th century.
  - Seedsmen and corn merchants were ordinary market-town trades.
  - Victorian shops closed on Sundays.
  - Post-and-rail fencing was common; Cornish stone hedges come with ranching in round 6.
- **Custom gaps:** the calendar and weekday model, the hunger penalty model, crop seasons and
  withering, the five crop plant sets, the second shop's interior and props, the season material
  parameter and winter masks, the sawhorse, fence and furniture meshes, and the new recipes.
- **No external assets or licences.**

## Smallest useful result and first playable demonstration

**Smallest result:** in a new game, the HUD reads "Mon, Spring 1". Tregear's sells spring seed
but no turnip seed, since turnips grow in autumn and winter. The HUD shows only an energy
meter, and a pasty from the standing-room chest restores energy and shows "Well fed".

**First playable demonstration:**

1. Buy potato and pea seed at Tregear's, plant them and water them.
2. Sleep through to the last days of spring and see the "3 days left" warning.
3. Harvest before Summer 1.
4. Watch an unharvested out-of-season plot wither at the rollover.
5. Build a workbench and sawhorse, saw planks, and put up a short post-and-rail fence with a
   gate around the plot.

**Full acceptance:**

- A crop from every season harvested, with `HomesteadGrowCrops` and sleep used to advance the
  year.
- Blackberries picked in late summer.
- Mushrooms found in autumn.
- The winter canopy and frost seen in ordinary play.
- Grain sold to Tregear's.
- Each period dish cooked and eaten.
- Both shops closed on a Sunday.
- The save reset notice.
- Mouse and controller parity.
- Packaged test suites updated.

**Deferred:**

- Quality tiers, compost and manure, and prices that respond to supply (round 3).
- The horse-drawn plough, seed drill, pump and irrigation channels.
- Oats and flower crops. Oats come with livestock feed in round 6; flowers as gifts and décor
  in round 12.
- Snow.
- Festivals.
- Cornish stone hedges (round 6).
- Sweeping furniture décor placement (round 5).

## Capabilities

### New Capabilities

- `estate-calendar-and-farming`: The calendar, weekdays and seasons; one energy meter with the
  Well fed meal benefit; seasonal crops and withering; the new crops; the seedsman shop;
  seasonal forage; the seasonal look; and the starter food and hoe signposting.
- `period-crafting`: The workbench, sawhorse and planks; fences and gates; first furniture;
  hearth dishes; and craft categories.

### Modified Capabilities

None. No main specs are archived yet.

## Impact

- **Simulation:**
  - `HomesteadSimulation.*`: day and season math, weekday and year, the hunger penalty model, the
    removal of hunger failure, and the season rollover.
  - `HomesteadCrops.*`: season masks, withering, and five new rows.
  - `HomesteadShops.*`: the seedsman, Sunday closing, and moving seed stock.
  - `HomesteadItems.*`: new rows.
  - The `Recipe` and `Piece` enums.
  - Forage season availability.
- **Presentation:**
  - `HomesteadHUD` calendar text.
  - `HomesteadController`: hunger and season toasts, plot focus text, fence gate interaction.
  - `SHomesteadMenu`: Craft categories.
  - `AHomesteadWorld`: the season parameter and withered-plot visuals.
- **Content:**
  - Blender plant sets for peas, wheat, barley, leeks and winter broccoli, plus a withered plant.
  - Field mushroom forage.
  - Workbench, sawhorse, fence, gate, stool, table, chair and shelf meshes.
  - Tregear's shopfront and interior.
  - `MPC_Season` and changes to the grass, foliage and tree materials.
- **Tests:** native calendar, hunger, season-gating, withering, shop and recipe tests. The
  packaged FullLoop and Smoke suites are updated. `SimulationSaveVersion` is bumped once.
