# Visual playtesting

Functional checks and visual playtests answer different questions. Passing
inventory, save, or interaction assertions does not show that a heroine looks
appealing or that walking feels natural. Do not select a replacement character
pipeline before inspecting the current game in motion.

## Repeatable observational route

For the separate opt-in45-minute stability exercise, see
`endurance-playtesting.md` and `Scripts\Test-Endurance.ps1`. Its sparse milestone
frames and actual sustained progression are distinct from the short pose route.
It uses a disclosed copied test homestead, fixed criteria and isolated graphics/saves.
The subsequent bounded renewal route is documented in `forage-renewal-playtesting.md`.
It tests actual24h/36h forage refresh via normal bedrest and save/load, with
same-node renderer/HUD frames; sleep advancement is not wall-clock endurance.

Build the editor target, then run:

```powershell
.\Scripts\Playtest-Visual.ps1 -EngineRoot 'E:\Program Files\UE_5.8'
python .\Scripts\Review-VisualPlaytest.py .\Saved\VisualPlaytests\<run-folder>
```

For ordinary mapped work-action review, add `-Clearing` for chopping or
`-Watering` for the combined till/plant/water route. Setup frames are sampled
sparsely, while the action phases are sampled at 8 Hz. These routes gather and
craft through gameplay, walk to generated targets and perform real
transactions; they do not inject inventory, plots, time or rewards. Review the
anticipation, drive/pour, follow-through and recovery frames individually.
Authoritative simulation removes a cleared tree immediately, so chopping
evidence does not claim exact trunk contact. Tilling and watering similarly do
not claim soil particles, terrain deformation or terrain-height IK.

Open the generated `review.html` locally to play, pause, scrub, or step through
the recording. It replays captured engine frames at their recorded timestamps;
it is a review artifact, not a browser version of the game.

The route closes the opening notes, idles, walks slowly and at full speed, turns,
stops, orbits the camera, inspects the face, and walks to an actual forage target
before gathering it. The picking segment settles, uses mapped camera orbit/
distance controls for a side view, then dwells through reach and recovery.
It sends ordinary mapped input events rather than calling
simulation actions directly. There are no teleports or time skips.

It runs offscreen with separate saves and does not commandeer the player's open
game or send mouse/keyboard input to other applications. It does not update the
packaged build or overwrite a player's session.

To observe a separate standalone candidate, pass `-Packaged -PackageDirectory
'Build\Releases\<candidate>'`. `-OutputDirectory` selects a fresh recording folder;
existing telemetry is rejected rather than overwritten. The script verifies
process exit and every frame's requested dimensions, not just file presence.

Shipping candidates require explicit `-ShippingQA` and an explicit, wholly
fresh `-OutputDirectory`. They use the actual Shipping executable, not a
Development fallback. Normal human launch remains QA-off. The native gate
requires exactly one test route, isolated graphics/UserDir and a fresh
`SmokeSave` sandbox before spawning test actors; invalid admission exits with
failure rather than starting a normal game. Injected weeding fixtures are not
admitted by this first Shipping gate.

The short Shipping route reuses `AuthoringLeafGuard` through
`Invoke-ShippingQA.ps1`: suspended root-only detached startup, existing empty
privacy-marker read sharing/inherited lifetime, zero allowed TCP/UDP endpoints,
100-second cooperative stop marker, 110-second owned-job fallback, and bounded
cleanup. These are test-case limits, not the autonomous run's duration.
An incomplete/cancelled route is failure. No console/debug/trace feature is
enabled. `qa-guard-result.json` records actual exit, finite sampling gaps and
unchanged marker release after observed death. It is not a sandbox or proof of
continuous network history. These controls need the live development run and
no competing authoring process; they do not affect an existing player window.
Ordinary captures now also record actual RHI/viewport/default-resolution policy
in `presentation-settings.txt`, without forcing screen percentage.

For the specifically admitted natural-play route, `Test-Endurance.ps1
-FreshWorld -ShippingQA` reuses the same lifetime/endpoint controls without a
synthetic supervisor timer; see `endurance-playtesting.md`. It does not change
the ordinary short-route limits. `Test-Game.ps1 -Clearing -CameraLifecycle
-ShippingQA -Packaged` selects the focused **controlled** clearing fixture:
mapped gather/craft with functional teleports, actual camera-channel canopy
queries, harvest removal, checkpoint restoration, permanent clear and save/load.
That fixture is not ordinary travel or natural sapling-regrowth evidence.

Ordinary tree-view captures also write `camera-foreground.json`: actual resource
component identities, spring-arm fix/unfixed position, matching sphere sweep and
sampled bounds/complex-collision rays. Bounds coverage is not visible pixel
coverage; inspect the original frames. The confirmed sapling correction changes
only its produce-cone camera query responses, not material, scale, Pawn collision
or navigation. Query components are destroyed/rebuilt with the existing produce
lifecycle rather than kept as separate invisible blockers.

The first integrated Shipping route is documented in
`research\environment-assets\clearing-playable.md`.

## Evidence and limits

- `Frames`: actual screenshots sampled at approximately 8 Hz.
- `telemetry.csv`: timestamps, actions, actor position/speed, facing, and toe
  positions in world space.
- `observations.txt`: actions and actual gathering outcome, including blocked
  approaches rather than silently moving past them.
- `motion-review.json`: limited stance/foot-speed diagnostics, not an authoritative
  foot-contact detector or a substitute for visible evidence.
- `gathering-sheet.png`: cropped/scaled before, reach, pick and recovery frames
  when gathering telemetry is present; full original frames remain in `Frames`.
- `review.html`: timestamped playback and frame stepping.

Screenshot readback can disturb pacing. This route is for pose, movement,
framing and interaction observation, not frame-rate benchmarking. Sampled
playback can look choppier than the game; do not call capture jitter a game bug.
Physical-controller comfort, continuous animation smoothness and listening
still benefit from a human playtest or denser video capture.

## Settings and wildflower delivery, 2026-09-23

`settings-wildflowers-03-shipping` adds a separate vertical Settings screen,
live 0-100% audio sliders, optional 5/10/20/30-minute autosaves, direct Save and
one-dialog Quit, and sparse decorative Flower Empodium groundcover. The ordinary
Shipping traversal validated exactly 368 expected flower instances in 50
collision-free HISM batches across 25 loaded chunks, then gathered a real berry
bush and contacted/retreated from a real tree through mapped controls. Reviewed
frames show small ground accents rather than resource-sized flower patches.

The final Shipping menu routes passed at 1280x720 and 3840x2160. The complete
homestead loop measured 46.30 mean FPS, p95 17.49 ms and p99 18.07 ms over
11,773 clean samples; the selected rollback measured p95 28.67 ms and p99
30.68 ms on its comparable full-loop route. Generated-world producer/consumer,
native save consumer, Save & Quit, save-failure retry, directional navigation,
and three-process valid/invalid preference routes also passed. These offscreen
checks do not replace human mouse/controller feel, listening, or final art taste.

## Camera-safe foliage delivery, 2026-09-23

`foliage-camera-05-shipping` separates leafy render surfaces from solid camera
collision and applies a bounded camera-to-hero dither corridor. Ordinary
Shipping routes at 720p, 1080p, and 4K retain woodland density away from the
corridor, keep the heroine and focused target readable, and preserve solid
mature-trunk contact. The focused sapling route verifies no camera sweep block
through harvest, reload, permanent clear, save, and reload.

The candidate full loop measured 46.31 mean FPS, p95 17.50 ms and p99 17.97 ms
over 11,779 samples, versus selected `settings-wildflowers-v14` at p95 17.49 ms
and p99 18.07 ms. Native 4K measured p95/p99 17.48/17.64 ms versus
17.46/17.63 ms. The difference is below the route's practical noise floor; no
full-screen foliage, trunk fading, dither trail, or cadence regression was
observed in reviewed frames.

## Tool hotbar delivery, 2026-09-23

`hotbar-02-shipping` adds ten compact bottom-center slots labeled `1-9/0`.
The 1280x720 bar occupies x=462.84–817.15 with 30.64-pixel slots, clear of the
needs and context panels. At 3840x2160 it scales to x=1388–2452 with 92-pixel
slots. Owned tools use original Homestead icons; stored/unowned references
remain visibly ghosted and unusable.

Shipping routes verify number `0`, wheel wrap, pointer selection without world
click-through, Ctrl+wheel zoom, R3, gameplay/menu LB/RB ownership, chest
ghost/restore, schema-7 save/reload, new-world reset, and left mouse/controller
RT Knife, Hatchet, Digging Stick, and Watering Can authority. The full loop
measured p95 17.49 ms and p99 17.97 ms, effectively identical to
`foliage-camera-v15` at 17.50/17.97 ms.

## Wardrobe presentation delivery

`wardrobe-presentation-08` retains the ordinary 23-stage mapped route and adds
native 720p/4K captures for all three visible base-only body presets plus an
authority-built tunic/apron/footwrap combination. The base-only images show
permanent bra/brief coverage and complete feet; the layered image shows the same
real equipment IDs in the portrait, Wearing grid and slot bar. The ordinary
405-frame route verifies idle, walk, turn, stop, orbit, portrait, gather and
recovery with the accepted tunic/shoes. These captures establish coverage and
parity, not aesthetic approval or physical-controller comfort.

The selected Shipping 720p native route measured p95 17.54 ms/p99 17.89 ms.
The matched Development 4K route measured 48.08 mean FPS, p95 20.42 ms and
p99 21.57 ms; it is explicitly not a 60 FPS claim.

## Initial run

`Saved\VisualPlaytests\20260919-211901` contains 290 frames covering approximately
40 seconds. The ordinary-control route reached its forage target and actually
collected the resource. The active user game and normal saves were left alone.

During the slow walk, actor speed was 67.5 cm/s. Near-lowest-toe samples measured
approximately 33 and 25 cm/s of world-space foot motion, consistent with possible
skating; this is a limited stance heuristic, not proof by itself. Faster/turning
samples were sparse and need cautious interpretation.

Review the captures before choosing corrections. Keep the current asset pipeline
unless observed defects justify replacing it; MetaHuman is not a settled decision.

## Reviewed findings from the initial route

The visual review actually inspected the chronological contact sheet and logs,
not a smooth video stream. It found:

- Portraits show a broad, rigid stance, arms held away from the torso, spread
  fingers, and a fixed expression. Hair reads as a thick sheet; the dress is
  tightly body-conforming with limited fabric structure. These are visible
  mannequin-like presentation issues, not evidence of a broken skeleton.
- Leg poses change, but upper-body movement is limited in the sampled frames.
  Body heading changes without a demonstrated planted turning action.
- Source confirms direct `PlayAnimation` switching between idle and walk, no
  crossfade, cadence based on an assumed 140 cm/s gait, and movement-facing
  rotation at 460 degrees/second. These are concrete candidates for correction;
  sampled frames cannot certify the severity of transition snapping.
- Successful gathering has no clear reaching/picking motion. The interaction
  handler changes inventory and plays feedback without requesting an action
  animation.

Prioritize gait calibration/blending, a relaxed stance and coordinated upper
body, then a short reach/pick/recover interaction. Refine the current hair and
garment before deciding whether replacement assets are necessary.

Do not label the 8 Hz recording's choppiness as a game frame-rate defect. Denser
motion capture or human feedback is still needed to judge heel/toe contact,
transition smoothness, turning feel, and hair/cloth deformation.

## Movement slice, 2026-09-20

The first bounded review uses `automation-setup` as the recorded baseline and
`20260920-050723-5cc6c8a5\movement-01-editor` as the new ordinary-control capture.
The latter has 273 actual 1280x720 frames over 40.085 seconds. Its
`contact-sheet.png` shows chronological idle/walk/turn/stop/portrait samples;
`posture-comparison.png` puts the before/after actual-game idle poses side by
side. The visible changes are closer feet, arms resting nearer the torso,
partially flexed fingers and softer knees. Face, hair and cloth remain the
same prototype, not a new appearance preset.

| Observed measure | Baseline | First movement candidate |
| --- | --- | --- |
| Idle horizontal toe separation | 34.59 cm | 17.50 cm |
| Slow-walk actor speed | 67.5 cm/s | 67.5 cm/s |
| Slow-walk low-toe world speed, left/right | 145.25 / 33.16 cm/s | 4.88 / 4.69 cm/s |
| Full-walk actor speed | 180 cm/s | 180 cm/s |
| Full-walk low-toe world speed, left/right | 268.31 / 177.42 cm/s (2/1 samples) | 18.52 / 11.89 cm/s (9/9 samples) |

These are descriptive samples, not a controlled estimate of percentage
improvement: old captures did not reject concurrent physical input, and
low-toe selection is not a contact detector. The new telemetry additionally
records native blend weight, play rate, clip phase and hand positions.
Using the known authored stance interval rather than height alone, slow-walk
foot motion was 4.41 cm/s on each side (10/8 samples); full-walk medians were
10.28/11.46 cm/s (4/3 samples). Measured local forward foot traverse was
119.42-119.89 cm per cycle versus approximately 120 cm of actor travel.
This supports the new cadence calibration but does not establish zero sliding.
Starts/stops include fractional weights, such as 0.71 while accelerating
and 0.26 while stopped, instead of an abrupt clip switch.

The first route also exposed a test-navigation error: stopping 53.8 cm from
the intended berry bush left neighboring reeds only 47.6 cm away. The game
correctly gathered the nearer reeds; the route's berry/herb result was false.
The corrected route approaches within 25 cm using normal movement, checks
the exact focused resource before gathering, and fails its process result
if the intended approach/gather outcome is absent. No teleport, resource
retargeting override, inventory edit or time skip was added.

Functional diagnosis is separate. Non-simulated stick and A-button events
entered the earlier offscreen smoke test while Jenny played another game.
The diagnostic log reproduces an extra physical A press followed by an
outfit assertion failure. Automation-only input isolation then passed all
433 editor mapped-input checks, including unchanged berry maturity after
six sleeps and genuine same-world checkpoint recovery. Historical berry
failure cannot be attributed conclusively without its missing input trace.
New per-step logs retain appearance, focus, position and crop state so future
failures have evidence. Physical input in ordinary gameplay is not filtered.

The authored files independently pass FBX round-trip checks: all original bind
matrices are identical, toe loop endpoint error is zero, and planted forward
velocity differs from 120 cm/s by less than 0.003 cm/s. Native rules retain
16 scenarios / 986 passing checks. None of these tests certifies Jenny's
controller comfort or aesthetic approval.

Residual limits: the gait is procedural, with noticeable knee lift and modest
upper-body motion; there is no terrain foot IK, planted turn or stop-step
selection. Stops blend to idle, and turns/slopes can still slide. Hair remains
rigid, the face fixed, and the tunic close-fitting. Gathering animation is
still deferred. Concurrent snowboarding uses the same PC: timing from this run
is concurrent-load evidence, not clean performance acceptance. Review is
limited to two capture passes for this slice.

### Packaged review, pass 2

The separately packaged candidate is
`Build\Releases\20260920-050723-5cc6c8a5\movement-01\Windows`.
The original `Build\Windows`, `Play.cmd` and player saves were not replaced.
Build bootstrap resaved the existing map, moss rocks and field/ground/rock
materials; these tested resaves are retained, with no authored landscape change.

`Saved\Automation\20260920-050723-5cc6c8a5\movement-01-packaged\smoke-result.txt`
records **433 passing mapped-input checks**, including native-animation stance,
cadence/stopping, exact forage focus, all appearance combinations, unchanged
berry maturation after six sleeps, and same-world checkpoint recovery.
The controller rejected 18,840 external-input events, including four deliberate
probes. This validates automation isolation while another game was running;
it does not test ordinary physical-controller comfort.

`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\movement-01-packaged`
contains **303 actual 1280x720 frames over 40.8876 seconds**, timestamped playback,
telemetry, observations and the inspected chronological `contact-sheet.png`.
The heroine walks, turns, stops and approaches berry bush 52 using ordinary
mapped input. Exact focus is confirmed and berries are actually gathered.
The final images retain the closer stance and arms near the torso; portrait
frames still show the fixed face, rigid hair and close-fitting tunic.
One turn sample is partly occluded by a sapling, so it is not useful evidence
of foot contact. No additional cosmetic iteration followed this second review.

| Final packaged measure | Result |
| --- | --- |
| Idle horizontal toe separation | 17.50 cm |
| Slow-walk authored-stance world foot speed, left/right | 4.54 / 4.30 cm/s (11/8 samples) |
| Full-walk authored-stance world foot speed, left/right | 11.68 / 11.40 cm/s (4/4 samples) |
| Turn authored-stance world foot speed, left/right | 9.30 / 8.80 cm/s (6/5 samples) |
| Cadence agreement across all 303 samples | `speed / 120`, within 0.002 cm/s after CSV rounding |
| Start/stop blending | Fractional weights captured; both stop segments end at zero speed and walk weight |

These samples support calibrated walking and gradual transitions, not perfect
foot locking or continuous smoothness. Packaged functional timing was 59.72 mean
FPS, 16.94 ms p95 and 17.24 ms p99 after startup/screenshot exclusions, under
concurrent load; it is not clean performance acceptance. Human feel, listening
and Jenny's visual approval remain separate. The candidate is not promoted.

## Face presentation slice: rejected hypothesis and diagnostics

The next bounded slice inspected actual close portraits, source smooth shading,
texture color space and the existing rig. The imported skin used Default Lit,
despite the Blender source having a 0.07 subsurface weight; the masked eye atlas
used one uniform surface response. This motivated a **hypothesis**, not proof
that adding subsurface scattering would improve this particular heroine.
The source has no eye/lid/jaw bones or retained shape keys, so no blinking,
eye scaling, facial resculpting or new facial-animation infrastructure was added.
Hair, world lighting/exposure, audio and locomotion were outside this slice.

### Controlled comparison protocol

`Test-Game.ps1 -Presentation -Width 1920 -Height 1080` captures five actual-engine
images: front and three-quarter at noon, front and three-quarter at 22:00 near
an ordinary fueled fire, and alternate skin/eye colors at noon. Use fresh output
folders for each run. `-Packaged -PackageDirectory <candidate>` selects a separate
package, as for other integration tests.

This is explicitly a **fixed visual fixture, not ordinary gameplay**. It disables
controller simulation ticking, supplies a copied world-visual state, samples the
existing relaxed idle at time zero, hides the HUD and uses a fixed test camera.
The real runtime meshes/materials, world lights and automatic exposure are used;
there are no added portrait lights or production camera changes. Frame metadata
and `presentation-fixture.txt` record the setup. The normal-input route remains
separate and still prohibits teleports, time skips and state edits.

The first setup capture (`presentation-before`) was rejected as a night
comparison: six seconds did not allow the existing 1 EV/s dark adaptation to
settle, and the fire was behind the heroine. The corrected fixture waits 18
seconds on the day-to-night transition and places the ordinary fire in front.
Exposure itself was not changed. This was a fixture correction before material
editing, not cosmetic tuning of the heroine.

### Outcome: do not promote presentation-01

The settled before evidence is
`Saved\Automation\20260920-050723-5cc6c8a5\presentation-before-settled`.
The experimental packaged evidence is
`Saved\Automation\20260920-050723-5cc6c8a5\presentation-01-portraits`.
All five originals are 1920x1080. Actor, camera, mesh and head transforms match
exactly in the recorded metadata; rounded projected bounds differ by at most
0.1 pixel. Native-resolution face crops are retained as
`face-*-comparison.png`, before on the left and rejected candidate on the right.
No private reference image appears in these comparisons.

The candidate's Subsurface Profile produced visibly red, noisy eye-socket,
nose and neck shadows in daylight. Firelight lost nose/lip definition rather
than gaining convincing skin response. Eye highlights became more distinct,
but that did not justify accepting the experiment. The coordinator independently
inspected the front comparison and agreed with rejection. Both experimental
skin and eye graphs were restored; no third cosmetic iteration or eye-only
candidate was authorized.

The rejected package remains at
`Build\Releases\20260920-050723-5cc6c8a5\presentation-01`, with a `REJECTED.txt`
marker. It passed 434 functional checks, including actual material bindings,
all 18 wardrobe combinations, editable color controls and save/load, but **functional success did not
override visual failure**. Its separate normal-control route reached and
gathered berry bush 52: 148 actual 1920x1080 frames over 42.8792 seconds.
This recording is unusually sparse (about 3.45 captured frames/second), contains
no sampled fractional blend weights and insufficient fast-walk stance pairs;
it cannot establish continuous smoothness or a fast-gait regression.
The native stance/cadence/stop assertions still passed. No production animation code or
locomotion assets were changed in this slice.

### Recovery and retained diagnostics

All 50 assets changed by the experiment/import/bootstrap were restored to
checkpoint `132898f`, with SHA-256 proof in
`Build\CharacterPreview\presentation-recovery-hashes.json`. The rejected
profile, configuration and production import wiring were removed. Only the
fixed comparison fixture, stronger baseline runtime material/color checks and
documentation are retained. The restored checks explicitly expect Default Lit
skin/eyes, the original masked eye surface and matching current skin/iris color
parameters after every successful smoke step, including wardrobe changes and
save/load. They also reject the discarded experiment's shader parameters.

The separate `presentation-recovery` package is for diagnostic recovery
verification, not a new visual hypothesis or a facial-quality success.
Neither the original player build nor `movement-01` was replaced.

Recovery verification completed against
`Build\Releases\20260920-050723-5cc6c8a5\presentation-recovery\Windows`:
**152 packaged checks passed**, covering native movement/cadence/stopping, all
18 wardrobe meshes, editable colors, appearance save/load, gathering and
building. Actual Default Lit/material color checks ran after every successful
step. Captures were verified as 1920x1080; they were not used for a third cosmetic
review. See
`Saved\Automation\20260920-050723-5cc6c8a5\presentation-recovery\smoke-result.txt`.
The larger full loop was not repeated during recovery: this was the smallest
existing runtime route covering the retained diagnostics, and all production
character assets were restored exactly. Native rules also passed 16 scenarios /
986 checks, and run/path controls passed 21 checks.

The recovery build log confirms unchanged character sources/importer and no
character reimport during packaging. All character material/mesh/texture hashes
remained at the known-good checkpoint. Five incidental world-bootstrap resaves
were restored afterward as well; no content assets are changed by the diagnostic
checkpoint. Experimental shader parameters, material graphs and profile assets
are not retained in production. The heroine's baseline facial limitations remain.

## Rejected wavy-length trial (2026-09-20)

**Requested mid-back length remains unmet.** `hair-length-01` is rejected, not
an accepted shorter hairstyle. Only this length correction was scheduled;
the blonde-bob redesign, additional face experiments and other feedback stayed
out of scope. The original player package and previous accepted candidates were
not overwritten.

### Baseline, target and technical evidence

`Test-Game.ps1 -HairLength -Width 1920 -Height 1080` captures all six long-wave
body/outfit combinations from back and three-quarter views at noon. It reuses
the isolated fixture, freezes the existing idle at time zero and uses a 260cm
camera distance/FOV40, targeting 22cm below the head. It asserts the exact mesh
selected for each case and records actor/camera/head/mesh framing. This is
not ordinary gameplay; lighting/exposure and player saves are unchanged.

Baseline: `Saved\Automation\20260920-050723-5cc6c8a5\hair-length-before`.
Final trial: `Saved\Automation\20260920-050723-5cc6c8a5\hair-length-01-portraits`.
Each has twelve actual 1920x1080 captures. All recorded numeric comparison
coordinates match exactly; signed zero is treated numerically, not as a false
camera difference. `Comparison\overview.png` and per-combination sheets are
explicitly labeled **REJECTED**. These sheets are cropped/scaled; originals are
retained. No private reference is included.

Authoring shoulder/waist landmarks were 126.03/96.07cm; the trial's midpoint target
was 111.05cm. Evaluated original hair tips were 89.15-89.29cm. A monotonic
lower-length remap below the 134.60cm neck anchor preserved crown/fringe and
front framing, width, source topology/UVs/weights, bind and all non-hair geometry.
Pure vertical shortening penetrated the wider upper back, so lower rear cards
received a bounded clearance fit of at most 4.11cm. Across eight idle/walk
samples per combination, maximum radial penetration improved from 7.75 to 4.23mm
(preferred), 7.32 to 4.73mm (Willow), and 9.55 to 6.71mm (Hazel). This was a diagnostic,
not proof of exhaustive collision freedom.

Only six existing Unreal meshes were refreshed. Actual geometry exported after
fresh Unreal reload confirmed 111.05cm tips and unchanged crown/width, oriented
UV boundaries, non-hair face surfaces and shared references. The other 44
character assets were hash-identical. UE's deformed-quad triangulation and
tangent splitting changed Willow's render vertex count by four; all meshes
retained 16,432 hair triangles, and source quads/UVs remained identical.
The experiment did not rewrite the general character-import cache.

### Failed visual gate and recovery

The final packaged back/three-quarter comparison shows obvious **accordion-like
horizontal ridges** in the compressed lower waves, ending in a blunt/frayed
shelf. It reaches the numerical length but does not preserve the established
wave silhouette well enough. The coordinator independently inspected
`Comparison\preferred-tunic.png` and agreed. No third sculpt/review pass was
attempted, and technical checks were not substituted for visual acceptance.

The rejected package passed **152 appearance/save/movement integration checks**
with four deliberate external-input rejection probes. Its separate normal-control
route recorded **144 actual 1920x1080 frames over 41.6769 seconds** and reached/
gathered berry bush 52. Idle toe span was 17.50cm. Sampling was sparse
(about 3.46 frames/second), with no sampled fractional blend values or fast-walk
stance pairs, so this does not establish continuous smoothness or controller
comfort. Timings are concurrent-load only. No simulation rules changed.

Production FBXs, all six imported meshes, manifests, provenance and export/build
wiring were restored to `5947edf`. Experimental scripts and the active contract
were removed. `Build\CharacterPreview\HairLength\recovery-hashes.json` proves
the original SHA-256 for all 50 character assets and the six source FBXs.
Rejected source/scripts/contracts, authoring and native-import measurements,
clearance diagnostics and receipts remain local under
`Build\CharacterPreview\HairLength`; copied scripts in `RejectedSource` are
forensic snapshots, not runnable in-place tooling. The discarded package has
`REJECTED.txt` at its archive root and inside `Windows`.

The retained changes are diagnostic: the fixed hair fixture, exact selected-mesh
assertions, additional framing landmarks and `Review-HairLength.py`.
The separate `hair-length-recovery` package verifies this diagnostic source with
baseline art; it is not another hairstyle hypothesis or a length improvement.

Recovery package:
`Build\Releases\20260920-050723-5cc6c8a5\hair-length-recovery\Windows`.
Its **152 packaged checks passed**, including all 18 wardrobe combinations,
editable colors/material bindings, appearance save/load, native locomotion,
gathering/building and four deliberate input-rejection probes. Evidence:
`Saved\Automation\20260920-050723-5cc6c8a5\hair-length-recovery\smoke-result.txt`.
The recovery build retained the restored character assets without reimporting
them. Its five incidental world-bootstrap resaves were restored afterward.
The final checkpoint contains no art/content or production movement changes.
Run/path controls passed 21 checks; the simulation suite was not repeated
because no simulation rules changed. Recovery captures were not used for a third
cosmetic review. No purchases, new assets, private-reference uploads or package
promotion occurred.

## Responsive wild gathering (2026-09-20)

Candidate: `Build\Releases\20260920-050723-5cc6c8a5\gathering-01\Windows`.
This is a small technical/action-presentation improvement, not Jenny's aesthetic
approval. The original player build, accepted movement and both recovery
candidates remain untouched; there is no automatic promotion.

### Two bounded visual reviews

The existing `movement-01-packaged` baseline's berry-gather frames (including
`frame-00268.png` and `frame-00286.png`) show the heroine upright with her hands
at rest while the resource renews. Source likewise had no gathering action.

Final evidence:
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\gathering-01-packaged`.
The route recorded **283 actual 1280x720 frames over 43.4895 seconds**, normally
walked to berry bush 52, gathered once, and observed both the action and recovery.
There were no teleports, state edits or time skips. The route may now stop
within 55cm **only when the intended resource is actually focused**, otherwise
it continues the approach. This is observation navigation, not an automatic
approach or changed reach/focus rule in gameplay. Actual final distance was
about 34.9cm. Settling avoids intentionally interrupting the gesture with
residual movement; mapped camera controls expose the picking arm.

The inspected `gathering-sheet.png` shows a small knee/torso dip, right-arm
reach and return to the relaxed stance instead of an unchanged upright pose.
It reads as a generic scooping/picking gesture, not a precise finger pluck.
The hand reaches near the foliage, but exact fingertip contact is not established.
Walking frames retain the accepted gait; the action layer stays at zero during
the walk/turn segments. No further animation tuning followed this final pass.

| Measured final observation | Result |
| --- | --- |
| Active action samples / action starts | 8 / 1 |
| Final action weight | 0 |
| Maximum right-wrist displacement | 26.77cm |
| Actor displacement during picking | 0cm |
| Maximum left/right toe displacement | 0.258 / 0.232cm |
| Closest wrist-to-target XY distance | 5.54cm; not a contact/IK proof |
| Idle toe span | 17.50cm |
| Slow walk / full walk actor speed | 67.5 / 180cm/s |
| Slow-walk authored-stance foot speed, left/right | 4.40 / 4.58cm/s (9/8 samples) |
| Full-walk authored-stance foot speed, left/right | 10.73 / 9.65cm/s (3/3 samples) |

Residual walking slip is still present, not fixed by the picking clip. Hair
remains rigid and the tunic close-fitting; foliage partly obscures lower-body/
garment contact in the gather view. One forward gesture cannot contact all
low, distant or behind-body resources, and there is no target-aware or terrain
IK. The simulation reward and existing sound occur immediately on success;
the pose does not delay/duplicate them. Moving can suppress or interrupt the
pose without suppressing an otherwise valid harvest.

### Independent technical acceptance

- Exported FBX: 97 samples over 1.6s, 53 original bones, identical bind,
  idle endpoint matrix error 2.38e-7, toe drift 0.0000543cm, bone-scale error
  6.56e-7, wrist travel 26.81cm. See
  `Build\CharacterPreview\gathering-export-validation.json`.
- Fresh Unreal reload: original shared skeleton, duration 1.600000024s,
  root motion disabled and zero notifies. See `gathering-reload.json`.
- **74 editor and 74 packaged focused gathering checks** passed. Packaged
  evidence is `Saved\Automation\20260920-050723-5cc6c8a5\gathering-01-lifecycle`.
  These use isolated teleport setup, unlike the ordinary route, and cover
  exact reward/regrowth deltas, A/E, failures, movement, pause, load and appearance.
- **434 packaged full-loop checks** passed, including all appearance controls,
  saves, crafting/building/gardening, berry maturation, failure and same-world
  retry. Evidence is the sibling `gathering-01-full-loop` folder. Four deliberate
  external-input probes were rejected. No existing assertion was weakened.
- Native simulation: **16 scenarios / 986 checks**; run/path controls: **21**.

All 50 baseline character content assets and 16 tracked character-source LFS
assets remain hash-identical to `7748763`; only the new gathering clip is added.
`gathering-baseline-preservation.json` records the hashes. Five incidental
world-bootstrap resaves were restored and verified in
`gathering-world-preservation.json`. No mesh/material import, external asset
acquisition, private-reference upload or face/hair experiment was involved.

Both recorded gameplay and smoke timing remain concurrent-load observations.
Requested 8Hz capture is not game FPS or proof of continuous smoothness,
controller comfort, listening quality or Jenny's approval.

## Watering action and contextual tool (2026-09-20)

Candidate: `Build\Releases\20260920-050723-5cc6c8a5\watering-01\Windows`.
It adds one 2.2-second lift/tilt/recover and a small original wood/fiber-bound
watering vessel with a spout and loop handle. The existing three-branch/two-fiber
recipe and all simulation rules remain unchanged. No player build, previous
candidate or normal save was replaced.

### Initial observation and concrete correction

The initial actual-game review at
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\watering-initial-02`
exposed an enormous held prop: the hand socket's inherited FBX unit-conversion
scale multiplied geometry already authored in centimeters. Correct grip position
and lifecycle checks had not caught size. The correction gives the prop absolute
world scale and adds actual scale/bounds assertions across all 18 appearances.
There was no arm-angle or clip redesign after this initial review.

An earlier six-frame setup diagnostic (`watering-initial`) failed before reaching
the action because the new observer checked queued input in its submitting tick.
The route now checks actual transaction results after input processing. Its
PowerShell launcher also rejects failed recorded outcomes independently of exit
status; the editor had exited zero despite the failed route. That diagnostic
was not a completed action-quality review and was not discarded or called a pass.

### Final ordinary-control evidence

```powershell
.\Scripts\Playtest-Visual.ps1 -Packaged `
  -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\watering-01' `
  -Watering -Width 1280 -Height 720 -OutputDirectory '<fresh-folder>'
python .\Scripts\Review-Watering.py '<fresh-folder>'
```

Final capture:
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\watering-01-packaged`.
It contains **122 actual 1280x720 frames over 72.9212 seconds**. Starting from the
normal clearing, the heroine walks to five supply patches, gathers branches/
stone/fiber/seeds, crafts a digging stick and can through the book, walks to the
stream, fills six portions, approaches clear soil, tills, plants and waters a
real root plot at cell `(3,-3)`. All mutations use mapped controls. There is no
injected setup save, teleport or debug time/state edit; ordinary crafting keeps
its existing time cost. Garden feasibility is checked on a simulation copy,
never by creating a plot in the live state.

Setup is sampled at 1 Hz; the final full-body/tool segment requests 8 Hz.
The inspected `watering-sheet.png` shows a hand-sized handle and modest wooden
can in front of the body, a downward tilt, return upright and disappearance
before the relaxed idle resumes. No gross body/tool penetration is visible in
these preferred-heroine frames. Functional attachment/scale/hide checks cover
all presets; this is not an exhaustive visual clearance proof for every mesh,
camera and terrain slope. The final correction is retained; no third visual
tuning pass followed it.

| Final ordinary watering measurement | Result |
| --- | --- |
| Real water debit / new action starts | 1 / 1 |
| Moisture before / after recovery | 0.348782 / 0.999069 (normal drying continues) |
| Held-tool samples / peak tilt | 10 / 32 degrees |
| Tool world scale / bounding-sphere radius | 1 / 28.924cm |
| Maximum wrist / actor displacement | 39.47cm / 0cm |
| Maximum left/right toe displacement | 0.371 / 0.324cm |
| Final action weight / tool visible | 0 / false |

The can appears and disappears contextually; there is no holster/equip animation
or visible water stream. It is a generic forward pour, not precise soil contact,
target-aware IK or a fluid simulation. The water/moisture transaction and existing
sound/toast happen immediately on success, not at an animation notify.
Movement can suppress/cancel the pose without undoing a legitimate transaction.
Repeated/alternating presentation requests never create competing pose layers
or delayed actions; separate valid watering inputs still consume their normal
water portions. Hair, face, clothing and residual gait limitations are unchanged.

### Packaged regressions and preservation

`Saved\Automation\20260920-050723-5cc6c8a5\watering-01-lifecycle` passed
**181 watering checks**, including the corrected world scale/bounds, exact
inventory/moisture/growth effects, A/E, failures, repeats, movement, pause,
save/load and all 18 appearances. Its setup uses explicitly test-only teleport
travel but real gather/craft/till/plant transactions, unlike the ordinary route.

The sibling `watering-01-gathering` and `watering-01-full-loop` folders passed
**74 gathering** and **434 full-loop checks**. Mature berry/root harvests now
explicitly prove no water debit or new watering action. Existing crop maturation,
appearance/colors/saves and real failed-vitals/same-world retry assertions remain
intact. The standard full loop rejected four deliberate external-input probes;
dedicated action modes correctly report zero such probes.
Native rules passed **16 scenarios / 986 checks**, run controls **21**.

The independently reloaded FBX has 133 samples, duration 2.2s, all 53 original
binds unchanged, idle seam error 3.58e-7, toe drift 0.0000573cm and bone-scale
error 7.15e-7. Fresh Unreal reload proves the shared skeleton, duration
2.200000048s, disabled root motion and zero notifies. Receipts are under
`Build\CharacterPreview\watering-*` and copied into the candidate's
`Verification` directory.

All **68** baseline character/source LFS assets remain byte-identical to
`08810ef`: the original 66 plus the two accepted gathering assets. Only the new
watering animation is imported; the prop is original runtime geometry using
an existing material. Five incidental world-bootstrap resaves were restored
with hash proof. No body/hair/material reimport, new purchase, private-reference
upload or promotion occurred.

This is verified technical/action-presentation progress, not Jenny's aesthetic
approval. Sparse screenshots and concurrent-load timings do not certify
continuous smoothness, controller comfort, listening quality or clean performance.

## Weeding feedback using the existing pick (2026-09-20)

Candidate: `Build\Releases\20260920-050723-5cc6c8a5\weeding-01\Windows`.
Successful planted-plot X/F now requests the accepted gathering clip. No new
animation, prop, mesh, material, simulation rule, interaction radius, forced
turn or camera behavior was introduced. Other secondary-input contexts remain
separate; a failed weed transaction requests no presentation.

### Explicit setup and two review sets

Visible weeds grow at 0.009 per game hour, with the first rendered weed at
0.125. Rather than silently advancing time or waiting hours, both recordings
load a byte-identical copy of
`Saved\Automation\20260920-050723-5cc6c8a5\watering-01-full-loop\SmokeSave\Homestead_Auto_0.sav.bak`.
This dedicated functional-world checkpoint has two planted plots at about
0.29 weeds, game hour 88.17 (16:10), and healthy enough vitals for the approach.
Its prior setup used functional teleports and repeated ordinary sleep, not a
fresh ordinary-play route. It also contains the test's saved Willow/bob/apron
appearance and colors; those are not new art choices or Jenny's preference.
The source is never overwritten. Each output's `fixture.json` records its
source, SHA-256 and setup boundary.

After mapped F9 loading, the observer walks from the saved bedroll through a
staging position to plot 108, settles, orbits/zooms using mapped controls, and
presses gamepad X. There are **no debug position/time/state edits during this
recorded approach/action**. Engine lifecycle tests use explicit teleport setup
separately; do not pass those off as ordinary movement footage.

The initial set is `Saved\VisualPlaytests\20260920-050723-5cc6c8a5\weeding-initial`
(119 actual 1280x720 frames / 42.9939s). The second and final set is
`...\weeding-01-packaged` (119 actual 1280x720 frames / 42.7352s).
Both use the same copied world, saved appearance and mapped camera route.
Only these two quality sets were reviewed; no clip/art retuning was attempted.

```powershell
.\Scripts\Playtest-Visual.ps1 -Packaged `
  -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\weeding-01' `
  -Weeding -FixtureSave 'Saved\Automation\20260920-050723-5cc6c8a5\watering-01-full-loop\SmokeSave\Homestead_Auto_0.sav.bak' `
  -Width 1280 -Height 720 -OutputDirectory 'Saved\VisualPlaytests\weeding-new-review'
python .\Scripts\Review-Watering.py 'Saved\VisualPlaytests\weeding-new-review' --weeding
```

| Final recorded measurement | Result |
| --- | --- |
| Pick starts / active samples | 1 / 9 |
| Weeds before / after ordinary progression | 0.292319 / 0.000314 |
| Water debit / visible can samples | 0 / 0 |
| Final action weight | 0 |
| Right wrist displacement | 25.84cm |
| Actor displacement during action | 0cm |
| Left / right toe displacement | 0.368 / 0.414cm |

The inspected `weeding-sheet.png` shows the visible weed props before X, a
gentle dip/forward hand-pull, and upright recovery without a can or gross new
body/garment penetration. It is **not ground-level weeding contact**: the hand
gestures above the soil, weeds disappear on the immediate transaction rather
than a hand contact, and close standing can overlap the fixed crop decorations.
The underlying rigid clothing, fixed face and existing hair limitations remain.
Approach sampling is requested at 2 Hz and action sampling at 8 Hz, not game FPS
or a proof of smoothness/controller comfort. Timings are concurrent-load only.

### Functional and preservation evidence

The fresh package passed **37 weeding, 74 gathering, 181 watering and 434
full-loop checks**, under
`Saved\Automation\20260920-050723-5cc6c8a5\weeding-01-{weeding,gathering,watering,full-loop}`.
The same 37 weeding checks passed in editor before packaging. Expected state
uses one real `Weed` on a simulation copy and normal clock advancement:
inventory, water, moisture and resource state must otherwise match. Mapped X/F,
same-frame already-clean rejection, real water/weed/gather/clear alternation,
movement, book/Look/pause/planning, appearance and save/load are covered.
Existing all-appearance and real failure/retry checks remain intact. Full-loop
assertions additionally reject pick starts during fueling, tilling and bare-plot
berry planting. Seven copy/argument guards verify fixture isolation and source
preservation. Portable simulation code did not change; its suite was not rerun
unnecessarily for this presentation-only hook.

All **70** existing character/source LFS assets match `27f83af`; no new
export/import was authored. The build reverified the existing animation sets.
Five known incidental world-bootstrap resaves were restored with hash proof.
Receipts under `Build\CharacterPreview\weeding-*`, packaged reports and the
final sheet are copied into candidate `Verification`; `acceptance-receipt.json`
ties the executable, source, fixture and observations together.
Original `Build\Windows`, `Play.cmd`, player saves and earlier candidates remain
untouched. This is technical feedback, not Jenny's aesthetic approval or an
authorization to resume the deferred face/hair/UI experiments.

## Sapling clearing and contextual hatchet (2026-09-20)

Candidate: `Build\Releases\20260920-050723-5cc6c8a5\clearing-01\Windows`.
Only a successful permanent sapling `Sim.Clear` requests the new 2-second
restrained lift/swing/recover. Its original small wooden haft, stone wedge and
fiber bindings fit the existing four-branch/three-stone/two-fiber recipe. The
tool has no collision/navigation/overlap, uses absolute world scale one, and
attaches to the right-hand grip. No body/hair/wardrobe/material replacement or
new sound, simulation, reach, camera, facing or root-motion behavior is added.

### Actual ordinary-control evidence

Both sets start from a fresh default clearing and preferred heroine. Mapped
controls walk to resource nodes 1, 2 and 6, gather supplies, craft the existing
hatchet, walk through a staging point to the actual sapling at (-700,-600), settle,
orbit/zoom and press X. There is no injected save, debug teleport or live
time/state rewrite. Normal crafting keeps its existing 0.05-hour cost.
Setup sampling is requested at 1 Hz and action at 8 Hz, not actual game FPS.

The initial set is
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\clearing-initial`
(81 actual 1280x720 frames / 41.2424s). The second/final packaged set is
`...\clearing-01-packaged` (85 actual 1280x720 frames / 40.8266s).
Only these two quality sets were used. The clip/prop was not retuned between
them; an additional real rapid-input test was added before packaging.

```powershell
.\Scripts\Playtest-Visual.ps1 -Packaged `
  -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\clearing-01' `
  -Clearing -Width 1280 -Height 720 `
  -OutputDirectory 'Saved\VisualPlaytests\clearing-new-review'
python .\Scripts\Review-Watering.py 'Saved\VisualPlaytests\clearing-new-review' --clearing
```

| Final recorded measurement | Result |
| --- | --- |
| Clearing starts / held-tool samples | 1 / 8 |
| Branch / fiber yield | 8 / 2 |
| Sapling cleared flag before / after | 0 / 1 |
| Water debit / visible watering-can samples | 0 / 0 |
| Final action weight / tool visible | 0 / false |
| Tool world scale / bounding radius | 1 / 19.607cm |
| Maximum forward tool tilt | 60 degrees |
| Wrist / actor displacement during action | 55.59cm / 0cm |
| Left / right toe displacement | 0.244 / 0.245cm |
| Energy change across the sampled action/recovery | -0.085844, ordinary clock progression |

The inspected final `clearing-sheet.png` shows the sapling before X, a modest
held hatchet lifting and moving forward/down, and upright idle recovery with
the prop hidden. No gross new body/tool/garment penetration is visible in these
frames. **The sapling disappears when the simulation commits, before the swing.**
This is generic clearing feedback, not impact synchronization, exact finger/
trunk/terrain contact or realistic tree felling. The blocky stone/fiber head and
contextual pop-in/out remain prototype limitations. Existing character-art
limitations remain unchanged. Sparse images and concurrent-load timings do not
establish continuous smoothness, controller comfort, audio quality or clean FPS.

### Transaction, lifecycle and asset proof

The final editor route passed **188 clearing checks**. The fresh package passed
the same **188**, plus the preserved **74 gathering, 181 watering, 37 weeding
and 434 full-loop checks**: **914 packaged checks** in
`Saved\Automation\20260920-050723-5cc6c8a5\clearing-01-{clearing,gathering,watering,weeding,full-loop}`.
Expected-state copies check exact inventory/yield, resource IDs, cleared flags,
regrowth timestamps, next ID and ordinary energy progression. Mapped X/F,
no-hatchet/capacity/range rejection, repeat input, real rapid branch harvest/
clear followed by sapling clear, movement, book/Look/pause/planning, all 18
appearance grip/scale/hide/tint cases and save/load are covered. Existing real
failure/retry, garden, storage and color/save assertions remain intact.

Rules were not rewritten for the tests: a ready sapling needs room for ten
yield items; an already-depleted sapling can still clear permanently with zero
yield even when the ready yield would not fit. Already-cleared nodes remain
cleared and subsequent input follows whatever new context is actually focused.
Sapling A/E harvest and non-sapling clearing do not request a hatchet pose.
Active requests coalesce rather than queue, and the watering test now also
challenges its active pose with a clear request. No simultaneous contextual
tools are permitted. Portable simulation code is unchanged and its suite was
not unnecessarily rerun.

The exported clip has 121 samples over 2s and all 53 original binds unchanged:
idle seam error 3.58e-7, toe drift 0.0000558cm, maximum bone-scale error 7.15e-7.
Fresh Unreal reload proves the existing shared skeleton, 2s duration, disabled
root motion and zero notifies. All **70** prior character/source LFS assets
match `42241c5`; only the new clearing FBX and animation uasset are added.
Five known incidental world-bootstrap resaves are restored with hash proof.
The shared review utility reproduces prior water/weed numeric reports with its
writes intercepted, so older evidence is untouched.

Authoring/import/preservation reports, all final frames, source hashes and
functional results are persisted under candidate `Verification`;
`acceptance-receipt.json` links them to the executable and private checkpoint.
Original `Build\Windows`, `Play.cmd`, player saves and all earlier candidates
remain untouched. This is technical/action feedback, not Jenny's aesthetic
approval or a completion claim for general chopping or deferred cosmetics/UI.
