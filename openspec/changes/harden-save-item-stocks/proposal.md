## Why

`Simulation::Serialize` writes every item stock (the pack and each chest) as exactly `ItemCount`
integers with no count in front (`WriteStock` in `HomesteadSimulation.cpp`). `Deserialize` reads
back the width it expects for the save's version. So appending one `Homestead::Item`, which every
round does (the store's goods, the clearing yields, the spring flowers), changes the stock width
without changing the version. Saves written a build earlier then fail to parse and are reported as
"corrupt or incomplete". That is exactly how version 11 became unreadable ("item stocks changed
width without a version bump") and had to be retired.

Lanes are adding items right now (general store, MVP woodland, clearing), each one a silent save
break unless the orchestrator remembers to bump the version for it.

## What Changes

- Stocks are written with their width first: `<count> v0 v1 ... v(count-1)`.
- Reading accepts any `count` from 1 to `ItemCount`, fills the rest with zero, and rejects a larger
  count (a save from a newer build) with the existing "incompatible version" message rather than
  "corrupt".
- `SimulationSaveVersion` goes to 13 in the same change (the orchestrator's bump). Version 12 saves
  keep loading exactly as today (fixed width); versions 7-10 are unchanged.
- After this, appending an `Item` needs no save-version bump. Reordering or removing items still
  does (enums that name data stay append-only).

## Reuse research

- The existing reader already supports shorter stocks for older versions (`storedItems`: version 7
  lacks the machete, versions before 11 lack fur), so padding with zeros is the established
  migration; this change only makes the width explicit instead of implied by the version.
- The tagged trailing sections (`tools <ToolKindCount> ...`) already write their own count; stocks
  adopt the same convention.
- No external library needed.

## Smallest useful result and first playable demonstration

Save a game in a build, add an item in the next build (or use a branch that appends one), load the
save: it opens with the new item at zero instead of failing. Covered by a native test that
serializes with a shorter catalogue width and loads it.

## Capabilities

### New Capabilities

- `save-compatibility`: item stocks carry their width, so appended items keep saves loadable.

## Impact

- `Source/SurvivalGame/Simulation/HomesteadSimulation.{h,cpp}` (`WriteStock`, `ReadStock`,
  `Deserialize`, `SimulationSaveVersion`), `Tests/HomesteadSimulationTests.cpp`.
- One save-version bump, scheduled by the orchestrator between integration batches. Saves are
  disposable test data; version 12 saves still load.
- Touches `HomesteadSimulation.cpp`, which several lanes edit: land it in a quiet window or between
  rounds.
