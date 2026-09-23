# Proposal

## Why

Homestead's work transactions are functional, but chopping/felling, tilling,
and watering still read as short generic gestures with weak contact and timing.
These actions are repeated throughout the core loop, so better motion is the
next high-impact presentation improvement after the heroine and wardrobe work.

## What Changes

- Upgrade the existing contextual hatchet motion into readable mature-tree and
  sapling chopping/felling presentation with anticipation, impact, follow-through,
  grounded feet, and tool/trunk alignment.
- Add a dedicated digging-stick tilling motion aligned to the selected ground
  cell rather than dispatching a static transaction.
- Improve watering into a deliberate lift, aim, pour, and recover with the
  existing watering can and readable plot-directed contact.
- Preserve existing immediate authoritative transactions, rewards, costs,
  interaction ranges, controller mappings, persistence, and cancellation rules;
  animation notifies or tool collision MUST NOT grant gameplay outcomes.
- Reuse the existing heroine skeleton, analytical authoring helpers, action
  evaluator, tool props, import scripts, and tests. Author only the motion and
  targeting gaps demonstrated in ordinary gameplay.
- Add synchronized sound/particles only if existing project-owned facilities
  can provide restrained feedback without introducing an audio/VFX framework.
- Keep weeding/cookfire animation polish as a later extension unless the first
  three actions pass and their reuse is clearly bounded.

## Capabilities

### New Capabilities

- `homestead-work-animations`: Responsive, grounded, target-readable chopping,
  tilling, and watering presentation that remains subordinate to simulation
  authority.

### Modified Capabilities

None.

## Impact

The round affects the project-authored heroine animation scripts and FBXs,
animation import mappings, `HomesteadAnimInstance`, contextual tool
presentation, controller action dispatch, focused animation lifecycle tests,
ordinary visual-playtest routes, documentation, and a fresh Shipping candidate.
It reuses already admitted local content and Unreal animation facilities; no
external assets, downloads, accounts, purchases, plugins, or new licenses are
required.

The smallest useful delivery is one mature-tree hatchet action whose ordinary
gameplay arc visibly approaches the trunk while the real felling transaction
occurs exactly once. Full-round acceptance additionally requires tilling and
watering, interruption/menu/load safety, supported body/garment combinations,
controller responsiveness, save/replay regression, cadence evidence, and
actual close plus gameplay-camera motion review. Exact finger contact, root
motion, procedural full-body IK, cloth/hair physics, combat, and final VFX/audio
remain deferred.
