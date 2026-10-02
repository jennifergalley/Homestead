# Spec Delta

## ADDED Requirements

### Requirement: Auto-store moves carried goods onto matching chest stacks
Auto-store SHALL, in one step, move carried goods whose item already has a stack in the open chest onto those stacks, excluding tools, the lamp, water and garments, filling from below the hotbar row before the row, stopping when the chest is full, and changing nothing when nothing matches.

#### Scenario: Branches after gathering
- **WHEN** the chest holds Branches, she carries 5 Branches and 3 Stone, and she presses T
- **THEN** the chest holds 5 more Branches and she still carries the 3 Stone

#### Scenario: A nearly full chest
- **WHEN** the chest has room for 3 and she carries 6 matching Branches
- **THEN** 3 are stored, 3 stay in her pack, and she is told the chest is full

