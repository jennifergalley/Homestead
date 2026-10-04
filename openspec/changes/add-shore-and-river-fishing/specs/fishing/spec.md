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
Catches SHALL store without spoilage and sell to the General Store. Preparations SHALL include raw sushi-style slices, grilled fish, soup, fish with potatoes and additional seasoned seafood. Cooked dishes SHALL require a lit fire/hearth and one kindling; raw slices SHALL need neither. Meals SHALL restore displayed Energy and grant Well fed on the estate.

#### Scenario: Cook a catch
- **WHEN** she inspects, prepares and eats a fish dish
- **THEN** the displayed ingredients are spent once, her Energy rises by the catalogue amount subject to its cap, and she becomes Well fed

### Requirement: Fishing items use original authored art
The pole, six catch species and eight fish preparations SHALL use newly authored Blender assets, not reused tool or food meshes. Each species and dish SHALL have distinct item presentation. Original PBR assets SHALL be reviewed at 4K before integrated acceptance.

#### Scenario: Inspect fishing item art
- **WHEN** she selects the pole and inspects caught fish or prepared dishes
- **THEN** she sees their original authored art, not the old digging stick or generic root/berry substitute
