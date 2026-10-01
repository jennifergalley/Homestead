# Proposal

## Why

Playtest feedback (docs/handoff/round-2.md, "Minimap/compass HUD trial"): the minimap's landmark
glyphs shrink to about 6 physical pixels at 720p, the Canvas clock is a bitmap font scaled to 42
units and reads soft beside the crisp Slate purse, and there's no way to tell which way she's facing
without looking down at the corner.

## What Changes

- **Minimap legibility.** Landmark badges, the north marker and her arrow are sized in HUD units with
  a floor in physical pixels (near badges 12.5 units / at least 11 px, off-crop ones 9.5 / 9 px), are
  centred on whole physical pixels, and keep clear of each other: near ones first, closest first, and
  a badge that would cover one already placed waits its turn. The 120 m crop and 220-unit disc are
  unchanged.
- **Compass trial.** A pine band with brass rules at the top centre of the HUD, level with the
  calendar, turns with the camera and shows 90 degrees either side: N (in red), E, S, W, the four
  intercardinals, ticks every 15 degrees, a brass lubber caret, and the Map tab's landmarks as tokens
  hanging beneath it at their bearings (nearest first, spaced, fading at the ends). It reads the map
  component's per-frame snapshot (camera yaw, her position, the landmark model), never the world. It
  keeps clear of the calendar, collapses when the view is too narrow, hides with the minimap, and is a
  protected region for the toast feedback checks.
- **Crisp clock.** The calendar's time is native Slate text (SHomesteadClock in SHomesteadHudScale)
  over the Canvas calendar panel, which keeps the dial, season, day and weather. No duplicate Canvas time.

## Impact

- `UI/HomesteadMapGeometry.h/.cpp` (portable: `AtLeastPhysical`, `SnapToPixel`, `SpacedCircles`,
  `BearingDegrees`, `RelativeDegrees`, `CompassOffset`) with native tests.
- `UI/SHomesteadMinimap.cpp`, new `UI/SHomesteadCompass.*`, `HomesteadMapComponent.*` (`CompassBox`,
  `IsCompassVisible`, the compass widget), new `UI/SHomesteadClock.*`, `HomesteadControllerHotbar.cpp`
  (the clock's root), `HomesteadHUD.cpp` (no Canvas time; the compass is protected).
- A trial: Jenny decides after playing whether the compass stays.
