# Tasks

- [x] 1.1 `Homestead::RainAmount` and `Overcast` beside `IsRainDay` and `IsRainingAt`, native-tested.
- [x] 1.2 `UHomesteadWeather`: streaks, cloud layer and rain sound following the camera; roofs as MPC shelters.
- [x] 1.3 `M_Rain`, `M_RainClouds`, `MPC_EstateWeather` and `SM_RainStreaks` (`Scripts/Terrain/build_weather.py`).
- [x] 1.4 Overcast lighting in `UpdateLighting`: sun, sky light, exposure, saturation, fog.
- [x] 1.5 Rain ripples on the landscape's standing water.
- [x] 1.6 CC0 rain loop (`Scripts/prepare_rain_loop.py`), credited.
- [x] 1.7 Verify in PIE on a rain day: falling rain, grey sky, darker exposure, sound, the blend in
  before 09:00 and out after 15:00, and rain stopping inside the standing room. _PIE day 2: 11:00 drizzle (Rain 0.3, Overcast 1), a 13:57 shower (Rain 1.0), overcast 0.59 at 08:47 before any rain and 0.42 at 15:17 after it, sound playing, the standing room at a shower. The standing room was only checked by a capture (indoor muffling isn't separately measured)._
- [ ] 1.8 GPU cost at 4K with rain on and off in a perf window.
- [ ] 1.9 Optional: a wet sheen on her hair, clothes and props.

## 3. Historical daytime recurrence

- [x] 3.1 Rain on two days in ten instead of every third day (Jenny, 2026-09-29: too often). A stable SplitMix64 hash of each ten-day block picks offsets 1 or 2 and 6 or 7: exactly 20% of days, rains 4-6 days apart, day 0 dry, and day 1 still the first rain. The 09:00-15:00 window, overcast, moisture and audio are unchanged, and there's no seed or save section. Old saves keep their plots' moisture, but their forecast from the current day on follows the new schedule. Native: counts over 10,000 days, gaps, all four offset pairs, whole days, save/reload.
- [ ] 3.2 PIE/package, superseded by section 4's spells:
  - days 0-5 stay dry;
  - the first spell rains from about 18:27 on day 6 (Sunday, Spring 7) through midnight to about 01:46, a night spell;
  - the next falls 15:07-16:52 on day 8, then 12:41-20:33 on day 9.
  Checked in 4.4.

## 4. Rain at any hour (Jenny, 2026-09-30: "let it randomize throughout the day/night cycle")

- [x] 4.1 `Simulation/HomesteadRain`: one possible spell per calendar day, from a hash of the day.
  - Any start hour; 1-8 h long; 0.5-1.5 h of cloud build-up and clearing.
  - Weighted by season (spring 5.0% of hours, summer 3.2%, autumn 6.0%, winter 6.8%; year 5.3%, against 5%
    before). Spells never merge, and they cross midnight and 06:00.
  - `IsRainingAt`, `RainAmount`, `Overcast`, `GroundWetness`, `IsRainDay`, `RainSpellOfDay`, `NextRainSpell`
    and `NextRainChange`. `Step` stops at a rain change.
  - No save data: the clock alone decides it, and old saves load with the new forecast from their hour on.
- [x] 4.2 Readers updated:
  - the landscape wetness reads `GroundWetness`;
  - night cloud dims the moon to 35%, keeping the sky light and exposure on their night floors;
  - the full-loop test and the UI gallery's `hud-rain`/`toast-rain` fixtures find the next spell (`NextRainSpell`).
  - Rain audio, plot watering and the HUD's Rain label were already hour-agnostic.
- [x] 4.3 Native tests:
  - spell lengths, cloud times, spacing and determinism over 20 years;
  - starts in every hour, about 43% at night and some in every season, crossing midnight and 06:00;
  - the wet share by season and year;
  - a reload during night rain;
  - one spell's cloud, rain, wetness and change points;
  - night rain watering a plot;
  - a step split at the rain's start.
- [ ] 4.4 PIE (low priority, after the night A/B slot):
  - the first spell (day 6, from 18:27 through midnight) at the town square and the meadow: rain, sound, the moon dimmed, and the lamp and hearth still readable;
  - the night A/B sheet's rain row;
  - the wet ground drying over the next four hours.
