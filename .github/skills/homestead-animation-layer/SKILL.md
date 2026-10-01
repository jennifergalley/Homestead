---
name: homestead-animation-layer
description: How to add or change a heroine action animation in Homestead (SurvivalGame) end to end - author the clip with the MetaHuman Control Rig in Python, load it on AHomesteadCharacter, play it through UHomesteadAnimInstance (full-body action slot or an upper-body layer), and land the gameplay effect on the clip's contact beat. Use when adding a new tool swing, gather, eat/craft-style layer or any new action clip, or when re-timing an existing one.
---

# Adding an animation layer or action

There is no Animation Blueprint. `UHomesteadAnimInstance` builds its node graph in C++
(`FHomesteadAnimProxy` in `HomesteadAnimInstance.cpp`):

```
Idle/Walk/Sprint blends ─► ActionBlend (B = the one full-body action evaluator, "Gather")
   ─► CraftLayer (spine_02 up) ─► EatLayer (clavicle_r, neck_01)
   ─► foot placement IK ─► hand grips (FHandGrip) ─► output
```

- **Full-body actions** (gather, water, clear, till, machete, fell...) share one
  `FAnimNode_SequenceEvaluator` (`Gather`). `Request<Action>()` sets `Requested`; when she's standing
  still and no action is playing, the proxy swaps in the clip for `EHandAction::<Action>` and plays
  it once, easing in over 0.12 s and out over 0.16 s. Movement, the book and planning cancel it.
- **Upper-body layers** (craft, eat) are `FAnimNode_LayeredBoneBlend`s with bone branch filters, so
  she can walk while they play. Use a layer only when the action must work while moving.

## 1. Author the clip (Python, editor, not PIE)

Design the poses with the `realistic-animation` skill first: per-joint ranges for the MetaHuman bones,
coupling (scapula with a raised arm, wrist extension in a power grip, weight shift before the action),
grips per tool and the review checklist.

Follow "Author an animation with the MetaHuman Control Rig" in the `unreal-editor-mcp` skill.
Copy the closest module in `Content/Python/homestead_agent/` (`hoe_till.py` for a two-handed tool,
`machete_hack.py` for one hand, `kneel_gather.py` for kneeling, `eat_berry.py`/`craft_hands.py`
for layers). Keep in the module, as constants:

- `FRAMES` (named keys at 30 fps) and `EVENTS` (seconds of the gameplay beats, e.g. the blade
  biting), which C++ must match;
- the held prop's transform in the hand (`HELD`), read from the running game;
- a `report()` that prints the contact point against its target at each key.

It bakes `/Game/Characters/Heroine_MH/Animations/AN_HeroineMH_<Name>`. Pass the module's `FRAMES` and
its strike or impact keys to `s.bake(ANIM, events=FRAMES, contacts=[...])`: the bake runs the anatomical
joint-limit checker (`joint_limits.py`) and logs `[anatomy AN_...]` lines. Fix every error, and every
warning that isn't a deliberate brief extreme, before committing. Save the asset, clean up the
authoring actor and sequence, and commit the `.uasset` with the `.py`.

## 2. Load it on the character

In `AHomesteadCharacter::LoadMetaHumanStack` (`HomesteadCharacter.cpp`), load it with
`LoadMetaHumanAsset<UAnimSequence>(TEXT("Animations/AN_HeroineMH_<Name>"))`, check its skeleton
matches `MetaHumanBody`'s, and keep it in a `UPROPERTY()` member with a `Get<Name>Animation()`
accessor. Fall back gracefully (a log line, the action still works) when it's missing, as the hoe
does with `bHoeTill`.

## 3. Play it

- **Full-body action:** append a value to `EHandAction`, add `Request<Name>()` (sets `Requested`),
  add the clip to the `Gather.SetSequence(...)` choice, and add `Is<Name>()` / `<Name>Phase()` /
  `<Name>Starts()` accessors like the existing ones (tests and the controller use the start counter
  to know it really began). If the action holds a tool in both hands, extend `bTwoHanded` so the left
  `FHandGrip` closes on the haft.
- **Layer:** add an evaluator and a `FAnimNode_LayeredBoneBlend` between the existing layers in the
  proxy constructor, choose the branch bones, drive its time and blend weight in an
  `Update<Name>()` called from the proxy's update (see `UpdateEating`, `UpdateCrafting`).
- Node code that runs `*_AnyThread` must read only proxy members the game thread set.

These tables of per-action counters and accessors are repetitive; a table-driven rewrite is planned
between rounds (`openspec/changes/improve-code-health-between-rounds`). Until then, follow the
existing pattern exactly rather than inventing a second one.

## 4. Land the gameplay on the beat

The controller starts the animation, then commits the simulation command at the clip's contact
beat, not on the key press: it records a pending action and checks the anim's phase each tick
(`UpdatePendingSwing` / `LandOvergrowthSwing`, `UpdatePendingFell`, `UpdatePendingHack` in
`HomesteadController.cpp`). Put the beat in one named constant (seconds, matching the Python
`EVENTS`) next to the character's other timings (`GatherPouchTiming`, `HoeFirstChop`...), with a
comment naming the `.py` it comes from. If the action is cancelled before the beat, nothing
happens in the simulation.

## 5. Check it

- Anatomy: the bake's `[anatomy]` lines, or `anim_audit.run(['AN_HeroineMH_<Name>'])` for a report, then
  the `realistic-animation` review checklist (front, side and top at the key frames).
- Character lab: `-HomesteadCharacterLab` or `homestead.CharacterLab 1`, then `LabAction <Name>`
  if you add a lab hook (`HomesteadLab.cpp`); capture a burst and look for limbs through the body
  (`kneel_gather.clearance`) and sharp wrists.
- PIE on the Estate: do the real action on a real target, capture it, and confirm the effect
  (toast, pack, world change) lands with the visible contact.
- Native tests if the simulation side changed; editor module build.
