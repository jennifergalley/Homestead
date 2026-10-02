# Spec Delta

## ADDED Requirements

### Requirement: Menus are aligned, readable, and place items exactly where she drops them
Menu panels SHALL share aligned edges and one selection treatment; every pack slot SHALL be a valid drop target that keeps its item at that index through save and load; the crafting grid SHALL use the full page width; map labels SHALL render without text shadows; the wardrobe preview SHALL show the heroine standing with no camera controls; and accent states SHALL keep text at 4.5:1 contrast or better.

#### Scenario: Dropping into an empty square
- **WHEN** she drags a tool from the hotbar onto the last pack square, saves and loads
- **THEN** the tool is still in the last pack square
