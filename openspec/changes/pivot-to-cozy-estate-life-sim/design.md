# Design

## Context

This document is the single authoritative record of the cozy-estate direction Jenny chose in
her 2026-09-27 planning interview. Round changes cite it rather than restating it. When a
round's own design refines a decision, that round's change updates this document too, so
there's always one current answer.

Primary comparative references are Coral Island (loop, economy, tool tiers, town and shops,
ranching, mining), Stardew Valley (mine levels, artisan goods, no-slaughter livestock) and
*Poldark* (the setting, the mine as the long-term fortune, restoring a family's name).
Minecraft and Disney Dreamlight Valley remain interaction references, as before. Homestead
adapts conventions only and copies no proprietary art, text or branding.

## Goals / Non-Goals

**Goals:**

- A cozy life sim whose goal is wealth and restoring the family name, with plenty of money
  sinks so the fortune is earned.
- One fixed, hand-authored world that Jenny and Cipher shape together.
- Physical, satisfying loops: clearing, farming, ranching, fishing, mining, hauling, selling
  and building.
- Deliver in playable rounds, each one an OpenSpec change.

**Non-Goals:**

- Survival death, cold, predators, hunting, carcass processing, spoilage, a family debt or
  rival, procedural worlds, multiplayer, online services.
- Strict historical costume. Jenny designs more modern custom clothing as we go.

## Decisions

### Setting

- An invented estate and town on a real-Cornwall-inspired coast, 1840s–1860s early Victorian.
  There's no real desert: coastal sand dunes ("towans") and open heather moorland with granite
  tors play the dry, open role.
- Money is shown in **US dollars and cents** (`$1.23`) for readability and stored as integer
  cents. This is a deliberate choice for accessibility over period accuracy.
- New game: a character creator with a player-chosen first name, family surname and estate
  name. It keeps the existing MetaHuman heroine and customization.

### World

- One fixed map about **4 × 4 km**. The estate is about **1 km** across.
- The terrain starts from real Cornish open elevation data (Open Government Licence; exact
  source and attribution are verified in `author-fixed-cornish-estate-map`). It's then
  reshaped together into the agreed layout.
- The map uses Unreal Landscape and World Partition, the Water plugin for ocean, river and
  lake, PCG/foliage for biomes, and a landscape spline for the dirt road.
- **Layout:**
  - The estate sits on a south-facing slope down to its own cove and cliffs.
  - A derelict mine and its engine-house ruin stand on the estate clifftop.
  - A small river runs through a wooded valley into the cove; the mill site is here.
  - A dirt road runs about 1.5 km to an inland town at the head of an estuary, which has a
    harbour.
  - Moorland and tors lie to the north, and dunes and beaches along the far coast.
- Biomes: woodland, pasture and plains, moorland, dunes, beach, cliff and coast, river, lake.
- The estate boundary appears on the world map and the minimap. There are **no pre-built
  fences**; fencing is player work.
- Outside the boundary the land is public and walkable. She can forage there but not build.
  Late-game, neighbouring parcels (woodland, a moor field, a second cove) are shown "for sale"
  and can be bought.

### Opening state

- The game starts on **Spring, day 1**, the cozy-sim default.
- Blackberries fruit on the bramble in late summer and early autumn, so the first slice's
  first trip to town sells other things: cut hay from the scythe, scrap metal salvaged from
  rubble and the ruin, and spring flowers and forage (primroses, bluebells, wild daffodils,
  wild garlic).
- The estate is massively overgrown with weeds, tall grass, dense bramble, saplings, stumps,
  fallen timber and rubble.
- The manor is a ruin in shambles on its original footprint. **One corner room still stands**,
  with a bed, a hearth and a chest. She sleeps and saves there until the rebuild.

### Meters, calendar, weather

- **Energy is the only personal meter**, as in Coral Island. Jenny decided this on 2026-09-29.
  - Cold and warmth are removed entirely: no meter, no cold nights, no clothing insulation.
    Clothing is purely expressive. Winter changes crops, forage and scenery, and it's never a
    threat.
  - There's no hunger meter, hunger penalty or hunger failure.
- **Food restores energy.**
  - Snacks restore a little: raw forage, bread, cheese.
  - Meals restore much more and also give **Well fed**. A meal is a cooked or cooked-and-bought
    dish: pasty, hearth dishes, fish dishes. While Well fed, every piece of work costs 15% less
    energy for a few game hours, and better dishes last longer.
  - Eating while Well fed refreshes the timer; bonuses don't stack.
  - This keeps a reason to cook and to stop for a proper meal, without a second bar to watch.
- There's no bedtime by the clock. Jenny ruled this in `flexible-sleep`: she can stay up all
  night, or sleep through the day to make up for it. At the bed she chooses between sleeping
  until morning, until rested, or a nap. If her energy runs out, she dozes off where she stands
  for a few hours of rough sleep and wakes only part rested. Nothing else is lost.
- A day lasts about **30 real minutes**, with the 30/60/120-minute setting kept. The day turns
  over at 6 AM. There are four 28-day seasons with named weekdays, starting on Monday, Spring 1,
  1851. Crops are tied to their seasons and wither when the season changes; round 2 has the
  details.
- Weather is sunny and cozy by default, with occasional rain that waters crops.
- There's no spoilage, in storage or anywhere else.

### Tools and tiers

- The flint knife and the machete are retired. Fibre inputs become bought twine and cord.
- Starter tools are crafted by hafting rusted heads salvaged from the ruin with wood cut on
  the estate, which keeps the crafting bootstrap:
  - **Axe:** the current hatchet, restyled in iron. It fells trees and clears stumps and
    fallen logs.
  - **Hoe:** restyled in iron, keeping the existing one-square-at-a-time hoeing.
  - **Pail:** the existing stream refill. A tin watering can comes from the general store
    later.
  - **Scythe:** cuts weeds and tall grass in a wide sweep. It needs a new mesh and a new
    two-handed sweep animation.
  - **Billhook:** clears bramble and saplings, reusing the machete's one-handed hack
    animation.
  - **Pickaxe:** breaks rubble and boulders on the estate from the first slice, then becomes
    the mining tool in round 7.
- **Fishing pole** arrives in round 4. It's crafted from cane or wood plus bought line and
  hooks.
- There's no mattock and no saw as a hand tool. Planks come from a sawpit or workbench.
- **Tier rule (Coral Island):** the blacksmith upgrades tools worn → iron → steel →
  master-forged.
  - A tier unlocks *what* a tool can clear, and also makes it faster or wider.
  - Worn → iron costs money plus scrap iron salvaged while clearing, so it's reachable before
    the mine. Steel and master-forged need bars smelted from mine ore.
  - Axe: the smallest stumps and thin branches, then large stumps and fallen logs, then
    ancient stumps and giant logs.
  - Pickaxe: small rocks and rubble, then boulders, then harder mine rock and deeper seams.
  - Billhook: thin bramble and saplings, then dense thickets, then thick old bramble banks.
  - Scythe, hoe and watering can cover a wider sweep or more squares per swing or fill.
  - Fishing poles have their own tiers from the general store and fishmonger.
  - An out-of-tier target shows a clear "needs a better …" prompt.
- The full crafting tree stays, restyled around a workbench and forge.

### Loops

- **Clearing.** Layered overgrowth, each layer gated by tool tier:
  - Grass and weeds: scythe.
  - Bramble and saplings: billhook. Yields canes, and blackberries in season.
  - Stumps and fallen timber: axe.
  - Rubble and boulders: pickaxe. Yields stone and scrap.

  Cleared ground stays cleared. Weeds creep back slowly on untended soil.
- **Farming.** Hand work on a snapped tile grid, growing later into a horse-drawn plough and
  seed drill on field strips.
  - Watering by can, then a pump, then irrigation channels.
  - Quality comes from manure or compost and care.
  - Field hands take over rows she assigns.
  - Crops: potatoes, turnips, wheat, barley, oats, cabbages, broccoli, peas, beans,
    strawberries, flowers, and an orchard.
- **Ranching.**
  - Animals: cows, goats, chickens, ducks, sheep (wool), pigs (truffles) and the horse. Bees
    come later.
  - Animals are bought young from the livestock dealer.
  - Care is gentle: feeding, or grazing a fenced pasture in season; petting; brushing;
    letting them out. Neglect lowers happiness and output. Animals never die.
  - Each animal has a name and a friendship level that improves its products.
  - **No slaughter.** Animals can be sold back to the dealer, and meat only comes from the
    town butcher.
  - She builds the fences, pens and barns herself.
- **Processing:**
  - Water mill: grain to flour. Later it drives other machinery.
  - Dairy: butter churn and cheese press.
  - Kitchen preserves, including blackberry jam.
  - Cider press, with an orchard.
  - Spinning wheel (wool to yarn) and loom (yarn to cloth).
  - Furnace: ore to bars.
- **Fishing.**
  - An early loop from the shore, river and lake, with a gentle timing minigame.
  - Fish store like any goods and never spoil.
  - Fish can be cooked into meals that restore energy and give Well fed, such as grilled fish, fish
    pie and stargazy pie, or sold to the fishmonger.
  - Later a boat opens pilchard and crab fishing as a fourth lucrative loop.
- **Mining.**
  - One large, fixed, hand-authored tunnel network that goes deeper level by level.
  - Ore veins regrow. Rare gem pockets appear at random within known veins.
  - It's a maze you learn, and you mark your way with chalk.
  - Danger is gentle, and nothing kills you:
    - When the lantern runs out, it's near-dark and you find your way out by the chalk marks.
    - Collapsing from exhaustion means waking at the surface minus some ore.
    - Floods and rockfalls block tunnels until they're shored.
  - Restoration steps:
    1. Clear the collapsed entrance and shore it with timber, opening the shallow level.
    2. Deeper levels are flooded until a water-wheel pump drains them.
    3. A steam pumping engine house is the big goal.
    4. Along the way come a winding gear, a dressing floor and hired miners.
  - **Output (Coral Island-style):**
    - Ore ladder: stone, coal, copper, iron, silver, gold. Deeper levels give better ores.
    - Gem ladder: quartz, amethyst, topaz, emerald, ruby, diamond.
    - Ores smelt into bars at a furnace, and both ores and bars sell.
  - The mine is the most lucrative loop and the hardest.

### Hauling and selling

- Hauling progresses in three steps:
  1. Carry a basket's worth by hand.
  2. Buy a wheelbarrow or handcart and push it.
  3. Buy a horse and wagon and drive it.
- Crates load visibly into the cart or wagon bed and unload at each shop door.
- The horse needs stabling and feed, which is a money sink.
- Once in town, selling to each shop is easy.
- **Prices:**
  - Each shop's price for an item falls as its stock of that item piles up, and recovers over
    the following days as townsfolk buy.
  - The goods she sold appear in the shop's stock and gradually sell down.
  - Quality tiers raise the price.
- A carter employee takes over selling later.

### Town

The town is established and fairly large. Its shops:

- General store.
- Seed and farm supply.
- Dressmaker and clothing.
- Blacksmith: tools, repairs, tier upgrades; buys scrap.
- Smelting house: buys ores and bars.
- Jeweller: buys gems.
- Builders' merchant: brick, slate, glass, lime, timber.
- Cabinetmaker and furniture shop.
- Livestock dealer.
- Fishmonger.
- Butcher.
- Inn.

The town opens with about 6–8 named MetaHuman shopkeepers at their counters, each with a
greeting and a gift and friendship track. Ambient townsfolk and daily schedules come in later
rounds.

### Manor

- The rebuild is **fully freeform**:
  - First she dismantles and salvages the ruin.
  - Then she rebuilds wall by wall on the old footprint with the existing foundation and wall
    snapping system.
- Materials come from four places:
  - Felled timber, sawn into planks.
  - Stone from the ruin, the fields and the mine.
  - Salvage from dismantling.
  - Bought brick, slate, glass, lime mortar and ironwork.
- The interior is decorated with crafted and bought furniture.
- The dog companion lives at the manor.

### Workers

- She hires named townsfolk into roles: field hand, dairymaid or stockman, miner, carter, and
  later a housekeeper or cook.
- Each worker has a weekly wage, a skill that grows, and needs somewhere to live on the
  estate, such as a rebuilt cottage.
- Workers are assigned to areas through the estate map.
- Unpaid wages lower morale and reputation, and eventually the worker quits.

### Money sinks and upkeep

- Recurring costs:
  - A parish rate and land tax, billed at the end of each season. They scale with land owned
    and buildings restored.
  - Wages, paid weekly.
  - Feed, lantern oil, tool repair and horse keep.
- One-off costs: land purchases, tool upgrades, building materials, livestock and furniture.
- Unpaid bills bring late fees and a reputation hit. **She never loses the estate.**
- There's no family debt and no rival family.

### Reputation

- The family name's standing rises with:
  - Visible restoration of the estate.
  - Fair wages and jobs for townsfolk.
  - Selling quality goods.
  - Friendships in town.
  - Town events and requests.
- Higher standing unlocks shop tiers, better hires, invitations and eventually courtship.

### Retained and dropped

- **Retained:**
  - Foraging: flowers, herbs, blackberries, mushrooms, and beachcombing.
  - One **dog** companion, dog first and cats later if ever:
    - Early breeds are pit bull and Great Dane, with coat choices including blue-nose grey
      and merle.
    - She can pet it, feed it and bond with it, and it can follow her on outings.
    - It's never harmed or lost.
- **Dropped:** hunting, predators, carcass scavenging, deer remains and fur, cold, spoilage.
- **Later:** courtship with a man from town, then raising a family.

### Round order

| # | Change | Playable outcome |
| --- | --- | --- |
| 1 | `author-fixed-cornish-estate-map`, `add-estate-boundary-map-and-minimap`, `add-overgrown-estate-clearing`, `add-ruined-manor-and-arrival`, `add-dollars-and-general-store` | **Walk your estate:** wake in the standing room on Spring day 1, see the boundary, clear overgrowth, walk to town, sell hay, scrap and flowers to the general store |
| 2 | `rework-farming-calendar-and-period-crafting` | Seasons and calendar, seed shop, period crops, restyled crafting |
| 3 | `add-handcart-hauling-and-dynamic-prices` | Handcart, blacksmith (repairs and the first tool upgrades), builders' merchant and dressmaker, stock-sensitive prices |
| 4 | `add-shore-and-river-fishing` | Fishing pole, catches, fish cooking, fishmonger |
| 5 | `rebuild-manor-and-decorate` | Dismantle and freeform rebuild, interior décor, dog companion |
| 6 | `add-ranching-and-livestock` | Livestock dealer, pens and barns, animal care and products |
| 7 | `open-estate-mine-shallow-level` | Entrance restoration, shallow level, lantern, chalk, furnace, smelter and jeweller |
| 8 | `add-horse-and-wagon` | Horse, stable, wagon loading and driving |
| 9 | `add-watermill-and-artisan-goods` | Mill, dairy, preserves, cider, spinning and weaving |
| 10 | `add-hired-workers-and-estate-upkeep` | Workers, wages, housing, seasonal rates and upkeep |
| 11 | `restore-mine-pumping-and-deep-levels` | Water-wheel pump, engine house, deep levels |
| 12 | `add-family-reputation-and-land-purchase` | Standing, town events and requests, parcel purchase |
| 13 | `add-boat-fishing` | Boat, pilchard and crab fishing |
| 14 | `add-courtship-and-family` | Courtship, marriage, and later children |

The order can change as Jenny playtests.

### Disposition of existing open changes

| Change | Disposition |
| --- | --- |
| `replace-heroine-with-metahuman`, `refine-playable-heroine-hairstyles`, `speed-up-pickup-animation`, `polish-locomotion-view-distance-and-time-hud` (locomotion part) | **Continue.** Character and animation work carries over unchanged. The view-distance part moves to World Partition HLOD on the fixed map (round 1). The time-HUD part moves to round 2's calendar. |
| `prioritize-heroine-quality-and-tool-clarity` | **Continue** the heroine-quality and Craft-icon work. The Fiber-route and flint-tool parts are superseded by the new starter tools. |
| `refine-equipment-preview-and-idle`, `add-heroine-footwear` | **Continue** as wardrobe and presentation work. Any warmth or insulation reading is superseded, and clothing is cosmetic. |
| `improve-menu-directional-navigation`, `improve-contextual-feedback-and-sprint`, `expand-randomized-music-playlist` | **Continue.** They're still valid, and sprint matters more on a 4 km map. |
| `polish-playtest-ui-and-fiber-discovery` | **Continue** the 4K UI parts. The Fiber-discovery part is superseded. |
| `fix-editor-playtest-findings` | **Continue** the Settings focus, feedback and shadow fixes. Vanishing generated trees and the flat creek are superseded by the fixed map. Re-check them there. |
| `upgrade-woodland-environment-assets` | **Fold into** `author-fixed-cornish-estate-map`. The admitted tree, understory and rock palette is reused as PCG/foliage on the fixed map. The procedural-placement acceptance is superseded. |
| Completed survival-era changes (`add-persistent-generated-woodland`, `add-regional-landforms-water`, `prototype-regional-landforms-hydrology`, `naturalize-woodland-creek`, `define-crafting-progression`, `add-timber-firewood-processing`, etc.) | **Historical.** They remain as records. Their procedural, warmth and primitive-crafting behaviour is superseded by this change and retired by the round that replaces it. |
| Parked branch `wip/foliage-night-dawn-wake` (d1b2535c, unmerged, unpackaged) | **Salvage later.** Clearing foliage under placed buildings belongs with the fixed map and manor rebuild. Dim but readable nights and a 6:45 bed wake belong with round 2's calendar. Re-apply them onto the fixed-world code rather than merging the procedural-world diff wholesale. |

### Save policy

Saves are disposable test data. The fixed map (round 1) resets test saves, and that's
reported plainly. Current-version save and load must keep working in every round.

## Risks / Trade-offs

- **4 km hand-authored world:** a large authoring load and streaming work.
  → Mitigation: World Partition, real terrain as a base, PCG biomes, and reuse of the admitted
  foliage. Round 1 needs only first-pass terrain and dressing. Detail is added gradually,
  area by area.
- **MetaHuman NPC cost:** each shopkeeper is authoring and runtime expense.
  → Mitigation: one shopkeeper in round 1 and about 6–8 in total before any ambient crowd.
  Use LODs and interior-only presence.
- **Item explosion:** items are a hard-coded C++ enum today.
  → Mitigation: round 1 introduces a data-driven item catalogue before the counts grow.
- **Economy balance:** money that's too easy or too hard.
  → Mitigation: stock-sensitive prices, sinks, tuning in data, and Jenny's playtests. Balance
  is provisional until round 10.
- **Anachronism:** modern clothing and dollars are deliberate choices. Everything else aims
  for verisimilitude, such as a real bramble-clearing technique and the mine's pumping.

## Open Questions

- Nothing about sleep or exhaustion remains open. `flexible-sleep` settled it.
- Dog breed coats and naming: resolved at round 5, possibly from Jenny's photos.
- The romance cast: resolved at round 14.
