# Spec Delta

## Purpose

Make the existing Fiber source legible as an actual plant beside the creek so the first hatchet can be crafted by ordinary exploration.

## ADDED Requirements

### Requirement: Bank reeds are visually identifiable and reachable
Ready reed patches SHALL present distinct upright reed-like stalks visible at ordinary third-person creek-bank distance, not indistinguishable grass tufts. An ordinary new woodland SHALL provide reachable reed patches along the stream without a hatchet, teleport, debug inventory injection or hidden quest marker.

#### Scenario: Follow the creek from a fresh clearing
- **WHEN** a new player reaches the creek bank on foot at the normal gameplay camera
- **THEN** at least one seeded patch reads as reeds against grass and other bank cover and can be approached without blocking water or tree collision

### Requirement: Gathering reeds supplies real Fiber
Gathering a ready reed patch SHALL use the existing authoritative harvest to grant Fiber in the pack with its existing yield and renewal/save behavior. The environment MUST show depleted/ready state without relying on explanatory renewal notices.

#### Scenario: Obtain first hatchet materials
- **WHEN** a player approaches a ready reed patch carrying the starter Knife and gathers it
- **THEN** Fiber is added once without requiring a hatchet, can be spent by the real hatchet recipe, and the same patch cannot immediately grant Fiber twice

#### Scenario: Save and revisit
- **WHEN** the world is saved after gathering and reopened
- **THEN** the patch retains its correct authoritative state and visual identity; it does not become generic grass or duplicate rewards
