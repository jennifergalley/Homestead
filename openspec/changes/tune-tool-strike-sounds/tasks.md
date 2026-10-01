# Tasks

- [x] 1.1 `PlayStrikeCue`:
  - Pickaxe: pings on every strike (A/B/C at 0.12, the final strike A at 0.15).
  - Billhook: `CaneCut` A/B/C at 0.75.
  - Other tools unchanged.
  - The breaking strike sounds only on success.
  - The `STRIKE_CUE` log carries the clip phase and its contact time.
- [x] 1.2 `Scripts/generate_billhook_sound.py`: three deterministic variants, peaking about -3.8 dBFS with the loudest 100 ms at -18.2 dBFS RMS (the chops are -16.7 to -19.6). Credited in `docs/asset-credits.md`, and imported by `bootstrap_unreal.py`.
- [x] 1.3 `docs/audio-checks.md`: rows for the pickaxe ping and the cane cut.
- [ ] 1.4 Unreal slot: import `CaneCutA`-`C` into `/Game/SurvivalGame/Audio/Effects/` (one-shots), build, and check in PIE:
  - pickaxe a rock: every strike logs `cue=CraftStrike*` with `phase >= contact`;
  - billhook a bramble thicket: `cue=CaneCut*`.
  Listen-check notes for Jenny.
