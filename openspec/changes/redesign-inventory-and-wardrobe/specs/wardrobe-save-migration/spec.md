# Spec Delta

## Purpose

Persist current-version wardrobe ownership and inventory layout reliably while
making incompatible disposable test saves explicit instead of hiding failures.

## ADDED Requirements

### Requirement: Current-version state round trips faithfully
Current-version saves SHALL retain quantities, wearable identities, single
owners, equipment references, dye, logical grid ordering, world IDs and gameplay
state across save/load and process restart. Recovery SHALL restore a coherent
same-world snapshot rather than merge old and current inventories.

#### Scenario: Stored garment survives relaunch
- **WHEN** the player stores one dyed tunic, equips another, splits a resource group and saves
- **THEN** relaunch restores those exact quantities, identities, owners, colors and logical order

### Requirement: Incompatible test saves are explicit
Existing saves are disposable test data. Unsupported older/newer schemas SHALL
produce a clear incompatibility/reset message and an explicit new-test-world
path. No legacy wardrobe migration, profile snapshot or downgrade system is
required. Corruption and IO failure MUST NOT be silently presented as a valid
load or successful save.

#### Scenario: Old test save is encountered
- **WHEN** the current build cannot read its schema
- **THEN** it explains that the test save is incompatible and provides an explicit reset choice rather than pretending progress was loaded

### Requirement: Ownership is validated before live mutation
Loading SHALL validate unique instance IDs, recognized definitions, valid
owners/chests, exact compatible equipment slot references, dye, counts/capacity
and layout partition before applying state. Invalid candidates SHALL leave
the current state unchanged and report the error.

#### Scenario: Garment has two owners
- **WHEN** a save places the same instance in a chest and carried inventory
- **THEN** load rejects it without duplicating, dropping or gifting items

### Requirement: Integrity and atomic writes remain intact
Save integrity checks, bounded input, temporary validation, backup/replacement
and explicit failures SHALL remain. Save-and-quit SHALL wait for actual success.
No elaborate historical-fixture or migration-interruption project is required.

#### Scenario: Save write fails
- **WHEN** serialization, write, validation or replacement fails
- **THEN** the game retains current committed state, reports failure and offers retry or explicit unsaved exit

### Requirement: Basic persistence and rendering regression evidence
Synthetic current-version tests SHALL cover round trips, rejected corrupt
ownership, failed writes, retry and recovery. Actual runtime verification SHALL
preserve Lit/Lighting-on/ShaderComplexity-off save/load behavior and the F5/F9
hotkey safety fix.

#### Scenario: Candidate is reviewed
- **WHEN** save/load verification is reported
- **THEN** the report distinguishes tested current-version behavior, explicit test resets and unverified runtime claims
