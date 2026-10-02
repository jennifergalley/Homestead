# Spec Delta

## Purpose

Make the hotbar represent tools the heroine actually carries and make the starter Knife visible during genuine Knife work, without creating a second inventory or combat system.

## ADDED Requirements

### Requirement: Uncrafted tools are not pictured as owned
The ten numbered hotbar positions SHALL remain available for `1-9/0`, wheel, pointer and controller cycling, but a tool icon SHALL appear only while at least one of that tool is carried. A missing, chest-stored or dropped tool MUST NOT display its icon as a ghost placeholder. Saved assignments MUST NOT grant ownership or make an unavailable tool usable.

#### Scenario: Start with a Knife
- **WHEN** a fresh homestead contains only the starter Knife
- **THEN** the hotbar shows its Knife icon but no Hatchet, Digging Stick or Watering Can icons; unfilled numbered positions do not claim ownership

#### Scenario: Craft and store a tool
- **WHEN** a Hatchet is crafted, then transferred to a chest
- **THEN** its icon becomes visible while carried and disappears while stored; retrieving it restores the appropriate assigned icon without duplicating the tool

### Requirement: Hover and selection show an actual held Knife
When a carried Knife's slot is hovered with the pointer or selected through any supported input, the heroine SHALL show a small original Knife attached to the appropriate hand. Hover alone MUST NOT change the selected slot, use a tool, consume an item or persist equip state. Leaving an unselected hover, changing selection, hiding the hotbar, or losing the Knife MUST clear the transient display.

#### Scenario: Hover while another tool is selected
- **WHEN** the pointer hovers the carried Knife's slot without clicking
- **THEN** the Knife appears in hand for preview, and moving the pointer away restores the selected-tool presentation without a gameplay transaction

#### Scenario: Controller selects Knife
- **WHEN** the player selects the carried Knife by number, wheel, pointer click or controller cycling
- **THEN** the held Knife remains visible until selection or ownership changes, without blocking ordinary movement or camera control

#### Scenario: Drop the only Knife
- **WHEN** the only carried Knife is dropped or stored
- **THEN** both the Knife hotbar icon and its held prop disappear immediately, while the existing authoritative hotbar assignment may be restored on pickup

### Requirement: Knife actions have distinct truthful presentation
Successful existing Knife-eligible clearing of low growth SHALL use a short Knife cutting/recovery animation, not a Hatchet felling pose. At least one Knife-dependent gathering action such as Reeds/Fiber SHALL also visibly use the Knife without changing its current authoritative yield, range or renewal rules. Failed actions MUST NOT trigger cutting, grant resources or replay a queued gesture.

#### Scenario: Clear low growth
- **WHEN** the player uses a carried selected Knife on an eligible low-growth patch and the action succeeds
- **THEN** the patch is cleared exactly once through the existing gameplay rule and the heroine performs one Knife-specific cut with the held prop

#### Scenario: Gather reeds
- **WHEN** an eligible reed patch is gathered with a carried Knife
- **THEN** Fiber is awarded once by the existing inventory authority and a restrained Knife-assisted gather is presented

#### Scenario: Failed or canceled action
- **WHEN** the selected target is invalid, too far away, or the player opens a menu during a Knife gesture
- **THEN** no new yield is granted and presentation stops without a later replay
