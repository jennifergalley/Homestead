# Spec Delta

## Purpose

Provides a legally admitted multi-track score that varies within and across no-save playtest launches without coupling music order to homestead saves.

## ADDED Requirements

### Requirement: The music catalog contains multiple verified tracks
The packaged game SHALL contain at least three playable music tracks with exact source identity, redistribution license, attribution, imported asset identity, and package inclusion verified. Unverified or mood-rejected candidates MUST NOT enter the catalog.

#### Scenario: Inspect a packaged candidate
- **WHEN** music assets and packaged attribution are audited
- **THEN** every catalog entry maps to one exact cooked asset and one complete license/credit record

### Requirement: A session uses a nonrepeating shuffle bag
Every valid catalog track SHALL play once in randomized order before any track repeats. A reshuffle MUST NOT place the just-finished track first when at least two alternatives exist.

#### Scenario: Play through one bag
- **WHEN** music advances for the number of valid catalog tracks
- **THEN** every track has started exactly once in that bag

#### Scenario: Cross a bag boundary
- **WHEN** the bag reshuffles after its final track
- **THEN** the next track differs from the just-finished track when another valid track exists

### Requirement: Fresh launches avoid the previous launch's last track
The most recently started track identity SHALL persist as a user-level audio preference independent from world saves. A new launch SHALL exclude that track from first choice when at least one other valid track exists.

#### Scenario: Relaunch without saving the world
- **WHEN** the app closes after starting one track and launches again without a homestead save
- **THEN** the first newly started track differs when the catalog has an alternative

### Requirement: Existing playback behavior remains coherent
The playlist SHALL retain current fade-in/fade-out, randomized silence, Music volume, mute, and ambience independence. Changing Music volume SHALL apply to the current and future tracks without resetting or reseeding the bag.

#### Scenario: Mute during a track
- **WHEN** Music volume becomes zero
- **THEN** score becomes inaudible without changing track order, while ambience follows its separate setting

### Requirement: Missing music fails explicitly and safely
Missing or invalid assets SHALL be skipped with diagnostics. One valid track SHALL use single-track playback without claiming random variety. Zero valid tracks SHALL leave ambience/effects operational and report music unavailable without repeated load spam.

#### Scenario: One track remains
- **WHEN** all but one catalog asset fail validation or load
- **THEN** the valid track can play with normal gaps/fades and no false shuffle assertion

### Requirement: Production order is fresh but tests are reproducible
Production shuffle entropy SHALL not derive from world seed, save slot, or a fixed startup constant. Tests SHALL be able to inject deterministic entropy to verify order properties without making packaged playback deterministic.

#### Scenario: Launch the same unsaved world twice
- **WHEN** two production sessions start with identical world state
- **THEN** playlist ordering is not forced to match by that world state

