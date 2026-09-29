# Design

## Context

- **Item metadata today:** `Item` is an enum. Names, requirements and icons come from
  switch-based functions and `SHomesteadIcon` glyphs. The inventory is a count array.
- **UI:** the field book is one Slate widget with regions, dialogs (including Quantity) and
  directional focus. Menus pause the Simulation.
- **Character:** the heroine is a MetaHuman authored in-editor.
- **Dependencies:**
  - The world lane publishes `TownSquare` and `GeneralStoreDoor`.
  - The clearing lane adds the new round-1 items to the catalogue.

## Goals / Non-Goals

**Goals:**

- A trustworthy whole-coin path: every stored unit is accounted for in one transaction.
- Selling in town is easy.
- The player's goods are visibly present in the shop.
- One item catalogue that later rounds extend.

**Non-Goals:**

- Dynamic prices, quality, other shops, townsfolk crowds, friendship, voice, or selling
  remotely.

## Decisions

### 1. Money is stable integer whole coins in Simulation

- The existing `State.money` `int64` raw value is semantically one whole coin per unit. It is not
  numerically converted: balances, catalogue prices and serialized bytes retain their value, so old
  raw 1000 becomes 1,000 coins rather than 10,000.
- Formatting lives in canonical `FormatMoney`/`Delta` helpers: grouped integers plus correct
  singular/plural `coin` text, with no `$` or decimal notation.
- The implementation performs the semantic field/type rename and routes all display surfaces through
  those helpers. `GrantMoney` guards against cap overflow.
- Every change goes through `Sell` or `Buy`, which validate before mutating.
- A negative balance is impossible in round 1, since there's no debt. Round 10's bills may
  add late fees, but never a debt that blocks play.

### 2. Catalogue table keyed by `Item`

- `HomesteadItems.cpp` holds a `constexpr` array of `ItemInfo` in `Item` order:
  - key, name, description, category, icon glyph;
  - base price in whole coins;
  - a mask of the shop kinds that buy it, for example `GeneralStore`;
  - an edible restore amount (hunger and energy) for food;
  - a hidden-from-new-games flag, for the retired items.
- A static assertion checks that the table covers `Item::Count`. `ItemName` and the other
  lookups are migrated to it.
- **Alternative considered:** UE `UDataTable` assets now. Rejected as premature. The portable
  Simulation tests need no UObject dependency, and the table can be generated into data
  assets when content grows.

### 3. Shops and stock

- `Shop` records:
  - kind and name;
  - open and close hours;
  - `ownStock`: a fixed catalogue with unlimited restock in round 1, the basic foods and
    twine;
  - `heroineStock`: a map from item to quantity.
- **Sell(item, qty):** the shop must be open, the heroine must be at its counter (the
  interaction focus proves presence), the shop must buy that item, and she must carry the
  quantity. Then it removes the items from her inventory, adds `price × qty` to her money, and
  adds `qty` to `heroineStock`, as one transaction.
- **Buy(item, qty):** the item must be in stock, and her money and capacity must be
  sufficient. Items from her stock sell back at their sell price, with no markup in round 1.
  Shop goods sell at their buy price, which is the base price × 1.25.
- **Daily sell-down:** at the 6 AM rollover, each item in `heroineStock` drops by
  `max(1, ceil(qty × 0.35))`. The value is tunable, and a deterministic formula keeps tests
  stable.

### 4. Shop screen

- `SHomesteadShop` is a modal Slate page that reuses the field-book frame, grids, icons,
  Quantity dialog and focus rules.
- Two tabs, **Sell** and **Buy**, switch with LB/RB or Tab. **From {Estate}** is a labelled
  group at the top of Buy.
- Each row shows the icon, name, quantity and unit price. The footer shows the running total,
  the wallet after the transaction, and Confirm.
- Opening the shop pauses the game, and so does the field book. Closing returns focus to
  the world.

### 5. Town and store presence

- **Massing:** the town square massing reuses the ruin kit's *intact* variants (full-height
  granite walls, slate roofs, chimneys, sash windows), and each building has its shop sign
  board as an empty placeholder. Non-store doors are inert.
- **Store interior:** the store is a single-room interior inside the level. There's no level
  streaming boundary, just a door opening.
- **Props:** a counter, wall shelving, barrels, sacks and crates, a candle lantern, and a
  shop bell.
- **Hours:** outside opening hours the door swings shut. Interacting shows "Closed — opens at
  8 AM", and a sign hangs on the door.

### 6. Shopkeeper

- `AHomesteadShopkeeper` is a MetaHuman Blueprint-free C++ actor. It loads its assembled
  MetaHuman and plays a looping idle, reusing the relaxed idle retargeted to her body.
- Head look-at uses a control rig or an anim-blueprint aim that looks at the heroine within
  about 4 m.
- The interaction prompt reads "Talk to Mrs. Pascoe". Talking shows a greeting panel, a small
  speech box with her name. Its lines are picked from a short original set by time of day,
  first visit and repeat visit. Continue opens the shop.
- A greeting counter is saved, as a stub for round 12's friendship.
- The MetaHuman is authored in-editor with Jenny's Epic sign-in for its cloud steps, as with
  the heroine. She's a woman in her fifties in a period dress and apron. Modern custom
  clothing is the heroine's alone, and townsfolk dress period-plausibly.

## Lanes and ownership

This lane owns:

- The money, the catalogue file, the shops and transactions, and the sell-down.
- `SHomesteadShop` and the wallet HUD.
- The town massing and store interior, the store props, and the shopkeeper actor and asset.

It shares:

- The catalogue file, which the clearing and arrival lanes add rows to; merges are serialized.
- The landmarks from the world lane.
- The save version, bumped once at integration.

The MetaHuman cloud steps need Jenny's sign-in, which is a scheduled human step.

## Risks / Trade-offs

- **The MetaHuman step is gated on Jenny's Epic sign-in.** → Author her early. Until she's
  ready, a clearly labelled mannequin stand-in keeps the store playable.
- **The catalogue migration touches many call sites.** → Migrate mechanically behind the same
  function signatures. The existing tests guard behaviour.
- **Fixed prices invite hay-spam.** → Accepted for round 1. Prices become supply-sensitive in
  round 3, and the sell-down already makes the stock visible.
- **Town massing looks like a copy-paste street.** → Vary heights, rooflines, chimney
  positions and wall materials (ashlar, rubble and lime-washed). Blockout is labelled as
  such.

## Open Questions

- The shopkeeper's name and look (a placeholder), the starting money, and the price table,
  all tuned with Jenny.
- The MetaHuman EULA text is to be confirmed before any distribution beyond Jenny's PC.
