# Spec Delta

## Purpose

Defines an original compact season, day, time, and weather HUD card positioned in the upper-right for a quieter cozy-game presentation.

## ADDED Requirements

### Requirement: Time and weather use a compact upper-right card
The gameplay HUD SHALL present season, day, clock time, and current weather/daylight state in a compact upper-right composition rather than the current wide upper-left calendar panel.

#### Scenario: Clear daytime
- **WHEN** the world is in clear daylight
- **THEN** the upper-right card shows an original daylight/weather icon, season and day, and readable clock time

#### Scenario: Night or rain
- **WHEN** the world transitions to night or rain
- **THEN** the icon/state presentation updates from authoritative world state without changing the clock or weather simulation

### Requirement: The presentation is original Homestead UI
The card SHALL use Homestead's own Pine, gold, linen, typography, spacing, and original procedural/icon assets. It MUST NOT copy the reference game's artwork, font, exact component shapes, currency display, quest tracker, hotbar, or companion UI.

#### Scenario: Compare with reference direction
- **WHEN** the finished card is reviewed beside the user-provided inspiration
- **THEN** it shares only the compact information hierarchy and cozy readability while remaining visibly original to Homestead

### Requirement: Time state remains truthful
Displayed season, day, time, weather, and paused state SHALL be derived from the existing simulation and world state with no independent UI clock.

#### Scenario: Menu or planning pause
- **WHEN** the field book or placement planning pauses time
- **THEN** the card clearly indicates pause without advancing or inventing time

#### Scenario: Day rollover
- **WHEN** authoritative time crosses midnight into the next day or season
- **THEN** the card updates the correct day and season on the next frame

### Requirement: Responsive layout remains unobstructed
The upper-right card SHALL remain fully visible and non-overlapping at supported 1280x720 and 3840x2160 views, including toasts, meters, contextual prompts, failure, planning, menus, and appearance preview.

#### Scenario: Active toast and context
- **WHEN** a wrapped toast and nearby interaction prompt are visible
- **THEN** the time/weather card remains readable and all protected regions remain disjoint

#### Scenario: Device or resolution change
- **WHEN** prompt device or supported resolution changes
- **THEN** the card keeps its information, hierarchy, and viewport containment without truncation
