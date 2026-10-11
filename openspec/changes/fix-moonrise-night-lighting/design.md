# Design

- `Simulation/HomesteadNightLight` gives a constant moon altitude (`moonAltitudeDegrees`, 50) and
  `night = 1 - daylight`; the moon lux is `moonGroundLux * night / sin(altitude)`. Dusk and dawn
  ride the existing smoothstep on sun elevation, so they stay smooth.
- `HomesteadWorldLighting.cpp` sets the moon rotation once in `BuildLighting` (`UpdateLighting`
  only moves the sun) and hides the moon disc.
- Removed the unused `AppliedMoonRotation` member from `HomesteadWorld.h`. No save change.
- Darkness is one number (`homestead.NightMoonLux`) if Balance or Jenny want it brighter or darker.
