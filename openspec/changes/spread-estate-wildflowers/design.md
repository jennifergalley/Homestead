# Design

## Context

See proposal.md. Existing flower records use scenery kinds 42-48 and seven original project assets. `lake_path_plants.py` limits them to a 30 m trail corridor. `ClearEstateSceneryUnderPieces` hashes structures/resources each refresh, but does not include plots.

## Goals / Non-Goals

**Goals:** Expand decorative habitat coverage and make cultivation exclusions follow authoritative plot state.

**Non-Goals:** New flowers/materials, forage or crop identities; terrain reshaping; save/version changes; per-tick placement scans; performance claims without exclusive measurement.

## Decisions

- Bake deterministic supplemental habitat drifts from existing terrain weights, water/path geometry and layout polygons rather than spawning individual flower actors or scattering each refresh. Preserve the lake-trail records and all non-flower records; make rebakes idempotent.
- Keep existing kind indices, scale conventions, cell batches, material application and short cull distances. Use restrained densities and gaps, with explicit count summaries, rather than a uniform grid carpet.
- Include farm/manor exclusions in the bake and runtime flower mask. Consume `State.plots`, `GardenCellCenter` and `GardenCellSize` rather than a second tilled-ground state or crop-dependent test. Mask the full flower footprint against the square.
- Gate expensive mask work on Simulation revision and a relevant layout key including plot coordinates. Unchanged refreshes do not inspect instances; energy/time-only revisions do not trigger instance remasking.
- Environment owns supplemental terrain script/data and scenery mask/helpers/tests. Gameplay/UI retains controller/crop files and query semantics. Any actual mesh/material authoring goes to Fishing Art. Integration alone packages.

## Risks / Trade-offs

- Broadening the scenery file could alter unrelated records -> preserve their bytes and validate counts, kinds, exclusions and deterministic rebakes.
- Flower bounds can over-clear neighbouring squares -> test actual scaled footprints against 1 m cells and retain nearby unaffected pockets.
- Cached masks could miss load/new-game changes -> include initial masking and relevant plot identity in invalidation, using existing authoritative state.
- Visual density cannot be accepted from counts alone -> owned in-game spot checks when the lane editor is available; Jenny retains final acceptance.

## Migration Plan

No save schema, placement IDs or bake version change. Deliver scenery data with its code and focused checks; revert the coherent commit if integration rejects it. Reuse existing caches and build once for the C++ batch.
