# Proposal

## Why

Jenny wants a grounded path from a knife and worn clothes to a winter-ready
homestead without carpeting a healthy woodland in loose sticks. The generation
lane needs resource-supply targets now, while new recipes must remain visibly
separate from the playable prototype.

## What Changes

- Deliver an explicitly non-runtime catalog of current items, recipes, building
  pieces, garments, gathering sources and current-unit costs, plus a bounded
  next slice and a separated advanced roadmap.
- Connect material, tool and station prerequisites in a dependency graph with
  a viable knife-to-hatchet bootstrap and no research currency or social gate.
- Quantify illustrative first-session, shelter, food and prewinter budgets,
  120-unit inventory trips and seeded-patch supply assumptions.
- Supply a small source/data-only validator for references, positive
  quantities, bootstrap reachability, prerequisites and budget arithmetic.
- Separate stable habitat/resource roles from tunable density and yields.

## Capabilities

### New Capabilities

- `crafting-progression-catalog`: inspectable, validated progression design data
  with implementation status, dependency reachability and resource budgets.

### Modified Capabilities

None. There are no published main specs in this checkout; this change does not
amend runtime survival or generation behavior.

## Impact

Owned new files: `docs/crafting-progression/`, the dedicated validator under
`Scripts/`, and this change. No edits to Simulation, World, Controller, existing
tests, saves, shared game plan or the current world OpenSpec round.

Reuse research: source baseline `c933270` already provides Item/Recipe/Piece
enums, transactional inventory, recipe/build costs, foraging renewal, garden
growth, wardrobes and fuel. `docs/game-plan.md`, `docs/asset-credits.md` and
`docs/research/environment-assets/resource-palette-20260921.md` establish the
product and admitted CC0 branch/plant/tree/rock palette. These remain usable;
no new engine framework, package, asset acquisition or licensing dependency is
needed. New work is the missing coherent dependency/budget data, not a crafting
engine or botanical/scientific model.

First delivery is validated design data, NOT new playable recipes. The smallest
future in-game demonstration is: find a few selected branch/stone/reed patches,
craft the existing hatchet, clear a chosen site, build a two-cell enclosed
shelter with bed/storage and outdoor fire, cook roots and plant a small garden.
Main owns integrated visual/play acceptance. Timber processing, preservation,
insulating garments and advanced joinery/ceramics/metalwork are separate
implementation steps; village/trade/hunting never gate this opening.
