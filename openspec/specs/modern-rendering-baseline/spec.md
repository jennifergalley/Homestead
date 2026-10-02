# modern-rendering-baseline Specification

## Purpose
Defines the renderer features and performance target that photorealistic characters and Homestead's generated world rely on for a modern PC presentation.

## Requirements

### Requirement: Modern PC rendering features are active
The Windows build SHALL render with DirectX 12 Shader Model 6, Virtual Shadow Maps, Lumen global
illumination and reflections backed by hardware ray tracing on supporting GPUs, and the skinning
settings MetaHuman requires. The editor and packaged game MUST NOT report missing project settings
for MetaHuman, Virtual Shadow Maps or Lumen ray-tracing data.

#### Scenario: Launch on the target PC
- **WHEN** the packaged game starts on the RTX 5080 target PC
- **THEN** the log shows SM6, Virtual Shadow Maps and hardware ray-traced Lumen in use, with no missing-settings warnings

#### Scenario: GPU without hardware ray tracing
- **WHEN** the game runs on a DX12 GPU without hardware ray tracing
- **THEN** Lumen falls back to software tracing and the game remains playable

### Requirement: Photoreal quality holds a measured frame budget
With the MetaHuman heroine on screen in the woodland, the game SHALL sustain 60 FPS at 3840x2160
output on the target PC using its upscaling setting, and SHALL report p95/p99 frame times in the
playtest evidence. Quality settings SHALL let the player trade fidelity for frame rate.

#### Scenario: Woodland walk at 4K
- **WHEN** the heroine walks through dense woodland at 4K output for two minutes in the packaged game
- **THEN** the recorded average is at least 60 FPS, and p95/p99 frame times are recorded with the evidence
