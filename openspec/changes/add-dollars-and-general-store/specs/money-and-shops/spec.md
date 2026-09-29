# Spec Delta

## Purpose

Earn money by physically selling estate goods in town, and see those goods for sale in the
shop.

## ADDED Requirements

### Requirement: Money is exact and visible
The simulation SHALL preserve money as an `int64` raw integer, where one raw unit is one whole coin.
It SHALL NOT numerically convert existing balances, catalogue prices or saved values. The canonical
money formatter and signed delta formatter SHALL display grouped whole integers with correct
singular/plural `coin` text, without `$` or decimal notation. The gameplay HUD SHALL show the current
balance, and each change SHALL be shown as a brief signed delta. Money SHALL persist across save and
load.

#### Scenario: Sale updates the wallet
- **WHEN** the heroine sells 5 Hay at 40 cents each
- **THEN** her balance rises by exactly 200 coins and the HUD shows "+200 coins"

#### Scenario: Existing money save keeps its numeric value
- **WHEN** a legacy v12 fixture stores raw money 2234
- **THEN** it loads as 2,234 coins without any numeric migration

### Requirement: One item catalogue
Every item SHALL have exactly one catalogue entry, giving its name, description, category, icon, base price and the shops that buy it. Item names shown anywhere in the game SHALL come from the catalogue.

#### Scenario: New item added
- **WHEN** a developer adds an item without a catalogue entry
- **THEN** the build fails its catalogue completeness check

### Requirement: Selling happens at an open shop in person
The heroine SHALL be able to sell only to an open shop, at its counter, and only items that shop buys. A sale SHALL remove the items, add the money and add the items to that shop's stock of goods she sold, all in one transaction.

#### Scenario: Shop is closed
- **WHEN** the heroine tries the general-store door at 7 PM
- **THEN** it is closed, with a sign giving its opening time, and no sale is possible

#### Scenario: Sell a stack
- **WHEN** the heroine sells 12 of her 20 Scrap iron
- **THEN** 8 Scrap iron remain carried, her money rises by 12 × the unit price, and the shop lists 12 Scrap iron from her estate

### Requirement: Her goods appear in the shop and sell down
The shop's Buy view SHALL list the goods she sold in a section named for her estate. Each morning, townsfolk purchases SHALL reduce that stock by a deterministic share until it is gone. She SHALL be able to buy her own goods back.

#### Scenario: Stock drains overnight
- **WHEN** she sells 10 Hay, sleeps, and returns the next day
- **THEN** the shop's stock of her Hay is lower than 10 and above zero

### Requirement: Buying food restores her
The general store SHALL sell basic foods. Eating purchased food SHALL restore hunger and energy by the catalogue amounts.

#### Scenario: Buy and eat a pasty
- **WHEN** she buys a Cornish pasty with enough money and eats it
- **THEN** her money falls by its price, and her hunger and energy rise

### Requirement: The shopkeeper greets and serves
A shopkeeper SHALL stand behind the general-store counter during opening hours, idling and looking toward the heroine when she is near. Talking to her SHALL show a short greeting and then open the shop.

#### Scenario: First visit
- **WHEN** the heroine talks to the shopkeeper for the first time
- **THEN** a first-visit greeting appears, followed by the shop screen

### Requirement: Shop screen parity and pause
The shop screen SHALL pause the game. It SHALL be fully usable with the mouse alone and with the controller alone, including quantity selection, and it SHALL preview the total and resulting balance before confirming.

#### Scenario: Controller purchase
- **WHEN** the player buys 3 bread using only a controller
- **THEN** quantity, preview and confirmation all work without the mouse, and no world action fires while the screen is open
