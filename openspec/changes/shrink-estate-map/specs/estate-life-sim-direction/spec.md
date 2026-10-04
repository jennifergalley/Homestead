# Spec Delta

## ADDED Requirements

### Requirement: Compact map keeps human scale

The playable landscape SHALL be roughly 2 km across in each direction, about half today's linear dimensions. The home estate SHALL be modestly smaller, targeting 85-90% of its previous usable parcel area rather than one-quarter area. The town SHALL remain outside the owned estate with its open square and usable lanes; characters, doors, farm cells, bridges and steps SHALL retain their usable human-scale dimensions. The fixed layout, ground, water, parcels, signs, map and travel destinations SHALL agree.

#### Scenario: Exploring the smaller estate
- **WHEN** Jenny starts a new game on the completed compact-map release and follows its routes from manor to town, lake, coast and mine
- **THEN** the world is visibly smaller without miniature buildings or blocked routes, and map markers, ownership and arrival points correspond to the actual places

### Requirement: Short trips are sized for sprint

With sufficient energy and the ordinary heroine's unchanged 4.8 m/s sprint, the intended unobstructed one-way routes SHALL take no more than 15 s from farm gate to manor, 45 s from manor to lake landing, 75 s from manor to town square AND store door, and 90 s from manor to both the beach arrival/named cove destination and the mine site. A farm/store/one-fishing-stop/home circuit SHALL fit within about three real minutes of sprinting without fast travel. Walking times SHALL be reported as fallback context, not substituted for the sprint sizing criteria.

#### Scenario: First closer-town delivery
- **WHEN** Jenny sprints normally from the manor to the relocated town and enters the general store
- **THEN** the store door is reached within 75 real seconds along the connected route, without teleporting or increasing movement speed

#### Scenario: Full compact-map circuit
- **WHEN** the five routes and daily circuit are timed without stops on the completed compact map at energy >=25
- **THEN** each route meets its sprint cap and the circuit fits about three minutes, with space to traverse normal gates, paths, bridge and stairs
