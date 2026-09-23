# Tasks

## 1. First playable chopping improvement

- [x] 1.1 Record current ordinary mature-tree/sapling chop motion, tool path,
  target distance and cancellation evidence; verify the reproduced contact,
  timing, clipping and camera defects before changing source.
- [x] 1.2 Author and verify a planted-foot anticipation/swing/impact/recover
  clip on the retained skeleton, preserving bind, scale, nonanimated bones,
  disabled root motion and zero gameplay notifies through source/FBX checks.
- [x] 1.3 Integrate target-directed chopping through the existing one-action
  evaluator and hatchet prop; verify successful mature/sapling transactions
  start once, rejected actions do not animate, rapid input coalesces, movement
  cancels cleanly and pawn/camera/feet remain stable.
- [x] 1.4 Build one playable chopping candidate and inspect close plus ordinary
  gameplay-camera arcs across representative bodies/hair/garments; verify real
  felling reward, permanent clearing, save/reload and cadence before continuing.

## 2. Dedicated tilling motion

- [x] 2.1 Add an original correctly scaled digging-stick prop using existing
  admitted project materials; verify hand attachment, no collision/overlap/nav,
  bounds and hide/cancel lifecycle.
- [x] 2.2 Author and verify a dedicated downward till/recover clip distinct from
  gathering/chopping, with grounded feet and representative cell-height contact
  through source/interchange checks.
- [ ] 2.3 Route only successful `Till` transactions to target-directed tilling;
  verify one plot commit, missing-tool/occupied/out-of-range rejection, rapid
  input arbitration, menu/load/failure cancellation and no orphaned prop.
- [ ] 2.4 Exercise actual cleared-site tilling through controller input across
  representative appearance/equipment combinations; inspect close/ordinary
  views and verify plots, action costs, persistence and cadence remain exact.

Tasks 2.1 and 2.2 passed in `work-animation-till-water-editor-03`. The original
procedural digging stick uses the admitted field material, one scale-one
section, collision/overlap/navigation disabled, hand attachment and bounded
35-60cm runtime radius; missing-tool rejection and natural recovery leave it
hidden. `AN_Heroine_Till` is a fresh 1.7s lift/drive/contact/settle/recover clip
with 53 retained bones, maximum bind error0, idle endpoint error0.0006381,
maximum scale error6.56e-7, root/foot travel0cm, right-wrist travel59.30cm and
zero notifies. Fresh import
`/Game/Trials/HomesteadWork_20260923_01/Animations/AN_Heroine_Till` reuses the
heroine skeleton, reports duration1.70000005s, root motion disabled and zero
notifies. The same native route passed one target-directed authoritative Till
and recovery; the broader 2.3 rejection/cancellation matrix remains open.

## 3. Refined watering contact

- [x] 3.1 Measure the incumbent watering arc/can path against representative
  plots and author only the demonstrated lift/aim/pour/recover correction;
  verify skeleton, scale, grounded feet, can tilt and no root motion/notifies.

Task 3.1 source verification passed for fresh
`Assets/Characters/Heroine/RefinedWatering/AN_Heroine_WaterRefined.fbx`.
The incumbent contract is a generic forward 2.2s lift with a 32-degree can
tilt and explicitly no plot contact; inspected native evidence
`work-animation-till-water-editor-08/watering-pour.png` shows the can held
beside the heroine rather than aimed toward the selected plot. The corrected
2.0s clip has explicit lift/aim/44-degree pour/recover phases, 53 retained
bones, maximum bind error0, idle endpoint error0.0006418, maximum scale
error6.56e-7, root/foot travel0cm, right-wrist travel32.84cm and zero notifies.
Runtime selected-plot targeting, can lifecycle and contact review remain tasks
3.2/3.3 and are not inferred from this analytical flat-ground check.
- [ ] 3.2 Pass the successful selected-plot target into presentation and keep
  the existing can bound/hidden lifecycle; verify one water cost/moisture change,
  full/dry/missing/out-of-range rejection, interruption and no queued replay.
- [ ] 3.3 Exercise watering on root and berry plots with controller input across
  representative bodies/hair/garments; inspect side/three-quarter/gameplay
  views and verify garden, save/reload and action arbitration regressions pass.

## 4. Cross-action acceptance and delivery

- [ ] 4.1 Run focused portable/source checks and native lifecycle tests for
  chop/till/water success, rejection, targeting, prop contracts, cancellation,
  rapid input and missing-presentation errors; verify gameplay authority remains
  independent from clips, notifies and props.
- [ ] 4.2 Compile Editor and package a fresh immutable Shipping candidate under
  the serialized engine slot; verify exact source/package identity and preserve
  the selected hairstyle/wardrobe build as rollback.
- [ ] 4.3 Run ordinary controller chop/till/water routes, full-loop, input,
  hotkey/save and separate-process replay checks with Lit/Lighting-on/
  ShaderComplexity-off evidence; verify no reward, world, UI or persistence
  regression and report instrumented cadence limits honestly.
- [ ] 4.4 Inspect bounded close and gameplay-camera motion frames/video for all
  three actions, fix demonstrated contact/clipping/readability defects, update
  animation/design/playtest/provenance docs, and promote only a genuinely
  improved candidate with receipt/proof/`Start-Preview -ValidateOnly`.

## Chopping baseline

`work-animation-baseline-clear-editor-03` is the current ordinary mapped
sapling baseline: real supply gathering, native hatchet crafting, walking,
camera orbit and X clear with no injected state/time/teleport. The action lasts
2.0s; the hatchet is visible from phase0.3424 through1.3984, uses a fixed60°
tilt, and the right wrist travels55.02cm. Actor travel is0cm, left/right toe
drift is0.164/0.181cm and view yaw remains17.108°. The tool stays81.30-91.84cm
from the selected target in XY.

The inspected sheet demonstrates the defect: simulation removes the sapling
immediately, focus switches to a nearby mature tree, and the clip lifts then
pushes the hatchet forward/down through empty space without trunk-directed
anticipation or readable impact. The first selected-Shipping and controlled
baseline attempts are retained failed because their old setup pinned stale
focus/one-step crafting. The repaired diagnostic uses stable focusable sapling
selection and current native recipe activation; task1.1 is complete evidence,
not a motion fix.

The fresh `AN_Heroine_Chop` source clip is1.8s/109 samples with explicit
anticipation0-0.34s, swing0.34-0.80s, impact0.80s, follow-through to1.12s and
recover through1.80s. Blender FBX verification reports53 unchanged bones,
maximum bind error0, idle endpoint matrix error0.00044305, maximum scale error
7.15e-7, root travel0cm, foot drift0cm, right-wrist travel66.33cm and zero
notifies. The target-facing runtime layer and ordinary contact remain separate
task1.3 evidence.

Task 1.3 passed in `work-animation-chop-clearing-focused-editor-13`. The
focused native route resolves generated saplings by stable key before every
approach, validates the selected target yaw, one authoritative Clear start,
fixed scale/collision-free hand prop, zero pawn/camera/foot displacement,
rejected/no-target/capacity paths, rapid request coalescing, natural recovery,
movement/book/planning/appearance/load cancellation, all 18 supported
body/hair/outfit combinations, save/reload persistence, and depleted-sapling
clearing. The route also exposed and fixed prepared wardrobe application not
cancelling an active hand action. The optional combined camera-lifecycle route
still has a separately preserved stale woodland-reservation expectation and is
not counted as chopping acceptance.

Task 1.4 passed with immutable Shipping candidate
`work-animation-chop-08-shipping` (executable SHA-256
`F17D1E8A07A3D05BBE6FD5C252C5AB0CC9ECC95CA8BB4368BFE401F4E90501CD`).
`work-animation-chop-08-shipping-clearing` passed sapling and mature-tree
transactions, permanent generated keys, reward/state equality, rejection,
coalescing, movement/menu/planning/appearance/load cancellation, save/reload,
and three representative production wardrobe combinations at mean32.85fps,
p95 17.73ms. Editor route `work-animation-chop-clearing-focused-editor-17`
retains all 18 body/hair/outfit combinations and the capacity/depleted cases.
Separate Development cooked-process write/reload/normal-preview consumption
passed in `work-animation-chop-04-hotkey`; the direct Shipping hotkey helper is
not admitted by Shipping QA and produced no result.

The ordinary mapped Shipping capture
`work-animation-chop-08-shipping-ordinary-clear` gathered real supplies,
crafted the hatchet, walked to and cleared a real sapling, then recovered with
the tool hidden. Its 8Hz action sample records one start, 0cm actor travel, 0cm
left/right toe drift, fixed view yaw, a 119.63cm right-hand path and nine
visible hatchet samples. Inspected anticipation/impact/follow-through/recovery
frames show a materially clearer overhead preparation and descending arc than
the incumbent generic forward/down lift. Immediate authoritative removal still
means the target is absent during the gesture, so exact contact and terrain IK
are not claimed.
