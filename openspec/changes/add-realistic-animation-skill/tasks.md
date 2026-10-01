# Tasks

## 1. Research and skill

- [x] 1.1 Research joint degrees of freedom and range of motion (spine by region, shoulder complex, elbow, forearm, wrist, thumb, fingers, hip, knee, ankle, toes), coupling, balance, motion quality, grips and contact, and map them to the MetaHuman skeleton
  - Status: six research threads (upper limb, hand and wrist, spine, lower limb, motion and balance, skeleton map). There are 104 source files with URLs and verbatim quotes, and six findings files under `E:\CopilotScratch\ce241dd6-2c0b-47ea-a402-ec9fe5dc3572\anatomy-research\`. The AAOS and AMA primary tables weren't retrievable, so their values come through clinical secondaries alongside CDC/Soucie data.
- [x] 1.2 Write `.github/skills/realistic-animation/SKILL.md` and reference it from `homestead-animation-layer` and `blender-assets`

## 2. Checker

- [x] 2.1 `joint_limits.py`: absolute and neutral-relative angles, bands, regional totals, coupling, speed, ground, slides, balance, summary. `Tests/JointLimitsTests.py` (18 synthetic-pose tests) passes
- [x] 2.2 `rig_authoring.Session.bake(..., events, contacts)` logs the anatomy summary; `anim_audit.run()` writes a whole-cast report
- [ ] 2.3 In an Unreal slot, calibrate on the real skeleton: confirm that `neutral_pose()` loads the reference pose, check the twist signs with a deliberately rolled hand, a turned head and an internally rotated hip, and confirm the clavicle's parent bone. Adjust `twist_left_sign` or the measures if they disagree

## 3. Audit and fixes

- [ ] 3.1 Run `anim_audit.run()` on every heroine clip (the kneels, weed pull, scythe mow, knife cut, chop, till, watering, locomotion, idle and sprint), and list the violations by severity
- [ ] 3.2 Fix the clear violations in the recipes, re-bake, verify each in PIE at gameplay speed, and report what changed

## 4. Animation Inspector

- [ ] 4.1 C++ `HomesteadAnimInspector` in the Character Lab. It plays a lab action at a fixed 1/30 s step, records bone and prop transforms per frame to JSON, and captures front, side, top and three-quarter views through scene captures. The lab gains weed pull, mow, pickaxe and billhook actions
- [ ] 4.2 `Scripts/anim_inspector_sheet.py`: overlays (a skeleton coloured by `joint_limits` status, contacts and penetration, prop clearance, the CoM over the support), contact sheets per view, a key-frame sheet, a GIF and `index.md` of flagged frames, with images sized for review
- [ ] 4.3 `Scripts\Inspect-Animation.ps1 -Clip -Every -Views` (hidden, `-unattended`, TEMP on E:, its own process only) and an `editor_mcp` `animinspect` verb
- [ ] 4.4 Use it on the weed pull, scythe mow and kneel gather, and document it in the skill under "Seeing every frame"
