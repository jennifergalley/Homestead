# Spec Delta

## Purpose

Render owned garments faithfully on the existing adult heroine while ensuring
safe coverage, compatible movement, trustworthy item imagery and licensed art.

## ADDED Requirements

### Requirement: Equipped ownership determines visible garments
The character preview and gameplay character SHALL render the same committed
equipped garment identities, fit and dye. Hovering an item SHALL not silently
change the world character's outfit. All supported body presets SHALL retain
their existing face, hair, eye and skin choices.

#### Scenario: Equip and close Inventory
- **WHEN** the player equips a dyed tunic, apron and footwear
- **THEN** the preview and gameplay character show those same individual items, and reopening Inventory does not revert to a cosmetic outfit preset

### Requirement: Unequipped states remain modest and complete
A permanently present, nontradeable modest base layer SHALL cover the adult
character when garments are absent. Every valid equipped combination SHALL have
complete supported body/foot geometry; removing a garment MUST NOT reveal holes
from deleted body vertices or expose unsupported unclothed states.

#### Scenario: All removable garments are stored
- **WHEN** the player unequips all clothing into available inventory/storage
- **THEN** a coherent modest base and complete feet remain visible in preview and gameplay, without invisible torso/feet, orphaned socks or garment remnants

### Requirement: Compatibility is demonstrated across movement and variants
Every admitted garment SHALL fit the retained skeleton, supported body presets,
material/dye contract and allowed equipment combinations during idle, walk and
existing gathering/watering/weeding/clearing actions. Unsupported combinations
SHALL be rejected before a committed equip rather than silently substituting a
different garment.

#### Scenario: Required garment rendering is unavailable
- **WHEN** a required mesh, compatible fit or material is missing
- **THEN** the new equip is refused with a readable error and existing ownership/appearance remains intact; the incomplete candidate cannot pass acceptance

### Requirement: Icons describe real content without copying reference art
Each item, garment, recipe, plan and menu tab SHALL have an original or
verified-license icon with documented provenance. Garment icons SHALL distinguish
shape and relevant dye, recipe/plan icons SHALL identify outputs without implying
ownership, and text names SHALL remain available independently of icons.

#### Scenario: Icon content is missing
- **WHEN** development validation finds an unmapped item or unreadable icon
- **THEN** it reports the exact entry and blocks complete-art acceptance; a labeled diagnostic placeholder may aid development but is not a finished icon

### Requirement: Authoring honors the native offline boundary
Asset work SHALL use the approved local authoring/import pipeline and retain
source/license/hash records. User references SHALL stay local; Coral Island
images SHALL be interaction references only, not extracted icons, meshes,
textures, fonts or character designs.

#### Scenario: Existing meshes cannot safely separate clothing
- **WHEN** the current joined export or body-deletion mask prevents a valid unequipped state
- **THEN** compatible original/verified-license modular geometry and the base layer are authored and verified before claiming wearable support, rather than relabeling cosmetic outfit switching

### Requirement: Visual acceptance is bounded and honest
The candidate SHALL undergo at most two matched visual review sets covering the
menu and garment states, with actual supported resolutions and gameplay movement.
Technical test success SHALL not be described as Jenny's aesthetic approval.
Failure after the second set SHALL be reported as incomplete, not trigger an
unbounded cosmetic iteration.

#### Scenario: Functional checks pass but garments clip visibly
- **WHEN** the final matched review finds unacceptable coverage, clipping or readability
- **THEN** the candidate is not promoted as complete and the prior accepted candidate remains available
