# Spec Delta

## ADDED Requirements

### Requirement: Estate flowers extend beyond the lake walk
The Estate SHALL reuse its existing decorative wildflowers in deterministic natural drifts along dry lake and river margins, beneath woods and across open fields, preserving the existing lake-walk flowers without uniform carpet coverage or new interactables.

#### Scenario: Walk through different Estate habitats
- **WHEN** the player walks beside the lake or river and through woodland and open fields
- **THEN** flowers appear in varied pockets among the existing vegetation and do not block movement or take interaction focus

### Requirement: Estate flowers stay outside farm ruins and cultivated ground
Decorative wildflowers SHALL stay outside the farm and broken-down manor footprint and SHALL be absent wherever their visible footprint intersects a tilled square anywhere on the Estate, including after saving and loading.

#### Scenario: Cultivate a flowered square
- **WHEN** the player tills a flowered square outside the original farm and saves and reloads
- **THEN** that square remains clear of decorative flowers while neighbouring uncultivated pockets remain

#### Scenario: Inspect excluded landmarks
- **WHEN** the player explores the old farm and manor ruins
- **THEN** neither contains decorative wildflower scatter
