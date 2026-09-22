# Design

## Context

See `proposal.md` for motivation and
`specs/timber-firewood-processing/spec.md` for observable behavior. The current
simulation already owns fixed-size item inventories, atomic candidate-state
commits, recipe costs, generated resource edits, chest transfers, fire fuel and
text saves. The controller builds Field Book rows from enum/count helpers and
routes the focused fire action through one mapped input. Generated mature trees
already use stable keys and the Shipping producer/consumer route already fells
and replays one actual tree.

Appending item values changes the fixed inventory width written throughout the
save body. Current test saves are disposable, but a mismatched width must reject
before materialization. The selected `regional-water-17` package remains the
usable preview until a replacement passes.

## Goals / Non-Goals

**Goals:**

- Extend the existing authority and UI patterns rather than introduce runtime
  data loading, a generic processing framework or new assets.
- Preserve numeric IDs for every current item and recipe by appending new values.
- Keep clearing, crafting, fueling and transfer mutations atomic under capacity
  failure.
- Exercise the feature through portable authority checks and one real
  generated-world producer/consumer path with mapped inputs.

**Non-Goals:**

- Timber structures, workbenches, cordage, durability, processing queues,
  ground drops, on-site stockpiles or final economy balancing.
- New tree, tool, fuel, fire, animation, sound or VFX assets.
- Migration of old disposable test saves or changes to world generation,
  vegetation placement, terrain, water, controls, needs or appearance.

## Decisions

### Append narrow enum values and bump the simulation save version

Append `Timber` and `Firewood` immediately before `Item::Count`, and append
`SplitFirewood` immediately before `Recipe::Count`. All current IDs remain
stable, existing enum-driven rows and inventory arrays expand naturally, and
the new recipe can use the same activation path and time increment.

Increase `SimulationSaveVersion` because serialized inventories have an
implicit compile-time width rather than a per-array count. The loader already
validates the version before committing a parsed candidate, so older test saves
reject transactionally and fresh profiles need no migration. Alternative:
special-case version-5 array widths. Rejected because Jenny has explicitly made
test saves disposable and this would add migration complexity without player
value.

### Reuse resource yield and candidate-state commit authority

Change only `ResourceKind::ForestTree` in the existing yield table to six
Timber and four Branch. Leave `ResourceKind::Sapling` untouched. `Clear`
continues to apply the yield to a copied state, records the stable resource edit,
and commits only after the inventory/layout/capacity invariants pass. Thus a
full pack cannot grant partial output or clear the tree.

Alternative: spawn world pickups or retain excess output on the stump. Deferred
because it requires new world entities, visuals and persistence; the bounded
first slice can communicate required pack space through the existing error.

### Model splitting as one ordinary recipe with a retained tool prerequisite

Add a cost change of `-1 Timber, +4 Firewood` and require a carried Hatchet for
`SplitFirewood`. The existing atomic inventory adjustment handles the net
three-unit capacity increase and layout update. Give the recipe its own
requirement text so it does not inherit the current generic knife prerequisite.
No station, knife, direct energy, durability or queue state is added.

Alternative: add a separate processing API. Rejected because one instantaneous
transaction does not justify a second framework or controller mode.

### Keep one fire input and use deterministic fuel priority

Retain the current focused-fire mapped action. `AddFuel` consumes one Firewood
when available and otherwise one Branch, adds four hours under the existing
48-hour cap, and names the consumed fuel in its result. This makes prepared fuel
immediately useful without adding a fuel-selection modal; Branch-only behavior
remains compatible.

Alternative: expose two fire actions or a selection menu. Deferred until
multiple fuels have different service values, because identical four-hour fuels
do not justify extra interaction cost.

### Extend the existing evidence route instead of building a new harness

Portable simulation tests cover IDs, names, recipe requirements, mature-tree
versus sapling yields, all missing/capacity branches, deterministic fuel
priority, transfer and save rejection/replay. Source/UI invariants cover the
new Field Book row and strings.

The generated-world producer route will use its existing actual mature-tree
clear to assert Timber/Branch yield, craft `SplitFirewood` through the current
book/mapped-input path, fuel its existing player-built fire, place at least one
new material in the chest, and persist the exact cleared key, inventory, chest
and fuel state. A separate consumer process replays those exact facts. This is
proportionate because the affected interfaces converge in Simulation and the
same Shipping fixture; implementation is serialized in this session rather
than split across overlapping workers.

## Risks / Trade-offs

- **Prepared fuel is consumed before loose Branch when both are carried** →
  state the priority in the prompt/result and test it; revisit selection only
  when fuels gain different values.
- **Ten units from one mature tree can exceed a nearly full pack** → preserve
  atomic rejection and a clear capacity message; on-site overflow is deferred.
- **Enum expansion invalidates current test saves** → bump the save version,
  use fresh candidate profiles and retain unrelated user files untouched.
- **Existing recipe UI may assume five rows** → audit enum-driven and hardcoded
  row/count sites before compile, then add focused source and runtime checks.
- **Changing the generated producer fixture can obscure regional regressions**
  → retain all existing terrain, water, batching and persistence assertions and
  add only the new economy facts to the same stable-key sequence.

## Migration Plan

1. Append enum/schema values and implement authority transactions with portable
   tests.
2. Wire requirement/presentation strings and extend focused source tests.
3. Extend the existing generated producer/consumer fixture and compile Unreal.
4. Package to a fresh immutable candidate directory using compatible caches,
   run the focused producer and separate consumer, then inspect ordinary
   gameplay evidence if the changed Field Book or interaction is visible.
5. Promote only after the acceptance receipt and launcher validation pass.
   Rollback keeps `regional-water-17` / `regional-review-v7` selected and
   reverts the source checkpoint; no existing candidate is overwritten.
