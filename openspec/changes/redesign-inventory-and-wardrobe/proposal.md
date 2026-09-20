# Proposal

## Why

Jenny cannot discover/use the current exit reliably, and the row-based field
book does not provide the visual inventory, contextual details, or wearable-item
ownership she wants. Plan a coherent native menu redesign now, for implementation
only after the woodland environment upgrade is completed and accepted.

## What Changes

- Replace the book's narrow row list with a fullscreen translucent menu shell,
  original top icon tabs with readable labels, item/recipe/plan grids, and a
  persistent contextual details pane for mouse hover and controller/keyboard
  focus. Preserve the warm Pine/cream/Gold identity without copying Coral Island.
- Keep possessions, crafting recipes, building plans, guidebook, settings,
  credits and body/hair appearance distinct. Make Inventory the entry point for
  equipped clothing and a character preview, not a catalog of things not owned.
- Promote Settings' existing save-and-quit operation to a pinned, immediately
  visible action. Add safe confirmation, persistent save-failure recovery and
  an explicit, separately confirmed unsaved exit, including survival recovery.
- Replace free cosmetic outfit toggling with individually owned tunic, apron
  and footwear items that can be crafted, moved, stored and equipped. Preserve
  legacy appearance/dye and existing resource quantities; no free UI spawning.
- Add atomic ownership/equipment transactions, real compatible garment
  geometry, a permanently modest base layer, truthful icons/details and a
  bounded original/verified-license content pipeline.
- Evolve both save layers with exactly-once legacy conversion, stable IDs,
  durable inventory ordering and recovery-safe backups. **Compatibility change:**
  new wardrobe saves are not readable by older builds; rollback must use the
  preserved pre-upgrade candidate/profile snapshot, not overwrite the new save.
- Preserve 120-unit inventory/chest capacity, existing input-intent filtering,
  world mappings, menu pause, Lit save/load guards and isolated preview routing.

## Capabilities

### New Capabilities

- `homestead-menu-overlay`: Fullscreen tabbed native menus, purpose-specific
  grids, details, accessible input navigation and pause/focus lifecycle.
- `safe-game-exit`: Discoverable Settings/recovery exits, truthful save status,
  retry and explicit unsaved-progress decisions.
- `owned-wearable-items`: Item definitions/instances, atomic carried/stored/
  equipped ownership, garment crafting and capacity-conserving transactions.
- `wearable-character-presentation`: Compatible modular garments, modest
  unequipped presentation, appearance separation, icons and provenance.
- `wardrobe-save-migration`: Versioned persistence, deterministic legacy
  conversion, grid ordering, integrity validation and same-world recovery.

### Modified Capabilities

None. `openspec list --specs --json` reports no existing main specifications.
The separate environment change's deltas are not modified by this proposal.

## Impact

Future implementation touches `HomesteadController`, `HomesteadHUD`,
`HomesteadCharacter`, `HomesteadAppearance`, `HomesteadSave`, portable
`Simulation\HomesteadSimulation`, new narrowly scoped native widget/presentation
adapters, the module's UMG/Slate dependencies, character authoring/export/import
scripts, item/icon/garment content definitions and focused native/packaged tests.
Existing save routing and receipt/preview mechanisms remain authoritative.
Design and reference documentation will be updated from the eventual build,
not rewritten now to imply the UI already exists.

## Prerequisites and authorization

Implementation depends on **completed and coordinator-accepted
`upgrade-woodland-environment-assets`**, its accepted package/source baseline,
and the then-current approved and proven offline authoring/import/build/test
workflow. Its current 4/29 progress is not acceptance. This change does not
inherit that run's deadline or permission to launch tools.

OpenSpec 1.13.1's inspected change commands expose no inter-change dependency
option; the prerequisite is explicit here, in design, specs and task 1.1, not
invented YAML metadata. A new apply authorization is required after those gates.
This deliverable is planning only: no engine/Blender launches, code or asset
edits, downloads, purchases, automation, merge, push or PR.

## Non-goals

No map/social/buff system, recipe unlock progression, shops, armor/durability,
new warmth balance or winter expansion, face/hair redesign, cloth physics,
CommonUI framework rollout, web interface, external image upload, or copying
Coral Island icons/fonts/characters. The first supplied inventory reference was
reviewed; the other two were unavailable under the image quota. Written
requirements govern anything not visible in that first image.
