# Proposal

Round 3: handcart hauling, more shops and supply-sensitive prices.

## Why

Carrying a basket's worth by hand limits how much she can sell per trip, and fixed prices invite
spamming a single good. Round 3 makes hauling a real loop and makes the town's prices respond to
supply, so money isn't infinite.

## What Changes

- **Wheelbarrow and handcart:**
  - Bought from the general store or blacksmith, and pushed by the heroine along the road.
  - Crates load visibly into the cart bed and unload at each shop door.
  - Cart capacity is separate from the pack.
- **More shops open:**
  - The **blacksmith** sells and repairs tools, buys scrap, and runs **tool tier upgrades**.
    Worn → iron costs money plus scrap iron.
  - The **builders' merchant**, and the **dressmaker** for Jenny's custom clothing designs.
- **Supply-sensitive prices:**
  - Each shop's price for an item falls as its stock of that item piles up.
  - Prices recover over the following days as townsfolk buy.
  - The quality tier multiplies the price.
- The Sell screen shows today's price trend for each item, so selling stays easy.

## Reuse starting points

- Round 1's shops, stock and sell-down.
- The existing carried-object presentation.
- Coral Island's blacksmith upgrades (conventions only).
- Physics-light cart pushing, using Unreal's Chaos Vehicles or a custom kinematic cart.

## Smallest useful result and first playable demonstration

Load a handcart with a day's harvest, push it to town, sell across two shops, watch prices dip for a flooded good, and upgrade the axe to iron at the blacksmith.

## Capabilities

### New Capabilities

- `hauling-and-market-prices`: outcome-level rules confirmed in the interview. They'll be expanded when this round is fully specified.

## Impact

To be defined when this round is fully specified.

## Status

This change is proposal-only. When the round starts, flesh it out with a design, spec deltas
and tasks. Do fresh, focused reuse research first (project code and assets, Unreal facilities,
licensed free assets), and fold in Jenny's latest playtest feedback. The confirmed decisions
for this round are in `openspec/changes/pivot-to-cozy-estate-life-sim/design.md`, and they
take precedence over anything inferred here.
