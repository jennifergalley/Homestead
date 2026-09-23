# Proposal

## Why

The current default camera pitch is opposite the Settings label: with `Invert camera Y: Off`, moving the mouse upward makes the heroine look downward. Camera sensitivity and inversion are also serialized inside each homestead save, so changing inversion is lost when the player quits without saving and can be overwritten by loading another world.

## What Changes

- Make non-inverted look the default: mouse-up and stick-up look upward when `Invert camera Y` is Off; On reverses that direction.
- Persist camera sensitivity and Y inversion immediately in the game's existing user-settings INI, independently of world save, manual save, autosave, and quit path.
- Load camera preferences before applying a world save, and stop world saves from overriding them.
- Make the left-side camera sensitivity and inversion rows activate the setting directly by click or A/Enter, and remove their redundant `Change setting` action from the right sidebar.
- Reuse the existing exact-property `GameUserSettings.ini` persistence and verification pattern already used for vertical sync; no external dependency or new generalized settings framework is needed.
- Report and roll back a preference change if the settings file cannot be updated and verified.
- Add isolated settings-path tests plus an ordinary relaunch check covering mouse and controller pitch direction.

The smallest useful result is a fresh launch where mouse-up looks up with inversion Off. The first playable demonstration additionally activates inversion from its left-side row, observes no separate `Change setting` action, quits without saving the homestead, relaunches, and observes the same inversion choice. Full acceptance includes sensitivity persistence, world-load independence, write-failure rollback, controller parity, mouse/controller/keyboard row activation, and Shipping verification. Remappable controls, separate mouse/controller inversion, acceleration curves, broader audio/settings migration, and redesign of unrelated Settings actions are deferred.

## Capabilities

### New Capabilities

- `camera-look-preferences`: Correct default vertical-look semantics and durable user-level camera sensitivity/inversion preferences.

### Modified Capabilities

None.

## Impact

- `Source/SurvivalGame/HomesteadCharacter.cpp`: vertical-look sign semantics.
- `Source/SurvivalGame/HomesteadController.cpp/.h` and `Source/SurvivalGame/UI/SHomesteadMenu.cpp/.h`: preference load, exact-property persistence, direct camera-row activation, menu feedback, and removal of world-save override behavior.
- Existing `UHomesteadSave` camera fields can remain readable/writable for current save compatibility but cease to be authoritative.
- Settings tests use the already isolated `GameUserSettings.ini` destination and must preserve unrelated graphics properties byte-for-byte or property-for-property.
- Implementation should land after the current work-animation source checkpoint because both changes touch controller/character source and share Editor/package resources; it precedes the creek presentation round.
