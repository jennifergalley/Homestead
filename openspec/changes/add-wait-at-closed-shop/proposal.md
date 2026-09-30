# Proposal

## Why

Jenny reached town at 7 PM and the general store was shut until 8 AM, with nothing to do but walk away. She asked for a safe "Wait until opening" at the closed door.

## What Changes

- The closed door's cue offers the wait with the real opening time and how long it is: `Closed - opens at 8 AM   [A] Wait until 8 AM (13 h)`.
- The first A (E) asks, `Wait 13 h until 8 AM?   [A] Wait   [B] Cancel`; a second A answers; B (Esc), walking away or eight seconds cancel. A second press within 0.35 s is ignored, so a doubled press can't confirm.
- `Simulation::WaitForShop` passes the time with the ordinary `AdvanceGameHours`: crops, weather, fires, vitals, the 6 AM sell-down and dozing off when worn out all happen as if she'd waited. It's refused, before any time passes, when the shop is open, she isn't by it, the calendar limit would stop the clock, she'd collapse from hunger first, or she'd doze off from exhaustion before it opens (both tried on a copy). Afterwards it checks the shop is open and says so, or reports that it isn't.
- `HoursUntilOpen(shop, hour)` is the reusable part for other places later.

## Impact

- `Simulation/HomesteadShops.h/.cpp`, `HomesteadSimulation.h` (the declaration), `HomesteadShopFlow.cpp`, `HomesteadController.h/.cpp` (Back cancels a pending question), `Tests/HomesteadEconomyTests.cpp`.
- No save change. Closed days (Sunday) aren't on main yet; when the calendar lands, `HoursUntilOpen` must skip them.
