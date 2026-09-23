# Proposal

## Why

Carried items can currently be eaten, equipped, split, merged, reordered, or stored, but cannot be put back into the world. Jenny needs a safe physical Drop action that frees pack capacity without deleting possessions or creating save/reload duplication.

## What Changes

- Add `Drop...` to valid carried item stacks and unequipped carried wearables in the inventory details/actions surface.
- Reuse the explicit quantity stepper for stack drops. One authoritative transaction removes the confirmed amount from the selected carried group and creates one stable world-drop record at a validated grounded point just in front of the heroine.
- Do not permit direct dropping from chests or equipped body slots. Chest items must first move to the pack; clothing must be unequipped before Drop appears.
- Represent a dropped stack or garment as a small original grounded pickup bundle/marker with its item icon/count and focused name, rather than deleting it, simulating physics, or pretending it is a natural resource patch.
- Let the player focus and collect a world drop through ordinary interaction. Pickup transfers the whole world-drop stack only when pack capacity allows; otherwise the drop remains unchanged with a clear rejection.
- Preserve wearable identity/dye when an unequipped garment is dropped and recovered.
- Persist world drops with stable IDs, exact item/quantity or wearable identity, position, and world ownership across save/load and active-region churn. Opening menus, canceling quantity, failed placement, failed pickup, or reloading MUST NOT duplicate or lose items.
- Merge newly dropped ordinary item quantities into a nearby compatible player-created drop where safe, while keeping unique wearables separate. Enforce a bounded persistent-drop limit with explicit rejection instead of silent despawn.
- Coordinate with the compact inventory, chest-storage, hotbar, contextual pickup-toast, camera-safe foliage, and save-version plans so dropped items stay visible, focusable, nonblocking, and truthful.

The smallest useful in-game result selects a carried Branch stack, chooses `Drop...`, confirms two, and shows one grounded `Branch x2` pickup while the pack loses exactly two. The first playable demonstration saves/reloads, collects it once, drops and recovers a dyed garment without identity loss, and verifies a selected hotbar tool becomes unavailable while dropped and returns when collected. Full acceptance covers every current item category, quantities, capacity rejection, blocked placement, multiple drops/merge/limit, pointer/keyboard/controller actions, save/chunk lifecycle, 720p/4K focus feedback, and immutable Shipping replay.

Deferred scope includes throwing items, physics/rolling, partial pickup, automatic vacuum pickup, decay/despawn, theft/NPC ownership, dropped containers, placing decorative item models, drag-outside-inventory gestures, death drops, multiplayer ownership, and dropping directly from storage/equipment.

## Capabilities

### New Capabilities

- `droppable-inventory-items`: Atomic carried-item/garment drop, persistent world representation, safe whole-stack pickup, limits, and lifecycle.

### Modified Capabilities

None.

## Impact

- `Simulation/HomesteadSimulation.cpp/.h`: persistent world-drop records, stable IDs, `DropGroup`/`DropWearable`/`PickUpDrop` transactions, validation, merge/limit, serialization, and portable tests.
- Save version/current test profile: new bounded world-drop section; existing test saves may reset under the disposable-save policy with explicit disclosure.
- `HomesteadController` focus/update/interact and `HomesteadWorld` visuals: world-drop targeting, grounded visuals, pickup feedback, chunk/window refresh, and no collision/navigation.
- `SHomesteadMenu` and compact inventory: `Drop...` action, quantity stepper, carried/equipped/chest eligibility, stale-revision handling, and hotbar refresh.
- Existing original icons/material primitives and Unreal facilities are sufficient; no external asset, download, package, account, purchase, or license is required.

