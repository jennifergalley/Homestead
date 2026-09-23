# Design

## Context

See `proposal.md` and
`specs/homestead-work-animations/spec.md`. Homestead already has project-authored
gather, water, and clear animation-only FBXs on the retained 53-bone skeleton;
an explicit-time single hand-action evaluator; original watering-can and
hatchet props; cancellation on movement/menu/load/failure; and focused
watering/clearing lifecycle tests. Transactions commit immediately in
`Simulation`; no notify or prop collision owns rewards.

The concrete gaps are visible motion and targeting: mature trees reuse a
sapling-scale generic swing after disappearance, tilling has no dedicated
gesture or digging-stick presentation, and watering pours generically forward
rather than reading against the chosen plot. All source and engine facilities
are project-authored or standard Unreal functionality. No external animation,
plugin, account, download, or license is required.

## Goals / Non-Goals

**Goals:**

- Improve the three most repeated tool actions without changing their gameplay
  transactions or controller timing.
- Reuse the existing skeleton, analytical Blender helpers, animation importer,
  action graph, props, focus targets, and test routes.
- Make trunk/cell/plot direction readable at ordinary camera distances while
  retaining planted-foot, no-root-motion behavior.
- Deliver frequent bounded slices—chop first, then till, then water—before
  full-round cross-action acceptance.

**Non-Goals:**

- Combat, damage, multi-hit tree health, root-motion movement, full-body
  procedural IK, motion matching, cloth/hair physics, exact finger placement,
  tool collision authority, or animation-driven rewards.
- New resource costs, interaction radii, action durations, stamina/needs rules,
  tree fall physics, terrain deformation, water simulation, or save migration.
- A generalized sound/VFX framework. Restrained existing-project cues may be
  added only after the motion works and can be tested independently.

## Decisions

### Extend the existing single-action presentation path

Add explicit Chop, Till, and Water action kinds to the current evaluator while
retaining one active hand-action layer, explicit phase time, blend-in/out, and
common cancellation. Till receives its own animation and digging-stick prop;
watering and chopping replace/refine their existing clips and reuse existing
props.

**Alternative:** animation montage/notify authority or multiple overlay layers.
Rejected because the current evaluator already prevents stacking/replay and
moving authority into notifies would make gameplay frame/asset dependent.

### Keep transactions immediate and target data presentation-only

After a successful transaction, Controller passes a transient target position
and action kind to Character presentation. Character computes only a bounded
facing/aim offset for the clip/prop; it never moves the pawn, expands reach, or
changes simulation. Invalid/missing target data falls back with an explicit
diagnostic and cannot undo or duplicate a committed transaction.

**Alternative:** delay transaction until impact. Rejected because interruption,
missing assets, low frame rate, and save/load would make gameplay outcomes
ambiguous or exploitable.

### Author planted-foot clips on the admitted rig

Reuse the analytical arm/leg helpers and existing idle endpoints. Chopping uses
anticipation, a stronger two-hand-compatible torso/arm arc where the retained
rig permits it, impact, and follow-through; tilling bends at hips/knees with
downward digging-stick travel; watering refines lift/aim/tilt/recover. Root
motion stays disabled and bind/nonanimated bones remain unchanged.

**Alternative:** acquire motion capture. Rejected for the first round because
the existing actions are short, target-specific, already have verified local
authoring/import paths, and retargeting would add risk before demonstrating a
quality gap the project tools cannot solve.

### Validate motion in three layers

1. Offline source/interchange checks: skeleton, scale, duration, non-static
   animated bones, planted feet, tool-hand/target paths, no root motion/notifies.
2. Native lifecycle checks: one transaction/start, target direction, prop
   scale/attachment, cancel/coalesce/recovery, menus/load/failure, no movement.
3. Shipping ordinary review: controller-driven real tree/plot actions across
   representative body/hair/garment combinations, close plus gameplay camera,
   save/replay and cadence.

The engine/build slot remains serialized in the main integration session.
Source authoring may run independently only on uniquely owned scripts/assets
and must hand off immutable hashes before import.

## Risks / Trade-offs

- **[Immediate world mutation makes impact look late]** -> design anticipation
  short enough that the visible impact follows promptly, and retain honest
  documentation that presentation follows committed authority.
- **[One clip cannot contact all trunk/terrain heights]** -> use bounded target
  orientation and representative-height checks; disclose exact contact/IK limits.
- **[Two-handed chopping conflicts with the existing one-hand prop]** -> prefer
  a readable dominant-hand arc unless the retained rig/prop supports the second
  hand without gross clipping.
- **[Tilling bends clip garments/hair]** -> test base, tunic/apron, footwear,
  waves, bob, and ponytail; reduce pose extremity rather than hiding geometry.
- **[Rapid input produces prop/action leakage]** -> retain the one-action
  coalescing/cancellation path and assert no queued replay/orphaned prop.
- **[New clip import triggers broad rebuilds]** -> import into fresh trial
  packages first, reuse compatible cook/shader caches, and stage immutable
  candidates without overwriting the selected build.

## Migration Plan

1. Keep the selected hairstyle/wardrobe build and current action assets as
   rollback.
2. Author and verify the chopping slice in fresh source/import namespaces;
   integrate and play it before starting tilling.
3. Add tilling and refined watering as separate checkpoints using the same
   target/presentation contract.
4. Run full native and Shipping cross-action acceptance, then promote only if
   ordinary motion is visibly better and all gameplay/persistence gates pass.
5. Roll back by restoring prior action mappings/assets and preview selection;
   no personal saves or simulation schema are changed.
