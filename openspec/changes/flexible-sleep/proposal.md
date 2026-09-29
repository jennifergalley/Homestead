# Proposal

## Why

Jenny: "Please don't enforce a bedtime on me by time, only by exhaustion. If I end up becoming a night
owl, then my character should be able to sleep through the day to make up for it. Or if I want to get
my sleep schedule back on track, I should be able to go to sleep before I'm fully tired so I can wake
up rested in the morning."

The bed picked the sleep length from the clock: by day it was a nap of at most two hours ending by
18:00, and at night it ran to 06:45 or eight hours. A night owl couldn't recover in the day. Running
Energy out failed the game like starving.

## What Changes

- **No bedtime by the clock.** She can stay up all night. Running out of Energy no longer fails her:
  she dozes off where she stands for six hours of rough sleep at a slower recovery rate and wakes
  stiff and only part rested, with a toast saying so. Only hunger fails her. The doze can't happen
  while a menu is open, because time is paused there.
- **Choose at the bed.** The prompt shows one sleep choice with its wake time; Up/Down (D-pad) steps
  through the others and A (E) confirms:
  - "Sleep until morning (wake 06:45)" in the evening and at night (18:00-05:00), for getting back on
    schedule. It's the default then.
  - "Sleep until rested (wake ~HH:MM)": her Energy deficit at 10 an hour, 1-10 h. It's the default
    by day, so a night owl sleeps through into the afternoon. It's folded into "until morning" when
    they'd wake within 45 minutes of each other.
  - "Nap 1 h": left out when "until rested" is already that short.
- **Recovery follows hours slept**, 10 Energy an hour capped at full, at any hour.

## Capabilities

### New Capabilities

- `flexible-sleep`

### Modified Capabilities

None.

## Impact

- `Simulation/HomesteadSimulation.*`: `SleepOptions`, the `Exertion` sleep and doze constants,
  `Simulation::DozeCount`, Energy no longer fails `Step`, `AdvanceGameHours` dozes. `BedSleepHours(hour)` is removed.
- `HomesteadController.*`: the bed prompt, the choice cycling, the wake messages and the doze toast.
- `SHomesteadMenu.cpp`: the failure text says food only.
- Tests: native `SleepOptionPolicy`, the doze cases, and FullLoop's outdoor sleeps (now night after
  night). No save change (SimulationSaveVersion stays 12).
