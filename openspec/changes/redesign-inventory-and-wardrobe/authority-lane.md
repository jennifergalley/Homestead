# Owned wardrobe authority execution

## Authorization and scope (2026-09-20)

The coordinator's explicit newer user authority permits independent isolated
source implementation now, integrated after environment delivery. Completion,
not a time/credit ceiling, governs this lane. Existing saves are disposable test
data until Jenny says otherwise: reliable current saving is required, historical
v2/v3 portable and v1-v4 wrapper migration/snapshot infrastructure is not.
These instructions supersede the old sequential apply gate and mandatory legacy
migration work for this lane; they do not authorize engine/authoring launches.
The UI owner is revising the central proposal/design/specs/tasks. This addendum
does not claim the old central tasks are fulfilled or edit them concurrently.

Ownership: `Simulation\*`, `HomesteadSave.h`, directly associated portable tests.
No Controller/HUD/UI/character/world/asset/shared-runner edits. No schedules,
Notion writes, purchases, firewall changes or public uploads.

## Implementation contract

- Keep the 14 fungible IDs and five existing recipe IDs. A separate four-entry
  wearable catalog defines linen tunic, linen apron, leather shoes (starter only)
  and woven footwraps. Fiber costs are 12/6/8; crafting requires the existing knife.
- Stable positive instance/group IDs are separate from world IDs. Torso/Legs
  reference the same equipped tunic; apron requires it; Feet holds either shoe
  definition. One owner per instance: pack, chest ID, or equipped.
- Containers remain 120 total units, garments cost one stored/carried unit and
  zero equipped units. Ordered positive fungible groups partition the aggregate
  stock; wearable entries reference the owned instances. Cells add no capacity.
- New actions use expected runtime inventory revision and `Result` with typed
  failure code/revision. Successful actions invalidate repeated confirmations;
  load/new-game invalidate stale UI drafts. Cancel submits nothing. Required
  render assets are preflighted by the caller before equipment/craft submission.
- Pack/chest transfer validates 280 cm reach at commit, including selected
  fungible groups. Craft/equip/replace/unequip/dye/layout commit atomically.
  Clothing never changes the persisted `warmOutfit` state or survival balance.
- Portable schema v4 is current-only, integrity checked and fully validated
  before replacing live state. Unsupported old test saves return an explicit
  error without resetting or writing anything. Wrapper v5 is coordinated with
  the UI owner, who owns Controller compatibility checks/reset notification and
  existing protected file IO. Legacy cosmetic fields remain ABI/source-compatible,
  not ownership authority. New game grants worn tunic/shoes plus existing knife.

## Lane checklist

- [ ] Stable catalog/ownership/equipment and capacity-safe atomic actions.
- [ ] Persistent layout split/merge/reorder and quantity reconciliation.
- [ ] Current portable v4/wrapper v5 contract, validation and defaults.
- [ ] Focused portable regression evidence, integration notes, private commit/push.

No compiled source in this lane alone establishes shipped wardrobe gameplay,
rendered modular garments, accepted art or current-playable integration.
