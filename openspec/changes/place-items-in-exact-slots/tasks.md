# Tasks

## Implementation status
Empty square zero now accepts drops; all occupied pack squares swap, including
matching stacks. Consumption reconciles metadata and loads validate references
atomically, migrating retired old-main stack references into gaps. A/Enter can
commit onto empty squares without permitting empty drag starts. Pack-row/chest
native suites and the follow-up editor compile passed; legacy
saves without positions retain packed ordering. Jenny considers pointer/reload
player checks complete as of 2026-10-03.

## Player acceptance
- [x] 1. Drag an item from the hotbar to any pack square; it lands in exactly that square, gaps stay, and the layout survives reload.
