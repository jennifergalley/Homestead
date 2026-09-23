# Proposal

## Why

The Craft page currently presents every recipe with the same active styling, prints names in the grid, flattens requirements into `:`/`;` prose, and crafts instantly through a separate action. Jenny wants a quieter icon grid whose details are genuinely scannable and whose direct hold interaction makes crafting feel tangible and repeatable.

## What Changes

- Show only the recipe icon in each compact grid tile; move recipe name, output, description, and all requirements into the details pane.
- Grey out unavailable recipe icons. Keep them selectable so details remain inspectable, but do not show or enable a separate Craft action button.
- Replace the opaque `Needs:` paragraph and `:`/`;` formatting with a vertical list: one semantic row per ingredient/tool/station/capacity requirement showing Have, Need, status, and concise source where short.
- When an ingredient is short, show one concise authoritative acquisition hint in that requirement row (for example `Fiber · Reeds near water`) rather than making the player search the Guidebook.
- Distinguish consumed ingredients from retained requirements such as the knife, hatchet, or nearby cookfire.
- Craft directly by pressing and holding an available recipe icon with pointer, Enter/Space, or controller A. A 1.2-second cycle starts grey and fills with color toward completion.
- Play three restrained hammer-on-anvil/metal strikes during each cycle using exact auditioned files from the already admitted Kenney Impact Sounds CC0 archive, respecting Effects volume.
- Commit one authoritative recipe transaction only when a cycle completes. Continuing to hold immediately starts another cycle while requirements remain valid; releasing, losing focus/capture, changing page, or becoming unavailable cancels only the incomplete cycle.
- Refresh availability, colors, details, and action state immediately after inventory, equipment, storage, movement/station proximity, crafting, load, or new-world changes.
- Preserve Simulation as the final transactional authority. UI readiness is explanatory and MUST NOT bypass or replace authoritative validation at activation.
- Reuse existing recipe definitions, inventory counts, station proximity queries, native Slate cards/details, icon renderer, selection/focus model, menu tests, and the already admitted Kenney archive. No new external source, plugin, network permission, purchase, or license is required.

The smallest useful in-game result opens Craft with icon-only recipe tiles, visibly greys unavailable icons, and shows the selected recipe's name/output plus a real requirement list. The first playable demonstration sees `Fiber · Reeds near water`, gathers one reed patch, then holds the crude-hatchet icon as three metal strikes accompany a grey-to-color fill and exactly one Hatchet commits at 1.2 seconds. Full acceptance also holds a repeatable recipe through multiple cycles, releases mid-cycle without spending ingredients, and covers all six recipes, authoritative source hints, retained tools, cookfire proximity, timber splitting, pointer/controller/keyboard hold/release, stale-state prevention, 720p/4K layout, audio volume/mute, authoritative rejection races, and immutable Shipping replay.

Deferred scope includes recipe search/filtering, categories, favorites, queued/offline crafting, explicit batch-count dialogs, recipe discovery, station inventories, pinned shopping lists, new recipes, rebalance, and changing where ingredients may be sourced. Hold repetition is the first batch mechanism; nearby chest contents remain unavailable unless separately moved into the carried pack.

## Capabilities

### New Capabilities

- `crafting-requirement-clarity`: Icon-only recipe selection, structured requirement/source details, hold-to-craft progress/repetition, and authoritative completion.

### Modified Capabilities

None.

## Impact

- `Simulation/HomesteadSimulation.cpp/.h`: read-only recipe requirement/availability descriptions plus tested primary acquisition hints derived from current recipe, resource-yield, retained-tool, and station rules.
- `HomesteadController.cpp/.h`: structured craft-row data and immediate menu refresh after relevant state changes.
- `UI/SHomesteadMenu.cpp/.h`: compact icon-only recipe tiles, clipped color-fill progress, hold/release state, details requirement rows, no separate Craft action, and selection/focus preservation.
- Audio bootstrap/import/controller: exact CC0 metal-strike assets from the admitted Kenney archive and Effects-volume playback.
- Native menu/crafting/full-loop/audio tests and HUD/menu evidence: all current recipes, devices, resolutions, press/hold/release/repeat, state refreshes, and authority-race rejection.
- Existing saves, item/recipe enum IDs, recipe costs, output quantities, pack capacity, crafting messages, and `work-animation-complete-02-shipping / work-actions-v13` rollback remain unchanged.
