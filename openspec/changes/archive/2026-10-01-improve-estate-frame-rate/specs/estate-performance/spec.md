# Spec Delta

## Purpose

Keeps the Estate smooth at Jenny's settings (4K, ray tracing on) without changing how it looks.

## ADDED Requirements

### Requirement: Performance changes keep the look
Frame-rate changes SHALL NOT visibly change what is drawn: the same scenery instances, cull distances, shadows and lighting at the manor, clear-out, farm, woods, drive, store and at night.

#### Scenario: Scenery batching
- **WHEN** the estate scenery is batched per cell instead of per kind
- **THEN** every instance that drew before still draws at the same distances, with the same shadows

### Requirement: The manor holds 60 fps at 4K
At Jenny's settings (Epic scalability, default resolution scale, ray tracing on, uncapped) the manor SHALL render at 60 fps or better at 4K and 1080p, measured from the engine's own frame times.

#### Scenario: Manor timing window
- **WHEN** the EstateSmoke route times the manor for 12 s
- **THEN** the median frame time is under 16.7 ms
