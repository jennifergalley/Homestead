# Spec Delta

## Purpose

Keep existing world feedback concise, consistently styled and clear of interaction hints and other HUD regions.

## ADDED Requirements

### Requirement: World notices share the parchment notice style at the top centre
The focus actions and the toast SHALL be drawn as parchment notices in the field book's NoticeCard
style at the top centre of the HUD, the toast stacked under the focus actions when both show, with
errors in rust ink and their existing priority.
Existing resource gain lines SHALL use the same treatment below any explicit notice, with their existing aggregation, visibility and lifetime. Routine success SHALL NOT add a tutorial toast or prematurely erase an active refusal, save confirmation or first-visit Travel unlock notice. Titles, keyed actions and messages SHALL fit the available HUD region at 720p and 4K.

#### Scenario: Focus and a toast together
- **WHEN** she faces a bramble with a knife and a "+3 Branch" style message is showing
- **THEN** the "Clear with Knife" card sits under the compass and the message slip sits under it, neither overlapping

### Requirement: The controls strip is a first-minute reminder
The top-left controls strip SHALL show only for its first 60 seconds of real time on screen after
boot, a new game or "Reset action hints", and time in a paused book or shop SHALL NOT count.

#### Scenario: Reading the book
- **WHEN** she opens the book 20 seconds after starting and reads for five minutes
- **THEN** the strip is still there for 40 more seconds when she closes the book

#### Scenario: Reset action hints
- **WHEN** she chooses Reset action hints in Settings
- **THEN** the strip shows again for a full minute and the keyed focus hints come back
