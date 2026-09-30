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

## 3. Recurrence

- [x] 3.1 Rain on two days in ten instead of every third day (Jenny, 2026-09-29: too often). A stable SplitMix64 hash of each ten-day block picks offsets 1 or 2 and 6 or 7: exactly 20% of days, rains 4-6 days apart, day 0 dry, and day 1 still the first rain. The 09:00-15:00 window, overcast, moisture and audio are unchanged, and there's no seed or save section. Old saves keep their plots' moisture, but their forecast from the current day on follows the new schedule. Native: counts over 10,000 days, gaps, all four offset pairs, whole days, save/reload.
- [ ] 3.2 PIE/package: day 1 rains, days 3-5 stay dry, and the rain returns on day 6 or 7.
