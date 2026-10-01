# Design

1. **Sleep outcome is pure.** The simulation derives the one immediate result from hour and Energy:
   sleep until full Energy, capped at 06:00 for an overnight sleep; if already rested at night,
   sleep until 06:00; if already rested in daytime, expose no sleep action. Native tests pin this
   policy.
2. **No controller choice state.** There is no `BedChoice`, picker or confirmation dialog. The
   same focused bed immediately executes the simulation-derived outcome on E/A.
3. **Dawn exception is narrow.** Ordinary `Simulation::Sleep` still requires 0.25–12 hours. The
   bed may pass `dawnLimited=true` only for a positive sub-quarter-hour interval ending exactly at
   06:00; this avoids a zero-time action just before dawn while keeping direct simulation callers
   from admitting arbitrary short rests.
4. **Low Energy is nonfatal.** `Step` never dozes, fails or recovers a checkpoint because Energy is
   low. Below about 25% the sprint admission turns off; below about 10% walking slows and tools
   refuse with `Too tired`. The HUD owns the colour/pulse plus `Getting tired` / `Exhausted`
   warnings. Food and bed sleep are the only recovery actions.
5. **Mode-specific recovery.** Estate recovery/checkpoint load evaluates valid candidate saves by
   timestamp then simulation revision, selecting the newest rather than privileging an older
   `Recovery` slot over newer autosaves. Legacy woodland retains lethal hunger: recovery first
   prefers a valid sheltered `Recovery` checkpoint with hunger and Energy at least 20, then selects
   the newest eligible candidate to avoid loading a nearly starved outdoor autosave.
6. **Sleep over 12 hours** runs as halves if needed because `Simulation::Sleep` accepts at most 12.
