# Design

## Context

See `proposal.md` and `specs/droppable-inventory-items/spec.md`. Simulation already has authoritative pack/chest inventory totals, stable stack group IDs, unique wearable IDs/dyes/owners, expected revisions, 120-unit capacity, atomic candidate commits, bounded object validation, and current save serialization. World resources are generated/renewable entities and are the wrong ownership model for player-dropped possessions.

The menu already supports explicit amount confirmation for split/transfer, and sibling plans replace it with a visible stepper. World already builds grounded noncolliding visuals and focus targets from Simulation state. The missing primitive is a persistent player-owned world drop.

## Goals / Non-Goals

**Goals:**

- Add physical, recoverable drops without deletion or inventory/world two-phase failure.
- Preserve stack/wearable identity, capacity, save authority, focus, and hotbar truth.
- Keep drops stationary, nonblocking, bounded, and visually distinct from natural resources.

**Non-Goals:**

- Physics throwing, partial pickup, decay/despawn, NPC ownership, death loot, world containers, arbitrary item models, drag-to-drop, or multiplayer.

## Decisions

### 1. Add one stable world-drop record type

Add `WorldDrop { id, position, item, quantity, wearableId }` to State. Exactly one payload form is valid:

- ordinary stack: valid item, positive quantity, wearableId zero;
- wearable: quantity one, stable wearableId whose owner is World.

Reuse the existing globally allocated `nextId` range for drop IDs and add at most 512 persistent drops. Add `WearableOwner::World` plus position ownership through the drop record. Validation rejects duplicate IDs, mismatched totals/owners, nonfinite/out-of-range points, unsupported payloads, and count overflow. Current save version gains a bounded drop section.

**Alternative considered:** encode drops as resource nodes. Rejected because resources have regeneration/generated keys and gathering semantics that would duplicate or respawn player possessions.

### 2. Make placement authority deterministic and atomic

Controller asks World for ordered grounded candidate points roughly 90-150 cm forward/around the heroine, excluding water, structure/plot footprints, solid collision, and inaccessible terrain. Simulation receives the chosen XY plus player point and independently validates finite bounds, maximum distance, active playable state, quantity/group/wearable ownership, revision, limit, and no structure/plot conflict.

`DropGroup`/`DropWearable` modify a candidate State: subtract/remove the pack entry, set wearable World ownership if applicable, then merge or append the drop. One `CommitInventory`-style commit publishes both sides. If no candidate passes or commit validation fails, nothing changes.

World drops reserve a small low-cover/build footprint so foliage/structures do not bury them, but have no collision/navigation themselves.

### 3. Merge only ordinary compatible drops

Before allocating a drop, find the lowest-ID ordinary drop of the same item within 120 cm and below safe integer quantity limits. Add quantity to it; do not merge wearables, different items, or distant drops. Merging does not change placement or ID and still commits atomically.

**Alternative considered:** always create one actor per action. Rejected because repeated small drops create clutter and exhaust persistent IDs without player value.

### 4. Pick up the whole drop through inventory authority

Add `PickUpDrop(id, player)` with range/playable/capacity validation. Ordinary stacks enter a new/current pack group under existing layout rules; wearables switch World -> Carried and re-enter pack layout with the same ID/dye. Candidate removal occurs only with successful insertion. No partial fit: the drop remains if the whole payload cannot fit.

Focus order treats a closer drop as an eligible interaction alongside resources/structures, with stable tie-breaking. Successful feedback uses the exact pickup-delta toast planned by `improve-contextual-feedback-and-sprint`; failed capacity remains an error.

### 5. Use one small original bundle presentation

Render drops as a low, stationary project-primitive bundle/token with item-category tint and a small count accent, grounded through `AtGround`. Focus title supplies exact item name/count; unique wearables retain their wearable icon/tint in the menu after recovery. Drops have no collision, overlap, nav influence, shadow-heavy geometry, camera blocking, animation, or physics.

This first slice intentionally does not build a 3D model for every item. The presentation must be recognizably player-dropped and not resemble forage/regrowing patches.

### 6. Integrate the menu through existing stepper semantics

Add `Drop` to item-group and carried-unequipped-wearable actions. Ordinary stacks open amount confirmation from one to stack quantity; wearables use a one-item confirmation. The dialog captures stable subject ID, expected revision, requested amount, and a placement snapshot. Confirm re-resolves a safe point and calls authority once. Cancel/back/focus movement is nonmutating.

Chest rows and paper-doll equipped slots omit Drop. Compact inventory refresh preserves nearest valid selection after a successful drop.

### 7. Coordinate save/window/hotbar surfaces

World builds visuals only for drops inside the active visual window and removes them when absent/outside, while State retains all records. Save/load/chunk churn cannot recreate collected drops. Hotbar resolves live carried ownership, so dropping a tool automatically ghosts its reference with no special mutation.

Simulation/drop tests and World visual/focus tests can proceed independently behind the record/API contract. Menu integration follows the compact inventory and quantity stepper interfaces; Editor/cook/package remain serialized.

## Risks / Trade-offs

- **[Drop placement buries items]** -> deterministic safe candidate search, low-cover/build reservation, grounded visual and blocked-placement rejection.
- **[Save corruption duplicates ownership]** -> strict one-payload validation, wearable World owner invariant, exact inventory/drop total tests and atomic commit.
- **[Player fills world with drops]** -> compatible merging plus explicit 512-drop limit; never silent despawn.
- **[Generic bundle is hard to identify]** -> item-category tint, visible count accent and exact focus title; defer bespoke models rather than misrepresent them.
- **[Dropping the only knife blocks progress]** -> allow it because recovery is immediate and physical; ensure the drop cannot become unreachable or silently vanish.
- **[Drops steal focus from intended targets]** -> range/distance/tie policy plus ordinary dense-target playtests.

## Migration Plan

1. Record pack actions, group/wearable/save totals, focus and world visual baselines.
2. Add strict world-drop state, atomic drop/pickup APIs and portable serialization/invariant tests.
3. Add grounded visual/focus/pickup lifecycle and active-window/build-cover integration.
4. Add menu Drop/stepper actions and hotbar/chest/paper-doll eligibility.
5. Exercise every item category, quantities, garments, merge/limit, blocked placement, capacity, save/chunk/reload and full-loop routes.
6. Build one immutable Shipping candidate and retain the selected work-animation build as rollback until explicit promotion.

