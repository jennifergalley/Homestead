## ADDED Requirements

### Requirement: Back selection controls rucksack appearance only
Inventory SHALL provide a Back selection beneath equipped slots with Leather Rucksack and None. Leather Rucksack SHALL require the purchased upgrade. The choice SHALL update character and inventory portrait visibility, persist across saves and explicitly state that the inventory-space upgrade remains available with None.

#### Scenario: Hide an owned rucksack
- **WHEN** the player chooses None after purchasing the rucksack upgrade
- **THEN** the character and portrait hide the bag, the choice persists, and capacity and carried contents remain unchanged

#### Scenario: No upgrade owned
- **WHEN** the player has not purchased the rucksack upgrade
- **THEN** Back shows None and Leather Rucksack is unavailable without granting capacity
