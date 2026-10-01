# Proposal

## Why

Jenny (2026-09-30 22:09) wants the heroine's animations to be "extremely realistic": arms, wrists and
fingers must never bend at unnatural angles or do anything without a basis in physical reality. Clips
are hand-keyed in Python against the MetaHuman Control Rig, and so far each was checked only by eye in
a few PIE captures. That has let through:

- wrists cranked past a comfortable range;
- forearm roll faked on the hand;
- limbs through the body;
- poses balanced over nothing.

She also asked (22:12) for an Animation Inspector, the counterpart to the UI Gallery. It would let
an agent see every frame of a clip from several sides, with the problems marked.

## What Changes

- **Research.** Anatomy research on degrees of freedom and range of motion per joint for an adult
  woman, plus coupling, balance, motion quality, grips and contact. Every number has a cited source,
  with notes and verbatim quotes under `E:\CopilotScratch\<session>\anatomy-research\`.
- **The `realistic-animation` skill** (`.github/skills/realistic-animation/SKILL.md`):
  - design rules;
  - a per-joint ROM table with comfortable and extreme bands for the MetaHuman bones;
  - coupling rules, joint speeds, grips per tool and posture specs for Homestead's actions;
  - failure patterns with fixes;
  - a review checklist with front, side and top views.

  `homestead-animation-layer` and `blender-assets` reference it.
- **A joint-limit checker** (`Content/Python/homestead_agent/joint_limits.py`) in the recipe pipeline.
  It reads component-space bone transforms per frame and measures anatomical angles: absolute ones
  from positions in body frames, and relative ones against the reference pose. It flags frames outside
  the bands, coupling violations, fast joints, ground penetration, foot slides and the centre of mass
  outside the support. Every `Session.bake` logs its summary, and synthetic-pose tests pin it down.
- **A whole-cast audit** (`anim_audit.py`) of every heroine clip, with violations listed by severity.
  Clear violations are fixed in later Unreal slots, following Jenny's realism preference.
- **The Animation Inspector.** It steps a clip deterministically on the real MetaHuman with props and
  captures:
  - several views (front, both sides, top, three-quarter);
  - overlays: ROM-coloured skeleton, contacts, prop clearance, and the CoM over the support;
  - per-frame PNGs, contact sheets, a GIF and an `index.md` of flagged frames.

  It runs hidden and unattended through `Scripts\Inspect-Animation.ps1` and an `editor_mcp` verb.

## Impact

- New: the skill, `joint_limits.py`, `anim_audit.py`, `Tests/JointLimitsTests.py`, and later the inspector
  (C++ in the Character Lab, a Python sheet builder, a launch script).
- Changed: `rig_authoring.Session.bake` gains optional `events` and `contacts` arguments and logs the
  anatomy check. Existing recipes bake unchanged.
- No gameplay, save or simulation changes. Clip fixes found by the audit land as ordinary recipe re-keys
  and bakes, each verified in PIE.
