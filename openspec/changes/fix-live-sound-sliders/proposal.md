# Proposal

## Why

Jenny (docs/handoff/round-2.md, "Live sound sliders"): the sound sliders don't feel live. A mouse drag already previewed through `SSlider.OnValueChanged` and saved on release, but a d-pad or arrow-key step went through `MenuAdjustSetting` -> `PersistAudioVolume`, writing GameUserSettings on every 5% step and offering no way back.

## What Changes

- On the Sound tab, a d-pad or Left/Right step on a slider only previews (`MenuPreviewAudioVolume`): she hears the level change at once, and nothing is written yet.
- The stepped level is saved once (`MenuCommitAudioVolume`) when she confirms (A/Enter), moves to another row, region, tab or page, starts a mouse drag, or closes the book (`HideNativeMenu`).
- B/Esc while a step is pending puts the level back as it was, unsaved, and keeps Settings open; a second press leaves as before.
- Mouse drag is unchanged. Not in this change: rolling back a drag interrupted by Esc or closing the book (round-2's transactional follow-up), and a one-shot Effects preview.

## Impact

`UI/SHomesteadMenu.h`, `UI/SHomesteadMenuFocus.cpp` (StepAudio, commit, cancel), `UI/SHomesteadMenu.cpp` (commit on leaving the slider), `UI/SHomesteadMenuInput.cpp` (confirm and Back), `UI/SHomesteadMenuPages.cpp` (a drag commits a pending step first), `HomesteadControllerInput.cpp` (closing the book commits), `HomesteadController.h` / `HomesteadControllerSettings.cpp` (`AudioPersistWrites` for the automation), `UI/HomesteadNativeMenuTest.cpp`.
