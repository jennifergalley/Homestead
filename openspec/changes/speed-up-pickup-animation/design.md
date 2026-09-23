# Design

## Context

See `proposal.md` and `specs/responsive-pickup-animation/spec.md`. `AN_Heroine_Gather` is a project-authored 1.6-second, 60 FPS reach/pick/recover clip with a 12 cm pelvis dip, planted feet, reachable analytical arm solve, idle endpoints, disabled root motion, and zero notifies. One explicit-time action evaluator blends in over 0.12 seconds and out over 0.16 seconds.

Controller requests it only after successful non-tree/non-sapling `Harvest` and successful `Weed`; it therefore covers stones, branches, berries, roots, flowers, reeds, and weeds. Movement/menu/planning/failure/load/appearance changes cancel it; repeated presentation requests coalesce. Simulation commits immediately.

## Goals / Non-Goals

**Goals:**

- Reduce frequent pickup presentation to a 0.9-1.0 second target without losing a readable contact beat.
- Preserve the existing skeleton, pose character, evaluator authority, caller set, cancellation, and verification/import pipeline.
- Review timing in ordinary keyboard/mouse and controller play rather than animation preview alone.

**Non-Goals:**

- New caller dispatch, unique clips per resource, target IK, held pickup objects, transaction/range/yield changes, or new sound/VFX.

## Decisions

### 1. Retime the authored keys rather than speed-multiply at runtime

Rebuild the source clip at 0.95 seconds with explicit phase targets: about 0.12 second anticipation, 0.28 second descent/reach, contact near 0.42-0.50, at most 0.10 second low hold, then decisive recovery. Preserve 60 FPS sampling, idle endpoints, planted feet, pelvis depth, reach validation, bind/scale/nonanimated bones, and zero notifies.

**Alternative considered:** set runtime play rate to roughly 1.68x. Rejected because the existing low hold/recovery proportions would feel mechanically sped up and source/provenance would no longer describe the visible timing.

### 2. Tune blending proportionally to the short clip

Reduce blend-in/out from 0.12/0.16 to approximately 0.08/0.10 after ordinary review so the visible action is not mostly crossfade. Completion derives from actual clip length, not a duplicated hard-coded duration. Cancellation may blend out faster but still cannot queue a replay.

### 3. Keep one shared gesture for its current callers

Do not add new dispatch. The faster imported asset automatically serves every existing gather/weeding request. Portable/source tests enumerate current callers so future changes cannot silently leave stones or weeds on old timing.

### 4. Preserve three-layer verification

Offline verification checks exact duration, contact/recovery phase, skeleton, endpoints, feet/root, reach, scale and notifies. Native lifecycle checks one start/success, rejected no-start, cancellation/coalescing and actual completion time. Ordinary evidence reviews stone/branch/forage/weeding across representative appearances at side, three-quarter, and gameplay views.

The animation source lane owns script/FBX/contract and no engine resources. Integration owns imported asset/evaluator/tests. Editor/cook/package remain serialized.

## Risks / Trade-offs

- **[Motion becomes a twitch]** -> Keep distinct anticipation/contact/recovery phases and reject sub-0.9 second candidates unless ordinary play remains readable.
- **[Feet or torso pop during crossfade]** -> Preserve identical idle endpoints and retune blend windows against the actual clip.
- **[Low contact no longer reaches stones]** -> Keep pelvis/reach depth while removing dwell; inspect representative terrain heights honestly.
- **[Rapid actions appear to miss presentation]** -> Preserve coalescing and prioritize responsive control over queued visual replays; document transaction/presentation distinction.

## Migration Plan

1. Measure current 1.6-second phase timing and ordinary stone/forage/weeding evidence.
2. Author/verify a fresh 0.95-second source candidate and import into a trial package.
3. Integrate the selected clip and proportional blend timing without changing caller dispatch.
4. Run lifecycle, rapid input, cancellation, appearance, save/full-loop, and ordinary visual acceptance.
5. Build one immutable Shipping candidate and retain the current selected build as rollback until explicit promotion.

