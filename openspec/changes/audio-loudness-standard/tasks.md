# Tasks

## 1. Standard

- [x] 1.1 Categories, bands and the cue table in `Simulation/HomesteadAudioLevels.h`; call sites read gains from it
- [x] 1.2 `Scripts/Audio/Measure-Loudness.py` (measure, flag, regenerate the measurements header and docs table)
- [x] 1.3 Native gate `HomesteadAudioLevelTests` (proved failing on the old scythe gain and an uncategorised WAV)
- [x] 1.4 Docs: the standard, bands and table in docs/audio-checks.md
- [ ] 1.5 Skill rule "measure against ambience before shipping a cue" (Docs agent)

## 2. Verification

- [x] 2.1 PIE ear-proxy (2026-10-01): STRIKE_CUE billhook CaneCutA 0.90 at phase 1.252 (contact 1.250); pickaxe
      CraftStrikeA 0.12 then final 0.15 at 1.148/1.142 (contact 1.133); the scythe mowed nettles 570085 (swish
      at 0.25, compiled from the catalogue; no log line for it).
