# Tasks

## 1. Correct Look Semantics

- [x] 1.1 Record mapped mouse-up and right-stick-up pitch deltas with inversion Off and On in the selected build, verifying the reported default-direction defect before changing source
- [x] 1.2 Correct the shared vertical-look multiplier so Off maps upward physical input to upward camera pitch and On reverses it, then verify targeted input tests cover both mouse and controller without changing yaw or look scale

## 2. Decouple Camera Preferences

- [x] 2.1 Load independently validated sensitivity and inversion values from a dedicated section in the resolved `GameUserSettings.ini`, verifying missing/invalid properties fall back independently to sensitivity 1.0 and inversion Off
- [x] 2.2 Persist sensitivity and inversion immediately through property-specific single-key writes with disk readback and runtime rollback, verifying unrelated synthetic graphics properties remain unchanged
- [x] 2.3 Stop world-save application from overriding camera preferences while retaining current save compatibility, then verify loading a save with conflicting legacy fields and starting a new woodland preserve the user-level values
- [x] 2.4 Add clear Settings success/failure feedback and verify a read-only synthetic settings file leaves both runtime behavior and disk bytes at their previous values
- [x] 2.5 Make the left-side camera sensitivity and inversion rows activate directly by pointer click or A/Enter and omit their right-sidebar Primary action, then verify each input changes the setting exactly once while details, focus synchronization, and unrelated Settings rows remain unchanged

## 3. Relaunch and Integrated Acceptance

- [x] 3.1 Add a guarded two-process camera-preference route using explicit synthetic config/user/save destinations, then verify direct left-row activation, absence of `Change setting`, toggle/cycle persistence without a world save, and rejection of any access to normal player settings
- [x] 3.2 Run strict OpenSpec validation, focused source/native tests, and SurvivalGameEditor Win64 Development build after integrating with the work-animation checkpoint, recording exact results
- [x] 3.3 Build one immutable Shipping candidate and verify ordinary mouse-up behavior, controller parity, quit-without-saving relaunch persistence, world-load independence, and existing work-animation camera routes before promotion
