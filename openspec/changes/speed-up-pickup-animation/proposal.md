# Proposal

## Why

The generic ground pickup motion used for stones, branches, forage, and weeding lasts 1.6 seconds and lingers too long for a high-frequency action. The transaction is already immediate, so the presentation should read clearly and recover much sooner without changing gathering rules.

## What Changes

- Retiming the existing project-authored `AN_Heroine_Gather` from 1.6 seconds to a target 0.9-1.0 seconds, with a short anticipation, decisive reach/pick, minimal low hold, and prompt recover to the accepted idle endpoint.
- Preserve the current planted feet, restrained pelvis dip, reachable arm solve, unchanged skeleton/bind/scale/nonanimated bones, zero root motion, zero gameplay notifies, and existing appearance compatibility.
- Apply the faster clip consistently to every current caller: successful non-tree/non-sapling resource harvesting (including stones, branches, berries, roots, flowers, and reeds) and successful weeding.
- Keep gathering transactions, rewards, renewal, capacity/range/tool checks, sounds, time, save state, and cancellation/coalescing authority unchanged and immediate.
- Preserve movement/menu/planning/failure/load/appearance cancellation and prevent repeated pickup input from queuing delayed gestures.
- Verify ordinary side, three-quarter, and gameplay-camera timing so the faster clip does not pop, snap through the ground, or look like an implausible twitch.

The current Blender source, verifier, import path, action evaluator, ordinary gathering routes, and local heroine skeleton are sufficient; no external animation, asset, package, account, download, or license is required.

The smallest useful in-game result picks up one stone with a complete readable motion in no more than one second. The first playable demonstration gathers representative ground resources in succession, weeds a plot, interrupts a pickup by moving/opening the menu, and shows responsive recovery with every authoritative result applied once. Full acceptance covers all callers, body/hair/garment combinations, rapid input, cancellation, save/replay, ordinary keyboard/mouse and controller routes, and immutable Shipping replay.

Deferred scope includes unique animations by resource height/type, target-aware hand IK, object-in-hand visuals, crouch locomotion, crop-harvest animation not currently dispatched, new pickup sound/VFX, or changing gather interaction speed/range/yield.

## Capabilities

### New Capabilities

- `responsive-pickup-animation`: Sub-one-second readable pickup/weeding presentation across current gather callers while preserving transaction authority and cancellation.

### Modified Capabilities

None.

## Impact

- `Scripts/Characters/build_gathering.py`, verifier, source FBX/contract, import mapping, `AN_Heroine_Gather`, and `docs/character-pipeline.md`: retimed authored clip and exact provenance.
- `HomesteadAnimInstance` and native/ordinary gathering tests: blend thresholds, completion/cancel/coalescing timing, shared caller matrix, and visible responsiveness.
- Simulation, rewards, save schema, item/resource IDs, interaction ranges, controller mappings, and current action sounds remain unchanged.

