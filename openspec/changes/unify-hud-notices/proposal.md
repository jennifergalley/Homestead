# Proposal

## Why

Jenny selected `unify-hud-notices` for `20261003-measured-02`: resource, refusal and save feedback should be concise and non-overlapping.

## What Changes

Use the existing parchment notice treatment for existing pickup gains as well as refusals/save notices, with a shared bounded top-centre stack. Keep routine success quiet.

## Capabilities

### New Capabilities

- `unify-hud-notices`: the existing focus/notice stack and first-minute controls contract.

### Modified Capabilities

None; quiet resource feedback remains unchanged.

## Reuse research

Reuse `HomesteadNoticeStyle`, Canvas font measurement/wrapping, `RecentPickups` gain detection and existing notice/gain clocks. Installed Unreal and existing project components suffice; no new asset, external dependency or license/access requirement.

## Smallest useful result and first playable demonstration

Gather a resource, provoke a refusal and save: existing feedback uses matching slips without colliding with focus or vitals. Source delivery precedes integrated compilation and Jenny's visual acceptance.

## Impact

HUD presentation and removal of the separate floating pickup renderer. Preserve all named refusal/save/Travel messages, inputs, delays, resource gain rules and hint learning. No simulation, catalogue or save changes.
