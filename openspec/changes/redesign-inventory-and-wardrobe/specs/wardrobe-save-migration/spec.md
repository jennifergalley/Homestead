# Spec Delta

## Purpose

Preserve existing homesteads and appearances while adding durable wearable
ownership, equipment and inventory layout with integrity-checked recovery.

## ADDED Requirements

### Requirement: Existing supported saves migrate without resetting worlds
Supported legacy portable versions 2 and 3 and Unreal wrapper versions 1 through
4 SHALL remain readable under their existing validity rules. Migration SHALL
preserve world identity, resource/structure/plot IDs, quantities, positions,
clock, needs, cooldowns, crops, settings, appearance and recovery eligibility.
Invalid or future versions SHALL fail explicitly rather than become a new world.

#### Scenario: Valid legacy save is opened
- **WHEN** a legacy homestead with stored resources and customized appearance loads
- **THEN** its same world resumes with all old progress and quantities intact and the new wardrobe initialized from its actual selected outfit

### Requirement: Legacy appearance becomes owned equipment exactly once
Migration SHALL materialize only the currently worn legacy tunic, footwear and,
if selected, apron as equipped instances, carrying forward actual garment dye.
It SHALL not grant all legacy outfit options, consume pack capacity, or issue
another starter set on repeated load, backup recovery or interrupted migration.

#### Scenario: Apron outfit migrates
- **WHEN** a valid legacy save with the apron outfit and Wine tunic dye is read
- **THEN** one tunic, one apron and one footwear instance are equipped, the tunic/apron reproduce their legacy dye, and all carried/chest resource counts remain unchanged

#### Scenario: Migration is interrupted before commit
- **WHEN** the process ends before the upgraded save replaces the original
- **THEN** the original remains recoverable and the next migration produces the same logical instance identities, not additional wardrobe items

#### Scenario: Already upgraded save is reloaded
- **WHEN** a new-schema save contains a migrated wardrobe whose original garments were later stored
- **THEN** stored ownership is restored exactly and no legacy cosmetic field recreates equipped copies

### Requirement: Persistence includes identity ownership dye and layout
New saves SHALL retain every wearable instance, its single owner and dye,
equipment references, chest IDs, monotonic ID allocation and valid display
ordering/split grouping. Grid layout SHALL be independent of the 120-unit
capacity; different viewport widths SHALL not change possessions.

#### Scenario: Layout changes and relaunch
- **WHEN** the player splits/reorders resource groups, equips one of two same-type garments, saves and relaunches at the other supported resolution
- **THEN** quantities, identities, owners and logical ordering persist while the grid reflows to the new viewport

### Requirement: Invalid ownership is rejected before live mutation
Reading a save SHALL validate unique instance IDs, recognized definitions,
valid owners/containers, occupied-slot compatibility, quantities/capacity, dye,
layout consistency and bounded collection sizes before applying anything.
Corruption SHALL not be repaired by duplicating, dropping or gifting items.

#### Scenario: Duplicate or dangling garment reference
- **WHEN** a save references one instance in both pack and equipment, or a nonexistent chest
- **THEN** the whole candidate save is rejected, live state remains unchanged, and existing valid backups can be considered with visible recovery feedback

### Requirement: Integrity and atomic replacement remain intact
The portable checksum and outer CRC integrity protections, size bounds,
validated temporary writes, backup-before-replace and explicit failure reporting
SHALL be preserved. Migration reads SHALL not eagerly rewrite every save slot.
Only a successful normal save SHALL durably advance the selected slot.

#### Scenario: Write or backup fails after wardrobe changes
- **WHEN** a new save fails validation, backup creation or replacement
- **THEN** live committed ownership remains available for retry, existing valid saves/backups remain recoverable, and save-and-quit does not exit as if saving succeeded

### Requirement: Recovery snapshots include coherent wardrobe state
Manual, rotating automatic, sheltered recovery and in-memory session
checkpoints SHALL restore coherent simulation, appearance and wardrobe state.
Same-world recovery filtering and isolated preview/profile routing SHALL remain
unchanged; recovery MUST NOT merge old and current inventories.

#### Scenario: Equipped garment differs from checkpoint
- **WHEN** survival recovery restores a same-world checkpoint made before a garment transfer
- **THEN** all checkpoint owners/equipment/dye restore together without retaining later transferred copies or pulling items from another world/profile

### Requirement: Backward migration has persistent regression evidence
Golden legacy fixtures and new-schema round trips SHALL cover legacy versions,
body/hair/outfit/dye combinations, full containers, corruption, interrupted
commit and backup recovery using synthetic worlds, never personal saves.
Save/load validation SHALL retain actual Lit, Lighting-on and
ShaderComplexity-off guard checks and the inherited F5/F9 hotkey fix.

#### Scenario: New candidate is accepted
- **WHEN** verification is reviewed
- **THEN** evidence includes separate-process legacy migration and new saves, failure/retry paths, unchanged original/other-profile saves, and actual renderer-state checks rather than screenshot dimensions alone

### Requirement: Rollback preserves both old and new progress
Before any accepted upgrade opens a player profile, a consented versioned
pre-migration snapshot SHALL be retained. Older candidates SHALL not be pointed
at a newly upgraded profile as an implicit rollback; downgrade/export is not
provided by this change.

#### Scenario: Candidate is rejected after migration testing
- **WHEN** the coordinator retains or returns to the previous candidate
- **THEN** synthetic test profiles remain isolated, and any human rollback uses its preserved old-schema snapshot while retaining the upgraded profile rather than overwriting either history
