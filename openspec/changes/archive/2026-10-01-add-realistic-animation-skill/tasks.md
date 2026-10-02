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
  - Status: 2026-10-01 slot (props-0930b).
    - Scythe: transitions re-baked with the blade at least 4 cm up on every laid frame; MaxTipUp capped at 0.2 rad (0ee02e34).
    - HoeTill: the recipe authored the blade on -Y while the imported hoe has it on +Y, and the game spun the hoe half a turn in her fist. Fixed in hoe_till.py and the runtime roll is removed: forearm-to-knuckle bend 115-157 to 31-59 degrees, the 180-degree wrist and forearm errors and every speed error cleared (ad7dabb7). Remaining: ulnar deviation (right 73, left 65) and left forearm supination 108.
    - AxeFell: each fist rolls on the haft (knob constant 105 degrees, which the game undoes for the edge; right per key), rig_authoring unwinds Euler keys (the haft had flipped 100-150 degrees mid-swing), and the game squares the fists' haft axes. Speed errors 28 to 16, ROM errors 342 to 279, right wrist 179 and right forearm 176 to 108, and the bit meets the authored strike within 3-5 cm instead of about 10 (24b74f21 on the slot branch). Remaining: left wrist 99 degrees of flexion and right forearm supination 108; a per-frame knob roll would need an anim curve.
    - 2026-10-01 second slot (jennifergalley-props-wrists-1001):
      - Watering: WaterRefined is only the no-MetaHuman fallback; in game she plays PailPour. Its upright holds pointed her fingers up the pail's axis (wrists 146 degrees back). Her fingers now lead the axis by 100 degrees and UpdateWaterPail turns them back on the same beats: inspector wrist pops 9 -> 0, ROM errors 208 -> 99.
      - GroundStrike: fists rolled on the haft (knob 90, right per key): the 175-177 degree wrist flexions, 80 degree ulnar deviation and wrist pops are gone.
      - AxeFell: knob roll 90 shared with the strike, left elbow pole 15 cm forward and up: left wrist flexion 99 -> 77, no wrist errors left.
      - HoeTill: left hand rolled (ulnar 65 -> 9); the right hand takes a working grip (the game turns the hoe 45 degrees in her fist as the clip blends in, the clip solved for that grip on every key): ulnar 74 -> 23, ROM errors 364 -> 211.
      - Remaining: forearm supination around 95-112 on the fell, strike and hoe (the swings' geometry); WaterRefined itself is untouched as it never plays with the MetaHuman.
    - 2026-10-01 third slot (jennifergalley-props-anatomy-1001): the game lays the axe/pick edge from the recipe's swing plane, so both fists roll per key (chosen against joint_limits); AxeFell and HoeTill have no wrist or forearm errors left, GroundStrike's forearm 115 -> 103; the scythe's left fist rolls on its nib (left-arm cost 1539 -> 80, blade unchanged); the kneels step instead of dragging the right foot (slides 126-306 -> 51-104 cm/s; the pouch's 8-frame rise unchanged) and the sticks cradle turns palm-in (supination 180 -> 86).
      - Remaining: GroundStrike's 1-2 frame elbow fold at the top of each downswing (needs a mid-swing hand key); KneelPullWeeds' toss (Jenny asked to keep it as is); the scythe's right wrist on the carry-to-address transition; ankle dorsiflexion and head extension on the kneels.

## 4. Animation Inspector

- [x] 4.1 C++ `HomesteadAnimInspector` in the Character Lab. It plays a lab action at a fixed 1/30 s step, records bone and prop transforms per frame to JSON, and captures front, side, top and three-quarter views through scene captures. The lab gains weed pull, mow, pickaxe and billhook actions
  - Status: compiled and run on Till and Fell (2026-10-01). `-Views` with commas needs FParse's bShouldStopOnSeparator=false. Prop transforms are in actor space and bone poses in component space; `mesh` gives the transform between them.
- [x] 4.2 `Scripts/anim_inspector_sheet.py`: overlays (a skeleton coloured by `joint_limits` status, contacts and penetration, prop clearance, the CoM over the support), contact sheets per view, a key-frame sheet, a GIF and `index.md` of flagged frames, with images sized for review
  - Status: `Tests/AnimInspectorSheetTests.py` passes on a synthetic recording. Prop bounds are drawn, but clearance distance isn't measured yet.
- [x] 4.3 `Scripts\Inspect-Animation.ps1 -Clip -Every -Views` (hidden, `-unattended`, TEMP on E:, its own process only) and an `editor_mcp` `animinspect` verb
- [ ] 4.4 Use it on the weed pull, scythe mow and kneel gather, and document it in the skill under "Seeing every frame"
