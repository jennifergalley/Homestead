# Design

## Context

See `proposal.md` for motivation and `specs/crafting-requirement-clarity/spec.md` for behavior. The current Craft page builds six generic legacy rows from `RecipeName` plus one flattened `RecipeRequirements` string. All cards use the same tint, every selected legacy row receives a Primary/Craft action, and the details pane has no structured cost data.

Simulation already owns every relevant rule in one transaction: `CraftChange` defines consumed inputs and outputs; `Craft` checks failure state, valid position, nearby fueled cookfire, retained knife/hatchet, ingredient sufficiency, and resulting pack capacity before committing atomically. Inventory counts, nearby-fire queries, menu rows, Slate card/detail widgets, icon rendering, focus regions, and native menu tests already exist. The gap is a read-only representation of those rules, not a second crafting system or a new asset.

## Goals / Non-Goals

**Goals:**

- Expose one read-only structured recipe assessment from Simulation and use it for card, detail, and action presentation.
- Preserve recipe selection while readiness changes and keep focus visible on greyed cards.
- Keep transaction-time validation authoritative and test display/authority agreement across every current recipe.
- Add no dependency, asset, save field, recipe ID, or alternate inventory source.

**Non-Goals:**

- New recipes, cost or yield balance, crafting duration, queues, batches, discovery, station inventories, chest-backed crafting, categories, filters, or shopping lists.
- Replacing native Slate menu architecture or generalizing every menu row into a new data framework.

## Decisions

### 1. Add a read-only Simulation recipe assessment

Simulation will expose a `RecipeAssessment` value for a valid recipe and player point. It contains output identity/count, one entry per consumed item with `have` and `need`, retained requirements with met state, nearby-station requirements with met state, capacity viability, and an overall `craftable` flag. The implementation derives consumed/output quantities from the same internal `CraftChange` used by `Craft` and shares small predicates for cooking/tool rules so UI and transaction cannot drift through duplicated constants.

Assessment is pure: it does not call `Craft`, modify inventory, increment revisions, emit messages, or approximate future state. Invalid recipe/position and failed player state return unavailable assessments with an explicit blocking reason.

**Alternative considered:** parse the existing `RecipeRequirements` and error strings in Controller. Rejected because prose is not a stable rule API and cannot reliably represent carried counts, retained tools, stations, capacity, or future recipe changes.

### 2. Carry structured assessment through craft-specific menu rows

Craft entries become an explicit recipe subject (or gain equivalent recipe metadata) rather than opaque legacy rows. Controller maps the assessment into menu-facing requirement rows while retaining the recipe enum ID and current label/icon. The existing generic row path remains for other pages.

The menu refresh preserves selection by stable row key. Inventory changes, crafting completion/rejection, load/new-world, and relevant player/station proximity refresh the open Craft page. A lightweight state signature (simulation revision plus current near-fire/failure state) avoids rebuilding every frame while still updating when movement changes station eligibility.

**Alternative considered:** let Slate query Simulation directly from paint lambdas. Rejected because it couples rendering to gameplay authority, makes coherent snapshots harder, and risks card/details/action disagreeing within one frame.

### 3. Grey unavailable cards without disabling selection

`CellColor` and icon/text tint distinguish unavailable recipes with muted neutral colors. The selectable button remains enabled so pointer/controller/keyboard can focus it and reveal shortages; focus adds the existing gold outline with enough contrast over the grey state.

The action list includes Craft only for an available assessment. Details always render for either state. If activation reaches Controller after a state change, Controller calls authoritative `Craft`; rejection remains atomic, is surfaced normally, and triggers an immediate assessment refresh.

**Alternative considered:** disable the whole recipe button. Rejected because disabled Slate widgets cannot be inspected or navigated consistently, hiding the information needed to resolve the shortage.

### 4. Render semantic requirement rows in the existing details pane

The selected recipe details use compact rows:

- consumed ingredient: icon/name, `Have N / Need M`, met or shortage color;
- retained tool: icon/name, `Carried` versus `Missing`, explicitly marked retained;
- nearby station: station name, `Nearby` versus `Move closer`;
- output and capacity: expected result plus a specific capacity blocker only when relevant.

Rows use current item/recipe names and existing icons. Color supplements but does not replace text. The current details scroll box handles overflow at 720p; no new panel or modal is introduced.

### 5. Validate display/authority parity as a table

Portable simulation tests iterate every recipe across satisfied, each-missing, retained-tool, station, capacity, failed, invalid-position, and stale-display cases. Native tests verify card tint/focus, exact Have/Need values, disabled Craft affordance, immediate refresh, and no mutation from unavailable activation for pointer, keyboard, and controller. Ordinary Editor/Shipping evidence covers the six-recipe progression at 720p and 4K.

The simulation/query lane owns `HomesteadSimulation` and portable tests. The menu lane owns controller row mapping, Slate styling/details, and native tests after the assessment shape is stable. Editor, cook, package, and full-loop acceptance remain serialized and reuse compatible caches.

## Risks / Trade-offs

- **[Display rules drift from Craft]** -> Derive both from `CraftChange` and shared rule predicates; require table-driven parity tests for every recipe and blocker.
- **[Grey cards look unfocusable]** -> Keep the card widget enabled, retain gold focus outline, and use muted content rather than disabled-opacity behavior.
- **[Movement makes cookfire status stale]** -> Include nearby-station state in the open-page refresh signature and verify approach/leave without reopening Craft.
- **[Capacity messaging is confusing when ingredients are consumed]** -> Assess the exact post-transaction inventory delta rather than output size alone and show capacity only when it is the actual blocker.
- **[Details become dense at 720p]** -> Use one compact row per requirement in the existing scroll pane and verify the largest current recipe without truncation.

## Migration Plan

1. Capture current six-card/details/action behavior with empty, partial, sufficient, cooking, and firewood inventories.
2. Add the pure recipe assessment and portable parity tests without changing Craft behavior.
3. Wire craft-specific rows, grey availability treatment, structured details, and unavailable-action behavior.
4. Exercise immediate refresh across gathering/crafting/load/new-world and cookfire approach/leave.
5. Run full menu, simulation, full-loop, 720p/4K, Editor, Shipping, and separate-process acceptance.
6. Retain `work-animation-complete-02-shipping / work-actions-v13` as rollback until an immutable candidate passes and is explicitly promoted.
