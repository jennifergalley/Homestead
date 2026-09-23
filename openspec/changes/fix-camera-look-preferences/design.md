# Design

## Context

See `proposal.md` for motivation. `AHomesteadCharacter::ApplyLook` currently multiplies vertical input by `-1` when inversion is Off and by `+1` when it is On; observed mouse behavior proves those semantics are reversed. `AHomesteadController` defaults inversion to Off, but camera sensitivity and inversion are copied into `UHomesteadSave`, written only when world progress is saved, and restored by `ApplySave`.

The project already resolves an isolated `GameUserSettings.ini` for normal, preview, and automation routes. Vertical sync demonstrates the supported safe pattern: use the resolved config branch, update one named property, read the file back, and restore runtime state when persistence cannot be verified. That existing engine/config facility is sufficient; no settings subsystem, dependency, or save migration framework is needed.

## Goals / Non-Goals

**Goals:**

- Make the Off/On label match actual mouse and controller pitch response.
- Load camera preferences from the resolved user-settings file before a world save is applied.
- Persist each camera preference at activation time with exact-property isolation and readback.
- Make each left-side camera setting row activate directly without a redundant right-side action.
- Keep current world saves readable while removing their authority over camera preferences.

**Non-Goals:**

- Separate inversion or sensitivity settings for mouse and controller.
- Input remapping, acceleration, smoothing, deadzone, or camera-collision changes.
- Moving audio volume, day length, appearance, or other current save fields into user settings.
- Changing save version solely to remove legacy camera fields.

## Decisions

### 1. Correct the shared vertical sign at the action boundary

`ApplyLook` will use a positive multiplier when inversion is Off and a negative multiplier when it is On. Mouse and right-stick mappings already feed the same normalized two-dimensional action path, so one correction keeps both devices consistent and leaves yaw, scale, deadzones, menu portrait orbit, and camera collision untouched.

**Alternative considered:** negate only the mouse mapping. Rejected because the Settings label describes camera Y globally and would leave controller behavior semantically inconsistent.

### 2. Store camera properties in the existing resolved user-settings INI

A dedicated `Homestead.Camera` section will hold `Sensitivity` and `InvertY`. Startup will read each property independently from the same resolved `GameUserSettings.ini` branch used by video settings. Missing or malformed values fall back independently to sensitivity `1.0` and inversion Off.

Each menu action will calculate a requested value and call a property-specific persistence function. The function writes only that key with Unreal's single-property file update, reads the disk file back, and commits the runtime field only after verification. On failure, runtime state remains or returns to the prior value and the existing notification surface reports the error.

**Alternative considered:** call `UGameUserSettings::SaveSettings` or flush the whole config cache. Rejected because the earlier vertical-sync implementation proved those paths can flush unrelated pending engine values.

**Alternative considered:** create a new `USaveGame` preferences slot. Rejected because the engine already supplies the correct per-installation settings destination and route isolation.

### 3. Keep legacy save fields compatible but non-authoritative

Current save serialization and validation will retain the camera fields so the save schema and current-version replay remain compatible. `ApplySave` will stop copying those values into the controller. New saves may continue writing the active preferences into the legacy fields until a future intentional save-version change removes them.

There is no automatic migration from the old world-save inversion field. Its meaning was used as a workaround for the reversed sign, so importing a stored `true` value into the corrected semantics would surprise the player by making the camera genuinely inverted. Installations without a valid user-level property therefore start with the corrected Off default.

### 4. Verify through an isolated two-process settings route

A focused camera-preference fixture will launch against an explicit synthetic `GameUserSettings.ini`. It will exercise actual mapped mouse and right-stick Y input, toggle and cycle the Settings rows, reject a read-only write with rollback, preserve sentinel graphics properties, load a world save with conflicting legacy values, and launch a second process to prove persistence without a world save.

The first ordinary playable demonstration will repeat the mouse direction and quit-without-saving relaunch path in an immutable Shipping candidate. It will not claim physical mouse feel from synthetic axis input alone.

### 5. Treat the camera rows as direct controls

The native menu will classify the legacy Settings rows for camera sensitivity and inversion as direct actions. Their content buttons will invoke the existing controller activation once on mouse click or A/Enter. `BuildDetails` will omit the generic Primary action for those rows, so directional navigation cannot move focus into a redundant `Change setting` button. Selecting either row still updates the details text and remembered selection normally.

Save, load, new-woodland, quit, video, audio, day-length, inventory, appearance, and other legacy rows keep their current interaction in this change. This is intentionally narrower than a general Settings-page redesign.

**Alternative considered:** make every immediate Settings row direct in the same patch. Rejected for this focused correction because it changes unrelated accepted navigation behavior and would require broader directional-navigation reconciliation.

### 6. Serialize integration with the active work-animation milestone

The camera correction touches `HomesteadCharacter.cpp` and `HomesteadController.cpp`, which are active work-animation surfaces. Implementation begins after that source milestone is checkpointed, then reuses the existing UBT/shader/build caches. The creek round is source-independent at planning level but final package/Editor resources remain serialized.

## Risks / Trade-offs

- **[Enhanced Input reports device Y with unexpected sign in one route]** -> Record before/after pitch for actual mapped mouse and stick inputs in Editor and Shipping; do not infer correctness from the multiplier alone.
- **[Legacy saved inversion no longer applies]** -> Intentionally prefer the corrected Off default because legacy `On` commonly represented the workaround; document the behavior and keep saves otherwise compatible.
- **[A config write partially changes unrelated properties]** -> Use one-property updates, sentinel-file comparison, and disk readback; reject the candidate if unrelated settings move.
- **[Automation accidentally touches Jenny's normal settings]** -> Require explicit synthetic config and user directories before the focused route will mutate any preference.
- **[Direct activation fires twice or keyboard focus jumps]** -> Test pointer click, Enter, gamepad A, resulting setting delta, focused region, and absence of the camera rows' Primary action.
- **[Camera fix changes animation visual-route framing]** -> Horizontal routes are unaffected; rerun the targeted work-animation camera checks after integration because shared controller/character source changed.

## Migration Plan

1. Record actual mapped mouse and stick pitch direction in the current selected build.
2. Correct the shared sign and add isolated immediate persistence/load logic.
3. Retain legacy world-save fields but remove their application authority.
4. Make the two camera rows direct controls and remove only their redundant right-sidebar action.
5. Run focused config, input, navigation, save-conflict, read-only, and two-process tests, then rebuild the Editor target.
6. Package one immutable candidate and verify ordinary mouse behavior plus quit-without-saving persistence.
7. Roll back by reverting the source change and retaining the prior selected packaged build; no world-save reset is required.
