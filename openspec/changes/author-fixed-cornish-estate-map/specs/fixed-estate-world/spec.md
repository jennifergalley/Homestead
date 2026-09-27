# Spec Delta

## Purpose

The fixed, authored Cornish estate world that all gameplay takes place on.

## ADDED Requirements

### Requirement: The game plays on the fixed Estate map
New games SHALL start on the fixed `Estate` World Partition level. Terrain, water bodies, the road and landmark positions SHALL be identical in every new game, and no runtime procedural generation SHALL determine them.

#### Scenario: Same layout in two new games
- **WHEN** Jenny starts two new games
- **THEN** the standing room, cove, river, road, mine site and town edge are at the same world positions in both

### Requirement: Terrain provenance is recorded and attributed
The shipped terrain SHALL derive from recorded open-licence elevation sources. The required attribution SHALL appear in `docs\asset-credits.md` and the in-game credits.

#### Scenario: Credits show the terrain licence
- **WHEN** the player opens the in-game credits
- **THEN** the Environment Agency Open Government Licence attribution for the terrain is shown

### Requirement: The agreed layout is walkable end to end
The map SHALL contain:

- the estate on a south-facing slope down to a cove with cliffs;
- a clifftop mine site;
- a river through a wooded valley to the cove;
- a dirt road of about 1.5 km to a town at an estuary head;
- moorland to the north;
- dunes and beaches on the far coast.

The heroine SHALL be able to walk from the estate to the town along the road without blocking terrain.

#### Scenario: Walk to town
- **WHEN** the heroine follows the dirt road from the estate gateway
- **THEN** she reaches the town edge on foot without being blocked or falling through terrain

### Requirement: Water supports existing tending
The ocean, the river and the lake SHALL render in ordinary play. Fresh water, meaning the river and the lake, SHALL satisfy pail refill.

#### Scenario: Refill at the river
- **WHEN** the heroine uses an empty pail at the estate river bank
- **THEN** the pail fills exactly as it did at the woodland creek

### Requirement: Interactive resources have stable authored identity
Every interactive world resource (trees, rocks, forage and overgrowth) SHALL come from a
versioned placement table with stable IDs and a minimum tool tier. Player edits SHALL persist
by those IDs across save and load.

#### Scenario: Felled tree stays felled
- **WHEN** the heroine fells a placed tree, saves, quits and reloads
- **THEN** that tree remains felled and every other placed tree is unchanged

#### Scenario: Placement version changes
- **WHEN** a save references an older placement bake version
- **THEN** the game reports that the test save is incompatible and offers a reset, without deleting unrelated files

### Requirement: Named landmarks are published for other systems
The world SHALL publish named landmark anchors and polygons in one data asset. Other systems SHALL locate the standing room, manor footprint, gateway, cove, mine entrance, mill site, road ends, town square, store door, estate boundary and for-sale parcels by name only.

#### Scenario: Spawn at the standing room
- **WHEN** a new game begins
- **THEN** the heroine spawns at the `StandingRoomSpawn` anchor
