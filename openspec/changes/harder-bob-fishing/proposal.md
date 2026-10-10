# Proposal

## Why
Jenny's planner card "Fishing is too easy": fish come too often and too fast, the timing is muscle memory, and the cue is a zoomed panel on the umber menu background instead of the bob on the water (she wants it like Coral Island and Hades).

## What Changes
- Simulation: a hooked fish gets away about 60% of the time, mid-fight, whatever the player does. The bite wait, the number of strikes and the waits between them are random and longer, and false nibbles/tugs punish clicking early.
- Presentation: the umber `SHomesteadFishing` panel is retired. A visible float rides the water; it sinks whenever a click is needed, while rings collapse onto it over the reaction window. Nibbles shiver it and send out a small ripple.

## Capabilities
### New Capabilities
- `bob-fishing`: harder, unpredictable fishing read from the float on the water.
### Modified Capabilities
None.

## Impact
`Simulation/HomesteadFishing.*`, fishing native tests, controller fishing presentation, character tackle, a new world-anchored ring overlay, a float prop. No save, enum or placement changes (the cast is transient).

## Smallest useful result and first playable demonstration
Cast at water: the float lands and waits a random while, shivers once or twice, then sinks with rings closing on it. Click in time and the fight starts; the float resurfaces, rests a random while, sinks again. Most fish still get away.
