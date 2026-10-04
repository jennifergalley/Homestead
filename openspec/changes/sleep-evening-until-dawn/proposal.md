# Proposal

## Why

Jenny's feedback `jenny-mut5v5yo-9f8fpx` asks evening bed sleep to last through the night rather than stop when energy fills.

## What Changes

- Sleep beginning at the earlier of 6 PM and sunset ends at the earlier of 6 AM and the following sunrise.
- Preserve ordinary daytime rest, bed reach checks, authoritative time stepping and sleep autosaves.
- First playable result: evening bed interaction wakes at dawn even from nearly full energy. Jenny owns integrated acceptance; animations are deferred.

## Capabilities

### New Capabilities

### Modified Capabilities

- `manor-arrival`: add the evening sleep timing contract.

## Impact

Reuse `BedSleepOption`, `Simulation::Sleep`, `Step` and existing bed presentation. The current lighting sun path crosses the horizon at 6 AM/6 PM; share that timing rather than invent seasonal astronomy. No new assets, external dependency or license admission. Native sleep tests and the editor module compile cover branch delivery.
