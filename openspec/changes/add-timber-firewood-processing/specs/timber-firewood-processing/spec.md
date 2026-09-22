# Spec Delta

## Purpose

Make deliberate mature-tree clearing supply useful construction wood and
prepared cookfire fuel through the existing inventory, recipe and persistence
systems without changing sapling gathering or world placement.

## ADDED Requirements

### Requirement: Timber and Firewood are distinct carried materials

The game SHALL represent Timber and Firewood as separate inventory items from
Branch and Fiber. Each unit SHALL consume one unit of the existing 120-unit
pack or chest capacity, participate in ordinary transfer and display behavior,
and preserve existing Branch semantics and numeric identifiers.

#### Scenario: New materials are stored in a chest

- **WHEN** the player transfers Timber or Firewood between a reachable chest and the pack
- **THEN** the requested quantity moves through the existing capacity-safe transaction
- **AND** the inventory and chest display the resulting exact quantities

#### Scenario: A transfer would exceed capacity

- **WHEN** a Timber or Firewood transfer would exceed the destination's 120-unit capacity
- **THEN** the transfer is rejected with a visible error
- **AND** neither source nor destination quantity changes

### Requirement: Mature-tree felling yields timber without changing saplings

A successful permanent clear of a generated mature tree SHALL produce six
Timber and four Branch, SHALL NOT produce Fiber, and SHALL retain the tree's
stable cleared-key persistence. Existing sapling gathering and clearing SHALL
retain their current Branch and Fiber yields.

#### Scenario: A mature tree is felled with pack capacity

- **WHEN** the player uses the Hatchet to clear a ready generated mature tree within interaction reach
- **THEN** the pack gains six Timber and four Branch
- **AND** the generated tree is permanently cleared under its existing stable key

#### Scenario: Mature-tree output does not fit

- **WHEN** the complete mature-tree output would exceed pack capacity
- **THEN** the action is rejected with a visible capacity error
- **AND** no output is added and the tree is not cleared

#### Scenario: A sapling is cleared

- **WHEN** the player clears an existing ready sapling with the Hatchet
- **THEN** its existing Branch and Fiber quantities are granted
- **AND** no Timber or Firewood is granted

### Requirement: The Hatchet splits Timber into Firewood

The recipe list SHALL expose a processing recipe that requires a carried
Hatchet and atomically converts one Timber into four Firewood. The Hatchet
SHALL be retained, and the recipe SHALL use the current recipe activation time
rather than introducing durability, a station, or a processing queue.

#### Scenario: Timber is split successfully

- **WHEN** the player activates the split-firewood recipe while carrying one Timber, one Hatchet and sufficient free capacity
- **THEN** one Timber is consumed and four Firewood are added
- **AND** the Hatchet remains in the pack

#### Scenario: Split output would exceed capacity

- **WHEN** consuming one Timber and adding four Firewood would exceed pack capacity
- **THEN** the recipe is rejected with a visible capacity error
- **AND** Timber, Firewood and Hatchet quantities remain unchanged

#### Scenario: Required material or tool is missing

- **WHEN** the player activates the split-firewood recipe without Timber or without a carried Hatchet
- **THEN** the recipe is rejected with a message naming the missing prerequisite
- **AND** no inventory quantity changes

### Requirement: Existing cookfires accept prepared firewood

A reachable cookfire SHALL accept either one Firewood or one Branch for four
game hours of fuel, subject to the existing 48-hour cap. The single mapped fuel
action SHALL consume Firewood first when both fuels are carried and SHALL fall
back to Branch when no Firewood is carried. The result message SHALL identify
which material was consumed.

#### Scenario: Firewood is available

- **WHEN** the player fuels a reachable cookfire while carrying Firewood
- **THEN** one Firewood is consumed before any Branch
- **AND** the fire gains four game hours without exceeding its cap

#### Scenario: Only a Branch is available

- **WHEN** the player fuels a reachable cookfire with no Firewood but at least one Branch
- **THEN** one Branch is consumed
- **AND** the fire gains the same four game hours as before this change

#### Scenario: Neither accepted fuel is available

- **WHEN** the player tries to fuel a reachable cookfire without Firewood or Branch
- **THEN** the action is rejected with a message naming both accepted fuels
- **AND** the fire's fuel time does not change

### Requirement: The complete processing loop persists

The current game version SHALL save and reload Timber, Firewood, mature-tree
cleared keys, chest quantities and cookfire fuel through the existing save
workflow. Incompatible older test-save formats SHALL reject transactionally
rather than partially materializing state; a fresh test profile is acceptable.

#### Scenario: Processing state is reloaded

- **WHEN** the player fells a mature tree, splits Timber, fuels a cookfire, stores any new material, saves and reloads
- **THEN** exact pack and chest quantities, positive cookfire fuel and the cleared mature-tree key are restored
- **AND** the felled tree does not rematerialize

#### Scenario: An incompatible test save is opened

- **WHEN** a save from an unsupported inventory schema is loaded
- **THEN** loading fails without partially replacing current simulation state
- **AND** the player can begin with a fresh current-version profile
