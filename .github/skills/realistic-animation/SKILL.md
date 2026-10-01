---
name: realistic-animation
description: Design and judge realistic heroine animations and poses in Homestead (SurvivalGame) against human anatomy - per-joint range of motion for the MetaHuman bones (comfortable and extreme bands), joint coupling, balance, joint speeds, grip mechanics, contact and clearance - plus the failure patterns to look for and a review checklist. Use before authoring or re-keying any Control Rig clip or pose (gathers, kneels, tool swings, carries, locomotion), when reviewing a baked clip or a PIE capture, and whenever an arm, wrist, finger, spine or leg looks bent wrong.
---

# Realistic animation for the heroine

Jenny's standing rule: when in doubt, choose the more realistic option and say what you changed.
This skill turns clinical anatomy and biomechanics into numbers you can key against and check.
It complements `homestead-animation-layer` (how a clip is authored, baked, loaded and timed). Use
this skill for whether the pose is physically believable.

Everything here is for an adult woman. Female norms run slightly looser than male ones: more elbow
hyperextension, forearm rotation, hip internal rotation, lumbar and cervical range. Don't borrow
male limits, and don't use child or hypermobile ranges unless a pose is marked stylised.

## 1. The ten design rules

1. **Big motion comes from big joints.** Power, arcs and reach come from the feet, hips, trunk and
   shoulders. The wrist and fingers finish the motion. An arm-only swing or a wrist-cranked arc is
   the most common way a clip reads fake.
2. **Never put a whole motion on one bone.** Spine bend, spine twist, neck turn and forearm roll are
   spread across a chain (section 3). One bone doing all of it reads as broken.
3. **Forearm roll lives in the forearm.** Pronation and supination happen at the radioulnar joints:
   the hand rides on the radius. Rolling `hand_*` (or the hand IK control) to fake it gives a
   candy-wrapper wrist. Key the wrist for flexion and deviation only.
4. **Twist the trunk from the pelvis and thorax, not the lumbar.** Lumbar twist is about 5° in
   total. A turn starts at the feet and pelvis, then the thoracic spine, then the neck and eyes.
5. **Power grips want a slightly extended wrist**, about 15–35° extension and 0–15° ulnar
   deviation. A clenched fist on a flexed wrist is weak and looks wrong (tenodesis).
6. **Tool swings follow the dart-thrower plane.** Wind up in radial extension and strike through
   ulnar flexion, about 27° off pure flexion and extension. The chain fires proximal to distal, each
   link peaking 1–3 frames after the one before, with the tool head peaking at or just before contact.
7. **Weight moves before limbs do.** Shift the centre of mass toward the support side 3–15 frames
   before a step, reach, pull or swing. Wind up 3–10 frames before a forceful action. Follow
   through and settle over 2–6 frames afterwards.
8. **Keep the centre of mass over the support.** In any held pose, the ground projection of the
   CoM stays inside the support polygon (feet, planted knees and shins, a planted hand), at least
   2 cm in from the edge.
9. **Planted contacts don't move.** A stance foot, kneeling knee or bracing hand drifts less than
   0.5 cm a frame and less than 2 cm in total. Feet roll heel, flat, heel-off, toe-off.
10. **Bodies have volume.** Limbs clear the body by at least 2 cm, a tool or pail by at least 5 cm,
    and a tool head clears the shins and feet by at least 10 cm. The exception is authored contact
    (a hand on the thigh, calf against thigh in a deep kneel).

## 2. Range of motion, per joint

Angles are degrees from anatomical neutral (standing, arms at the sides, palms forward).
- **Comfortable:** key freely and repeatedly; the working band.
- **Extreme:** possible but brief and intentional; it needs the coupled motion beside it, such as
  the scapula for overhead arms or calf-to-thigh contact for a deep kneel.
- **Beyond extreme:** wrong unless the pose is tagged stylised.

Values combine CDC/Soucie normative data, AAOS/Norkin-style clinical tables and task (ADL)
studies. Section 9 lists the sources; the research notes give the per-source numbers and how
disagreements were resolved.

### Spine, neck and head

| Bones | Motion | Comfortable | Extreme | Per-bone cap |
|---|---|---:|---:|---|
| `spine_01..05` total | forward bend | 35–55 | 70 (more only with hip flexion) | 18–20 pitch any bone |
| `spine_01..05` total | extension | 15–25 | 30 | 12–15 any bone |
| `spine_01..05` total | side bend, each way | 15–30 | 35 | 12 roll any bone |
| `spine_01..05` total | axial twist, each way | 20–35 | 40–60 thoracic | 15 yaw any bone |
| `spine_01`+`spine_02` (lumbar) | axial twist | 0–5 | 8–12 | 5 each |
| `neck_01`+`neck_02`+`head` | yaw | 45 (casual), 60–75 (alert) | 85 | 35–40 any one bone |
| `neck_01`+`neck_02`+`head` | flexion (chin down) | 30–45 | 60 | `head` 15–20 |
| `neck_01`+`neck_02`+`head` | extension | 35–55 | 70 | `head` 12 |
| `neck_01`+`neck_02`+`head` | side bend | 20–35 | 45 | — |

Suggested split (inference, so the chain curves rather than hinges):
- Forward bend: `spine_01` 8–12, `spine_02` 10–14, `spine_03` 8–12, `spine_04` 6–10, `spine_05` 4–8.
- Twist: `spine_01` 0–2, `spine_02` 1–3, `spine_03` 5–8, `spine_04` 7–10, `spine_05` 8–12.
- Neck yaw: `neck_01` 10–20, `neck_02` 20–35, `head` 10–20.
- Neck flexion: `neck_01` 12–20, `neck_02` 10–15, `head` 5–10.

### Shoulder complex (`clavicle_*`, `upperarm_*`)

| Motion | Comfortable | Extreme | Beyond |
|---|---:|---:|---:|
| Flexion / forward elevation | 0–150 | 150–180 (needs scapula and clavicle) | >185 |
| Abduction | 0–150 (ADL ~130) | 150–180 (needs scapula and external rotation) | >185 |
| Extension (arm behind) | 0–45 | 45–60 | >65 |
| Horizontal adduction (across chest) | 0–105 | 105–130 (needs protraction) | >135 |
| Horizontal abduction | 0–60 | 60–90 | >90 |
| Internal rotation | 0–70 | 70–100 | >100 |
| External rotation | 0–90 | 90–110 | >110 |
| Clavicle elevation / protraction / axial roll | small | 40 / 35 / 20–40 | — |

**Scapulohumeral rhythm.**
- Below 30° of elevation, the arm moves mostly alone.
- From 30–90°, the scapula adds roughly 1° of upward rotation for every 2° of arm.
- Above 90°, the scapula supplies about half of each extra degree. Full overhead means about 60°
  of scapular upward rotation, with clavicle elevation and a posterior roll.
- A raised arm on a frozen clavicle is a failure, even if the humerus angle is legal.
- High abduction carries some external rotation.

### Elbow and forearm (`lowerarm_*`, twist bones)

| Motion | Comfortable | Extreme | Beyond |
|---|---:|---:|---:|
| Elbow flexion | 0–130 | 130–150 (watch forearm-to-arm/chest clipping) | >155 |
| Elbow hyperextension | 0–5 | 5–10 | >10 (Beighton hypermobility) |
| Carrying angle (valgus, arm straight) | 10–15 target | 5–10 or 15–20 | <5, >20 |
| Pronation | 0–50 casual, to 80 | 80–85 | >90 |
| Supination | 0–50 casual, to 80 | 80–90 | >90 |

`upperarm_twist_*` and `lowerarm_twist_*` are deformation followers driven by RigLogic in
`ABP_Body_PostProcess`. Don't limit or key them as joints. Measure pronation from the hand's
across vector (`index_01` − `pinky_01`) about the forearm axis, against the elbow plane.

### Wrist (`hand_*` relative to `lowerarm_*`)

| Motion | Comfortable | Extreme | Beyond |
|---|---:|---:|---:|
| Flexion | 0–40 | 40–80 | >90 (or >70 while gripping hard) |
| Extension | 0–40 | 40–70 (90 weight-bearing push) | >90 |
| Radial deviation | 0–15 | 15–25 | >30 |
| Ulnar deviation | 0–25 | 25–40 | >45 |
| Power grip target | 15–35 extension, 0–15 ulnar | — | flexed >20 with a tight fist |

Deviation shrinks away from neutral flexion:
- within ±15° of neutral, the full deviation band;
- between 15° and 45°, 50–70% of it;
- beyond 45°, radial ≤10–15 and ulnar ≤15–20.

### Fingers (`*_metacarpal_*` → `*_01` MCP → `*_02` PIP → `*_03` DIP)

| Joint | Comfortable flex | Extreme flex | Hyperextension limit |
|---|---:|---:|---:|
| MCP (index–pinky) | 0–75 | 85–100 | 0–15 relaxed; warn >30; fail >45 |
| PIP | 0–90 | 100–120 | 0; warn >10; fail >15 |
| DIP | 0–55 | 70–90 | 0–5; warn >10; fail >20 |
| MCP spread (abduction) | 0–15 | 20–30 | spread fades to 0 as MCP flex passes 45 |

- **Thumb (`thumb_01..03`):**
  - CMC flexion 0–35 comfortable, 45–70 extreme; abduction 20–50, 60–70 extreme;
  - MCP flexion 0–35, 50–55 extreme, fail >60;
  - IP flexion 0–60, 85–90 extreme, fail >95;
  - thumb hyperextension: warn >10, fail >20–30 under load.
- **Opposition** is CMC abduction, flexion and rotation together. The thumb tip reaches the
  fingertips in order; it never crawls through the palm.

### Hip, knee, ankle, toes

| Joint | Motion | Comfortable | Extreme | Beyond |
|---|---|---:|---:|---:|
| Hip (`thigh_*`) | flexion, knee bent | 0–120 | 120–135 | >135 |
| Hip | flexion, knee straight | 0–90 | 90–110 | >110 |
| Hip | extension | 0–15 | 15–25 | >25 |
| Hip | abduction / adduction | 0–40 / 0–25 | 55 / 35 | beyond |
| Hip | internal / external rotation (hip flexed) | 0–40 / 0–60 | 55 / 75 | beyond |
| Knee (`calf_*`) | flexion | 0–140 | 140–156 (with calf-thigh contact) | >160 |
| Knee | hyperextension | 0–5 | 5–10 | >10 loaded |
| Knee | axial (tibial) rotation | ≤10 when flexion <30; IR 25 / ER 35 at 90 | IR 30 / ER 40 | beyond |
| Knee | varus / valgus | ~0 (knee over 2nd–3rd toe) | — | any loaded collapse |
| Ankle (`foot_*`) | dorsiflexion, knee bent / straight | 0–20 / 0–12 | 30 / 15 | beyond |
| Ankle | plantarflexion | 0–50 | 50–60 | >60 |
| Subtalar (loaded) | inversion / eversion | 0–15 / 0–8 | 25 / 15 | beyond |
| Big toe MTP (`ball_*`, `bigtoe_01`) | extension (toes tucked, push-off) | 0–70 | 70–90 | >90 |
| Big toe MTP | flexion | 0–30 | 30–45 | >45 |

### Joint speed (30 fps)

The check uses the frame-to-frame rotation delta. A single frame above the hard pop is a snap
unless it's the contact frame of a strike and follows through.

| Joint | Everyday warn (deg/s) | Tool swing warn (deg/s) | Hard pop (deg/frame) |
|---|---:|---:|---:|
| Head | 120 | 180 | 8 |
| Trunk (thorax, lumbar) | 120 | 240–300 | 12 |
| Shoulder | 350–450 | 800–900 | 40 |
| Elbow | 400–500 | 900–1000 | 50 |
| Wrist and forearm | 500–600 | 1000–1200 | 60 |
| Hip | 450–600 | 700 | 33 |
| Knee | 500–700 | 900 | 43 |
| Ankle and foot | 450–650 | 900 | 40 |

Also flag a velocity jump between adjacent frames of more than 300 deg/s (large joints) or
600 deg/s (distal joints).

## 3. Coupling rules (things that move together)

- **Scapulohumeral rhythm:** see section 2. The arm never rises above about 90° alone.
- **Lumbopelvic rhythm:** a forward bend starts lumbar-heavy and ends hip-heavy. The ratio of
  lumbar to pelvic contribution is about 2:1 early, 1:1 mid-bend and 0.4:1 late. A deep bend with
  straight hips is wrong.
- **Head, eyes and trunk:**
  - Eyes take small shifts.
  - From about 20–25°, the head joins.
  - Past about 75° of head and neck yaw with less than 15° of torso turn, the torso and pelvis
    should be turning too.
- **Wrist tenodesis:** extending the wrist curls the fingers and flexing it opens them. A wrist
  flexed more than 20° with average PIP flexion over 70° is a conflict.
- **PIP and DIP:**
  - DIP ≈ 0.6–0.75 × PIP (±15°).
  - The DIP doesn't flex past 30° while the PIP is under 20°.
  - The DIP doesn't extend while the PIP is past 95°.
- **Finger cascade:** curl increases from index to little finger. On a handle, the ring and little
  fingers wrap more than the index. Never curl all four the same.
- **Knee rotation is flexion-coupled:** near-straight knees barely rotate (screw-home). Stance
  width comes from the hips and feet, never from a sideways knee.
- **Ankle dorsiflexion depends on the knee:** a straight knee allows only about 12°. Past that, lift
  the heel or shift the pelvis.
- **Toe-tucked kneeling bends the big-toe MTP** (55–90° extension), not the ankle.
- **Arm-leg opposition in gait:** the right arm swings with the left leg. The shoulders and pelvis
  counter-rotate mid-stride.

## 4. Grips and tool postures

| Tool | Grip | Wrist | Fingers and thumb |
|---|---|---|---|
| Axe, hatchet, pickaxe | Cylindrical power grip, handle diagonal across the palm from the hypothenar to the index MCP | 15–35 extension, 0–15 ulnar; radial extension on the wind-up, ulnar flexion through the strike | MCP and PIP flexed, DIP engaged; ring and little fingers most wrapped; thumb wraps over |
| Scythe nibs | Relaxed cylindrical or oblique power grip | Neutral to 25 extension; dart-thrower arcs through the sweep, never locked at end-range deviation | MCP 40–70, PIP 50–90, DIP 20–60; little and ring fingers lead; the thumb never passes through the nib |
| Hoe, rake | Two-handed cylindrical or oblique grip | Radial extension on the lift, ulnar flexion on the pull; no fixed deviation over 30 under force | Relaxed power grip; the arc comes from shoulders, elbows, trunk and forearm, not the wrist |
| Billhook, knife | Small cylindrical power grip (key pinch only for fine control) | Neutral to 20 extension, slight ulnar | The index may lie a little straighter; never hyperextended |
| Pail bail | Hook grip: load in the curled PIP and DIP, thumb only stabilises | Neutral to slight extension; never flexed (the load would open the fingers) | The bail sits in the finger hook, not pinched at the fingertips |
| Weeds and stems (pull) | Power grip, then a pull with preload | Neutral to slight extension | Grip, set the shoulder and scapula, lean the body away from the pull, release, recoil and settle |

## 5. Posture specs for common Homestead actions

- **Kneeling (gather, pouch, plant, weed pull, harvest, pail fill):**
  - Hips 60–100°, knees 145–156° in a full kneel. A full kneel shows calf-to-thigh and heel
    compression or contact; otherwise, stop the knee at 130–140°.
  - Ankles plantarflexed 30–50°, or toes tucked with the MTP extended.
  - Getting down: step and widen, hip hinge, one knee down, then the other. Getting up is the
    reverse: a half-kneel, a hand on the thigh or ground, the CoM over the front foot, then extend.
    Never pop straight up.
- **Half-kneel:** front hip and knee 80–110°, front ankle dorsiflexed 10–25°. The front knee tracks
  over the toes. Support is a tripod: front foot, rear knee or shin, rear toes or instep.
- **Deep squat, heels down:** hips 95–120°+, knees 130–150°, ankle dorsiflexion 15–25°. The CoM
  sits over the midfoot. If the heels lift, the load moves to the balls of the feet and the
  forefoot.
- **Tool stance (axe, scythe, hoe):** widen through stride length, toe-out and pelvis yaw, not hip
  abduction past 40°. Rotate the hips and pelvis to aim rather than twisting a planted knee.
  Repeated strokes shift weight between the feet.
- **Lifting and carrying:** keep the load close; hinge at the hip and bend the knees; no deep twist
  under load. A carried pail hangs at least 5 cm outside the thigh, the carrying shoulder drops,
  and the trunk leans slightly the other way.
- **Walking:**
  - Heel contact with the hip flexed 20–30° and the knee nearly straight.
  - The knee loads to 15–20°, is about 40° at toe-off and peaks around 70° mid-swing.
  - The ankle plantarflexes about 25° at toe-off; the big toe extends 50–65° before toe-off.
  - Stance is about 60% of the cycle. Running has a flight phase and stance under 50%.

## 6. Failure patterns and fixes

| You see | Why it's wrong | Fix |
|---|---|---|
| Wrist hyper-bent (flexed or extended past 70) while holding something | A power grip wants a slightly extended wrist; a flexed grip is about 60% weaker | Move the arc into the elbow, shoulder, forearm and tool angle; return the wrist to 15–35 extension |
| Forearm roll on the wrist (hand spun, forearm still) | Pronation happens at the radioulnar joints; the skin candy-wrappers | Roll through the forearm and twist chain; keep the hand's roll relative to the forearm small |
| Elbow hyperextension past 10 | Hypermobility territory; the mesh collapses at the olecranon | Keep 0–5 with a 10–15° carrying angle; soften with slight flexion under load |
| Finger hyperextension (PIP or DIP bent backward) | PIP normal extension is 0 | Clamp PIP to 0–5 and DIP to 0–10; open the hand at the MCP instead |
| Identical sausage-finger curls | Real hands cascade from index to little | Add a per-finger offset; keep PIP and DIP coupled |
| Arm overhead on a frozen clavicle | No scapulohumeral rhythm; the arm looks jammed | Add scapular upward rotation, clavicle elevation and posterior roll, and humeral external rotation |
| One-bone spine or neck hinge | Real motion spreads across the chain | Redistribute per the section 2 splits |
| All the twist in the lumbar | Lumbar twist is about 5° | Move it to the pelvis and `spine_03..05` |
| Knee valgus, or a knee bending sideways | The knee is a hinge with flexion-coupled rotation | Knee over the 2nd–3rd toe; width from the hips and feet |
| Feet sliding in stance | No foot lock | Lock the contact while planted; move the pelvis over it; translate only in swing |
| Interpenetration (forearm in thigh, tool through shin, pail through leg) | No clearance envelope | Hold the clearances in design rule 10; adjust the arc, stance or carry side |
| CoM outside the base in a held pose | No balance shift | Move the pelvis, widen the stance or plant a hand or knee; keep a 2 cm margin |
| Limb lifts with no weight shift | No anticipatory adjustment | Shift the CoM 3–15 frames earlier toward the support |
| Arms-only tool swing | No kinetic chain | Hips and trunk lead; shoulder, elbow and wrist peaks staggered 1–3 frames; the tool head peaks last |
| Kneel-to-stand pops up vertically | No support transfer | Half-kneel, a hand on the thigh, CoM over the front foot, then extend |
| Single-frame snap | Keys too tight | Respect the hard-pop table; spread the motion with slow-in, slow-out and follow-through |

## 7. Review checklist for a clip

Run the checker first (section 8), then look. For each clip, at the key frames from its `FRAMES`
dict (contact beats, extremes, transitions) plus the first and last frame:

1. **Front view:**
   - knees over the toes, no valgus;
   - carrying angle visible on straight arms;
   - the shoulder line tilts only with a spine curve;
   - the hands' pronation reads through the forearm.
2. **Side view:**
   - spine curve distributed, not hinged; lumbopelvic rhythm in bends;
   - wrist extension on power grips;
   - knee hyperextension under 5; ankle and toe angles legal;
   - CoM over the support.
3. **Top view:**
   - pelvis and thorax yaw split, little lumbar twist;
   - tool arc clear of the shins and feet;
   - the pail outside the thigh envelope;
   - arm-leg opposition in gait.
4. **Hands close-up:**
   - finger cascade, PIP and DIP coupling, no backward fingers;
   - the thumb wraps, not through the palm or handle;
   - the grip type matches section 4.
5. **Feet close-up:** contacts locked while planted; heel-to-toe roll; toes don't pass 90 MTP.
6. **Timing (scrub at gameplay speed):**
   - weight shift before the action, wind-up before force;
   - proximal-to-distal peaks;
   - follow-through settles;
   - no single-frame snaps;
   - contact beats match the gameplay events.
7. **Clearance:** no limb, prop or tool intersects the body or the ground at any frame (scrub, don't
   sample).
8. **Report** each issue with the frame, the bone, the measured angle and band, and the fix you
   applied or propose. Fix clear issues using Jenny's realism preference, and say what changed.

## 8. Automated checks

- **Status:** the checker (`Content/Python/homestead_agent/joint_limits.py`, tests in
  `Tests/JointLimitsTests.py`) and the clip audit (`anim_audit.py`) are on branch
  `jennifergalley-realistic-animation`. The in-engine calibration (twist signs on the real skeleton) and
  the first full audit happen in an Unreal slot. The Animation Inspector (multi-view frame captures with
  overlays) is planned under the same OpenSpec change, `add-realistic-animation-skill`.
- **Every bake is checked.** `rig_authoring.Session.bake(anim, events=FRAMES, contacts=[...])` runs the
  checker on the new clip and logs `[anatomy AN_...]` lines: a count by kind and severity, then the worst
  issue per joint with its frame and `FRAMES` key. Pass the recipe's `FRAMES` and its strike or impact
  keys so the report names beats and fast contact frames only warn. A recipe's `report()` can add the
  same lines with `joint_limits.report(anim, events=FRAMES, contacts=[...])`.
- **Whole-cast audit:** `from homestead_agent import anim_audit; anim_audit.run()` checks every heroine
  clip and writes `audit.md` under `E:\CopilotScratch\anim-audit\<stamp>\`.
- **How it measures** (pure Python, so it runs anywhere on `{bone: (location, quaternion)}` component-space
  poses):
  - Absolute, from positions in body frames: shoulder and hip flexion and abduction (chest and pelvis
    frames) and elbow and knee flexion. These read 0 at anatomical neutral whatever the bind pose is.
  - Relative to the skeleton's reference pose (`neutral_pose()`): the spine, neck and head per bone,
    the clavicle, wrist, fingers, thumb, ankle and toes as swing along anatomical directions (toward the
    front, the palm, up), and the rotations as twist about the segment: shoulder and hip rotation,
    forearm pronation, tibial rotation and subtalar inversion. Forearm roll is read as the hand's twist
    relative to the upper arm, wherever the rig keys it.
  - Regional totals: lumbar twist, spine flexion, side bend and twist, and neck turn and flexion.
  - Coupling: arm overhead on a still clavicle, a fist on a flexed wrist, wrist deviation off neutral
    flexion, an isolated DIP, a spread fist, hip flexion with a straight knee, tibial rotation near
    extension, dorsiflexion with a straight knee, head turned with the torso still, and a deep bend with
    straight hips.
  - Motion and contact: per-joint speed against the section 2 table (warnings only within two frames
    of a listed contact), ground penetration, planted foot or knee slides (over 0.5 cm a frame) and the
    centre of mass outside the support in held frames (de Leva female segment fractions).
  - It never reads raw local Euler channels, because MetaHuman's local axes differ per bone and side.
- **Seeing every frame:** the Animation Inspector (planned) plays a clip at a fixed 1/30 s step in
  the Character Lab with props. It captures front, side, top and three-quarter views, with:
  - the skeleton coloured by ROM status;
  - contact and penetration markers;
  - the CoM over the support polygon.

  It writes per-frame PNGs, contact sheets and an `index.md` of flagged frames under
  `E:\CopilotScratch\<session>\anim-inspector\<clip>\<stamp>\`. Until it lands, review with PIE
  captures from `unreal-editor-mcp` at the clip's key frames.
- **Coordinate frames:**
  - The authoring component frame is +Y forward, +X her left, +Z up (`rig_authoring.py`).
  - The UE world frame is X forward, Y right, Z up.
  - Blender FBX bones import with their own orientation, so judge angles in UE component space,
    never from Blender armature axes.

## 9. Sources

The research notes, with verbatim quotes per source and the reasoning for each band, are under
`E:\CopilotScratch\ce241dd6-2c0b-47ea-a402-ec9fe5dc3572\anatomy-research\` (`findings\A_upper_limb.md`,
`B_hand_wrist.md`, `C_spine.md`, `D_lower_limb.md`, `E_motion_balance.md`, `F_skeleton_map.md`).
Main sources:

- Normative ROM:
  - [CDC/Soucie normal joint ROM](https://archive.cdc.gov/www_cdc_gov/ncbddd/jointrom/index.html)
  - [AAOS/Norkin chart](https://goniometer.io/range-of-motion)
  - [VA 38 CFR 4.71a spine ROM](https://www.ecfr.gov/api/versioner/v1/full/2024-09-30/title-38.xml?part=4)
- Shoulder:
  - [Scibek & Carcia scapulohumeral rhythm](https://pmc.ncbi.nlm.nih.gov/articles/PMC3377910/)
  - [OrthoFixar shoulder phases](https://orthofixar.com/special-test/shoulder-range-of-motion/)
  - [Kenhub SC joint](https://www.kenhub.com/en/library/anatomy/sternoclavicular-joint)
  - [Gates upper-limb ADL ROM](https://pmc.ncbi.nlm.nih.gov/articles/PMC4690598/)
- Elbow and forearm:
  - [carrying angle study](https://ijos.co.in/archive/volume/9/issue/4/article/17174)
  - [Beighton score](https://www.ehlers-danlos.com/resource/the-beighton-scoring-system/)
  - [Kenhub DRUJ](https://www.kenhub.com/en/library/anatomy/distal-radioulnar-joint)
  - [forearm twist rigging](https://www.3dfiggins.com/writeups/forearmTwist/)
- Wrist and hand:
  - [Ryu functional wrist ROM](https://d.docksci.com/functional-ranges-of-motion-of-the-wrist-joint_5efc1f0d097c47cd2c8b457c.html)
  - [Palmer](https://researchconnect.suny.edu/en/publications/functional-wrist-motion-a-biomechanical-study/)
  - [Li wrist coupling](https://experts.arizona.edu/en/publications/coupling-between-wrist-flexion-extension-and-radial-ulnar-deviati/)
  - [dart-thrower's motion](https://pmc.ncbi.nlm.nih.gov/articles/PMC5837914/)
  - [wrist position and grip strength](https://pmc.ncbi.nlm.nih.gov/articles/PMC3111126/)
  - [tenodesis](https://journalmsr.com/practical-applications-of-tenodesis-in-hand-surgery/)
  - [OUHSC hand biomechanics](https://www.ouhsc.edu/bserdac/dthompso/web/namics/hand.htm)
  - [Cornell grips](https://www.ergo.human.cornell.edu/studentdownloads/DEA3250pdfs/grips.pdf)
  - [GRASP taxonomy](https://www.eng.yale.edu/grablab/pubs/Feix_THMS2016.pdf)
  - [Hand Surgery Resource ROM](https://www.handsurgeryresource.net/rangeofmotion-active)
- Spine:
  - [Zebris cervical ROM](https://pmc.ncbi.nlm.nih.gov/articles/PMC12124152/)
  - [upper cervical rotation by age/sex](https://pmc.ncbi.nlm.nih.gov/articles/PMC10155033/)
  - [thoracic sagittal ROM](https://pmc.ncbi.nlm.nih.gov/articles/PMC3940794/)
  - [lumbar 6-DoF ROM](https://www.frontiersin.org/journals/surgery/articles/10.3389/fsurg.2022.1002133/full)
  - [trunk rotation, pelvis vs spine](https://pmc.ncbi.nlm.nih.gov/articles/PMC5645112/)
  - [spinopelvic rhythm](https://pmc.ncbi.nlm.nih.gov/articles/PMC6698516/)
  - [eye-head coordination](https://pmc.ncbi.nlm.nih.gov/articles/PMC2605952/)
  - [trunk motion in gait](https://pmc.ncbi.nlm.nih.gov/articles/PMC2843703/)
- Lower limb:
  - [OrthoFixar hip](https://orthofixar.com/special-test/hip-range-of-motion-and-biomechanics/)
  - [knee](https://orthofixar.com/special-test/knee-range-of-motion-test/)
  - [ankle](https://orthofixar.com/special-test/ankle-range-of-motion/)
  - [screw-home mechanism](https://pmc.ncbi.nlm.nih.gov/articles/PMC6584384/)
  - [high-flexion kneeling and squatting](https://pmc.ncbi.nlm.nih.gov/articles/PMC5842511/)
  - [squat depth vs hip and ankle ROM](https://pmc.ncbi.nlm.nih.gov/articles/PMC4415844/)
  - [gender, Q-angle and hip rotation](https://pmc.ncbi.nlm.nih.gov/articles/PMC5300795/)
  - [first MTP ROM](https://podiatry-anatomy-app.qut.edu.au/content/joints/1st-mtp-joint-rom.html)
  - [gait kinematics](https://podiapaedia.org/wiki/biomechanics/gait/angular-kinematics-of-gait/)
  - [sprint kinematics](https://www.frontiersin.org/journals/sports-and-active-living/articles/10.3389/fspor.2019.00037/full)
- Motion and balance:
  - [Hof dynamic stability (XCoM)](https://research.rug.nl/en/publications/the-condition-for-dynamic-stability/)
  - [de Leva segment data](https://exrx.net/Kinesiology/Segments)
  - [ANSUR II](https://ph.health.mil/topics/workplacehealth/ergo/Pages/Anthropometric-Database.aspx)
  - [anticipatory postural adjustments](https://www.frontiersin.org/journals/human-neuroscience/articles/10.3389/fnhum.2021.709780/full)
  - [Flash & Hogan minimum jerk](https://web.archive.org/web/20231202000156/https://www.jneurosci.org/content/5/7/1688)
  - [hammering wrist kinematics](https://pmc.ncbi.nlm.nih.gov/articles/PMC2901240/)
  - [kinetic chain](https://pmc.ncbi.nlm.nih.gov/articles/PMC7174497/)
  - [trunk angular velocity in ADLs](https://pmc.ncbi.nlm.nih.gov/articles/PMC4127473/)
  - [arm swing and gait stability](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0218644)
  - [foot locking](https://theorangeduck.com/page/inverse-kinematics-foot-locking)
  - [follow-through and overlap](https://www.animationmentor.com/blog/follow-through-and-overlapping-action-the-12-basic-principles-of-animation/)
  - [manual handling](https://www.ccohs.ca/oshanswers/ergonomics/mmh/hlth_haz.html)
- Rig:
  - [UE coordinate system](https://dev.epicgames.com/documentation/unreal-engine/coordinate-system-and-spaces-in-unreal-engine?lang=en-US)
  - [Blender FBX](https://docs.blender.org/manual/en/4.5/addons/import_export/scene_fbx.html)
  - [MetaHuman DNA](https://github.com/EpicGames/MetaHuman-DNA-Calibration/blob/main/docs/dna.md)

**Caveats:**
- AAOS and AMA primary tables weren't retrievable directly; the values come through clinical
  secondaries alongside CDC data.
- The joint-speed ceilings are conservative visual-pop limits; everyday velocity norms are sparse.
- The squat and kneel pelvis numbers and the spine-bone splits are inference.
- de Leva's data comes from young athletic women.

Treat the bands as animation judgement, not diagnosis.
