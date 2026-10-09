# metahuman-store-clerk Specification

## Purpose
TBD - created by archiving change metahuman-store-clerk. Update Purpose after archive.

## Requirements

### Requirement: The general-store clerk is a period MetaHuman
The general-store clerk SHALL be a MetaHuman man of about fifty-five in original period
shopkeeper's clothing (shirt, waistcoat, apron), idling behind the counter and turning toward the
heroine when she is near. He SHALL show no placeholder label. His MetaHuman, garment and animation
sources SHALL be recorded with their licences.

#### Scenario: Visit the store
- **WHEN** the heroine walks into the open general store
- **THEN** the clerk stands behind the counter in his period clothes, idling, and is readable at the normal gameplay camera

#### Scenario: Assets missing
- **WHEN** the clerk's MetaHuman assets fail to load
- **THEN** the store still opens and trades, with the legacy stand-in visible
