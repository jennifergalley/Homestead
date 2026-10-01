# Design

1. **Sleep outcome is pure.** The simulation derives the one immediate result from hour and Energy:
   sleep until full Energy, capped at 06:00 for an overnight sleep; if already rested at night,
   sleep until 06:00; if already rested in daytime, expose no sleep action. Native tests pin this
   policy.
2. **No controller choice state.** There is no `BedChoice`, picker or confirmation dialog. The
   same focused bed immediately executes the simulation-derived outcome on E/A.
3. **Doze in the simulation.** `AdvanceGameHours` calls `DozeOff` the moment awake Energy reaches
   zero. That's six hours of sleep `Step`s at 6 Energy an hour (a bed gives 10), counted against the
   time asked. `DozeCount()` is a session counter, never saved; the controller toasts when it rises.
   Hunger during the doze can still fail her, as in any sleep.
4. **Sleep over 12 hours** runs as halves if needed because `Simulation::Sleep` accepts at most 12.
