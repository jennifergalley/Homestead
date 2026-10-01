# Design

## Decision: no stored "last pass" day

Sleep (at most 12 hours a call) and DozeOff (6 hours) run their whole span inside one call and run the
pass on waking. An awake Step runs it as it crosses 6 AM. So whenever control returns to the game, the
pass for the current calendar day has run, and a reload can derive that from the hour alone. The save
format and `SimulationSaveVersion` stay as they are.

## Decision: day boundary tolerance

A sleep summed from hour-long steps can end at 29.9999999999994 instead of 30. `WeedDay` adds 1e-9 of a
day before flooring so that counts as 6 AM.

## Not changed

- `SkipToHourOfDay` (the HomesteadMorning playtest aid) still jumps without simulating, so no pass.
- The estate's overgrowth weed creep near remaining overgrowth (`CreepWeeds`) is a separate system.
