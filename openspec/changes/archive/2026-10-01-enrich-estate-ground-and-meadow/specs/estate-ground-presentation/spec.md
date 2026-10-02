# Estate ground presentation

## ADDED Requirements

### Requirement: Detailed estate ground

The fixed estate's landscape SHALL show near texture detail without visible tiling at range, broad
colour variation, trodden soil where people and carts go, leaf litter under trees and stony soil on
steep banks.

#### Scenario: Pasture by the manor

- **WHEN** the heroine stands in the pasture near the manor at midday
- **THEN** the ground shows varied grass colour and no repeating texture pattern out to the horizon

### Requirement: Near-camera meadow

The fixed estate SHALL draw 3D grass blades round the camera wherever pasture, moor or woodland
grass grows, fading out with distance without popping, moving in the wind and parting round the
heroine. It SHALL NOT grow inside the manor, on the road's wheel tracks, on water, in town, or within
the clear radius of any interactable, world drop, plot or building piece.

#### Scenario: Interactables stay visible

- **WHEN** a resource node sits in a meadow
- **THEN** no grass blade grows within its clear radius

### Requirement: Soft steps on grass

The heroine's footsteps SHALL be quieter and duller on grass, moor and woodland floor than on other
ground, and unchanged elsewhere.


#### Scenario: Soft steps on grass

- **WHEN** the heroine walks across grass, moor or woodland floor and then walks across another ground type
- **THEN** her footsteps are quieter and duller on the grass, moor and woodland floor, and return to the unchanged sound elsewhere
