# Spec Delta

## Purpose

Makes clothing state compact and visually scannable while presenting the heroine as a living part of the inventory rather than a separate portrait tool.

## ADDED Requirements

### Requirement: Equipment uses compact icon-only body slots
Inventory SHALL show compact icon-only Head, Hands, Torso, Legs, and Feet slots plus a visually separate Apron/accessory-layer slot. The equipment cluster SHALL have no `Equipped slots` heading and no persistent slot-name text.

#### Scenario: View empty body slots
- **WHEN** Head, Hands, or another body slot is empty
- **THEN** its small slot shows an original silhouette icon identifying that body area

#### Scenario: View equipped clothing
- **WHEN** a wearable occupies one or more slots
- **THEN** each occupied slot shows that wearable's icon and no item-name label inside the slot

### Requirement: Slot identity remains available contextually
Pointer hover or focused inspection SHALL identify the body/accessory slot and current wearable in the details surface. Empty, unavailable, and occupied slots SHALL remain distinguishable without relying only on color.

#### Scenario: Focus an empty hand slot
- **WHEN** the player focuses the Hands slot
- **THEN** contextual details identify `Hands` and `Empty` without adding a permanent label beside the slot

### Requirement: Body and accessory state remains authoritative
Head and Hands SHALL be valid empty equipment slots even before matching wearable definitions exist. Existing tunic, apron, shoes, and footwrap equip/unequip/ownership rules SHALL remain authoritative. Multi-slot wearables SHALL be represented consistently in every occupied slot they own.

#### Scenario: Equip the linen tunic
- **WHEN** the tunic equips into its existing torso-and-legs coverage
- **THEN** Torso and Legs both reflect the equipped tunic while its ownership and capacity change only once

### Requirement: Character preview is fixed, transparent, and alive
Inventory and Appearance SHALL show one fixed full-body character view at a readable slightly three-quarter angle, composited directly over the menu layout without a separate visible background, border panel, status paragraph, rotation control, or zoom control. The preview SHALL play the current relaxed idle continuously and update worn clothing promptly.

#### Scenario: Open inventory
- **WHEN** Inventory opens
- **THEN** the heroine appears full-body without a portrait backplate or control strip and continues subtle natural idle motion

#### Scenario: Try portrait navigation input
- **WHEN** the player uses right stick, pointer drag, or former portrait rotation/zoom controls
- **THEN** the preview orientation/framing remains fixed and menu navigation does not expose obsolete portrait controls

#### Scenario: Change clothing
- **WHEN** a wearable is equipped, removed, or dyed
- **THEN** the fixed animated preview refreshes to the authoritative appearance without freezing or resetting to a different angle

### Requirement: The compact composition remains navigable and readable
Body/accessory slots, preview, inventory grid, details, and actions SHALL fit supported 720p and 4K layouts with synchronized pointer/keyboard/controller focus. The preview MUST NOT consume directional focus as an inert region.

#### Scenario: Navigate around the paper doll
- **WHEN** directional input reaches the equipment cluster
- **THEN** focus moves among real slots and adjacent inventory/details controls without landing on the noninteractive character image

