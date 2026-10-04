# Proposal

## Why

Jenny's selected feedback `backlog:jenny-mut6p856-a7gysc`: the chest name is too small on the interaction hint. Target: `20261003-measured-02`.

## What Changes

Make the focused chest's name visibly larger and stronger than its keyed Open action, without clipping either.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `chest-storage-interaction`: prominent, resolution-safe chest names in world focus hints.

## Reuse research

Reuse `FocusTitle`, `ChestDisplayName`, Canvas font measurement/wrapping, and `HomesteadNoticeStyle`. Existing project code and installed Unreal facilities suffice; no new assets, dependencies or licensing/access requirements. Only the title hierarchy and bounded focus-card layout need changing.

## Smallest useful result and first playable demonstration

Face a named chest: its name leads the existing card, with E/A Open readable below it. First delivery is a source checkpoint; integrated compilation and Jenny's 720p/4K check remain acceptance gates.

## Impact

HUD presentation only. No rename, storage transaction, save, hint-learning or input changes. Other selected notice work is tracked in `unify-hud-notices`; unrelated work is excluded.
