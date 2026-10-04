# Spec Delta

## Purpose

Lets Jenny actively fish with a purchased pole, catch different river, lake and sea fish, and sell, store or prepare those catches without spoilage.

## ADDED Requirements

### Requirement: Pole purchase enables active habitat-specific fishing
The General Store SHALL sell a carried hotbar-selectable fishing pole for exactly 1500 coins, available alongside the backpack. Fishing SHALL require the carried pole and nearby river, lake or ocean water, show active timing feedback, and award one habitat-appropriate fish only after successful input. Failed/cancelled casts SHALL award nothing.

#### Scenario: Buy and use the pole
- **WHEN** she buys the pole at the open General Store, selects it and completes the active timing interaction at a river, lake or sea
- **THEN** 1500 coins are spent and one fish from that habitat enters her pack

### Requirement: Fish can be prepared, stored and sold
Catches SHALL store without spoilage and sell to the General Store. This playtest SHALL offer raw sushi-style mackerel slices, grilled trout and grilled perch. Grilled mackerel, fish soup, fish with potatoes, herbed carp and mackerel chowder SHALL remain deferred: neither cookbook-offered nor craftable, with failed attempts leaving state unchanged. Their append-only enum/table rows SHALL remain readable. Cooked dishes SHALL require a lit fire/hearth and one kindling; raw slices SHALL need neither. Meals SHALL restore displayed Energy and grant Well fed on the estate.

#### Scenario: Cook a catch
- **WHEN** she inspects, prepares and eats a fish dish
- **THEN** the displayed ingredients are spent once, her Energy rises by the catalogue amount subject to its cap, and she becomes Well fed

### Requirement: Fishing items use original authored art
The pole, six catch species and three admitted fish preparations SHALL use original Blender assets, not reused tool or food meshes. Each species and admitted dish SHALL have distinct original item imagery and authored dropped-item presentation. Eating SHALL select the admitted item's original edible portion while preserving existing forage behavior. Jenny accepts these viewed source prototypes provisionally; final realism is not accepted. Deferred preparations SHALL receive no placeholder food artwork.

#### Scenario: Inspect fishing item art
- **WHEN** she selects the pole and inspects caught fish or prepared dishes
- **THEN** she sees their original authored art, not the old digging stick or generic root/berry substitute
