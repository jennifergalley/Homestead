# estate-map-and-minimap Specification

## Purpose
Communicate the estate's boundaries and the world's layout through a minimap and a world map,
and restrict building to owned land.

## Requirements

### Requirement: Estate parcels and ownership
The simulation SHALL model estate parcels as polygons with an owned flag. A new game SHALL own the home estate parcel only. Parcel ownership SHALL persist across save and load.

#### Scenario: New game ownership
- **WHEN** a new game begins
- **THEN** the home estate parcel is owned and every for-sale parcel is unowned

### Requirement: Building is limited to owned parcels
Build placement SHALL be valid only if the whole footprint lies within owned parcels. Invalid placement outside the estate SHALL explain the reason and SHALL consume nothing.

#### Scenario: Placing outside the estate
- **WHEN** the heroine previews a foundation beyond the estate boundary
- **THEN** the preview shows as invalid with the reason "Outside your estate", and committing does nothing

#### Scenario: Gathering outside the estate
- **WHEN** the heroine picks forage outside the boundary
- **THEN** the gather succeeds normally

### Requirement: HUD minimap
Ordinary gameplay SHALL show a minimap with the heroine's position and facing, the owned estate boundary, and nearby landmarks. It SHALL be north-up by default, and a setting SHALL let it rotate with the camera. The minimap SHALL hide whenever the rest of the gameplay HUD is hidden.

#### Scenario: Walking shows on the minimap
- **WHEN** the heroine walks along the road
- **THEN** the minimap arrow moves and turns accordingly over the map texture

#### Scenario: Menu hides the minimap
- **WHEN** the field book opens
- **THEN** the minimap is not drawn until gameplay resumes

### Requirement: World-map page
The field book SHALL include a Map tab. It SHALL show the whole world with owned and for-sale parcels, named landmarks, and the heroine's current position. It SHALL support pan and zoom with the mouse and with the controller, and directional focus SHALL step between landmarks.

#### Scenario: Find the town
- **WHEN** the player opens the Map tab
- **THEN** the town, the dirt road, the manor, the mine ruin and the cove are labelled, and the heroine's marker is visible

#### Scenario: Controller map use
- **WHEN** the player pans and zooms the map using only a controller
- **THEN** the map responds, and LB/RB still switch field-book tabs

### Requirement: Boundary crossing feedback
Crossing the owned boundary SHALL show a brief non-blocking toast naming the estate. Movement SHALL NOT be blocked. Standing on the boundary line SHALL NOT repeat the toast.

#### Scenario: Leave and return
- **WHEN** the heroine walks off the estate and back
- **THEN** one "Leaving" toast and one "Entering" toast appear, each naming the player-chosen estate name
