# droppable-inventory-items Specification

## Purpose
Lets players remove carried possessions from the pack as persistent physical world pickups and recover them later without deletion, duplication, or hidden storage.

## Requirements

### Requirement: Carried possessions can be dropped
Valid carried item stacks and unequipped carried wearables SHALL offer `Drop...`. Stack drops SHALL allow a confirmed quantity from one through the selected stack count. Equipped wearables and chest-owned possessions MUST NOT offer Drop.

#### Scenario: Drop part of a material stack
- **WHEN** the player selects a carried stack of five Branch and confirms Drop quantity two
- **THEN** the pack retains three Branch and one world drop owns exactly two Branch

#### Scenario: Try to drop equipped clothing
- **WHEN** a wearable is currently equipped
- **THEN** Drop is unavailable until that wearable is unequipped into the pack

### Requirement: Dropping is one atomic grounded transaction
A drop SHALL commit inventory removal and world-drop creation together at a valid grounded point near/in front of the heroine. Invalid quantity, stale revision, failed/blocked placement, invalid coordinates, or the persistent-drop limit SHALL leave inventory and world state unchanged.

#### Scenario: No safe drop point exists
- **WHEN** nearby candidate ground is obstructed, unsafe, or otherwise invalid
- **THEN** Drop is rejected with clear feedback and the selected possession remains in the pack

#### Scenario: Cancel the quantity dialog
- **WHEN** the player closes Drop confirmation without committing
- **THEN** no inventory quantity or world state changes

### Requirement: World drops are identifiable and nonblocking
Each drop SHALL have a small grounded presentation distinct from natural resources, with focused item name and quantity. World drops MUST NOT block Pawn/camera, generate overlaps, affect navigation, take resource focus from a closer eligible target incorrectly, or move through physics.

#### Scenario: Approach a dropped stack
- **WHEN** the player enters interaction range of `Stone x3`
- **THEN** the drop becomes a clear pickup focus without behaving like a renewable stone resource patch

### Requirement: World drops can be recovered exactly once
Ordinary interaction with a focused drop SHALL move its complete item quantity or unique wearable back into the pack only when capacity and identity rules allow. Successful pickup SHALL remove the world-drop record atomically. Failed pickup SHALL leave it unchanged.

#### Scenario: Pick up a saved stack
- **WHEN** sufficient pack capacity exists and the player collects `Fiber x5`
- **THEN** exactly five Fiber enter the pack and that world drop cannot yield again

#### Scenario: Pack cannot hold the drop
- **WHEN** the complete dropped stack would exceed pack capacity
- **THEN** pickup is rejected, no partial quantity is silently taken, and the drop remains

#### Scenario: Recover a dyed garment
- **WHEN** a dropped dyed wearable is collected
- **THEN** its stable wearable identity, definition, and dye return to carried ownership unchanged

### Requirement: Drops persist without silent cleanup
World drops SHALL persist with stable identity, quantity/wearable identity, world position, and ownership across current-version save/load and active-region churn. The system SHALL enforce a bounded persistent-drop limit with explicit rejection; it MUST NOT silently despawn player possessions.

#### Scenario: Save away from a drop
- **WHEN** the player leaves the region, saves, reloads, and returns
- **THEN** the same drop appears once at its saved position with identical contents

### Requirement: Compatible ordinary stacks merge safely
Dropping a non-wearable item near a compatible player-created drop MAY merge quantities into that existing stable drop. Unique wearables MUST remain separate. A merge SHALL preserve exact total quantity and MUST NOT cross item identity or occur outside the bounded merge distance.

#### Scenario: Drop Branch beside Branch
- **WHEN** `Branch x2` is dropped beside an eligible `Branch x3` world drop
- **THEN** one compatible drop contains exactly five Branch and no second yield exists

### Requirement: Dropping integrates with inventory and hotbar state
The Drop action SHALL use the current quantity-stepper, input, focus, and stale-revision conventions. Dropping a carried tool referenced by the hotbar SHALL make that slot unavailable until the same tool type is carried again. Chest storage SHALL require transfer to pack before dropping.

#### Scenario: Drop the selected hatchet
- **WHEN** the player drops the carried Hatchet
- **THEN** its hotbar reference becomes unusable and world tool actions cannot use it until recovery
