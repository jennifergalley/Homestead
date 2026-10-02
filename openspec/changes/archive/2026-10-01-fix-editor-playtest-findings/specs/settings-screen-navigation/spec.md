# Spec Delta

## Purpose

Defines Settings as a separate vertical preference screen with direct Esc/controller access, independent from the inventory and guidebook tab set.

## ADDED Requirements

### Requirement: Settings directional steps move one visible row
Each Up or Down step from the D-pad, left stick, or keyboard arrows on the Settings screen SHALL
move focus to the adjacent visible row in that direction, at every supported window size. Every
Settings row SHALL be reachable by directional input alone. Left and Right SHALL continue to adjust
the focused setting and MUST NOT move between rows.

#### Scenario: Step down from Save
- **WHEN** Settings is open with Save focused and the player presses D-pad Down once
- **THEN** Load latest save is focused and highlighted, not Game speed

#### Scenario: Reach every row
- **WHEN** the player presses Down repeatedly from the first row to the last
- **THEN** focus visits every Settings row exactly once, in visual order, with the list scrolling to keep the focused row visible

#### Scenario: Small editor or windowed viewport
- **WHEN** Settings is shown in a viewport about 1000 px wide, such as the default Play-In-Editor viewport
- **THEN** one Up or Down step still moves exactly one visible row
