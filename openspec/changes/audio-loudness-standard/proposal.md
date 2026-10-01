# Proposal

## Why

Jenny (2026-09-30): "The scythe sound is about twice as loud as it needs to be. Is there any way we can get
better at audio balancing sounds from the get-go by balancing it against the ambience sounds? Like the scythe
is way louder than the birds, and the billhook sound is almost too quiet."

Cue gains were literals scattered over a dozen controller files, set by ear one at a time, with nothing to
compare a new sound against.

## What Changes

- A loudness standard: categories with bands in LU over the forest ambience bed at its in-game gain
  (docs/audio-checks.md, "Loudness standard").
- `Simulation/HomesteadAudioLevels.h`: every cue's category and gain; the playback code reads its gains from it.
- `Scripts/Audio/Measure-Loudness.py`: measures every source (EBU R128), applies gain and default sliders, flags
  anything out of band or uncategorised, and regenerates `HomesteadAudioMeasurements.h` and the docs table.
- Native gate `HomesteadAudioLevelTests`: out-of-band cue, missing measurement, uncategorised file under
  Assets/Audio, or drifted bed gains fail the build's tests.
- First pass: scythe swish 0.8 to 0.25, billhook cane cut 0.75 to 0.9, shop sale tap 0.35 to 0.16. The rest
  already sat in band.
