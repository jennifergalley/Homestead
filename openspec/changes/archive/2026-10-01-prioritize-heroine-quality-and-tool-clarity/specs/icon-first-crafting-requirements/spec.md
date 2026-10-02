# Spec Delta

## Purpose

Make recipe readiness understandable at a glance with icons and numbers rather than repeated ingredient prose, while preserving exact crafting rules and discoverable material sources.

## ADDED Requirements

### Requirement: Recipe costs are compact and icon-led
The Craft details pane SHALL show each consumed ingredient with its recognizable item icon, a short item name, and a compact owned/required quantity such as `0 / 4`. It MUST show an additional shape or symbol for met versus missing requirements; color alone MUST NOT carry availability. It MUST NOT repeat `Have`, `Need`, or a source sentence on every ingredient card.

#### Scenario: Missing ingredients
- **WHEN** a selected recipe requires Branch x4, Stone x3 and Fiber x2 and the pack has none
- **THEN** three compact icon/count rows show the exact `0 / 4`, `0 / 3` and `0 / 2` shortages, with recognizable missing cues

#### Scenario: Sufficient ingredients
- **WHEN** the carried quantities meet or exceed every consumed cost
- **THEN** each row shows the true carried count against its required count and has a distinct met cue without extra explanatory prose

### Requirement: Extra requirement details appear only when relevant
Tool, station and capacity requirements SHALL remain visible and accurate without consuming a full prose card apiece. A retained tool MUST be identifiable as kept, a cooking station MUST reflect actual readiness/proximity, and capacity SHALL draw attention only when it blocks crafting.

#### Scenario: Retained Knife
- **WHEN** a recipe requires a carried Knife but does not consume it
- **THEN** the Knife icon and kept cue identify its requirement and its carried/missing state without suggesting the Knife is spent

#### Scenario: Cooking and full pack
- **WHEN** a cooking recipe lacks a fueled cookfire or its output cannot fit after ingredients are consumed
- **THEN** the station or capacity blocker is identifiable in the details pane, and the actual craft attempt remains unavailable

### Requirement: Sources and accessibility remain discoverable
Hovering or keyboard/controller focusing an unmet requirement SHALL reveal its complete item name, owned/required quantities and primary acquisition hint. The same information MUST be exposed through an accessible label or description without relying on color. Already-met requirements need no persistent source prose.

#### Scenario: Find Fiber before crafting a hatchet
- **WHEN** Fiber is short and its requirement receives pointer hover or directional focus
- **THEN** the player can discover `Reeds near water` without opening another screen, and this hint does not imply a hatchet is needed first

#### Scenario: Navigate at 720p
- **WHEN** the most demanding current recipe is inspected with keyboard or controller at 1280x720
- **THEN** its requirement icons/counts, shortage state and focused explanation remain readable without hiding the recipe output or hold progress

### Requirement: Readability cannot alter crafting authority
Icon, count and focus state SHALL reflect the authoritative recipe assessment. Selection or focus alone MUST NOT craft; the existing held craft cycle, repeated cycles, revision checks, and atomic failure behavior MUST remain unchanged.

#### Scenario: Inventory changes while a recipe is focused
- **WHEN** ingredients, retained tools, station state or pack space change
- **THEN** the visual requirement states refresh to the new authoritative values and a stale hold cannot produce an item
