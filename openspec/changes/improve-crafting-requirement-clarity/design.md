# Design

## Context

See `proposal.md` for motivation and `specs/crafting-requirement-clarity/spec.md` for behavior. The current Craft page builds six generic legacy rows from `RecipeName` plus one flattened `RecipeRequirements` string. Grid cells include icon and name, all use the same tint, every selected legacy row receives a separate Primary/Craft action, and activation commits instantly. The details pane has no structured cost data.

Simulation already owns every relevant rule in one transaction: `CraftChange` defines consumed inputs and outputs; `Craft` checks failure state, valid position, nearby fueled cookfire, retained knife/hatchet, ingredient sufficiency, and resulting pack capacity before committing atomically. Inventory counts, nearby-fire queries, menu rows, Slate card/detail widgets, icon rendering, focus regions, native menu tests, and an admitted CC0 impact-sound archive already exist. The gap is a read-only representation of those rules plus direct hold presentation, not a second crafting authority or a new external asset source.

## Goals / Non-Goals

**Goals:**

- Expose one read-only structured recipe assessment from Simulation and use it for tile, details, and hold admission.
- Make recipe icons themselves the only selection/crafting surface through a safe real-time hold state machine.
- Preserve recipe selection while readiness changes and keep focus visible on greyed cards.
- Keep transaction-time validation authoritative and test display/authority agreement across every current recipe.
- Add no dependency, asset, save field, recipe ID, or alternate inventory source.

**Non-Goals:**

- New recipes, cost or yield balance, game-time crafting costs, queues/offline work, explicit batch-count dialogs, discovery, station inventories, chest-backed crafting, categories, filters, or shopping lists.
- Replacing native Slate menu architecture or generalizing every menu row into a new data framework.

## Decisions

### 1. Add a read-only Simulation recipe assessment

Simulation will expose a `RecipeAssessment` value for a valid recipe and player point. It contains output identity/count, one entry per consumed item with `have`, `need`, and primary acquisition hint, retained requirements with met state, nearby-station requirements with met state, capacity viability, and an overall `craftable` flag. The implementation derives consumed/output quantities from the same internal `CraftChange` used by `Craft` and shares small predicates for cooking/tool rules so UI and transaction cannot drift through duplicated constants.

Assessment is pure: it does not call `Craft`, modify inventory, increment revisions, emit messages, or approximate future state. Invalid recipe/position and failed player state return unavailable assessments with an explicit blocking reason.

**Alternative considered:** parse the existing `RecipeRequirements` and error strings in Controller. Rejected because prose is not a stable rule API and cannot reliably represent carried counts, retained tools, stations, capacity, or future recipe changes.

### 2. Carry structured assessment through craft-specific menu rows

Craft entries become an explicit recipe subject (or gain equivalent recipe metadata) rather than opaque legacy rows. Controller maps the assessment into menu-facing requirement rows while retaining the recipe enum ID and current label/icon. The existing generic row path remains for other pages.

The menu refresh preserves selection by stable row key. Inventory changes, crafting completion/rejection, load/new-world, and relevant player/station proximity refresh the open Craft page. A lightweight state signature (simulation revision plus current near-fire/failure state) avoids rebuilding every frame while still updating when movement changes station eligibility.

**Alternative considered:** let Slate query Simulation directly from paint lambdas. Rejected because it couples rendering to gameplay authority, makes coherent snapshots harder, and risks card/details/action disagreeing within one frame.

### 3. Use compact icon-only recipe tiles

Recipe cells reuse the compact square icon language planned for Inventory. They render no name or requirement copy. A muted grayscale icon communicates unavailability; the selectable tile remains enabled so pointer/controller/keyboard can focus it and reveal shortages. Focus adds the existing Gold outline with enough contrast over either state.

There is no separate Craft action/button for recipes. Details always render for either state. Hold is admitted only for an available assessment. If completion reaches Controller after a state change, Controller calls authoritative `Craft`; rejection remains atomic, is surfaced normally, and triggers an immediate assessment refresh.

**Alternative considered:** disable the whole unavailable tile. Rejected because disabled Slate widgets cannot be inspected or navigated consistently, hiding the information needed to resolve the shortage.

### 4. Render semantic requirement rows in the existing details pane

The selected recipe details use compact rows:

- consumed ingredient: icon/name, `Have N / Need M`, met or shortage color, plus a concise source only while short;
- retained tool: icon/name, `Carried` versus `Missing`, explicitly marked retained;
- nearby station: station name, `Nearby` versus `Move closer`;
- output and capacity: expected result plus a specific capacity blocker only when relevant.

Rows use current item/recipe names and existing icons. Color supplements but does not replace text. The current details scroll box handles overflow at 720p; no new panel or modal is introduced.

Do not build one formatted `FString`. Use one Slate row per semantic requirement so names, Have, Need, status, source, retained tool, station, and capacity align consistently without `:`/`;` delimiters.

### 5. Drive hold-to-craft with a UI-owned real-time state machine

`SHomesteadMenu` owns transient hold state: recipe ID, physical input owner (pointer/key/gamepad), elapsed real time, completed-cycle count, and next sound beat. Pointer down selects and captures; pointer up/capture loss cancels. Key/controller Pressed starts and Released stops. A quick press selects because no transaction occurs before 1.2 seconds.

Tick advances only while Craft is visible, focused recipe matches, no dialog/page transition is active, input remains held, and a fresh assessment is available. At 1.2 seconds Controller requests one normal `Simulation::Craft`. On success, reset elapsed to zero and re-assess; continued hold starts the next cycle. On rejection/unavailability, stop and refresh details. Page/focus change, Back, menu close, load/recovery, application deactivation, rebuild, or physical release cancels incomplete progress.

The icon renderer draws a grayscale base plus a full-color overlay clipped bottom-to-top by normalized progress. Availability grey and in-progress grey share the base treatment, while only an admitted active hold reveals color. Focus/hover outlines remain separate.

**Alternative considered:** queue a requested batch when hold starts. Rejected because materials/capacity can change after each cycle and the user must be able to stop without pending work.

### 6. Synchronize admitted metal strikes without making audio authoritative

Audit the already admitted Kenney Impact Sounds CC0 archive for exact suitable metal impacts, retain file/hash/import receipts, and import two or three variants (for example `CraftMetalA/B`) only after audition confirms they read as a restrained hammer striking an anvil. The existing attribution already covers the pack; package identity still requires exact files.

Each cycle schedules strikes near 0.08, 0.52, and 0.92 normalized progress, alternating variants/pitch narrowly and respecting Effects volume. Release/cancel stops future beats; no loop component survives the hold. The third beat may coincide with visual completion, but Simulation completion is driven by elapsed time and revalidation, never by audio callbacks.

### 7. Back acquisition hints with current source contracts

Add read-only item-acquisition metadata for current recipe inputs and test it against real authority:

- Branch: fallen branches;
- Stone: loose stone patches;
- Fiber: reeds near water as the pre-hatchet source, with saplings only as a later secondary source;
- Roots and Flowers: their matching forage patches;
- Timber: mature trees with a carried hatchet.

The displayed line stays short (`Reeds near water`); details may include the later sapling alternative without obscuring the bootstrap path. Retained tools use action hints such as `Craft a crude hatchet`, and stations retain specific state such as `Fueled cookfire nearby`.

Tests must prove the named gather source currently yields the item and that its prerequisites do not create a bootstrap cycle. This metadata is explanatory only: it neither chooses a target nor grants resources.

**Alternative considered:** put all acquisition guidance in the Guidebook. Rejected because Jenny encountered the uncertainty while reading a blocked recipe; the answer belongs beside that unmet requirement.

### 8. Validate display/authority parity as a table

Portable simulation tests iterate every recipe across satisfied, each-missing, retained-tool, station, capacity, failed, invalid-position, stale-display, and source-hint parity cases. Native tests verify icon-only tiles, grayscale/color clip progress, exact 1.2-second completion, quick-release no-op, repeated cycle counts, pointer/key/gamepad release, exact Have/Need/source list rows, no separate Craft action, immediate refresh, audio beat bounds, and no mutation from unavailable/stale holds. Ordinary Editor/Shipping evidence covers the six-recipe progression at 720p and 4K.

The simulation/query lane owns `HomesteadSimulation` and portable tests. The menu lane owns controller row mapping, Slate styling/details, and native tests after the assessment shape is stable. Editor, cook, package, and full-loop acceptance remain serialized and reuse compatible caches.

## Risks / Trade-offs

- **[Display rules drift from Craft]** -> Derive both from `CraftChange` and shared rule predicates; require table-driven parity tests for every recipe and blocker.
- **[Grey cards look unfocusable]** -> Keep the card widget enabled, retain gold focus outline, and use muted content rather than disabled-opacity behavior.
- **[Pointer capture/rebuild crafts after release]** -> Menu-level input ownership, explicit release/capture-loss cancellation, stable recipe ID, and no widget-rebuild continuation.
- **[Holding overcrafts accidentally]** -> First completion cannot occur before 1.2 seconds; every cycle is visibly full, revalidated, and immediately stops on release/unavailability.
- **[Audio becomes repetitive or harsh]** -> Audition restrained admitted variants, use three bounded beats, narrow variation, and Effects-volume control.
- **[Movement makes cookfire status stale]** -> Include nearby-station state in the open-page refresh signature and verify approach/leave without reopening Craft.
- **[Capacity messaging is confusing when ingredients are consumed]** -> Assess the exact post-transaction inventory delta rather than output size alone and show capacity only when it is the actual blocker.
- **[Details become dense at 720p]** -> Use one compact row per requirement in the existing scroll pane and verify the largest current recipe without truncation.
- **[Source hints become stale or circular]** -> Validate every named source against real yields/prerequisites and keep the pre-hatchet Fiber route explicitly noncircular.

## Migration Plan

1. Capture current six-card/details/action behavior with empty, partial, sufficient, cooking, and firewood inventories.
2. Add the pure recipe assessment, tested acquisition metadata and portable parity checks without changing Craft behavior.
3. Wire icon-only tiles, grey availability, structured list details, and hold progress without a separate Craft action.
4. Import/audit admitted metal strikes and exercise one/repeated/canceled cycles across gathering, load/new-world and cookfire approach/leave.
5. Run full menu, simulation, full-loop, 720p/4K, Editor, Shipping, and separate-process acceptance.
6. Retain `work-animation-complete-02-shipping / work-actions-v13` as rollback until an immutable candidate passes and is explicitly promoted.
