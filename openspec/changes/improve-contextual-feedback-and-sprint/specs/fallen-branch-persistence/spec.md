# Spec Delta

## Purpose

Defines fallen branches as finite one-time woodland pickups whose removal persists, while leaving other renewable forage behavior intact and quiet.

## ADDED Requirements

### Requirement: Fallen branches do not regrow
Successfully gathering a fallen-branch patch SHALL grant its normal yield, mark the generated resource cleared, and prevent that patch from renewing.

#### Scenario: Gather fallen branches
- **WHEN** the player successfully gathers a ready fallen-branch patch
- **THEN** its branch yield is added once and the patch disappears permanently

#### Scenario: Return later
- **WHEN** any amount of game time passes after a fallen-branch patch was gathered
- **THEN** that exact generated resource key remains cleared and cannot be gathered again

### Requirement: Permanent branch removal persists
Fallen-branch removal SHALL use the existing bounded persistent generated-resource edit authority and MUST survive save/load, active-window churn, and world revisit.

#### Scenario: Save and reload
- **WHEN** the player saves after gathering fallen branches and reloads that world
- **THEN** the patch remains absent and inventory contains only the single earned yield

#### Scenario: Leave and revisit the chunk
- **WHEN** the patch's chunk leaves and later re-enters the active window
- **THEN** no visual, focus, collision, or second yield returns for that key

### Requirement: Rejected pickup remains atomic
If fallen-branch gathering is rejected, the patch MUST remain available and no inventory or persistent edit mutation may occur.

#### Scenario: Pack cannot accept the yield
- **WHEN** the carried inventory lacks capacity for the full fallen-branch yield
- **THEN** gathering is rejected atomically and the patch remains ready

#### Scenario: Missing knife or out of range
- **WHEN** the player lacks the required tool or is outside gather range
- **THEN** no branch quantity, clearing state, renewal state, or animation start changes

### Requirement: Other forage renewal is unchanged and silent
Berry, root, flower, reed, and other currently renewable forage SHALL retain their existing renewal timing and persistence, but successful gather feedback MUST NOT announce that renewal.

#### Scenario: Renewable forage gathered
- **WHEN** a renewable forage patch is successfully gathered
- **THEN** it schedules the existing renewal internally while the visible pickup toast reports only inventory gains
