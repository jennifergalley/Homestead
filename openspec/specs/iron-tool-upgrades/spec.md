# iron-tool-upgrades Specification

## Purpose
Buying iron upgrades for owned tools at the General Store until mining exists.

## Requirements

### Requirement: Iron upgrades at the General Store

The General Store SHALL sell a worn-to-iron upgrade for each tool for 2,000 coins, only for a tool she has crafted and only while it is worn.

#### Scenario: Buying an iron axe

- **WHEN** she holds a worn axe and 2,000 coins and buys the upgrade
- **THEN** the axe becomes iron, 2,000 coins are spent, and the row disappears

#### Scenario: Tool not crafted

- **WHEN** she has no axe
- **THEN** the row is disabled and reads "Craft an axe first."
