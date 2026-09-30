# Proposal

## Why

The pivot's goal is a fortune (`estate-life-sim-direction`). That needs money, a town to sell
in, and goods that physically reach the shop. Round 1 closes the first loop: clear the estate,
walk to town, sell, and see your goods on the shelf. Homestead has no currency, shop, NPC or
town today. Items are a hard-coded enum whose names and metadata are scattered, and the item
count is about to grow a lot.

## What Changes

- **Money.**
  - Simulation preserves money as its existing integer `int64` raw value, semantically one whole
    coin per unit. It shows grouped whole coins, for example `12,400 coins`; no numeric migration
    changes balances, prices or saves.
  - The HUD shows a wallet readout beside the energy and hunger meters. Every change animates
    with a small delta, such as "+320 coins".
  - A new game starts with **1,000 coins**, a placeholder for tuning.
- **Item catalogue foundation.** One authoritative table in `Simulation\HomesteadItems.*`,
  keyed by `Item`. For each item it holds:
  - a stable key, the display name and a description;
  - a category (Tool, Material, Forage, Food, Salvage…);
  - an icon reference;
  - the base price in whole coins;
  - a sellable flag and the shop kinds that buy it.

  Existing `ItemName` and similar lookups read from the table, and the scattered metadata is
  folded into it. Moving to data assets waits until the content scale needs it.
- **Town blockout** at the estuary head, where the road enters:
  - A small square with about 8–12 massing buildings in granite and slate. They reuse the
    ruin kit's intact wall, window and roof variants, and they're honestly labelled as
    blockout in the playtest notes.
  - Only the **general store** has an interior this round.
- **General store:**
  - An interior with a counter, shelves, barrels, crates and sacks. They're procedural Blender
    props in period style.
  - A door on the square at `GeneralStoreDoor`.
  - Opening hours of **8 AM–6 PM**. Outside those hours the door shows a "Closed" sign and a
    reopening time.
- **Shopkeeper:**
  - One MetaHuman, placeholder name **Mrs. Martha Pascoe**, standing behind the counter.
  - She has an idle animation and a gentle look-at, and shows a short greeting in a speech
    panel that varies by time of day and visit. There's no voice.
  - Talking to her opens the shop.
- **Shop screen**, a modal Slate page in the field-book style. It pauses the game and has two
  panes:
  - **Sell:** the carried items this shop buys, at fixed prices this round. A quantity dialog
    handles stacks, and the total previews before confirming.
  - **Buy:** the shop's own stock, such as Cornish pasties, bread and cheese, whose meals give
    hunger and energy, and twine. Under it, a **From {Estate name}** section lists the goods
    she sold, which she can buy back.
  - **Sold goods sell down.** At each 6 AM rollover, townsfolk buy a share of the shop's
    stock of the goods she sold, so the section visibly drains over the following days.
    Prices stay fixed this round; supply-sensitive pricing arrives in round 3.
- **Round-1 sellables at the general store:** Hay, Scrap iron, Scrap lead, Bramble canes,
  Kindling, Firewood, Stone, and spring Flowers and forage (primrose, bluebell, wild daffodil,
  wild garlic). New spring forage placements come from the world lane's PCG bake.

## Reuse research

- **MetaHuman:**
  - The project already runs the in-editor MetaHuman Creator (the `MetaHumanCharacter` and
    `MetaHumanCoreTech` plugins) for the heroine, with Jenny's Epic sign-in for the cloud
    steps.
  - The shopkeeper follows `docs\character-pipeline.md` and `replace-heroine-with-metahuman`.
  - Her idle reuses the heroine's relaxed idle on `metahuman_base_skel`.
  - Licence: shipped in-Unreal use falls under the Unreal Engine EULA terms for MetaHumans.
    The current EULA text couldn't be loaded behind Epic's login during research, so Jenny or
    a human confirms it before the build is distributed. See the report.
- **Town art:**
  - No verified free Victorian English town pack was found.
  - Megascans' fully free access ended on 31 December 2024.
  - Fab Standard License assets are allowed in shipped games, but claiming them needs an
    account, which isn't authorised here.
  - Quaternius CC0 medieval packs are available but stylised, so they're a last-resort
    blockout only.
  - So: reuse the project's own procedural granite and slate ruin kit in intact form, and
    author the store props in Blender.

  See `docs\research\estate-pivot\first-slice-reuse.md` §5 and §7.
- **Project UI and input:** `SHomesteadMenu`'s pointer-first grids, the Quantity dialog,
  directional navigation, `SHomesteadIcon`, the toast system, the HUD meters, and the
  interaction focus/prompt system.
- **Comparative references (conventions only):**
  - Coral Island's shop counters, opening hours and closed signs, and a buy/sell split.
  - Stardew Valley's shopkeeper greeting, then the shop menu.
  - Dreamlight Valley's clear price and total previews.
- **Custom gaps:** the money model, the catalogue table, the shop stock model, the shop
  screen, the town and store blockout and props, the shopkeeper character and her dialogue,
  and the hours.

## Smallest useful result and first playable demonstration

**Smallest result:** in the packaged game, walk into town with cut hay and scrap. Talk to Mrs.
Pascoe, sell the hay, and watch the wallet rise. The Buy pane's "From Trevennor" section lists
the hay. Buy a pasty and eat it, and hunger and energy go up.

**Full acceptance** adds:

- Every round-1 sellable, stack quantities, buy-back, and the daily sell-down across sleeps.
- Opening hours and the closed sign.
- The full catalogue migration.
- Mouse and controller parity, and 720p/4K layouts.
- Save and load of money and shop stock.
- The rest of the town massing and the store props.

**Deferred:**

- Supply-sensitive prices, quality tiers and other shops (round 3).
- Townsfolk and schedules.
- Friendship and gifts, beyond a greeting-count stub.
- Voice.
- Carter automation (round 10).

## Capabilities

### New Capabilities

- `money-and-shops`: Money, the item catalogue, shop definitions and hours, selling and
  buying, the stock of goods she sold with its daily sell-down, the shopkeeper interaction,
  and the town's presence on the map.

### Modified Capabilities

None.

## Impact

- `Homestead::Simulation`:
  - `money` (whole coins, retaining its raw saved integer).
  - `Shop { kind, openHour, closeHour, ownStock, heroineStock }`.
  - `Sell` and `Buy` transactions.
  - A daily sell-down at the rollover.
  - Save fields and a version bump, coordinated at integration.
- New `Simulation\HomesteadItems.*`, the catalogue. Callers of `ItemName` and similar
  lookups are migrated.
- New `UI\SHomesteadShop.*` and the HUD wallet readout in `HomesteadHUD`.
- The Estate level: the town square blockout, the general-store interior, and the shopkeeper
  actor (a new `AHomesteadShopkeeper` with idle, look-at and greeting).
- New Blender recipes: the intact granite and slate building variants, and the counter,
  shelves, barrels, crates, sacks and signs.
- The MetaHuman shopkeeper asset under `Content\Characters\Townsfolk\Pascoe`.
- Tests:
  - Portable money, shop, sell-down and save tests.
  - Native shop-screen navigation and quantity tests.
  - A full-loop route that sells in town.
