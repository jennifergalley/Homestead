# Tasks

## 1. Opening dependency catalog

- [x] 1.1 Create the current-status catalog with exact enum IDs, source baseline, materials, five recipes, seven pieces, garments and gathering sources; verify costs and current-unit action timing against Simulation and controller call sites.
- [x] 1.2 Document the bounded knife-to-hatchet-to-shelter/cooking/garden arc and generator-facing sparse-resource roles; verify the graph has a startup path without a pre-cleared home site or mandatory social/hunting gate.

## 2. Quantified handoff and progression

- [x] 2.1 Add illustrative opening and first-shelter supply/action/trip cases with explicit 120-unit pack accounting; verify cumulative 60 Branch, 12 Stone, 22 Fiber construction/tool/fuel demand and source sufficiency without treating the example as a world-wide quota.
- [x] 2.2 Add a provisional prewinter food/fuel/storage budget preserving current needs, and clearly separate current nonperishable storage from proposed preservation and insulation; verify food energy, fuel hours, chest capacity and trips arithmetically.
- [x] 2.3 Add next/later material, tool, station, building, preservation, farming, clothing and comfort tiers with concrete dependency/cost proposals and missing runtime work; verify proposed keys never claim numeric runtime IDs or current playability.

## 3. Data validation and delivery

- [x] 3.1 Implement a small standalone source/data validator for references, quantities, statuses/IDs, tool/station reachability and budget/pack limits; verify the committed catalog passes without native tools or new dependencies.
- [x] 3.2 Exercise malformed reference, invalid quantity, dependency cycle, capacity overflow and insufficient-resource mutations; verify each fails with a specific diagnostic.
- [x] 3.3 Reconcile OpenSpec artifacts with delivered data, run strict validation, inspect the isolated diff and send exact handoff/commit details to coordinator and main; verify no shared runtime files changed.

## Integrated acceptance boundary

This change delivers design data and its checks, not playable new recipes.
Main's later runtime round owns ordinary walking/gathering, chosen-site
clearance, enclosed-shelter placement, food and garden interaction, capacity
rejection, save/reload and visual/play acceptance on representative seeds.
Those are not marked complete by data checks, and no engine execution is
authorized in this lane.

Data evidence: `Scripts/Validate-CraftingProgression.ps1 -SelfTest` passes
51 entities, 55 actions and 11 intentional rejection cases. Both stock ledgers
peak at 80/120 carried units; the winter reserve occupies 88/120 chest units.
Current-source parity includes enum IDs, recipe/build/gather/garment quantities,
nutrition, renewal and wrapper timing. These are not native playtest results.
