# Spec Delta

## Purpose

Confirmed outcome-level rules for this round. Expand them when the round is fully specified.

## ADDED Requirements

### Requirement: Prices respond to supply
Each shop's offered price for an item SHALL fall as its stock of that item grows and SHALL recover over later days as it sells down. Quality tiers SHALL raise the price.

#### Scenario: Flooding the market
- **WHEN** she sells 50 hay to one shop in a day
- **THEN** the offered price for hay at that shop falls and recovers over the following days

### Requirement: Carts carry goods to town
A purchased handcart SHALL carry goods separately from the pack. Loaded crates SHALL be visible in the cart bed and SHALL unload at a shop door for sale.

#### Scenario: Unload at a shop
- **WHEN** she pushes a loaded cart to the general-store door and unloads it
- **THEN** the unloaded goods are available to sell in that shop's Sell view
