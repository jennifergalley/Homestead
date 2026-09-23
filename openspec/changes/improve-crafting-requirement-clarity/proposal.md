# Proposal

## Why

The Craft page currently presents every recipe with the same active styling and a single `Needs:` sentence, so Jenny cannot scan which recipes are craftable or compare carried supplies with each requirement before attempting one. Crafting should communicate readiness and shortages before activation without changing recipe balance or transaction authority.

## What Changes

- Grey out recipe cards when the carried inventory, retained tool, or required nearby station cannot satisfy the recipe now.
- Keep unavailable recipes selectable so their name, output, and requirements can still be inspected; prevent their Craft action from looking enabled.
- Replace the opaque `Needs:` prose in the details pane with one row per requirement showing the item/tool/station, amount carried or available, amount needed, and met/short status.
- When an ingredient is short, show one concise authoritative acquisition hint in that requirement row (for example `Fiber · Reeds near water`) rather than making the player search the Guidebook.
- Distinguish consumed ingredients from retained requirements such as the knife, hatchet, or nearby cookfire.
- Refresh availability, colors, details, and action state immediately after inventory, equipment, storage, movement/station proximity, crafting, load, or new-world changes.
- Preserve Simulation as the final transactional authority. UI readiness is explanatory and MUST NOT bypass or replace authoritative validation at activation.
- Reuse existing recipe definitions, inventory counts, station proximity queries, native Slate cards/details, icon renderer, selection/focus model, and menu tests. No asset, plugin, network access, or licensing work is required.

The smallest useful in-game result opens Craft with no gathered materials and visibly greys every unavailable recipe while its selected details explain each shortage as `Have / Need` plus a concise source. The first playable demonstration sees `Fiber · Reeds near water`, gathers one reed patch for five Fiber, watches the crude hatchet become available, crafts it, and sees the list update without closing the field book. Full acceptance covers all six current recipes, authoritative source hints, retained tools, cookfire proximity, timber splitting, pointer/controller/keyboard navigation, stale-state prevention, 720p/4K layout, authoritative rejection races, and immutable Shipping replay.

Deferred scope includes recipe search/filtering, categories, favorites, batch crafting, queued crafting, recipe discovery, station inventories, pinned shopping lists, new recipes, rebalance, and changing where ingredients may be sourced. Nearby chest contents remain unavailable unless separately moved into the carried pack.

## Capabilities

### New Capabilities

- `crafting-requirement-clarity`: Pre-activation recipe availability styling and structured Have/Need requirement details backed by current authoritative crafting rules.

### Modified Capabilities

None.

## Impact

- `Simulation/HomesteadSimulation.cpp/.h`: read-only recipe requirement/availability descriptions plus tested primary acquisition hints derived from current recipe, resource-yield, retained-tool, and station rules.
- `HomesteadController.cpp/.h`: structured craft-row data and immediate menu refresh after relevant state changes.
- `UI/SHomesteadMenu.cpp/.h`: unavailable card tint, details requirement rows, disabled Craft affordance, and selection/focus preservation.
- Native menu/crafting/full-loop tests and HUD/menu evidence: all current recipes, devices, resolutions, state refreshes, and authority-race rejection.
- Existing saves, item/recipe enum IDs, recipe costs, output quantities, pack capacity, crafting messages, and `work-animation-complete-02-shipping / work-actions-v13` rollback remain unchanged.
