# Homestead balance sheet

A living sheet maintained by the Balance Agent. It records the economy's current numbers, read from
`Source/SurvivalGame/Simulation` on `main` at `b77c5c38` (2026-10-04). Fishing values come from the
Gameplay UI lane's 9 PM slice. Lanes implement numbers; this sheet records them and flags where
they break the pillars. When a value changes in code, update its row.

**Pillars, in priority order:**

1. Cozy, casual and fun: steady progress inside one play session, and no grinding.
2. Beauty.
3. Verisimilitude, which loses to 1 and 2.

**Flags:** 🔴 breaks a pillar, fix soon. 🟡 worth tuning. ✅ fine.

## 1. Time

| Value | Now | Source | Note |
| --- | --- | --- | --- |
| Real minutes per game day | 60 by default (Settings: 30, 60 or 120) | `State::dayMinutes` | One game hour is 2.5 real minutes. |
| Waking day | 06:00 to 02:00 | `HomesteadDaylight.h` | Twenty game hours, about 50 real minutes. |
| Season and year | 4 seasons of 28 days, starting 1851 | calendar | One season is about 28 real hours of play at 60 minutes a day. |
| Shop hours | 08:00–18:00, closed Sundays | `HomesteadShops` | |

🔴 **Crop timers are long in real time.** At 60 minutes a day, a 4-day turnip takes about 4 real
hours. A typical 1–2 hour session never harvests anything it planted, which breaks pillar 1. See §9.

## 2. Money and shop

| Value | Now | Note |
| --- | --- | --- |
| Starting coins | 1,000 | ✅ |
| Shop buy price | base × 1.25, rounded | ✅ A 25% spread is standard for the genre. |
| Sell price | base, at the General Store | ✅ |
| Townsfolk demand | 35% of her sold stock bought out each day at 06:00 | ✅ Flavor only; the sell price doesn't drop. |
| Leather backpack | 1,500 coins; doubles the pack from 120 to 240 units | ✅ About two days of mixed income (§9). |
| Storage chest | holds 1,200 units | ✅ |
| Fishing pole | 1,500 coins (`Fishing::PolePriceCoins`; the catalogue base of 1,200 is only the nominal value) | ✅ A first major purchase, alongside the backpack. |

### Store goods

| Item | Base → buy | Energy | Class | Coins per energy |
| --- | --- | --- | --- | --- |
| Cornish pasty | 80 → 100 | 40, plus Well fed for 3 h | Meal | 2.5 |
| Bread | 40 → 50 | 12 | Snack | 4.2 |
| Cheese | 60 → 75 | 15 | Snack | 5.0 |
| Twine | 20 → 25 | – | Material | |
| Lamp oil flask | 12 → 15 | – | 6 h of lamp | ✅ |

🟡 Bread and cheese cost 4–5 coins per energy, twice the pasty's rate. A snack should be the cheap
option. Suggest bread 40 → 50 for **20** energy and cheese 60 → 75 for **28** energy, which brings
both to about 2.5–2.7 coins per energy, in line with the pasty.

**Starter kit:** 3 pasties, 2 bread and 3 oil flasks. ✅ That's 144 energy of food on day 1:
generous and cozy.

## 3. Energy

| Value | Now | Note |
| --- | --- | --- |
| Maximum | 100 | |
| Awake drain | 0.6 an hour, about 12 a day | ✅ Barely noticeable. |
| Sleeping in bed | Always ends at 100 (Jenny, 2026-10-04; 9 PM build). The time it takes is still the deficit ÷ 10 an hour, from 0.25 h up to 10 h. | ✅ Supersedes the 15-an-hour proposal. |
| Dozing off (passing out at 02:00) | 6 h at 6 an hour, so 36 | ✅ A mild penalty, which is right. |
| Sprint floor / slow-walk floor | 25 / 10, where walking drops to 75% speed | ✅ She never faints on the estate. |
| Well fed | 3 h of work at × 0.85 cost, from every Meal (pasty, roots, crop and fish dishes); snacks give none | ✅ |

### Action costs

| Action | Energy | Actions per full bar |
| --- | --- | --- |
| Gather | 0.5 | 200 |
| Clear by hand | 1.0 | 100 |
| Soft underbrush | 0.8 | 125 |
| Woody underbrush or sapling | 1.5 | 66 |
| Fell a tree (axe, any unprotected estate tree; +6 timber, +4 branches) | 4.0 | 25 |
| Till | 2.0 | 50 |
| Plant, water (each) | 0.4 | 250 |
| Weed | 0.8 | 125 |
| Harvest | 0.6 | 166 |
| Fill the pail | 0.3 | |
| Craft, or make a garment | 0.8 | |
| Cook | 0.3 | |
| Split firewood, build | 1.5 | |
| Deconstruct | 1.0 | |
| Fuel the lamp | 0.2 | |
| Fishing cast | 1.5 | 66 |

**Store clothing (cosmetic, from day one; 2026-10-09):** linen shirt 120, long-sleeved linen shirt
180, homespun trousers 200, woven sandals 60, turnshoes 150, fur boots 300, fur coat 600 (1,610 for
the full set). Each garment is bought once; a garment she already owns shows "Owned" and can't be
bought again. Modern batch (Jenny waived the period for these): scoop tank top 100, cropped top 120,
matte leggings 160, low-rise skinny jeans 220, low-top sneakers 180, flat leather ankle boots 240,
cropped biker jacket 450 (1,470). Listed after the period garments.

✅ A full bar is a long, varied morning's work, and food tops it up. Energy is a gentle pacing cue,
not a wall.

✅ **Resolved:** a late night used to leave her short (bed at 02:00 restored only 40). From the
9 PM build, any bed sleep refills her to 100.

## 4. Crops

The store sells seeds at base × 1.25. Profit/plot/day is (yield × sell − seed buy price) ÷ days to
the first harvest.

| Crop | Seed base → buy | Days | Regrows | Yield × sell | First harvest | Profit/plot/day | Season |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Turnip | 16 → 20 | 4 | – | 2 × 20 | 40 | 5.0 | Autumn, winter |
| Carrot | 20 → 25 | 5 | – | 3 × 16 | 48 | 4.6 | Spring, summer |
| Potato | 24 → 30 | 6 | – | 4 × 14 | 56 | 4.3 | Spring |
| Cabbage | 32 → 40 | 9 | – | 1 × 90 | 90 | 5.6 | Autumn, winter |
| Broad bean | 36 → 45 | 7 | every 3 days | 6 × 8 | 48 | 0.4, then 16 | Spring |
| Strawberry | 48 → 60 | 8 | every 3 days | 5 × 13 | 65 | 0.6, then 21.7 | Spring, summer |
| Wild roots (found seed) | not sold | 30 h | – | 4 × 4, plus 1 seed back | 16 | free | Spring–autumn |
| Wild berry bush | – | 42 h | every 24 h | 6 × 6 | 36 | free | |

Care: the pail holds 15 portions, and planting and watering cost 0.4 energy each. Each plot has a
35% daily chance of gaining weeds (+0.2).

✅ The shape is good: fast wild roots, steady root crops and greens, and slow but lucrative
regrowers. Margins of about 100% (turnips) are healthy.

🔴 **Absolute pacing is too slow for one session**, because of the real-time timers in §1. Twelve
turnip plots earn about 240 coins over 4 real hours.

🟡 **Regrowers barely break even on their first harvest:** broad beans net 3 coins. That's fine, as
long as the detail pane says "Regrows every 3 days" so the player knows the payoff is coming.

## 5. Meals

A meal gives `round(12 + 0.6 × coins forgone)` energy, where coins forgone is what the ingredients
would have sold for. Cooking also costs one kindling (3 coins) and 0.3 energy.

| Meal | Ingredients | Energy | Coins forgone | Coins per energy |
| --- | --- | --- | --- | --- |
| Roasted turnips | 1 turnip | 26 | 23 | 0.9 |
| Stewed carrots | 2 carrots | 33 | 35 | 1.1 |
| Baked potatoes | 2 potatoes | 31 | 31 | 1.0 |
| Herbed broad beans | 3 pods, herb | 34 | 37 | 1.1 |
| Cabbage & potato stew | cabbage, potato, herb | 82 | 117 | 1.4 |
| Berry compote | 3 berries | 25 | 21 | 0.8 |
| Strawberry compote | 2 strawberries | 29 | 29 | 1.0 |
| Root vegetable hotpot | 2 roots, turnip, herb | 37 | 41 | 1.1 |
| Grilled trout (9 PM) | 1 trout | 38 | 43 | 1.1 |
| Grilled perch (9 PM) | 1 perch | 31 | 31 | 1.0 |
| Fresh mackerel slices (9 PM; no fire) | 1 mackerel | 30 | 30 | 1.0 |
| *Shop pasty, for comparison* | – | 40 | 100 to buy | 2.5 |

✅ Home cooking is about 2.5× better value than shop food, which is the right incentive.

✅ The biggest dish, the stew, gives the most energy per meal.

✅ Cooking never beats selling in coins; it turns coins into energy. Every Meal already grants Well
fed (3 h at × 0.85), which makes a cooked meal feel special. A native test covers it from the 9 PM
build.

## 6. Foraging and clearing

Expected sale value per clear, from `HomesteadOvergrowth.cpp` yields × sell prices. Items with no
buyer (timber, twine, seeds, bramble canes, weeds) count as 0. The swings are with the worn tool.

| Node | Tool, tier | Swings | Energy | Yield | ≈ coins |
| --- | --- | --- | --- | --- | --- |
| Tall grass | scythe, worn | 1 | 0.3 | 1–2 hay (6 each) | 9 |
| Sapling | billhook, worn | 1 | 1.5 | 3–4 branches (4 each), kindling | 17 |
| Fallen branch | axe, worn | 1 | 0.8 | 3 branches, kindling | 15 |
| Small stump | axe, worn | 3 | 3.0 | 2–3 firewood (15 each), kindling | 40 |
| Medium stump | axe, worn | 5 | 4.0 | 3–4 firewood, 1–2 kindling | 57 |
| Rubble | pickaxe, worn | 2 | 2.0 | 2–3 stone (5 each), scrap iron 50% (25), lead 20% (40) | 33 |
| Salvage pile | hand | 1 | 0.5 | 1 scrap iron, plus the next rusted tool head | 25 |
| Broken crate | hand | 1 | 0.8 | 2–3 kindling, scrap iron 40%, twine 15% | 18 |
| Broken barrel | hand | 1 | 1.0 | 1–2 scrap iron, 2–3 kindling, seeds 12% | 45 |
| Rubbish heap | hand | 1 | 1.2 | 1–2 scrap iron, 1–2 stone, lead 25% | 55 |
| Rotten planks | hand | 1 | 0.6 | 1–2 kindling, scrap iron 50% | 17 |
| Slate heap | hand | 1 | 0.8 | 1–2 stone, lead 30%, scrap iron 30% | 27 |
| Ruin timbers | axe, worn | 3 | 3.0 | 1–2 timber, 2–3 firewood, scrap iron 30% | 45 |

Iron and steel tools gate thickets, bramble banks, large and ancient stumps, logs and boulders. Iron
costs × 0.85 energy per swing and steel × 0.7, with fewer swings on the big nodes.
Spring flowers sell for 10–15: wild garlic 10, primroses 12, bluebells 15. Berries sell for 6 each
and roots for 4.

✅ Clearing the ruin pays well, 25–55 a node, so the first real hour funds the estate. That's the
right early loop: tidy up, earn, plant.

🟡 Scrap iron at 25 dominates early income. That's fine for days 1–2, but by the time the salvage
runs out, crops and fishing should already be paying.

## 7. Tools

Each tool is crafted free from a rusted head (from salvage) and 2 branches. The catalogue values
(axe 120, hoe 80, pail 90, scythe 250, billhook 180, pickaxe 220, lamp 150) are nominal, and no one
buys tools. ✅ There's no coin gate on the core toolset, which is very cozy.

**Iron upgrades (store, 2026-10-09):** 2,000 coins per tool (Jenny's number; 12,000 for all six).
That's about 2.5 days of income each, so it's a mid-game goal, not a day-1 buy.
- The upgrade only appears once she has crafted that tool.
- Iron already pays off at once: energy × 0.85 per swing and fewer swings on stumps, rocks and
  thickets (steel × 0.7). Keep that ladder.
- The row disappears once the tool is iron.

## 8. Fishing (9 PM slice)

| Value | Now | Verdict |
| --- | --- | --- |
| Cast | 1.5 energy | ✅ |
| Bite wait | 2 s + 0–2 s | ✅ Snappy. |
| Hook window | 0.9 s | ✅ |
| Landing | 2 strikes, each 0.65–1.25 s after the last, with a **0.7 s** window (widened from 0.5 s) | ✅ |
| Contact fail-safe | 2.0 s (was 1.2 s) | ✅ Prevents false losses from hitches. |
| Species | 50/50 per water | ✅ |
| River | trout 40, salmon 54 | |
| Lake | perch 28, carp 22 | |
| Sea | mackerel 30, bass 52 | |
| Meals now | fresh mackerel slices 30, grilled trout 38, grilled perch 31 | ✅ |

🔴 **Fishing outearns everything.** An attempt takes about 8–10 s and averages about 38 coins:
roughly 200 coins a real minute, or 2,000+ in ten minutes. Compare twelve turnip plots at 240 coins
over four real hours. Once she owns the pole, fishing trivialises crops, clearing and the backpack.
This doesn't block the 9 PM playtest (Jenny chose these numbers), but it's the top tuning
follow-up. The cozy fix keeps fishing short and fun rather than slow or stingy:

- **A daily catch per water:** 5 fish per water type per day, resetting at 06:00. After the fifth,
  show "The fish have stopped biting here today." That caps fishing at about 570 coins a day.
- Optionally lengthen the bite to 3 s + 0–4 s for a calmer rhythm. Never make it longer than 8 s.
- Keep the prices: they feel rewarding, and the cap does the balancing.

### Hard-fishing card (Jenny, 2026-10-09, Next build): approved values

Jenny asked for fishing to fail about 60% of the time and to be unpredictable. The 60% is read as
the total failure rate an attentive player sees, so the random escape is 50% and normal mistiming
makes up the rest.

| Value | New |
| --- | --- |
| Bite wait | 4–12 s uniform, with 0–3 nibbles (a shiver of 0.3 s or less, no rings) |
| Hook window | 0.8 s after the float sinks under collapsing rings |
| Strikes | 2–4 random, 1.0–3.5 s apart, 0–1 fake tug per wait, **0.7 s** window (kept) |
| Escape | 50% rolled at the hook, thrown at the first or second strike, never on the last |
| Clicking on a nibble | the fish spooks: "Too soon – it shied away." |

That lands about 40% of attempts, roughly one fish a minute (about 45 coins a minute). That is still
above crops, so the daily-cap idea above is parked until a playtest.

## 9. Target pacing

These are targets for lanes to tune toward; there's no telemetry yet. They assume a 60-minute day.

| Moment | The player should… |
| --- | --- |
| First 15 real minutes | Have found salvage, crafted an axe or hoe, cleared a patch and sown the first roots, and seen the shop. |
| End of day 1 (the first real hour) | Have about 1,200–1,500 coins from scrap and firewood, a dozen plots sown, and every starter tool crafted or in sight. |
| Day 2 (the second real hour) | Have harvested the first wild roots, cooked a first meal, and bought the pole **or** the backpack. |
| Day 3 | Have harvested the first bought-seed crop, bought the other big purchase, and started a second crop. |
| Days 4–7 | Have regrowers paying daily, a meal routine, and coins climbing steadily past 2,000 toward the next upgrade. |

Rules of thumb:

- After day 3, **no single purchase costs more than about 2 days of typical income.**
- **Something matures every session:** at least one harvest per real hour of play.
- **Energy never ends a play session.** Food in the pack should always cover another hour's work.

### Travel times (map resize, card `jenny-muucy9hz-jv5gx7`)

Times are one-way at a **sprint** (Jenny, 2026-10-04), measured along the walkable route in real
seconds. The MetaHuman sprints at 4.8 m/s. Sprinting costs no energy of its own; below 25 energy
she walks at 2.1 m/s, about 2.3× slower, so walking is secondary context only. There's no
movement-speed buff, and the map is scaled to fit these times.

Stage 2 targets (Jenny's revised scope, 2026-10-10). The manor core, pond and village walk stay as
they are; the cove and mine come much closer.

| Route | Comfortable | Upper bound (max route length) | Today (sprint) |
| --- | --- | --- | --- |
| Farm → manor | 5–10 s | 15 s (72 m) | 8–12 s ✅ keep |
| Manor → pond | 20–35 s | 45 s (216 m) | 44 s ✅ keep (Jenny: fine) |
| Manor → village store | 40–60 s | 75 s (360 m) | about 60 s (290 m walked, stage 1) ✅ keep |
| Manor → cove (first dry sand) | 35–50 s | 60 s (288 m) | 109 s (525 m); 129 s to the CoveBeach marker 🔴 |
| Manor → mine entrance | 35–50 s | 60 s (288 m), on a woodland path like the pond's | about 91 s (estimate, no authored path) 🔴 |

- **Daily loop:** farm, then store, then one fishing spot, then home. That's at most about 3 minutes
  of sprinting, or 6% of the ~50-minute waking day; 60 s real is about 24 game minutes. On a
  low-energy walk the same loop is about 7 minutes, which is still acceptable.
- **Western loop (optional):** a mine–cove link path so manor → mine → cove → manor fits in about
  3 minutes.
- **Distance order:** farm and manor are nearest, then the pond, then the village, cove and mine at
  about the same distance. The mine becomes a frequent hauling trip once mining ships, so nearer the
  low end is better.
- **Beauty:** no stretch longer than about 10 s of sprinting (about 50 m) without something to see
  or gather, such as flowers, a stile, a view or a forageable.

### Estate, neighbours and communal land (stage 2)

- **Her estate:** about 35–50 ha, down from about 90 ha. It keeps the manor core, farm, opening
  clearing, pond and its woods, her private cove, the mine, and enough woodland more than 60 m from
  home for felled trees to regrow.
- **Neighbour estates:** two or three, smaller than hers (about 15–40 ha each), their houses
  300–600 m from her manor and separated by hedgerows or woods. None sits across her routes to the
  cove, mine, pond or village.
- **Communal land clusters on the village:** a village green and allotments (the 1851 word; not
  "community garden") within about 15–30 s of the square, a communal beach on the estuary within
  about 30 s of the village, and a 60–100 m growth ring around the village pad.
- **Reserved land looks finished today:** fields, hedges, woods and flowered verges, never bare
  ground. World edges are natural (sea, moor, dense wood, hedged land) with no visible walls. Map
  labels use period names and Cornish surnames; player copy never says "reserved" or "future".

### Proposed fixes to hit the targets

These are proposals for the Orchestrator to offer Jenny. None is scheduled.

1. 🔴 **Halve crop growth.** Turnip 2 days, carrot 3, potato 3, cabbage 5, broad bean 4 (regrows
   every 2), strawberry 4 (regrows every 2). Keep yields and prices. The first harvest then lands in
   the 3rd real hour, and profit per real hour doubles. The alternative, a 30-minute default day,
   halves everything else too: shop hours, lamp oil and sleep.
2. 🔴 **A daily fishing catch:** 5 per water type per day (§8).
3. ✅ **Sleep:** superseded by Jenny's rule that any bed sleep refills to 100 (9 PM build).
4. 🟡 **Better-value snacks:** bread 20 energy, cheese 28 (§2).
5. ✅ **Cooked meals grant Well fed:** already true for every Meal (§5).

## Changelog

- 2026-10-04: first sheet (Balance Agent). Fishing strike window 0.5 → 0.7 s adopted by the Gameplay
  UI lane.
- 2026-10-04: travel-time targets for the map resize (§9), sent to the Map planning agent. Restated
  in sprint seconds the same day at Jenny's direction.
- 2026-10-04 (9 PM planning, Gameplay agent): bed sleep always refills to 100; Well fed confirmed on
  every Meal; tree felling on the map at 4 energy, with forest trees regrowing after 3 days beyond
  60 m of the manor and farm (a stump, then a sapling at day 1), and trees lining the roads and the
  town square left unchoppable. The shopkeeper is renamed Mr. Josiah Trethewey.
- 2026-10-04 (batch2, Gameplay agent): felling yields +6 timber, +4 branches (about 16 coins of
  branches per 4 energy, so it's no coin farm); stump 24 h → sapling → full tree at 72 h, only for
  trees 60 m or more from home; road-verge (9 m) and village-square (45 m) trees are protected;
  reach 4.5 m.
- 2026-10-10: stage 2 travel targets for Jenny's revised map-shrink scope (cove and mine 35–50 s,
  cap 60 s; manor core, pond and village unchanged) and estate/neighbour/communal sizing (§9), sent
  to the Map Agent.
