# Tasks

- [x] 1.1 `Homestead::NightLightAt` schedule and native tests (no brightening after 18:50, night key, no pops, no moon glare, smooth sky blend, the old schedule's failure reproduced).
- [x] 1.2 `UpdateLighting` wired to it; CVar defaults 0.2 / 0.3 / -1 (the test checks them against `HomesteadWorldLighting.cpp`).
- [x] 2.1 Editor build; PIE at 18:00, 19:00, 21:00 and midnight. _Done 2026-09-30 (ray tracing off in the agent editor): 18:00 warm dusk; 21:00 in town nearly black; midnight dim blue moonlight that doesn't read as sunlight (`night_meadow_*`, `town_square_21h.png`). 21:00 may now be too dark: judge in 2.2._
- [ ] 2.2 Integration: packaged ray-traced fixed-camera captures (clear and rain) at 18:00, 19:00, 21:00 and midnight; check that lamps and the hearth read well. Tune the CVars if needed, then update the editor skill's default row.
- [x] 2.3 A/B for 2.2 (Water, 2026-09-30). **Decided: A** (Jenny, 2026-09-30 21:54). She compared `docs/night-ab/2026-09-30/sheet-1.jpg` and `sheet-2.jpg`: Old (her build: 2.0 / 0.6 / -2, the old lamp) | A | B | C, in editor PIE with ray tracing on, clear weather, at the town square 21:00, the meadow 21:00 and 00:00, the manor forecourt 19:00, the held lamp on the drive 22:00 and the manor room by the hearth 22:00. The defaults stay A (moon 0.2, sky 0.3, floor EV -1), pinned in `HomesteadNightLightTests`. The original note follows. 21:00 is three game hours after sunset (the day circle sets the sun at 18:00), so it's full night by design; only the level is in question. The displayed brightness of moonlit ground relative to an adapted daylight grey (`NightGroundKey`, 1 = daylight) is the same from 19:00 to dawn:

  | Set | `homestead.NightMoonLux` | `NightSky` | `NightMinExposure` | Moonlit ground | Ground key | Below daylight |
  | --- | --- | --- | --- | --- | --- | --- |
  | Old (Jenny: "the moon lights like the sun") | fixed 2 lux moon | 0.6 | -2 | 1.44 lux at 21:00 | 1.00 | 0 stops |
  | A (current default) | 0.2 | 0.3 | -1 | 0.20 lux | 0.16 | 2.6 stops |
  | B (proposed) | 0.3 | 0.4 | -1.25 | 0.30 lux | 0.29 | 1.8 stops |
  | C (brighter bound) | 0.3 | 0.5 | -1.5 | 0.30 lux | 0.34 | 1.6 stops |

  B keeps night clearly night (under a third of daylight grey; a bright full moon is 0.1-0.3 lux) while lifting the moonlit ground 1.8x. `NightSky` 0.4 lifts the walls and facades that face away from the moon: they have only the sky light, which is why the town square at 21:04 read nearly black in the RT-off editor. The lamp (about 4 lux at her hand, 1 lux at 10 m) and the hearth stay well above all three. Run each set by console at fixed cameras, clear and rain:
  - the town square facing the store from (-53300, 113500) at 21:00;
  - the meadow below the manor from (-33000, -62000) looking toward (-45000, -58000) at 21:00 and 00:00;
  - the manor forecourt at 19:00, with the held lamp at 22:00.
  Pick the set where the ground and buildings read as shapes and the lamp pool clearly stands out, then bake it into the CVar defaults and `HomesteadNightLightTests`.
