# Spec Delta

## Purpose

Provide clear, readable native menus for managing possessions and the homestead with equal controller, keyboard and mouse access while the world is paused.

## ADDED Requirements

### Requirement: Menu feedback never reflows page content
Showing or clearing a field-book feedback message, such as a successful eat, craft, or settings
confirmation, SHALL NOT move or resize tabs, grids, rows, details, action buttons, focus targets,
or the character portrait. Feedback SHALL appear in reserved space or an overlay that does not
obscure those elements.

#### Scenario: Eat from the Inventory
- **WHEN** the player eats one item from the Inventory details and "Ate Berries." appears
- **THEN** the pack grid, details panel, action buttons, and portrait keep their positions and sizes, and the focused action stays under the same screen position

#### Scenario: Message clears
- **WHEN** the feedback message expires
- **THEN** no page element moves or resizes
