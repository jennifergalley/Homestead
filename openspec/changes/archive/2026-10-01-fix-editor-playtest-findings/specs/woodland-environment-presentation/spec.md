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

### Requirement: Sun shadows move smoothly with the time of day
As game time advances, sun and moon shadows SHALL move continuously, with no visible jumps in shadow
position, and the movement MUST NOT cause frame-time spikes. Where hardware ray tracing is
unavailable, the game MAY fall back to stepped shadow updates that keep frame pacing smooth.

#### Scenario: Watching shadows at a low sun
- **WHEN** the player stands still in the woodland near dawn or dusk for a minute of real time
- **THEN** tree and heroine shadows drift smoothly rather than jumping every few seconds

#### Scenario: Frame pacing while the sun moves
- **WHEN** the 4K presentation route runs capped at 60 FPS with the sun moving
- **THEN** its p99 frame time is no worse than the stepped-sun baseline and no shadow update produces a stall

#### Scenario: Shade stays clean while moving
- **WHEN** the heroine walks through dappled canopy shade with the camera following
- **THEN** shadows on the ground and on her clothes don't crawl, sparkle or break into visible pixel noise, and static shade is no noisier than the Virtual Shadow Map look
