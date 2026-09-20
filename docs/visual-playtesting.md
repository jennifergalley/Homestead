# Visual playtesting

Functional checks and visual playtests answer different questions. Passing
inventory, save, or interaction assertions does not show that a heroine looks
appealing or that walking feels natural. Do not select a replacement character
pipeline before inspecting the current game in motion.

## Repeatable observational route

Build the editor target, then run:

```powershell
.\Scripts\Playtest-Visual.ps1 -EngineRoot 'E:\Program Files\UE_5.8'
python .\Scripts\Review-VisualPlaytest.py .\Saved\VisualPlaytests\<run-folder>
```

Open the generated `review.html` locally to play, pause, scrub, or step through
the recording. It replays captured engine frames at their recorded timestamps;
it is a review artifact, not a browser version of the game.

The route closes the opening notes, idles, walks slowly and at full speed, turns,
stops, orbits the camera, inspects the face, and walks to an actual forage target
before gathering it. It sends ordinary mapped input events rather than calling
simulation actions directly. There are no teleports or time skips.

It runs offscreen with separate saves and does not commandeer the player's open
game or send mouse/keyboard input to other applications. It does not update the
packaged build or overwrite a player's session.

To observe a separate standalone candidate, pass `-Packaged -PackageDirectory
'Build\Releases\<candidate>'`. `-OutputDirectory` selects a fresh recording folder;
existing telemetry is rejected rather than overwritten. The script verifies
process exit and every frame's requested dimensions, not just file presence.

## Evidence and limits

- `Frames`: actual screenshots sampled at approximately 8 Hz.
- `telemetry.csv`: timestamps, actions, actor position/speed, facing, and toe
  positions in world space.
- `observations.txt`: actions and actual gathering outcome, including blocked
  approaches rather than silently moving past them.
- `motion-review.json`: limited stance/foot-speed diagnostics, not an authoritative
  foot-contact detector or a substitute for visible evidence.
- `review.html`: timestamped playback and frame stepping.

Screenshot readback can disturb pacing. This route is for pose, movement,
framing and interaction observation, not frame-rate benchmarking. Sampled
playback can look choppier than the game; do not call capture jitter a game bug.
Physical-controller comfort, continuous animation smoothness and listening
still benefit from a human playtest or denser video capture.

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
