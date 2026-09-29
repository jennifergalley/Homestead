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
2. **Version 13 writes the prefix; version 12 migrates by measuring each stock.** A version 12 stock is
   the rest of its line (the pack line, or a structure line after its seven fields), so the reader
   counts the values: every build of `main` that wrote version 12 had 40 items (checked across git
   history and the pending lane branches on 2026-09-28), and a wider line is still read exactly as long
   as this build knows that many items. Fewer than 40 is corrupt. Versions 7-10 keep their fixed widths.
   The equipment slots get the same width prefix in version 13 (version 12 had five).
   Verified against a real version 12 estate save written by `main` at `8762ba46` (embedded in
   `HomesteadSimulationTests.cpp`), and with a probe item appended to the catalogue.
3. **Width above `ItemCount` means a newer build.** Return `ResultCode::UnsupportedVersion` with a
   "comes from a newer build" message, not "corrupt".
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
