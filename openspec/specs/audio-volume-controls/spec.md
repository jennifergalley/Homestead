# audio-volume-controls Specification

## Purpose
Defines continuous, accessible, immediately applied and independently persisted controls for music, ambience, and effects volume.

## Requirements

### Requirement: Audio volumes use sliders
Music, Ambience, and Effects SHALL each use a labeled slider with a visible percentage rather than a button that cycles fixed values.

#### Scenario: Pointer drag
- **WHEN** the player drags an audio slider with the pointer
- **THEN** the value and corresponding audio level update continuously between 0% and 100%

#### Scenario: Keyboard or controller adjustment
- **WHEN** an audio slider is focused and the player presses left/right
- **THEN** the value changes in predictable bounded increments and the percentage label updates

#### Scenario: Minimum and maximum
- **WHEN** adjustment reaches 0% or 100%
- **THEN** further input remains clamped and no invalid volume is applied

### Requirement: Audio preferences persist outside world saves
Audio volume changes SHALL persist as user-level game settings without requiring a world save, and loading, resetting, or switching homesteads MUST NOT overwrite them.

#### Scenario: Quit without saving world progress
- **WHEN** the player changes all three sliders, quits without saving the homestead, and relaunches
- **THEN** every audio slider and runtime audio component uses the saved user preference

#### Scenario: Load conflicting legacy world values
- **WHEN** a current world save contains different legacy volume fields
- **THEN** the user-level Music, Ambience, and Effects preferences remain active

### Requirement: Persistence is isolated and recoverable
Saving one audio slider SHALL modify only its requested user-settings property, verify disk readback, preserve unrelated settings, and restore the previous runtime value with a clear error if persistence fails.

#### Scenario: Read-only settings file
- **WHEN** the user adjusts a slider while the resolved settings file is read-only
- **THEN** the slider/runtime audio return to the previous value, unrelated bytes remain unchanged, and an error is shown

#### Scenario: Missing or malformed property
- **WHEN** startup encounters a missing, non-finite, malformed, or out-of-range audio property
- **THEN** only that property uses its safe default and no world save is invalidated

### Requirement: Slider editing preserves menu behavior
Pointer capture, controller editing, keyboard focus, modal traps, back behavior, pause, and prompt-device switching SHALL remain stable while sliders are adjusted.

#### Scenario: Back during slider editing
- **WHEN** the player finishes or cancels slider editing and presses back
- **THEN** no stuck capture or hidden edit remains and Settings closes normally
