# Proposal

## Why

Jenny selected smaller quit controls and a Back equipment choice for the October 4 afternoon build. Wearing no rucksack must not revoke the purchased inventory upgrade.

## What Changes

- Match quit dialog type and button scale to Settings.
- Add Back: Leather Rucksack / None beneath equipped slots, with a short capacity distinction.
- Keep the existing ownership, character visibility and save flags.

## Reuse research

Reuse Settings typography, Slate focus/popup patterns, Simulation::SetBackpackShown, the backpack save section and character visibility path. Existing repo code is sufficient; no external assets or new dependencies are needed.

## Smallest useful result and first playable demonstration

Open Settings to quit, then Inventory to choose None and Leather Rucksack; the visible bag changes while purchased capacity remains unchanged. Jenny's in-game acceptance is separate from branch implementation.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `safe-game-exit`: compact quit-control typography.
- `equipment-paperdoll-ui`: Back visibility choice independent of capacity.

## Impact

Menu dialog/equipment files and controller equipment presentation; focused native capacity/persistence checks. No save schema, placement, anatomy or package changes.
