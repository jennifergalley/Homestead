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
