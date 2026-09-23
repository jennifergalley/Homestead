# Spec Delta

## Purpose

Provides a familiar, persistent ten-slot toolbelt for rapid mouse, keyboard, pointer, and controller tool selection and authoritative world use.

## ADDED Requirements

### Requirement: Gameplay shows a ten-slot toolbelt
Ordinary gameplay SHALL show ten bottom-center slots labeled `1` through `9` and `0`. Each slot SHALL reference a carried tool without adding inventory capacity or duplicating the item. The selected slot SHALL remain unmistakable without obscuring its icon or number.

#### Scenario: Start with the knife
- **WHEN** a new homestead begins with only the starter knife
- **THEN** the knife appears in its stable slot, unowned tool slots are empty, and the bar does not increase carried capacity

#### Scenario: Store a referenced tool
- **WHEN** a slotted tool is moved from the pack into a chest
- **THEN** its slot becomes unavailable and cannot use the stored tool
- **AND** returning that tool to the pack restores the reference

### Requirement: Mouse and keyboard select slots directly
Keys `1-9` and `0` SHALL select their matching slot. Unmodified mouse-wheel movement SHALL cycle slots one step per admitted wheel unit with consistent direction and end-to-start wrapping. Clicking a visible slot SHALL select it.

#### Scenario: Select slot ten
- **WHEN** the player presses `0`
- **THEN** slot ten becomes selected without triggering another gameplay action

#### Scenario: Wheel crosses the end
- **WHEN** the selected slot is at one end and the player wheels beyond it
- **THEN** selection wraps to the opposite end exactly once

#### Scenario: Click a slot
- **WHEN** the player clicks a visible toolbelt slot during gameplay
- **THEN** that slot becomes selected and no world tool action fires from the same click

### Requirement: Controller retains equivalent selection
Gameplay LB/RB SHALL cycle toolbelt selection with the same wrap behavior. While a menu is open, those buttons SHALL retain menu tab behavior rather than changing the gameplay tool.

#### Scenario: Cycle then open a menu
- **WHEN** the player cycles tools with RB and then opens the field book
- **THEN** subsequent LB/RB changes field-book tabs and leaves tool selection unchanged

### Requirement: Selected tools drive authoritative primary use
Left mouse and controller right trigger SHALL request primary use of the selected carried tool against the current valid target. Tool selection or presentation MUST NOT grant items, change world state, expand range, or bypass authoritative requirements.

#### Scenario: Fell with selected hatchet
- **WHEN** a carried Hatchet slot is selected and a valid tree is targeted
- **THEN** primary tool use requests the existing authoritative clear/fell action once and its normal presentation follows only on success

#### Scenario: Use an unavailable slot
- **WHEN** the selected slot is empty, ghosted, or references a non-carried tool
- **THEN** primary tool use does not mutate world or inventory state and gives concise contextual rejection

#### Scenario: Interact without a tool
- **WHEN** the player presses E or controller A near forage, a chest, bed, fire, or water interaction
- **THEN** the existing context interaction remains available independently of the selected tool

### Requirement: Toolbelt and camera wheel inputs do not conflict
Unmodified gameplay wheel input SHALL select toolbelt slots. Ctrl+wheel SHALL adjust gameplay camera distance without changing the selected slot. Existing controller camera-distance input SHALL remain unchanged.

#### Scenario: Zoom with a selected tool
- **WHEN** the player holds Ctrl and wheels during gameplay
- **THEN** camera distance changes while the selected slot remains unchanged

### Requirement: Toolbelt state and UI remain safe
Tool use and selection SHALL be blocked or contextually reassigned while menus, dialogs, planning, failure, load/recovery, or appearance preview own input. Toolbelt assignment and selected slot SHALL survive a current-version save/reload without becoming a user-global preference.

#### Scenario: Click through an open menu
- **WHEN** a menu overlays the toolbelt and the player clicks or presses a number
- **THEN** the menu exclusively owns the input and no tool selection or world action occurs

#### Scenario: Reload a homestead
- **WHEN** a current-version save with toolbelt state is loaded
- **THEN** valid slot references and selection return, while unavailable references remain safely nonusable

### Requirement: Toolbelt coexists with the quiet HUD
The bar SHALL use Homestead's original visual language, scale at supported 720p and 4K views, and avoid overlap with survival meters, contextual feedback, toasts, planning, and protected menu geometry. It MUST NOT add persistent instructional prose.

#### Scenario: Play at 720p
- **WHEN** all ten slots are visible with the lower-left survival stack and lower-right context
- **THEN** slot numbers, icons, availability, and selected state remain readable without overlap or a controls legend

