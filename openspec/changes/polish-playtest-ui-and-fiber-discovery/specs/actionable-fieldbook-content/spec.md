# Spec Delta

## Purpose

Give informational book entries a clear reading path without pretending that selection offers an additional action.

## ADDED Requirements

### Requirement: Informational entries do not offer inert buttons
Guidebook and Credits selections SHALL display their text directly in the details area. The interface MUST NOT display a "Read" action that does nothing; pointer, keyboard and controller navigation MUST still reach the entries and their details.

#### Scenario: Select a Guidebook entry
- **WHEN** the player clicks or focuses an entry such as "Choose your own home"
- **THEN** its content appears without a dead "Read" button, a second activation step or an accidental return to gameplay

#### Scenario: Select Credits
- **WHEN** an informational Credits row is selected
- **THEN** its attribution can be read and no misleading primary action is offered
