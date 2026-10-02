# Crafting progression catalog

## Purpose

Give world and gameplay implementers a grounded, auditable crafting progression
and provisional supply budgets without mistaking design data for playable rules.

## ADDED Requirements

### Requirement: Explicit implementation status and existing identifiers

The catalog SHALL preserve exact current item, recipe, resource, piece and
garment identifiers and numeric IDs. It SHALL distinguish current, next and
later content, leaving proposed additions without assigned runtime IDs.
Current values SHALL cite a source baseline. No new recipe SHALL be described
as playable merely because the catalog validates.

#### Scenario: Future timber recipe is inspected

- **WHEN** an implementer reads a planned timber-processing recipe
- **THEN** its status, missing runtime support and separate design key are visible
- **AND** existing Branch semantics and save IDs remain unchanged

### Requirement: Bootstrap and first-slice costs are inspectable

Every first-slice recipe SHALL specify positive consumed quantities, outputs,
retained tools, stations, time in game hours and direct energy in current
points. Unsupported durability SHALL be explicitly marked unsupported, not
given an invented current value. Starting supplies SHALL allow the hatchet
without obtaining materials exclusively gated behind that hatchet.

#### Scenario: Hatchet is the first crafted tool

- **WHEN** reachability starts with the current knife and worn clothes
- **THEN** ready branch, stone and fiber sources unlock the hatchet
- **AND** no knife, rope, station or hatchet circular dependency blocks progress

### Requirement: Gathering and habitat contracts are separate from scenery

Materials SHALL identify gathering action, tool, reach, yield, renewal or
depletion, seasonal limits, habitat/patch placement and inventory consequences.
Generator guidance SHALL distinguish scenic abundance from interactive resource
density and source identity from tunable yield. Standing-tree clearance SHALL
not be modeled as automatic endlessly renewable construction wood.

#### Scenario: Dense woodland supplies the first home

- **WHEN** a representative nearby resource neighborhood is evaluated
- **THEN** limited selected branch sites support bootstrapping
- **AND** deliberate tree clearance supplies most construction wood
- **AND** no house clearing or blanket loose-stick distribution is required

### Requirement: Budgets disclose assumptions and pack pressure

The handoff SHALL provide cumulative ingredients, source actions and trips for
an illustrative leisurely opening, first shelter/food and prewinter reserve.
It SHALL preserve current needs rates, use the 120-unit pack/chest constraint,
and label estimates as provisional rather than playtested or scientific facts.

#### Scenario: Player gathers before storing

- **WHEN** a proposed sequence exceeds pack capacity
- **THEN** validation fails rather than dropping excess output
- **AND** the documented remedy is earlier construction/storage or another trip

#### Scenario: Winter reserve is reviewed

- **WHEN** current food/fuel arithmetic is used to plan winter supplies
- **THEN** the handoff separates that arithmetic from unsupported winter,
  spoilage, preservation and insulating-clothing gameplay

### Requirement: Lightweight validation rejects invalid design data

The validator SHALL reject unknown references, duplicate keys, nonpositive
quantities, impossible tool/station prerequisites, unreachable first-slice
outputs, unbootstrappable dependency cycles and undersupplied representative
resource assumptions. It SHALL test at least one failing example for each
major rejection class without launching native game/build tools.

#### Scenario: Circular cordage prerequisite is introduced

- **WHEN** fiber requires a hatchet and the only hatchet requires fiber
- **THEN** validation reports the blocked dependency rather than a passing tree

#### Scenario: Patch minimum drops below shelter demand

- **WHEN** the representative stone supply is less than cumulative demand
- **THEN** validation identifies the missing quantity and exits unsuccessfully
