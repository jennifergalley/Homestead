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

- [x] Stable catalog/ownership/equipment and capacity-safe atomic actions.
- [x] Persistent layout split/merge/reorder and quantity reconciliation.
- [x] Current portable v4/wrapper v5 contract, validation and defaults.
- [x] Focused portable regression evidence, integration notes, private commit/push.

No compiled source in this lane alone establishes shipped wardrobe gameplay,
rendered modular garments, accepted art or current-playable integration.

## Verification and integration handoff

The existing portable suite plus six wardrobe scenarios passed with MSVC
14.44.35207, `/std:c++17 /W4 /WX /EHs-c- /D_HAS_EXCEPTIONS=0 /MT /Od`.
Separate direct `cl /c`, `cl /c`, `link /INCREMENTAL:NO /MANIFEST:NO`, and test
execution used the coordinator's unchanged pinned guard/evidence helpers, the
approved existing SDK 10.0.26100.0 include/lib directories, and fresh session-only
outputs. No UBT, UE, Blender, vcvars, engine headers or player save access.
Initial adapter attempt failed before process creation because PowerShell
converted a null optional argument to an empty string; using the existing
boolean overload fixed it without changing containment.

Evidence: session `6e4e8162-6f64-459c-a766-ba613d11c82f`,
`files\native-tests-02\{compile-simulation,compile-tests,link,tests}\result.json`
and `stdout.log`. All four returned 0 with one total process, zero active
processes at completion, verified marker cleanup and no owned endpoints in
their finite samples. The test run passed 22 scenarios / 1714 explicit checks.
Sampling is not continuous network tracing. CMake's generic launcher was not
run under the current coordinated-tool restriction.

UI/controller integration:

- Read `Simulation::GetRevision()` when constructing a transaction dialog and
  pass that exact revision at confirmation. Success and stale-result revisions
  are available in `Result`; refresh snapshots via `GetRevision()` after other
  outcomes. Rejected operations preserve serialized bytes and revision.
  A duplicate successful confirmation becomes stale. NewGame/Deserialize
  invalidate existing drafts; revisions are session-local, not saved data.
- `GetWearable`, `GetLayout` and `GetState` return transient const views.
  Retain IDs, not their pointers/references, across successful mutations.
  Container 0 is the pack; positive IDs identify chests. `ChestUsedCapacity`
  returns -1 for a missing/non-chest ID.
- `GetWearableDefinition`, `WearableName`, `WearableDescription`,
  `GarmentRequirements` and `DyeName` are the UI/catalog contract. The catalog
  exposes stable string keys, occupied-slot bitmask, dye support and actual
  fiber cost. Do not infer recipe identity or actions by parsing display copy.
- Garment caller must preflight prepared compatible render assets before
  craft/equip submission. This portable lane deliberately has no UE asset
  handles and does not claim art is available.
- Preserve existing `Recipe` IDs for tools/cooking; new garment crafting uses
  `CraftGarment(WearableDefinition, Point, expectedRevision)`. Old aggregate
  `Transfer` remains supported, now with combined garment capacity and 280 cm
  reach; selected-group UI uses revision-guarded `TransferGroup`.
- Controller must require `UHomesteadSave::CurrentVersion`/`IsCurrentVersion()`
  plus successful portable decode, retain protected temp/readback/backup IO,
  and handle explicit incompatible-test-save reset separately from corruption.
  Old Outfit/TunicColor fields remain source-compatible legacy fields, not
  wardrobe ownership. This lane does not edit Controller or its save routing.
- The central plan remains owned by the UI lane. Its mixed persistence/UI/art
  tasks cannot be checked complete from this source-only test result.

## Selected food group follow-up (2026-09-20)

UI integration identified one missing selected-source operation: aggregate
`Eat(Item)` consumes fungible groups in display order rather than the selected
split stack. Add `EatGroup(groupId, expectedRevision)` for carried food only,
sharing the existing food effects, preserving zero-time semantics, and staging
the chosen group's debit with hunger and aggregate stock in one transaction.
Test all three foods, selected non-first and one-unit groups, repeated/stale
confirmation, full hunger, invalid/nonfood/stored-only groups and failed state.
This is the existing plan's explicit-source-first reconciliation contract, not
a new food mechanic. Schema and prior published APIs remain unchanged.

- [ ] Implement, verify and privately checkpoint selected-group eating.

Source and focused checks are implemented. The follow-up has not yet been
compiled/executed: the coordinator paused new compiler admission to attribute
an unexpected VCTIP process outside this lane's recorded launches. No new tool
launch or unowned-process stop was attempted here. This source checkpoint does
not inherit the earlier 22-scenario pass as proof of the new action.
