# Proposal

Round 2: farming on the new calendar, the seed shop, and period crafting.

## Why

Round 1 leaves the heroine with a cleared patch, a hoe, a pail and a few dollars, but the only
crops are the survival-era roots and berries on the old clock. The estate life sim needs a real
calendar and period crops she buys and sells, plus a crafting tree restyled for the 1850s.

## What Changes

- **Calendar:**
  - Four 28-day seasons, starting on Spring day 1.
  - Days last about 30 real minutes, from 6 AM to 2 AM.
  - A compact date and clock HUD shares the top-right corner with the minimap.
  - Sleep, energy exhaustion and the 2 AM pass-out rule: wake at home with a small, disclosed
    penalty.
  - Salvage the dim, readable nights and the 6:45 bed wake from `wip/foliage-night-dawn-wake`.
- **Weather:** sunny by default, with occasional rain that waters crops.
- **Crops:**
  - A period Cornish catalogue: potatoes, turnips, wheat, barley, oats, cabbages, broccoli,
    peas, beans, strawberries and flowers, each tied to its season.
  - Crops use hand work on the existing snapped garden grid.
  - Quality tiers come from care, and later from manure and compost (weeds become compost).
- **Seed and farm supply shop** in town, with a shopkeeper and seasonal stock.
- **Foraging turns seasonal.** Spring flowers give way to blackberry picking from uncleared
  bramble in late summer and autumn, and to mushrooms in autumn.
- **Period crafting restyle:**
  - A workbench and a sawpit (planks from timber).
  - Fences and gates, the first furniture, and preserves prep.
  - Retire the remaining primitive recipes.
- **Hunger tuning.** Hunger slows energy recovery and raises work cost. Eating fixes it
  immediately.

## Reuse starting points

- The existing gardening, watering and weeding code and assets: the tilled bed, seeds and
  soil mound.
- The shopkeeper, shop and catalogue from round 1.
- The parked night and wake work.
- Coral Island and Stardew Valley calendars and seed shops (conventions only).

## Smallest useful result and first playable demonstration

A full season of planting bought seeds, watering, weeding, harvesting and selling crops at the general store, with the date visible on the HUD.

## Capabilities

### New Capabilities

- `estate-calendar-and-farming`: outcome-level rules confirmed in the interview. They'll be expanded when this round is fully specified.

## Impact

To be defined when this round is fully specified.

## Status

This change is proposal-only. When the round starts, flesh it out with a design, spec deltas
and tasks. Do fresh, focused reuse research first (project code and assets, Unreal facilities,
licensed free assets), and fold in Jenny's latest playtest feedback. The confirmed decisions
for this round are in `openspec/changes/pivot-to-cozy-estate-life-sim/design.md`, and they
take precedence over anything inferred here.
