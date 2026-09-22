# Proposal

## Why

Mature trees currently collapse into the same Branch/Fiber abstraction as
small vegetation, so clearing a woodland site does not yet create believable
construction wood or deliberate fuel preparation. The completed progression
catalog already identifies Timber and Firewood as the smallest useful next
runtime loop.

## What Changes

- Add distinct carried Timber and Firewood items while preserving Branch as
  small wood for current crude tools and shelter.
- Change permanent mature-tree felling to yield a bounded Timber/Branch bundle
  rather than the provisional Fiber byproduct.
- Add a knife/hatchet processing recipe that splits one Timber into four
  Firewood.
- Let an existing cookfire consume one Firewood for the same provisional
  four-hour service while retaining Branch as an accepted fallback fuel.
- Persist, transfer, display, reject on capacity, and replay the new items
  through the existing inventory, chest, recipe, save, and generated-world
  routes.
- Keep Cordage, workbenches, timber building pieces, durability, on-site heavy
  stacks, spoilage, and final tree economics deferred.

## Capabilities

### New Capabilities

- `timber-firewood-processing`: Mature-tree timber yield, explicit firewood
  processing, dual cookfire fuel, inventory behavior, and persistence.

### Modified Capabilities

None.

## Impact

This slice reuses the project-authored `Simulation` inventory arrays,
transaction helpers, generated mature-tree authority, field-book recipe rows,
chest transfers, save format, Shipping generated-world fixture, and current
cookfire state. No external assets, code, plugins, downloads, accounts, or
licenses are needed. Runtime source, portable tests, menu/source invariants,
current-version save checks, and the generated-world gameplay route are
affected. The smallest playable demonstration is: fell one mature tree, keep
its stable clearing, split one Timber, fuel a player-built cookfire with the
result, save, and reload with exact item/fire state.
