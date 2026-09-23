# Tasks

## 1. First playable chopping improvement

- [x] 1.1 Record current ordinary mature-tree/sapling chop motion, tool path,
  target distance and cancellation evidence; verify the reproduced contact,
  timing, clipping and camera defects before changing source.
- [ ] 1.2 Author and verify a planted-foot anticipation/swing/impact/recover
  clip on the retained skeleton, preserving bind, scale, nonanimated bones,
  disabled root motion and zero gameplay notifies through source/FBX checks.
- [ ] 1.3 Integrate target-directed chopping through the existing one-action
  evaluator and hatchet prop; verify successful mature/sapling transactions
  start once, rejected actions do not animate, rapid input coalesces, movement
  cancels cleanly and pawn/camera/feet remain stable.
- [ ] 1.4 Build one playable chopping candidate and inspect close plus ordinary
  gameplay-camera arcs across representative bodies/hair/garments; verify real
  felling reward, permanent clearing, save/reload and cadence before continuing.

## 2. Dedicated tilling motion

- [ ] 2.1 Add an original correctly scaled digging-stick prop using existing
  admitted project materials; verify hand attachment, no collision/overlap/nav,
  bounds and hide/cancel lifecycle.
- [ ] 2.2 Author and verify a dedicated downward till/recover clip distinct from
  gathering/chopping, with grounded feet and representative cell-height contact
  through source/interchange checks.
- [ ] 2.3 Route only successful `Till` transactions to target-directed tilling;
  verify one plot commit, missing-tool/occupied/out-of-range rejection, rapid
  input arbitration, menu/load/failure cancellation and no orphaned prop.
- [ ] 2.4 Exercise actual cleared-site tilling through controller input across
  representative appearance/equipment combinations; inspect close/ordinary
  views and verify plots, action costs, persistence and cadence remain exact.

## 3. Refined watering contact

- [ ] 3.1 Measure the incumbent watering arc/can path against representative
  plots and author only the demonstrated lift/aim/pour/recover correction;
  verify skeleton, scale, grounded feet, can tilt and no root motion/notifies.
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
