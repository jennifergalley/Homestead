# Spec Delta

## ADDED Requirements

### Requirement: Items stay in the exact pack square she drops them on
Every pack slot SHALL be a valid drop target; dropping into an empty slot SHALL place the item at that index, dropping onto an occupied slot SHALL swap, empty gaps SHALL persist, and slot positions SHALL survive saving and loading. Saves without slot positions SHALL load with their existing order.

#### Scenario: Dropping into the last square
- **WHEN** she drags a tool from the hotbar onto the last pack square, saves and loads
- **THEN** the tool is still in the last pack square and the squares before it are unchanged
