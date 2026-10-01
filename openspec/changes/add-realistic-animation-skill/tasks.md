# Tasks

## 1. Research and skill

- [x] 1.1 Research joint degrees of freedom and range of motion (spine by region, shoulder complex, elbow, forearm, wrist, thumb, fingers, hip, knee, ankle, toes), coupling, balance, motion quality, grips and contact, and map them to the MetaHuman skeleton
  - Status: six research threads (upper limb, hand and wrist, spine, lower limb, motion and balance, skeleton map). There are 104 source files with URLs and verbatim quotes, and six findings files under `E:\CopilotScratch\ce241dd6-2c0b-47ea-a402-ec9fe5dc3572\anatomy-research\`. The AAOS and AMA primary tables weren't retrievable, so their values come through clinical secondaries alongside CDC/Soucie data.
- [x] 1.2 Write `.github/skills/realistic-animation/SKILL.md` and reference it from `homestead-animation-layer` and `blender-assets`

## 2. Checker

- [x] 2.1 `joint_limits.py`: absolute and neutral-relative angles, bands, regional totals, coupling, speed, ground, slides, balance, summary. `Tests/JointLimitsTests.py` (18 synthetic-pose tests) passes
- [x] 2.2 `rig_authoring.Session.bake(..., events, contacts)` logs the anatomy summary; `anim_audit.run()` writes a whole-cast report
- [x] 2.3 In an Unreal slot, calibrate on the real skeleton: confirm that `neutral_pose()` loads the reference pose, check the twist signs with a deliberately rolled hand, a turned head and an internally rotated hip, and confirm the clavicle's parent bone. Adjust `twist_left_sign` or the measures if they disagree
  - Status: 2026-10-01. `neutral_pose()` loads the reference pose (64 bones, metacarpals present). The checker's wrist readings match the convention-free angle between the forearm and the hand (for example, GroundStrike frame 15 reads 62° raw and 69° from the checker; ActiveIdle frame 0 is clean). Deep hip flexion in the kneels read as about 170° of abduction, because the coronal projection degenerates; fixed by reading each angle in its own plane only near that plane.

## 3. Audit and fixes

- [x] 3.1 Run `anim_audit.run()` on every heroine clip (the kneels, weed pull, scythe mow, knife cut, chop, till, watering, locomotion, idle and sprint), and list the violations by severity
  - Status: 2026-10-01, every second frame (`E:\CopilotScratch\ce241dd6-2c0b-47ea-a402-ec9fe5dc3572\anim-audit\latest-audit.md`).
    - Clean: ActiveIdle, CraftHands.
    - Errors, worst first: HoeTill, AxeFell, WaterRefined and GroundStrike bend the right hand 130-180° off the forearm in the baked clips. The scythe mow has ulnar deviation to 53° and the left elbow at 153°. Eat has 69° of ulnar deviation. PailPour and MacheteHack have wrist pops over 2500 deg/s.
    - To refine in the checker: the kneels' foot "slides" look like low steps read as planted.
    - The GASP locomotion clips weren't found at the expected path.
- [ ] 3.2 Fix the clear violations in the recipes, re-bake, verify each in PIE at gameplay speed, and report what changed. Highest severity first (the Orchestrator, 2026-10-01): the HoeTill, AxeFell and WaterRefined wrist breaks, checked in the Animation Inspector before and after; then the scythe's carry and recovery transitions (the blade dips 30-40 cm into the ground), before capping MaxTipUp

## 4. Animation Inspector

- [x] 4.1 C++ `HomesteadAnimInspector` in the Character Lab. It plays a lab action at a fixed 1/30 s step, records bone and prop transforms per frame to JSON, and captures front, side, top and three-quarter views through scene captures. The lab gains weed pull, mow, pickaxe and billhook actions
  - Status: source only (`HomesteadAnimInspector.h/.cpp`, lab actions Weeds, Mow, Pickaxe, AxeStrike, Billhook, and LabHold for the estate tools). Not compiled yet: lanes were held to source during the build window. Compile and first run are in the next Unreal slot.
- [x] 4.2 `Scripts/anim_inspector_sheet.py`: overlays (a skeleton coloured by `joint_limits` status, contacts and penetration, prop clearance, the CoM over the support), contact sheets per view, a key-frame sheet, a GIF and `index.md` of flagged frames, with images sized for review
  - Status: `Tests/AnimInspectorSheetTests.py` passes on a synthetic recording. Prop bounds are drawn, but clearance distance isn't measured yet.
- [x] 4.3 `Scripts\Inspect-Animation.ps1 -Clip -Every -Views` (hidden, `-unattended`, TEMP on E:, its own process only) and an `editor_mcp` `animinspect` verb
- [ ] 4.4 Use it on the weed pull, scythe mow and kneel gather, and document it in the skill under "Seeing every frame"
