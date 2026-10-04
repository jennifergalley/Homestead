# Spec Delta

## Purpose
Give farm crops and wild berries or roots useful cookfire meals with inspectable ingredients and Energy balanced against their sale value.

## ADDED Requirements

### Requirement: Eight basic crop meals use the existing cookfire
The cookbook SHALL add eight recipes using all six shop-seed crops plus wild berries and roots. Every batch SHALL consume one kindling, require a nearby fueled cookfire or existing hearth, and produce one edible meal. Some recipes SHALL consume Meadow herb. Ingredients and nominal Energy SHALL be visible before cooking.

#### Scenario: Cook and eat farm produce
- **WHEN** she brings the displayed ingredients and kindling to a fueled cookfire and prepares a crop meal
- **THEN** one batch consumes exactly those ingredients and gives the displayed meal and Energy
- **AND** an unfueled fire or missing ingredients refuses without spending anything.

### Requirement: Crop-meal Energy reflects ingredient sale opportunity
New crop meals SHALL restore nominal Energy equal to the nearest whole number of 12 plus 0.6 times the total current General Store sale value of their consumed ingredients, including herbs and kindling. Existing dishes SHALL retain their balance.

#### Scenario: Compare cheap and valuable meals
- **WHEN** she compares new berry, potato and cabbage dishes
- **THEN** the more valuable consumed ingredients buy proportionally more Energy according to the same rule
- **AND** their values fit the Energy meter without requiring increased crop prices.
