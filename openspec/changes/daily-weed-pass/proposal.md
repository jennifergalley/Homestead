# Proposal

## Why

Jenny (2026-09-30): "Weeds are also growing a little too quickly in my crops for my liking. They should
regrow only when I sleep (or if I don't sleep for a prolonged period, once every day - no getting around
weeds), so I have something to clear in the morning and I don't feel like I have to clear them twice a
day or more."

Plot weeds crept up every game hour (0.009 an hour, 0.216 a day), so a square weeded at breakfast
showed tufts again by evening.

## What Changes

- Plot weeds come up in one pass a day, never in between.
- Awake at 6 AM (the day boundary, `DayRolloverHour`), the pass runs then. Asleep or dozing across 6 AM,
  it runs when she wakes. Either way each calendar day gets exactly one pass.
- Each pass adds `CropCare::DailyWeeds` (0.2) to every plot, a little under the old daily total, so a
  single pass shows tufts (`VisibleWeeds` 0.125) and two passes stay below `WeedyFrom` (0.5).
- Simulation only (`Crops::WeedDay`, `Crops::GrowDailyWeeds`, `Simulation::Step/Sleep/DozeOff`). Saves
  are unchanged: after every call the current day's pass has already run, so nothing extra is stored,
  and old saves keep their weeds.
