# Proposal

## Why

Jenny cannot correct feedback she has already submitted. Selected feedback
`jenny-mut5x32c-7715t2` requests a pencil that reopens the same entry in the form.

## What Changes

- Edit feedback title, description and optional screenshot in place.
- Keep its stable ID, release assignment, priority and removal identity.
- Support cancel, validation and explicit save failures without agent notifications.

## Capabilities

### New Capabilities

- `planner-feedback-editing`: Safe, agent-free editing of persisted feedback.

### Modified Capabilities

None.

## Impact

Planner extension data, HTTP routes, renderer and focused Node tests only.
No game code, save format, new dependencies or live-data test writes.

## Reuse research

Reuse the existing feedback form, validators, inbox/markdown helpers, screenshot
endpoint and Node test runner. Reuse the sibling accounting writer's temporary-file
and rename pattern. All are repository-owned; no external assets or licensing changes.
Custom gaps are the update endpoint, edit mode and safe read-modify-write persistence.

## Smallest useful result and first playable demonstration

In the existing planner, pencil a feedback card, change its text and screenshot,
then save without moving or rescheduling it. First delivery is this tooling slice
for `20261003-measured-02`; integrated acceptance follows the coordinator's refresh.
Gameplay changes and multiple screenshot galleries are outside scope.
