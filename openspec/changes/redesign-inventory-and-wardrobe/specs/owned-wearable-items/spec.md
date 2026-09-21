# Spec Delta

## Purpose

Make clothing persistent possessions that can be crafted, carried, stored and
worn without duplication, loss or an unrequested slot-based inventory economy.

## ADDED Requirements

### Requirement: Wearables have stable identity and one owner
Each wearable SHALL have a stable instance identity, definition and persistent
dye/variant data, with exactly one owner: carried inventory, one identified chest,
or equipped. Equipped references SHALL never be additional copies.
Fungible quantities SHALL remain distinct from nonstacking wearable instances.

#### Scenario: Two tunics share an appearance
- **WHEN** two tunics use the same definition and dye
- **THEN** they remain independently selectable instances and moving one preserves the other's identity and location

### Requirement: Inventory exposes genuine equipment
Inventory SHALL show the character and equipment slots and support equip,
unequip and replace using owned items. This release SHALL support an individually
owned tunic occupying torso/legs, a separate apron layer and separate footwear.
Body, face, hair and eye choices SHALL remain appearance choices, not owned items.

#### Scenario: Unowned outfit is requested
- **WHEN** the player inspects crafting or appearance choices
- **THEN** no outfit toggle creates or equips an unowned garment; a recipe is clearly a recipe and requires a successful craft transaction

### Requirement: Compatibility and replacement are atomic
Equip operations SHALL validate fit, occupied slots, dependent layers and the
capacity needed to relocate every displaced item before changing ownership.
An apron SHALL require a compatible tunic. Removing/replacing a garment SHALL
either handle all dependent garments in one confirmed transaction or reject
with an explanation; partial equipment changes are forbidden.

#### Scenario: Remove a tunic under an apron
- **WHEN** unequipping the tunic requires moving both tunic and apron to the pack
- **THEN** both move together only if capacity permits, otherwise both remain equipped with unchanged IDs and dye

#### Scenario: Replace from a full pack
- **WHEN** a carried compatible tunic replaces an equipped tunic
- **THEN** final-state capacity accounts for the outgoing carried item before the displaced garment returns, allowing a valid one-for-one swap without temporary overfill or duplication

### Requirement: Capacity remains a unit count
Pack and each chest SHALL retain a total capacity of 120 units, counting each
carried/stored wearable as one unit and each existing material/tool unit as one.
Equipped items SHALL not consume carried capacity. Grid cells, split groups and
sorting SHALL NOT impose a new slot limit or erase overlaid quantities.

#### Scenario: Full chest receives clothing
- **WHEN** a chest containing 120 units is the target of a one-item clothing transfer
- **THEN** the transfer is rejected with no ownership change even if the grid visually has empty cells

#### Scenario: Unequip into a full pack
- **WHEN** the pack already contains 120 units and the player tries to unequip footwear
- **THEN** the footwear stays equipped and the UI explains that one pack unit must be freed

### Requirement: Transfers and quantity operations conserve possessions
Move, reorder, split, merge and chest transfer SHALL validate source, amount,
destination, reach and final capacity before commit. Nonstacking clothing SHALL
not expose split. Canceling or rejecting any operation SHALL preserve all counts,
instance IDs and ownership. Chest access SHALL preserve the current 280 cm reach.

#### Scenario: Move a selected quantity to storage
- **WHEN** the player confirms transferring seven units from a larger stack to a reachable chest
- **THEN** exactly seven leave the source and enter the chest, with valid remaining grouping and no capacity change caused by the number of displayed stacks

#### Scenario: Stale target or repeated confirm
- **WHEN** a chest disappears, is no longer reachable, or the same submitted UI operation is delivered twice
- **THEN** stale/duplicate submission cannot apply a second transfer and the UI reports rejection or returns the existing result without losing items

### Requirement: Clothing crafting has authoritative costs
Each craftable garment SHALL have a named recipe, finite positive resource
costs, truthful prerequisites and an admitted renderable definition. Crafting
SHALL atomically consume costs and create one owned instance only when the
result fits. Existing recipes and starting resource quantities SHALL remain
unchanged; previewing items SHALL never create them.

#### Scenario: Garment recipe cannot complete
- **WHEN** resources, prerequisite tool, output capacity or compatible garment content are missing
- **THEN** no ingredient is consumed and no wearable appears, and the exact blocking reason is shown

#### Scenario: Successful garment craft
- **WHEN** the player confirms an affordable admitted garment recipe
- **THEN** costs are deducted once, one uniquely identified item appears in carried inventory, and recipe details match the actual transaction

### Requirement: Dye belongs to clothing rather than the character
Garment dye SHALL be per-instance appearance data. The existing limited
recoloring affordance SHALL operate only on an owned selected garment, preserve
its identity, and remain explicitly cosmetic; a paid/resource dye economy is
not part of this change.

#### Scenario: Store a recolored tunic and equip another
- **WHEN** an owned tunic is recolored, stored and later retrieved after another tunic is worn
- **THEN** each retains its own color across the transfer and save/reload rather than inheriting a global outfit tint

### Requirement: Clothing does not silently rebalance survival
This wardrobe SHALL not introduce armor, durability, warmth bonuses or other
new gameplay statistics. Existing survival state and any legacy warmth flag
SHALL be preserved independently; cosmetic apron selection SHALL not claim
winter insulation.

#### Scenario: Equip a different outfit at night
- **WHEN** the player changes between admitted cosmetic garments
- **THEN** hunger, energy and warmth rules remain unchanged and the details pane makes no unsupported gameplay-benefit claim
