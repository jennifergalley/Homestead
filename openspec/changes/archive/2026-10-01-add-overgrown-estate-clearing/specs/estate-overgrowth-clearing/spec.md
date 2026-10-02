# Spec Delta

## Purpose

Clear the overgrown estate with period hand tools whose tiers gate what they can clear.

## ADDED Requirements

### Requirement: Layered overgrowth covers the estate
The estate SHALL begin covered in persistent overgrowth: tall grass, weeds, thin and dense bramble, saplings, stumps, fallen timber, rubble and boulders. Each kind SHALL declare its required tool and minimum tier.

#### Scenario: New game estate
- **WHEN** a new game begins
- **THEN** the manor surroundings are covered in overgrowth, and the dirt road remains passable

### Requirement: The right tool clears each kind
The scythe SHALL clear grass and weeds, the billhook SHALL clear bramble and saplings, the axe SHALL clear stumps and fallen timber and fell trees, and the pickaxe SHALL break rubble and rocks. Using a wrong tool SHALL NOT clear the target.

#### Scenario: Billhook on thin bramble
- **WHEN** the heroine uses a worn billhook on thin bramble in reach
- **THEN** the bramble is cleared, bramble canes are added to her inventory, and energy is spent once

#### Scenario: Scythe sweep
- **WHEN** the heroine swings the scythe facing a patch of tall grass
- **THEN** every grass and weed target in the forward arc is cleared, and one feedback summarises the hay and weeds gained

### Requirement: Tool tier gates clearing
Each tool SHALL have a tier of Worn, Iron, Steel or Master-forged. A target whose minimum tier exceeds the tool's tier SHALL remain unchanged and cost no energy, and the game SHALL name the needed upgrade. Higher tiers SHALL clear faster, or over a wider area for the scythe and hoe.

#### Scenario: Worn axe on a large stump
- **WHEN** the heroine strikes a large stump with a worn axe
- **THEN** the stump is unchanged, no energy is spent, and a prompt says "Needs an iron axe"

#### Scenario: Multi-swing stump
- **WHEN** the heroine strikes a small stump with a worn axe until it breaks
- **THEN** the stump is cleared and yields firewood once, only on the final swing

### Requirement: First tools are hafted from salvage
Salvage piles SHALL yield rusted heads for the axe, scythe, billhook, pickaxe and hoe. A hand recipe SHALL combine a rusted head with branches to make the worn tool. No station and no knife SHALL be required. A new game SHALL provide the pail and a reachable supply of branches.

#### Scenario: Haft the first billhook
- **WHEN** the heroine holds a rusted billhook head and two branches and crafts "Haft a tool"
- **THEN** both inputs are consumed and a worn billhook appears in her inventory and hotbar

### Requirement: Cleared ground mostly stays cleared
Cleared overgrowth SHALL persist across save, load and sleep. Untended cleared grass or weed ground near remaining overgrowth MAY slowly regrow sparse weeds. Tilled, built-on or road ground SHALL never regrow overgrowth.

#### Scenario: Built-on ground
- **WHEN** a foundation stands where bramble was cleared
- **THEN** no overgrowth ever regrows under it

### Requirement: Survival-era tools and warmth are retired
New games SHALL NOT include the knife, machete, fibre-gated recipes, deer remains or fur. The simulation and HUD SHALL NOT track or display warmth, and clothing SHALL have no warmth effect.

#### Scenario: HUD meters
- **WHEN** gameplay is running
- **THEN** only energy and hunger meters are shown
