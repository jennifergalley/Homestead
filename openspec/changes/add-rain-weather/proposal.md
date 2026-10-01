# Proposal

## Why

Jenny playtested a rain day: "I don't see any rain." The historical daytime-only schedule reported
Rain and wet ground while the sky stayed clear blue with sharp shadows, auto-exposure undid the
dimmer sun, nothing fell and there was no sound. Jenny's 2026-09-30 direction supersedes that
window: rain occurs at random times through the full day/night cycle, with seasonal weighting,
reload-stable scheduling and night-rain lighting.

## What Changes

- **Falling rain round the camera**: a single mesh of streak quads that `M_Rain` lays out as a
  world-anchored lattice wrapped round the camera. It has a dense near box and a fainter far box. The
  streaks slant with the wind (the meadow's heading), swell and thin with the rain's strength, fade
  with distance, and stop under roofed building pieces. The rain fades out when the camera is under
  any other roof.
- **Ripples**: rain rings on the standing water and soaked, trodden ground in the landscape.
- **Overcast**: a cloud layer drawn over sky pixels only (`M_RainClouds`). The sun dims to 12% with a
  wide, soft disc, so shadows soften and fade. The sky light lifts and is warmed back to neutral grey.
  Auto-exposure is held down by 0.8 EV and saturation drops to 72%. The haze thickens and lowers. It
  builds over the half hour before the rain and clears over the half hour after, with no pops.
- **Rain ambience**: a CC0 rain loop (Ylmir, OpenGameArt) scaled by the rain's strength and the
  Ambience setting. It's muffled by a low-pass and quieter indoors (under a building piece's roof or
  any overhead cover).
- The simulation owns the schedule: `Homestead::RainAmount(hour)` (a drizzle swelling into showers)
  and `Homestead::Overcast(hour)`, beside `IsRainDay` and `IsRainingAt`. Its full-cycle schedule
  is reload-stable and seasonally weighted; rain/overcast lighting must work at night as well as
  daylight.

## Capabilities

### New Capabilities

- `rain-weather`

### Modified Capabilities

None.

## Impact

- New: `HomesteadWeather.{h,cpp}`, `Scripts/Terrain/build_weather.py`, `Scripts/prepare_rain_loop.py`,
  `Content/SurvivalGame/Estate/Weather/*`, `Content/SurvivalGame/Audio/Ambience/RainLoop`.
- Changed: `AHomesteadWorld::BuildLighting`, `UpdateLighting` and `Tick`, `HomesteadSimulation.{h,cpp}`
  (`RainAmount`, `Overcast`), `build_landscape_material.py` (ripples), native tests, asset credits.
- No save change.
