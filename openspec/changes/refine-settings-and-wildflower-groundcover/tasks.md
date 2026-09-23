# Tasks

## 1. Separate Settings Screen

- [x] 1.1 Record selected-build Esc/controller menu routes, gameplay/menu `G` behavior, raw `Day length: 30/60/120 minutes` cycle, top-level Save progress/Save-and-quit actions and chained Exit/Unsaved confirmations, seven-tab traversal, Settings grid/focus/action behavior, preview/gameplay footers, recovery/save-failure paths, Credits content, and 720p/4K frames so the baseline is reproducible
- [x] 1.2 Separate Settings from the field-book tab order, bind gameplay Esc and controller Menu/Start directly to Settings, bind gameplay `G` directly to Guidebook while preserving menu-owned `G`, retain planning cancel and other field-book purpose bindings, replace Day length with direct Game speed choices, add direct Save and Quit game rows, replace chained exit prompts with one Save & Quit/Quit without Saving dialog plus Back cancel, move preview identity to a conditional Settings-only row, remove gameplay footers and Credits page/tab/icon, and render all Settings controls/actions as one vertical list; verify source/native navigation/focus/save contracts
- [ ] 1.3 Exercise keyboard, controller and pointer Settings entry/exit, direct G Guidebook, contextual menu G, Game speed mapping, Save-success/failure/failed-state, Quit cancel, Save & Quit success/failure and immediate Quit without Saving with no second confirmation, every other adjustable/action row, field-book wrap, Appearance, quantity editing, planning, recovery, restart and graphics failure, verifying pause, safe defaults, focus return, no chained Unsaved prompt, Settings-only preview metadata and no Credits surface

## 2. Persistent Audio Sliders

- [x] 2.1 Load independently validated Music/Ambience/Effects values from exact user-settings properties and stop world saves from overriding them while retaining current save compatibility; verify missing/malformed defaults and conflicting save/new-world cases
- [ ] 2.2 Implement labeled 0-100% sliders with continuous pointer runtime preview, commit-on-release persistence and 5% keyboard/controller left/right increments; verify min/max, device switching, exact one-property readback, read-only rollback and no stuck capture/edit state
- [x] 2.3 Run fresh write/read/invalid separate-process settings routes without a world save, verifying all three runtime components and sliders persist exactly while unrelated graphics/camera preferences and world state remain unchanged

## 3. Configurable Autosave

- [x] 3.1 Record the selected build's hidden fixed 240-second ordinary-play countdown, three-slot rotation, sleep autosave, pause/failure/test-reset guards, load priority, backup/readback and failure behavior so current safety semantics are reproducible
- [x] 3.2 Add exact user-level Autosave On/Off and 5/10/20/30-minute interval persistence with On/5 defaults, direct Settings controls, reset-on-enable/change behavior and read-only/malformed rollback; verify separate-process relaunch without a world save and no unrelated preference/world mutation
- [ ] 3.3 Apply the preference to periodic and sleep rotating autosaves while preserving manual Save/Save & Quit/F5, existing files/backups/loadability and sheltered recovery checkpoints; verify timer pause states, three-slot rotation, one write per interval, disable/re-enable, rate-limited failure retry and long endurance counts

## 4. Decorative Wildflower Groundcover

- [x] 4.1 Reconfirm exact admitted Flower Empodium mesh/material/triangle/license identities and record ordinary resource-flower versus flower-free ground baselines before integrating decorative instances
- [ ] 4.2 Add two bounded deterministic chunk-owned DecorativeWildflower HISM batches using restrained scale/pocket density and existing low-cover plus stream/resource/plot/structure/path exclusions; verify no collision, overlap, navigation, focus, reward, save edit or component accumulation
- [ ] 4.3 Exercise ordinary woodland walks, active-window churn, cleared resource sites, building/tilling and save/revisit across representative lighting; inspect close/gameplay/wide frames for readable but non-misleading color and compare count/triangles/transition/clean cadence against the selected build

## 5. Integrated Acceptance and Promotion

- [ ] 5.1 Run focused menu/audio/autosave/flower source contracts, full portable simulation/world/regional suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [ ] 5.2 Build one immutable Shipping candidate under the serialized engine slot, verify exact source/package/attribution identity and retain `work-animation-complete-02-shipping / work-actions-v13` as rollback
- [ ] 5.3 Run Shipping native-menu, directional input, settings write/read/invalid, configurable rotating autosave/failure/recovery, feedback layout at720p/4K, ordinary flower traversal, full-loop, generated-world, save/reload and fresh consumer routes, rejecting UI, input, attribution, persistence, collision, visual or performance regressions
- [ ] 5.4 Inspect final Settings/sliders/autosave and decorative flower evidence, update design/playtest/setup documentation, seal preview-routing proof, and promote only if Settings is visibly simpler, save controls remain trustworthy and flowers improve the ground without becoming clutter or fake resources
