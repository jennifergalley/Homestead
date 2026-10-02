# camera-safe-foliage Specification

## Purpose
Prevents decorative and leafy vegetation from compressing or obscuring the third-person camera while retaining solid world collision and woodland density outside the view corridor.

## Requirements

### Requirement: Foliage does not compress the camera
Leaves, grass, ferns, flowers, shrubs, reeds, sapling foliage, and equivalent non-solid vegetation SHALL ignore the camera collision channel. Terrain, trunks, rocks, structures, and deliberately solid obstacles SHALL retain camera collision.

#### Scenario: Orbit beside a sapling
- **WHEN** the desired camera path crosses a leafy young-tree mesh
- **THEN** the foliage does not shorten the spring arm into the heroine

#### Scenario: Orbit behind a trunk
- **WHEN** a solid trunk lies between heroine and desired camera
- **THEN** the camera still resolves safely in front of the trunk

### Requirement: Near and intervening foliage clears from the view
Camera-sensitive foliage pixels close to the camera or within a bounded corridor between camera and heroine SHALL fade/dither away smoothly. Foliage outside that region SHALL retain normal visibility and density.

#### Scenario: Camera enters a leafy cluster
- **WHEN** near-camera leaves would otherwise fill most of the viewport
- **THEN** those leaves clear enough to keep the heroine and central target readable without a full-screen artifact

#### Scenario: Foliage lies between camera and heroine
- **WHEN** grass, fern, shrub, reed, flower, or canopy surfaces cross the camera-to-heroine corridor
- **THEN** only the interfering region clears while surrounding woodland remains visible

### Requirement: Every admitted foliage family follows one policy
Generated tree foliage, resource foliage, groundcover HISM batches, creek-bank cover, decorative wildflowers, and farther visual foliage rings SHALL use the same camera-safe classification and visibility behavior. Solid material slots on mixed tree meshes MUST NOT be faded as foliage.

#### Scenario: Rebuild the active window
- **WHEN** chunks stream/rebuild or resources clear/regrow
- **THEN** new foliage components inherit camera-safe materials/responses and removed components leave no stale fade/collision state

### Requirement: Camera safety does not change gameplay authority
Foliage clearing for camera visibility MUST NOT change Pawn collision, navigation, interaction focus, range, rewards, resource state, generation identity, save data, or world-edit persistence.

#### Scenario: Gather through faded foliage
- **WHEN** a focused resource's leaves are temporarily faded for camera visibility
- **THEN** its authoritative gather/clear behavior and save result remain unchanged

### Requirement: Camera-safe foliage is robust and measured
Missing or incompatible fade material coverage SHALL be reported explicitly and SHALL block promotion when it can reproduce full-screen foliage. The policy SHALL remain readable in day, night, rain, zoom, slope, close-canopy, 720p and 4K routes without unacceptable cadence or overdraw regression.

#### Scenario: Compare performance
- **WHEN** matched dense-woodland camera routes run before and after the change
- **THEN** no full-screen foliage occurs and measured frame/camera/material costs remain within the accepted candidate threshold
