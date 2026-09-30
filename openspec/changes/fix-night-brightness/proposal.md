## Why

Jenny's 2026-09-29 playtest: around 9 PM the Estate visibly brightens and the moonlight reads like
sunlight. The moon was held at a fixed 2 lux while it climbed from the horizon at dusk to 46 degrees by
21:00, so the moonlit ground brightened about threefold after dark. Auto-exposure could adapt down to
EV100 -2, so the camera brought it back up to a daylight grey, and a 0.6 night sky light lifted the rest.

## What Changes

- `Homestead::NightLightAt` (Simulation, native-tested) holds the moonlit level ground at 0.2 lux from
  dusk to dawn. The moon's intensity compensates for its altitude, capped at 1 lux while it is low, and
  it fades in over its first 3 degrees.
- The night sky light drops from 0.6 to 0.3, blended smoothly with daylight.
- The night auto-exposure floor rises from EV100 -2 to -1, so moonlit ground sits about 2.6 stops
  under an adapted grey. Night reads as night, while lamps and the hearth read bright.
- `UpdateLighting` feeds the schedule to the moon, the sky light and exposure. The console variables
  `homestead.NightMoonLux`, `NightSky` and `NightMinExposure` keep working, with the new defaults.
  Noon is unchanged.

## Impact

- `Simulation/HomesteadNightLight`, `HomesteadWorldLighting.cpp`, `Tests/HomesteadNightLightTests.cpp`.
- Visual sign-off needs Integration's packaged ray-traced fixed-camera captures.
