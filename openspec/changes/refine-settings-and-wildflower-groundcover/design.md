# Design

## Context

See `proposal.md` for motivation. The native Slate overlay currently uses seven field-book tabs: Inventory, Craft, Build, Guidebook, Settings, Credits, and Appearance. Settings is internal page 4 but still uses the same two-column `SUniformGridPanel`, generic content cells, detail/action sidebar, and tab traversal. Esc calls the controller's general Back path: it closes an open book, cancels planning, or opens page 4 from gameplay. Controller Menu/Start currently toggles the field book, while controller View/Back opens Guidebook.

Day length is row 2 and currently exposes implementation values as `Day length: N minutes`, cycling 30/60/120 real minutes per game day. Music, Ambience, and Effects are rows 5-7 whose activation cycles values by 20%; their values are still loaded from and written into world saves. Autosave is currently hidden, always enabled, counts down only during ordinary unpaused gameplay, and writes one of three rotating slots every fixed 240 seconds plus another rotating autosave after sleep. Camera/video settings already establish a safer exact-property user-settings pattern.

The admitted Flower Empodium CC0 asset already supplies two 758-triangle clumps, authored alpha/PBR material, verified import, and cooked packages. Interactive flower resources use grouped full-size clumps. Existing generated cover is deterministic, chunk-owned HISM with no collision/navigation/overlap and reservation-aware rebuilds.

## Goals / Non-Goals

**Goals:**

- Make Settings a distinct Esc/Menu overlay with a vertical preference list.
- Replace audio cycling with real accessible sliders and user-level persistence.
- Remove Credits from in-game navigation while retaining legal attribution files.
- Add restrained noninteractive flower color through existing cover batching.
- Preserve every safety modal, save/recovery path, field-book destination, interaction resource, current save compatibility, and selected-build performance.

**Non-Goals:**

- Keybinding UI, dropdown framework, audio device selection, localization, advanced graphics presets, or a full menu visual redesign.
- Deleting or weakening repository/package attribution.
- New flower downloads, seasonal ecology, pollinators, decorative gathering, collision, or gameplay rewards.

## Decisions

### 1. Separate overlay mode from field-book page identity

Controller will distinguish gameplay overlays semantically: FieldBook, Settings, and Recovery. Esc from ordinary gameplay opens Settings; Esc/B/Menu inside Settings closes it. If placement planning is active, the first Esc retains existing safe cancellation and a subsequent Esc opens Settings. Controller Menu/Start opens Settings; controller View/Back retains Guidebook. `I`/Tab, `C`, and `B` continue opening Inventory, Craft, and Build; `G` opens Guidebook directly during gameplay.

`G` remains state-aware: `InputKey` already lets the open native menu consume physical input before gameplay bindings, so contextual menu withdrawal/secondary behavior can remain available while `G` routes to Guidebook only when no menu owns the key.

The field-book tab bar becomes Inventory, Craft, Build, Guidebook, and Appearance. Internal page IDs may remain noncontiguous to minimize save/test churn; tab navigation uses an explicit ordered page list rather than modulo seven. Settings and Recovery render outside that list.

**Alternative considered:** keep Settings as hidden page 4 but only restyle it. Rejected because Esc routing and the requested separation would remain conceptually tangled.

### 2. Build a dedicated vertical Settings renderer

Settings uses one wide scroll column with semantic row types: action, toggle, choice, cycle, and slider. Resume, Save, Quit game, load/new woodland, camera/video preferences, and audio sliders share the list but not inventory cells or item actions. The detail/action sidebar is omitted; descriptions and values live in each row.

Directional navigation is vertical. Left/right edits the focused adjustable row; Enter/A toggles or activates; pointer clicks and slider drags focus the exact row. Existing modals remain over the same overlay and preserve focus traps/defaults.

### 3. Implement sliders with runtime preview and commit-on-release

Each audio row uses `SSlider` from 0 to 1 with percent text. Pointer movement applies runtime volume continuously for audible feedback but writes one exact config property on mouse/controller capture end. Keyboard/D-pad left/right changes in 5% increments and persists each discrete adjustment. A failed write restores the pre-edit slider and audio multiplier.

`Homestead.Audio` stores Music, Ambience, and Effects in the resolved `GameUserSettings.ini`, using the camera preference snapshot/single-property/readback/rollback pattern. Startup loads each independently before a world save. Current world-save fields remain readable/writable for compatibility but `ApplySave` stops making them authoritative.

**Alternative considered:** save on every pointer movement. Rejected because it would write disk dozens of times during one drag.

### 4. Present day duration as Game speed

Render the existing three values as a segmented/choice row:

- Leisurely -> 120-minute day;
- Balanced -> 60-minute day;
- Fast -> 30-minute day.

Pointer clicks select a labeled segment directly. Keyboard/controller Left/Right moves among the three choices. The selected option has the standard visible focus/selection treatment. Do not show raw minutes or add explanatory instructions; optional concise row descriptions can say `Long days`, `Default pace`, and `Short days`.

This is a presentation/input refinement over the existing simulation value. It preserves current world/save authority and timing math; it does not invert the mapping, add new speeds, or turn day pace into an audio/camera user preference.

**Alternative considered:** keep one cycling row and rename only its title. Rejected because the available values should be visible and directly selectable rather than hidden behind repeated activation.

### 5. Use one quit-choice dialog

Replace the current top-level `Save and quit` action plus chained Exit -> Unsaved confirmation flow with two Settings actions:

- `Save`: call the normal manual-save path, keep Settings open, and show concise success/error state in the existing Settings feedback area;
- `Quit game`: open one modal with `Save & Quit` and `Quit without Saving`; Back cancels.

Initial focus is `Save & Quit`, not the destructive no-save option. Save & Quit sets the existing in-progress guard, writes the manual save, and exits only on success. Failure leaves the same modal open with its error while both choices remain available. Quit without Saving exits immediately from that dialog. Remove `Stay in Settings` as a dialog button and remove the chained `Unsaved` confirmation state from this route.

Failed/recovery state cannot save over the checkpoint: direct Save and Save & Quit are disabled or reject in place with the existing reason; Quit without Saving remains available without another prompt.

**Alternative considered:** retain a separate no-save confirmation for safety. Rejected because the two explicit quit choices already communicate the consequence and Jenny wants no additional confirmation.

### 6. Remove in-game Credits but retain attribution

### 6. Make the existing rotating autosave configurable

Add two user-level Settings rows:

- Autosave: On / Off;
- Autosave interval: 5 / 10 / 20 / 30 minutes.

Store exact validated properties in `GameUserSettings.ini` under `Homestead.Autosave`, using the camera/audio snapshot-write-readback-rollback pattern. Defaults are enabled and five minutes. World saves do not override these preferences.

The timer retains current real-unpaused-gameplay semantics: it advances only while the world is playable, no menu/planning/failure/load/save is active, and the session is allowed to save. Enabling or changing interval resets to one full selected interval; disabling stops the timer without deleting autosave files. Successful writes rotate Auto0/1/2 and reset the full interval. Failure preserves existing saves, reports once, and schedules a bounded 60-second eligible-gameplay retry rather than frame-spamming.

When Autosave is Off, sleep skips the rotating auto slot but may still write the separately named sheltered recovery checkpoint under its existing safety conditions. Manual Save, Save & Quit, F5, loading existing autosaves, and retained backups are unaffected.

The interval row remains visible but disabled/muted while Autosave is Off so its configured value is inspectable without implying an active countdown. Pointer selects options directly; Left/Right edits focused choices.

**Alternative considered:** store autosave controls per world. Rejected because optional save cadence is a player preference and must work even when Jenny playtests without saving the current homestead.

### 7. Remove in-game Credits but retain attribution

Credits is removed from tab labels/icons, controller row generation, navigation tests, page summaries, and captures. The icon implementation may be deleted if no other surface uses it. `docs/asset-credits.md` remains copied into every package and is included in acceptance receipts.

This is a UI removal, not removal of copyright/license notices from distributed files.

### 8. Show preview identity only in Settings

The existing `PreviewLabel()` remains the single source for isolated preview profile/save-routing identity, but the gameplay Canvas footer stops drawing it. Settings conditionally adds a noninteractive build-information row only when `PreviewLabel()` is nonempty. The generic heroine/technical-stand-in gameplay footer is removed with the same permanent chrome; character maturity remains documented in project/release notes rather than occupying the HUD.

**Alternative considered:** retain a shortened version watermark. Rejected because Jenny wants preview/version information only in Settings.

### 9. Reuse Flower Empodium as distinct decorative HISM cover

The cover builder loads `SM_FlowerEmpodium_a/b` through their admitted authored material and creates at most two chunk-owned HISM components tagged `DecorativeWildflower`. A bounded deterministic subset (target approximately 12-20 accepted clumps per intersecting 24 m chunk) uses modest 0.55-0.8 uniform scale and small pocket clustering. Interactive flower resources retain larger grouped presentation and separate focus/produce state.

Placement calls the existing low-cover reservation authority and additionally keeps clear of stream/muddy-bank distance. Decorative batches have no collision, overlap, navigation, reward, save record, or focus path. Their chunk cover signature already includes resource/structure/plot changes, so cleared sites remain reserved and new player content triggers correct rebuild.

**Alternative considered:** create primitive colored flower dots. Rejected because the verified authored CC0 mesh/material already exists and visually belongs to the world.

### 10. Separate independent lanes and serialize integration

The settings lane owns controller overlay routing, Slate list/sliders, audio config, and menu tests. The flower lane owns only world-cover batching/exclusions and environment tests. Shared Editor/cook/package execution and final full-loop/menu acceptance remain serialized. Both reuse selected build caches and `work-actions-v13` rollback.

## Risks / Trade-offs

- **[Esc conflicts with close/cancel expectations]** -> Preserve planning cancel first and back-to-close inside open overlays; test gameplay, field book, Settings, modals, quantity edit, planning, failure, and Look separately.
- **[Controller has no obvious field-book button after Menu becomes Settings]** -> Retain View/Back for Guidebook plus purpose-specific face/keyboard paths; update on-screen/onboarding prompts rather than adding permanent HUD chrome.
- **[Slider disk writes or pointer capture become noisy]** -> Apply runtime during drag, persist on capture end, and verify exactly one property write/readback per completed edit.
- **[Speed labels imply the opposite mapping]** -> Assert Leisurely=120, Balanced=60 and Fast=30 in source/native/save routes and avoid raw minute copy in the player-facing row.
- **[No-save exit happens accidentally]** -> Keep one explicit Quit game entry, default modal focus on Save & Quit, visually separate the no-save choice, and let Back cancel; do not add another modal.
- **[Save & Quit failure strands or exits the player]** -> Exit only after verified save success and retain the same dialog plus error on failure.
- **[Autosave off weakens recovery unexpectedly]** -> Disable only periodic/sleep rotating autosaves; preserve explicit/manual saves and the separately governed sheltered recovery checkpoint.
- **[Autosave failure writes every frame]** -> Preserve existing files, report once, and retry after a bounded eligible-gameplay delay.
- **[Preference write fails]** -> Exact readback and rollback restores the prior enabled/interval pair; malformed startup uses On/5 defaults.
- **[Legacy save load changes audio]** -> Load user preferences first and remove only volume authority from `ApplySave`; retain serialized fields until a future save-version change.
- **[Decorative flowers look gatherable]** -> Keep them smaller/sparser than interactive grouped patches, never focus them, and inspect ordinary approach behavior.
- **[Flower overdraw or cover rebuild regresses cadence]** -> Cap accepted instances/components, use existing cull distances/HISM, record triangle/count metrics, and reject on matched performance evidence.
- **[Credits removal weakens attribution]** -> Gate packaging on the existing `asset-credits.md` copy and receipt checks.
- **[G conflicts with contextual menu actions]** -> Route the open native menu before gameplay bindings and test storage/recovery/quantity contexts separately from ordinary gameplay.
- **[Preview build identity becomes hard to inspect]** -> Keep exact routing/profile metadata in Settings and automated receipts while removing it only from gameplay chrome.

## Migration Plan

1. Record current Settings grid, Credits navigation, Esc/controller routes, volume cycling/persistence, and representative flower-free ground baselines.
2. Deliver separate Esc/Menu Settings with vertical action/toggle/choice rows, direct Save plus one-dialog Quit flow, Game speed labels, gameplay G-to-Guidebook routing, Settings-only preview metadata, and remove Credits while preserving recovery and contextual menu routes.
3. Add user-level Autosave On/Off plus interval controls over the existing rotating-save system.
4. Add persistent audio sliders with pointer/controller/keyboard and read-only rollback coverage.
5. Add decorative flower batches and verify visual distinction, exclusions, lifecycle, and performance.
6. Run Editor native-menu/directional/input/HUD/environment/full-loop/save tests and inspect 720p/4K Settings plus ordinary flower walks.
7. Build one immutable Shipping candidate, run producer/consumer and normal-launch acceptance, and promote only if Settings is simpler and flowers improve the ground without confusion or regression.
8. Retain `work-actions-v13` as rollback until all gates pass.
