# Spec Delta

## Purpose

Defines a natural forest-creek presentation that replaces the current canal-like sandy strips while preserving deterministic world generation and existing water gameplay.

## ADDED Requirements

### Requirement: Creek water surface reads as natural water
In ordinary gameplay views, with shaders fully compiled, the creek water surface SHALL show
natural shallow-water cues, such as depth-dependent tint, reflection or specular response, and
visible bed or edge transition. It MUST NOT present as a flat, uniformly saturated blue ribbon.
Water gameplay and creek geometry rules SHALL remain unchanged.

#### Scenario: Look along the creek in daylight
- **WHEN** the player views the creek along its course in daylight in the packaged game
- **THEN** the water shows depth or tint variation and light response rather than one flat saturated color from bank to bank

#### Scenario: Refill still works
- **WHEN** the player refills a watering can at the creek after the change
- **THEN** refill behaves exactly as before
