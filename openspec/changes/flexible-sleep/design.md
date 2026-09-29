# Design

1. **Options are pure.** `Homestead::SleepOptions(hour, energy)` returns the choices, the default
   first, so native tests pin the policy and the controller only renders and cycles them.
2. **The choice lives in the controller** (`BedChoice`, `BedChoiceBed`). It's valid only while the
   same bed is focused and resets after sleeping, so the next visit starts on the default.
   `BedSleepHours()` returns the shown choice's length (the tests use it).
3. **Doze in the simulation.** `AdvanceGameHours` calls `DozeOff` the moment awake Energy reaches
   zero. That's six hours of sleep `Step`s at 6 Energy an hour (a bed gives 10), counted against the
   time asked. `DozeCount()` is a session counter, never saved; the controller toasts when it rises.
   Hunger during the doze can still fail her, as in any sleep.
4. **Sleep over 12 hours** (an early night from 18:00, 12.75 h) runs as two halves, since
   `Simulation::Sleep` accepts at most 12.
