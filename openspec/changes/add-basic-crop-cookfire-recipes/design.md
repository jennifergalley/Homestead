# Design

## Context
See proposal.md. CraftChange and Cooking currently define two meals; RecipeName and the field book contain parallel metadata. Item stocks already save their width. SellPrice currently equals BasePrice, in whole coins.

## Goals / Non-Goals
Extend authoritative cooking with eight dishes, consistent inspection and sale-cost tuning. No recipe unlock system, new station/tool requirements, crop-price changes, dish selling, animations or assets.

## Decisions
- Add a small recipe-data helper beside the simulation and use it from crafting, assessment and cookbook metadata, rather than grow divergent switches. Append Item/Recipe values; add the source to CMake.
- Reuse the existing station, consumption, food and icon machinery. New dishes are Meals and can be pinned/eaten like existing dishes.
- Dishes: roasted turnips (1 Turnip), stewed carrots (2 Carrot), baked potatoes (2 Potato), herbed broad beans (3 BroadBeans + 1 Flowers), cabbage and potato stew (1 Cabbage + 1 Potato + 1 Flowers), berry compote (3 Berries), strawberry compote (2 Strawberries), root vegetable hotpot (2 Roots + 1 Turnip + 1 Flowers). Each also consumes 1 Kindling.
- Tune Energy as round(12 + 0.6 * consumed sale coins), including seasoning/fuel. The modest shared cooking benefit avoids arbitrary per-dish bonuses while valuable cabbage remains worth more Energy. Native tests derive costs from the authoritative recipe and sale tables so later price edits expose drift.

## Lanes and ownership
Farming Fishing exclusively owns Item/Recipe/catalogue, craft helpers and cookbook. Travel Rest's State/discovery/sleep changes must survive localized simulation integration. Integration owns compile/package admission and shared documentation.

## Risks / Trade-offs
Existing native cases iterate all Recipe values and hard-code the old eight outputs/count; update those directly coupled fixtures. Cheap roots remain useful and purchased pasties remain convenient; do not rebalance existing food outside Jenny's request.

## Migration Plan
Append-only items widen counted stocks; older saves initialize new food at zero. No save/bake version changes or tagged sections. Keep compatible build caches; focused simulation/economy tests plus coordinated editor compile precede [ready]. Jenny checks the integrated cookbook/cook/eat path.
