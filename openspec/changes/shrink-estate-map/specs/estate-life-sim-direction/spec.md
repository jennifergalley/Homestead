# Spec Delta

## ADDED Requirements

### Requirement: Compact map around a fixed manor core

The manor ruin, derelict farm, opening clear-out, pond and its woodland path, the village, its street and the road from the manor SHALL keep their positions and placement ids. The home estate SHALL hold about 25 ha of land and keep the manor core, pond woods, the cove, the beach below it and the mine ruin; the village SHALL lie outside it. The playable area and map sheet SHALL be about 1.5 km across, bounded by natural edges (sea, dense wood, hedged fields) with no visible walls.

#### Scenario: Starting a game on the compact map
- **WHEN** Jenny starts a new game and opens the map
- **THEN** the sheet shows only the cropped area, her estate is visibly smaller, and the manor, farm, pond path and village look and sit as before

### Requirement: Near trips are sized for sprint

With energy of at least 25 and the unchanged 4.8 m/s sprint, the walked one-way routes from the manor SHALL take no more than 60 s to the cove's first sand and 60 s to the mine ruin, and farm, pond and store trips SHALL be no longer than today. Dry sand SHALL run continuously from the cove along the bay onto the long beach.

#### Scenario: Walking to the cove and along the beach
- **WHEN** Jenny sprints from the manor to the cove and on along the sand
- **THEN** she reaches the sand within 60 s and can walk from the cove onto the long beach without swimming or climbing

#### Scenario: Walking to the mine
- **WHEN** Jenny sprints from the manor along the mine path
- **THEN** she reaches the mine ruin within 60 s

### Requirement: Reserved land is outlined on the map

The map SHALL show faint dashed outlines with period names for neighbouring estates and communal land (a village green, allotments and a communal beach) and for land for sale next to her estate. Reserved land SHALL look finished in the world and SHALL NOT sit across her routes to the cove, mine, pond or village. Player copy SHALL NOT say "reserved" or "future".

#### Scenario: Reading the map
- **WHEN** Jenny opens the map
- **THEN** she sees Penhallow, Tregarthen and Polwhele, the village green, allotments and Chapel sands outlined and named, with no neighbour gameplay
