# Spec Delta

## Purpose

Confirmed outcome-level rules for this round. Expand them when the round is fully specified.

## ADDED Requirements

### Requirement: The manor is restored in stages commissioned from a village mason
The manor SHALL be restored in five stages (great hall, kitchen, bedchamber, upper floor, roof and
chimneys), each with a cost in stone, timber and coins. She SHALL commission a stage from a mason
who keeps a business in the village, separate from the General Store. Commissioning SHALL take the
stage's materials from chests on her estate, charge the fee, and restore the stage overnight. A stage
SHALL NOT be commissioned until her estate chests hold all its materials.

#### Scenario: Commissioning the great hall
- **WHEN** her estate chests hold the great hall's stone and timber and she commissions it from the mason, paying the fee
- **THEN** those materials are deducted from her estate chests, and the next morning the great hall is restored

#### Scenario: Not enough stone yet
- **WHEN** her estate chests hold 140 of the 200 stone the great hall needs
- **THEN** the mason shows "Stone 140 / 200" and won't take the commission

### Requirement: The dog companion is never harmed or lost
The dog companion SHALL be offered in pit bull and Great Dane breeds, SHALL follow or stay home on request, and SHALL never be harmed, lost or taken away.

#### Scenario: Neglected dog
- **WHEN** the dog is not petted or fed for several days
- **THEN** the dog remains at the manor, unharmed, with a lower bond only
