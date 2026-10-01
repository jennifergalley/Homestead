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

- **No failure or fainting.** She can stay up all night. Low Energy never forces sleep, failure,
  death or checkpoint recovery: below roughly 25% sprint is unavailable; below roughly 10% she
  walks more slowly and tool work says `Too tired`. Eating or bed sleep restores her. The Energy
  meter changes colour and pulses, with short `Getting tired` and `Exhausted` warnings.
- **One-press bed sleep (Jenny, 2026-09-30).** E/A immediately sleeps; there is no confirmation
  dialog, choice picker or nap-hours control:
  - When not rested, `Sleep until rested` recovers Energy to full, but an overnight sleep stops at
    06:00 rather than advancing into the morning.
  - When already rested at night, `Sleep until morning` advances to 06:00.
  - When already rested during the day, the bed has no sleep verb.
- **Recovery follows hours slept**, 10 Energy an hour capped at full, at any hour.

## Capabilities

### New Capabilities

- `flexible-sleep`

### Modified Capabilities

None.

## Impact

- `Simulation/HomesteadSimulation.*`: the sleep outcome and Energy thresholds; `Step` does not
  fail/doze at low Energy. `BedSleepHours(hour)` is removed.
- `HomesteadController.*`: the bed prompt, one-press admission and wake messages, low-Energy movement/
  tool gates, and newest-valid recovery/checkpoint selection.
- HUD/menu: Energy colour/pulse plus concise threshold warnings.
- Tests: native low-Energy/no-failure, sprint/tool/walk thresholds, newest-valid recovery by
  timestamp/revision, and FullLoop sleep/input routes. No save version bump.
