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
