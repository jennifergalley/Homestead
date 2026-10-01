# Design

1. **Sleep outcome is pure.** The simulation derives the one immediate result from hour and Energy:
   sleep until full Energy, capped at 06:00 for an overnight sleep; if already rested at night,
   sleep until 06:00; if already rested in daytime, expose no sleep action. Native tests pin this
   policy.
2. **No controller choice state.** There is no `BedChoice`, picker or confirmation dialog. The
   same focused bed immediately executes the simulation-derived outcome on E/A.
3. **Low Energy is nonfatal.** `Step` never dozes, fails or recovers a checkpoint because Energy is
   low. Below about 25% the sprint admission turns off; below about 10% walking slows and tools
   refuse with `Too tired`. The HUD owns the colour/pulse plus `Getting tired` / `Exhausted`
   warnings. Food and bed sleep are the only recovery actions.
4. **Newest valid recovery.** Any recovery/checkpoint load evaluates valid candidate saves by
   timestamp then simulation revision, selecting the newest rather than privileging an older
   `Recovery` slot over newer autosaves.
5. **Sleep over 12 hours** runs as halves if needed because `Simulation::Sleep` accepts at most 12.
