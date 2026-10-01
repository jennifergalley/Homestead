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

### Requirement: Holding the tool button repeats strikes on one target
Holding the tool button (left mouse button or the gamepad's right trigger) SHALL keep striking the aimed overgrowth with the axe, billhook, pickaxe or scythe until it clears, then stop. Releasing SHALL finish the blow under way. A click SHALL be exactly one blow. The repeat SHALL stop when energy falls below the tired floor ("Too tired."), when the tool is put away, when she turns from the target or moves out of reach, or when a menu opens. It SHALL never move on to another target after a clear. E and the interact button SHALL never perform a tool action, and the focus card SHALL show only the keyed verb.

#### Scenario: Holding the axe on a stump
- **WHEN** the heroine holds the left mouse button with a worn axe aimed at a medium stump
- **THEN** each blow lands in turn within one continuous strike until the stump clears, the stump yields once, and no other target is struck while the button stays held

#### Scenario: Letting go mid-swing
- **WHEN** the heroine releases the button after a blow lands but before the next wind-up
- **THEN** that blow finishes, she recovers, and the toast says how many swings remain

#### Scenario: Too tired to continue
- **WHEN** her energy falls below the tired floor during a held run
- **THEN** no further blow starts and the toast says "Too tired."

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
