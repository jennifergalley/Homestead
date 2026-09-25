# Spec Delta

## Purpose

Present a coherent natural woodland clearing while preserving the readable, controller-first homesteading world and its existing resource and building rules.

## ADDED Requirements

### Requirement: One directional light owns forward shading
At any time of day, exactly one directional light SHALL be the designated owner of forward
shading, translucency, water, and volumetric fog lighting, chosen deterministically (sun by day,
moonlight by night or an equivalent rule). The engine MUST NOT report that multiple directional
lights are competing for that role, and day/night sky and lighting behavior SHALL otherwise remain
as today.

#### Scenario: Play through dawn
- **WHEN** a new woodland starts at dawn and the player plays in the editor or the packaged game
- **THEN** no "Multiple directional lights are competing" warning appears on screen or in the log

#### Scenario: Day to night transition
- **WHEN** game time passes from day into night
- **THEN** the forward-shading owner switches without a visible lighting pop and without the competing-light warning
