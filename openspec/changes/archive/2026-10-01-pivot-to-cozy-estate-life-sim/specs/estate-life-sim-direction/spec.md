# Spec Delta

## Purpose

Product-level rules for Homestead as a cozy Victorian Cornish estate life sim. Every later
round's change must honour them.

## ADDED Requirements

### Requirement: Pressure is cozy and never lethal
The game SHALL NOT include player death, injury, illness, cold exposure, predators, or any
permanent loss of the estate, livestock or companion. Setbacks SHALL be recoverable: a small
money or item penalty, lost time, lower output, or a reputation dip.

#### Scenario: Collapse from exhaustion
- **WHEN** the heroine's energy runs out at any hour
- **THEN** she dozes off for a few hours of rough sleep and wakes only part rested, with no other loss

#### Scenario: Unpaid bills
- **WHEN** a seasonal rate, tax or wage goes unpaid
- **THEN** late fees and a reputation reduction apply, and the estate, its buildings and parcels remain hers

### Requirement: Energy is the only personal meter
The game SHALL track energy as the heroine's only personal meter and SHALL NOT track hunger, warmth or cold. Eating SHALL restore energy. Meals SHALL restore more than snacks and SHALL grant a temporary Well fed benefit that reduces work costs. No meter SHALL cause fainting, damage or failure.

#### Scenario: Meal versus snack
- **WHEN** the heroine eats a Cornish pasty, and at another time eats a loaf of bread
- **THEN** the pasty restores more energy and grants Well fed, and the bread restores a little energy without Well fed

#### Scenario: A day without eating
- **WHEN** the heroine works through a whole day without eating
- **THEN** no hunger warning, penalty or failure occurs, and only her energy limits her work

#### Scenario: Winter clothing
- **WHEN** it is winter and the heroine wears any outfit
- **THEN** no warmth or cold effect is applied and clothing affects appearance only

### Requirement: The world is one fixed authored map
The game SHALL play on a single fixed, hand-authored map with a stable layout across new
games. Procedural world generation SHALL NOT determine terrain, water or landmark placement
on the play path.

#### Scenario: Two new games
- **WHEN** Jenny starts two separate new games
- **THEN** the estate, manor ruin, mine, river, road, town and coast are in the same places in both

### Requirement: Estate boundaries are communicated, not fenced
The estate boundary SHALL be shown on the world map and minimap. The world SHALL NOT start
with fences marking it. Building SHALL be permitted only inside owned parcels. Areas outside
SHALL remain walkable and foraging SHALL remain possible there.

#### Scenario: Leaving the estate
- **WHEN** the heroine walks past the estate boundary
- **THEN** she is not blocked, the map shows her outside the boundary, and build placement there is rejected with a clear reason

### Requirement: Goods reach town physically
Selling SHALL require the goods and the heroine, or later a carter employee, to be present
at the buying shop. Goods SHALL NOT be sold from the estate by a shipping bin or remote
menu.

#### Scenario: Selling from the estate
- **WHEN** the heroine tries to sell while on the estate
- **THEN** no sale option exists, and she must carry or haul the goods to a shop

### Requirement: Money has meaningful sinks
Money SHALL be displayed in dollars and cents and stored as integer cents. The economy SHALL
include recurring costs and prices that respond to supply, so income cannot scale without
limit from one repeated sale.

#### Scenario: Flooding a shop
- **WHEN** the heroine repeatedly sells the same item to one shop in a short period
- **THEN** that shop's offered price for the item falls and recovers only over subsequent days

### Requirement: Tool tier gates what can be cleared
Each clearing or mining tool SHALL have tiers (worn, iron, steel, master-forged). A target SHALL declare the minimum tier it needs. A tool below that tier SHALL NOT clear the target and SHALL show which upgrade is needed.

#### Scenario: Worn axe on a large stump
- **WHEN** the heroine uses a worn axe on a large stump
- **THEN** the stump is unchanged, no energy is spent, and a prompt says a better axe is needed

### Requirement: Livestock are never slaughtered
The game SHALL NOT offer slaughter or butchering of livestock. Livestock SHALL be sellable
back to the livestock dealer, and meat SHALL be obtainable only by purchase in town.

#### Scenario: Selling a grown cow
- **WHEN** the heroine chooses to part with a cow
- **THEN** the only option is selling it to the livestock dealer, and no meat item is produced

### Requirement: Stored goods do not spoil
Items SHALL NOT decay, rot or lose value over time while carried or in storage.

#### Scenario: Fish kept for a season
- **WHEN** a fish sits in a chest for 28 in-game days
- **THEN** it is unchanged in quality and value
