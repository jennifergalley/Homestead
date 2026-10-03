# Tasks

## Implementation status
Empty square zero now accepts drops; all occupied pack squares swap, including
matching stacks. Consumption reconciles metadata and loads validate references
atomically. Pack-row/chest/map native suites and editor compile passed; legacy
saves without positions retain packed ordering. Pointer/reload acceptance remains
Jenny's playtest.

## Playtest queue
- [ ] 1. Drag an item from the hotbar to any pack square; it lands in exactly that square, gaps stay, and the layout survives reload.
