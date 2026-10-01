# Tool strike sounds: the pickaxe ping and the billhook cane cut

## Why

Jenny, on 2026-09-30: "Can the first and last swings with the pickaxe have the same noise as the last swing? I like
the ping of it hitting stone over the more default initial noise. Clearing canes with the billhook needs a new sound,
too."

## What Changes

- **Pickaxe:** every landed strike plays the stone ping (the Kenney `CraftStrike` impacts), not the wood chop.
  - Earlier strikes rotate the three variants at the breaking strike's old level.
  - The breaking strike keeps `CraftStrikeA`, about 2 dB louder.
  - Pitch varies +/-4% (`PlayEffect`).
- **Billhook:** a new original cue, `CaneCutA`/`B`/`C`, synthesised by `Scripts/generate_billhook_sound.py`: a hooked slash, a woody fibrous snap, then a leaf rustle. It's mastered to the chops' loudness and plays on every landed swing, the breaking one included.
- Both fire on the clip's contact frame, as the chops did: the strike clip's `FellStrikeSeconds(0)` for the pickaxe, and the hack's `MacheteClearSeconds` for the billhook. The breaking strike sounds only when the clearing succeeds.
- A `STRIKE_CUE` log line records each cue, its gain, and the clip phase against its contact time, and `docs/audio-checks.md` lists the checks.

## Impact

`HomesteadControllerClearing.cpp` (`PlayStrikeCue`), `HomesteadControllerAudio.cpp`, `Scripts/generate_billhook_sound.py`,
`Scripts/bootstrap_unreal.py`, `Assets/Audio/Effects/CaneCut*.wav`, `/Game/SurvivalGame/Audio/Effects/CaneCut*`,
`docs/asset-credits.md`, `docs/audio-checks.md`. Other tools' strikes are unchanged.
