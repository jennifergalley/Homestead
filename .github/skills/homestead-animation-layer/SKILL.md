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

**Audit imported prop axes before posing a tool.** Blender FBX props mirror Y on Unreal import:
the report's blade-facing `-Y` is engine `+Y`. Read the imported prop and running-game grip before
authoring, then bake the intended engine-facing motion. Do not leave a wrong authoring axis in the
clip and compensate with a runtime half-turn: that can place the blade correctly while folding the
wrist wrong.

For a two-handed haft, do not take `index_01` → `pinky_01` as square across the fist: it slants
about 16° toward the fingers. Square it against wrist → `middle_01` before using it as the haft
line. If a gripping wrist folds, search the fist's free roll about that haft first, re-solving the
forearm-to-knuckle angle at each key, before changing the arm pose.

**Do not derive a held tool's working direction from knuckles.** That pins the hand to one roll.
Each recipe supplies a fixed component-space swing-plane normal; derive the edge as
`normal × haft`, then allow both fists to roll freely per key. Treat the reference hand's forearm
twist as anatomical—not arbitrary rig—space: neutral palms face the thighs with thumbs forward, so
a 180° palm-up-across-chest reading is genuine over-rotation.

For kneeling foot changes, key a travel arc rather than dragging the foot along the ground: lift at
about 10% of travel to 60% height, peak at 50%/100%, then descend at 90%/60%. Move one foot at a
time on the rise, and give a 39 cm lifted step more than eight frames.

**Treat a hand-bone-laid prop and finger direction as one constraint.** If gameplay lays a prop
along the fingers, do not author fingers directly up its axis: that can fold the wrist even if the
prop appears aligned. Define a constant finger lead over the prop axis in the recipe and apply the
equal, opposite lead in the runtime placement on the same beats (`pail_pour.LEAD` is the pattern).
For an action that changes a held-tool grip, use the action's blend weight to ease between carry and
work placement—not the clip phase. The clip's standing/end keys are authored for the work grip and
pose evaluation can lag phase by one frame.

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
