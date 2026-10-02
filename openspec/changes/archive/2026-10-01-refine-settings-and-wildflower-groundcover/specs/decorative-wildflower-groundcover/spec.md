# Spec Delta

## Purpose

Defines sparse deterministic decorative wildflowers that add woodland color without becoming resources, obstacles, or misleading gameplay targets.

## ADDED Requirements

### Requirement: Decorative wildflowers are visibly present but restrained
Generated woodland groundcover SHALL include sparse small wildflower clumps distributed in natural-looking deterministic pockets rather than uniform carpet coverage.

#### Scenario: Ordinary woodland walk
- **WHEN** the player traverses representative active chunks
- **THEN** occasional flower color is visible among grass and leaf litter without dominating the forest floor

#### Scenario: Rebuild the same chunk
- **WHEN** the same world/chunk is rebuilt
- **THEN** decorative flower mesh choice, position, yaw, and scale reproduce deterministically

### Requirement: Decorative flowers are not resources
Decorative wildflowers MUST NOT receive interaction focus, prompts, rewards, renewal state, save edits, overlap, navigation, or collision.

#### Scenario: Stand beside decorative flowers
- **WHEN** the player approaches or presses gather/clear near decorative flowers
- **THEN** they do not take focus, do not block movement, and do not grant or consume anything

### Requirement: Decorative flowers respect gameplay exclusions
Wildflower placement SHALL avoid protected spawn/home paths, streams and muddy banks, resources and their cleared sites, plots, structures, building previews/sites, and other reserved low-cover areas.

#### Scenario: Clear a resource site
- **WHEN** an interactive flower or other resource is gathered or cleared
- **THEN** decorative flowers do not appear in its reserved site after refresh

#### Scenario: Build or till nearby
- **WHEN** a structure or plot occupies a candidate flower position
- **THEN** the decorative instance is absent after the existing cover rebuild

### Requirement: Groundcover cost is bounded
Decorative wildflowers SHALL reuse admitted authored meshes/materials and existing chunk-owned HISM lifecycle with explicit density, cull, triangle, component, and cleanup bounds.

#### Scenario: Active-window transition
- **WHEN** chunks enter and leave the visual window
- **THEN** flower batches rebuild/teardown with existing cover ownership and do not accumulate actors or components

#### Scenario: Matched performance route
- **WHEN** selected and candidate builds traverse the same woodland route
- **THEN** the candidate records flower count/triangles and is rejected for material sustained cadence or transition regression
