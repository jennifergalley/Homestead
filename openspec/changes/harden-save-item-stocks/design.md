## Context

`WriteStock` (`HomesteadSimulation.cpp`) streams `' ' << value` for all `ItemCount` entries.
`ReadStock(stream, stock, stored[, capacity])` reads `stored` values, where `Deserialize` derives
`stored` from the version (`Item::Machete` for version 7, `Item::Fur` before version 11, else
`ItemCount`). The pack stock is on line 2 of the payload; each chest's storage follows its structure
line.

## Decisions

1. **Count prefix, not item keys.** Enums that name data are append-only by rule, so a width is
   enough and keeps saves compact and the reader simple. Keys would also survive reordering, but
   nothing reorders items.
2. **Version 13 writes the prefix; 12 and older read as before.** `ReadStock` gets a
   `bool prefixed` (or the version) so the fixed-width path stays byte-for-byte the same for old
   saves.
3. **Width above `ItemCount` means a newer build.** Return `ResultCode::UnsupportedVersion` with
   the existing "incompatible version" message so Jenny sees "start a new game", not "corrupt".
4. **Width 0 is invalid** (a stock always has at least the first item slot).

## Lanes and ownership

One small change, one owner (Architecture Agent or whichever lane the orchestrator picks), in a
window when no lane has uncommitted edits to `HomesteadSimulation.cpp`. The orchestrator makes the
version bump call.

## Risks

- A lane that appended an item on an older base and merges after this lands: no conflict in
  behaviour (its item is simply appended); only the version constant may conflict textually.
- Test fixtures in the in-game routes that compare serialized strings (`HomesteadNativeMenuTest`,
  `HomesteadForageRenewal`) compare fresh serializations, so they're unaffected.
