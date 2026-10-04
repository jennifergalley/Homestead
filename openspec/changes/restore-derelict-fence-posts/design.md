# Design

## Context

See proposal.md. Baseline `490b9d22` uses `AHomesteadDerelictFarm`, a no-tick scenery actor that rebuilds separate ISM batches on construction/BeginPlay. Post/rail placement is not linked to Simulation clearing. The authorized feedback screenshot retains dark mortise liners and small cracks, but not the outer post body.

Confirmed root cause: the recipe's sequential EXACT boolean collapsed the hewn post body at
the second mortise for this seed. Fishing Art repaired the joined mortise cutter with a guarded
EXACT/FLOAT fallback, rebuilt and reimported only the original upright asset at `3e994653`.
Environment cherry-picked that isolated commit as `80d6a496`; no fence C++ repair was needed.
Owned fresh-editor PIE shows the post bodies and seated rails, with all six fence batches
unchanged through actual bramble clearing, adjacent tilling, walking back and save/reload.

## Goals / Non-Goals

**Goals:** Fix the actual rendering defect and preserve existing support ownership.

**Non-Goals:** A new fence-disassembly feature, new placement identities, floating-rail geometry hacks, unrelated fence layout changes or general-code lane asset authoring.

## Decisions

- Audit existing post meshes, LOD/body geometry, material usage and renderer/culling state before choosing a repair. Existing placement code explicitly supplies full-scale transforms; do not assume clearing is responsible simply because the screenshot shows tilled ground.
- Keep supporting posts in the farm actor, independent of cleared resource instances. No new save state is required because there is no existing whole-fence disassembly command in this baseline.
- Environment owns any demonstrated actor/render-code repair and regression evidence. Fishing Art owns the narrow mesh/material repair/import if the audit confirms an asset defect. The audit request names the exact two post assets and supplied screenshot.
- Reuse original FarmFence provenance and ordinary ISM drawing; do not introduce replacement posts or blanket material overrides that hide an authoring failure.

## Risks / Trade-offs

- Body loss may be authored/imported geometry rather than C++ -> specific asset slice goes to Opus Art, with exact root cause and its own coherent commit.
- A material/DDC discrepancy can hide a packaged defect -> inspect final asset material flags and fresh-load evidence, not only local warm-cache appearance.
- New disassembly logic would broaden scope and save semantics -> preserve current scenery lifetime; this fix does not implement disassembly.

## Migration Plan

No save, enum, placement or bake-version change. Deliver the proved code/asset repair together through Integration. Reuse existing build/shader caches; obtain one owned in-game fence check after the shared editor slot is granted.
