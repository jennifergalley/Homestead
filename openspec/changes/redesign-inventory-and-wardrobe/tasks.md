# Tasks

September 20 22:35 policy revision: isolated parallel source work is authorized.
Environment acceptance gates reconciliation/integration/delivery, not UI source
work. Existing saves are disposable test data; current saves must still work.
Old migration tasks were revised, not falsely completed. Partial/source-only
work stays unchecked until its complete verification passes.

Early lane: 1.1, 5.1-5.3, Settings portions of 5.6/5.7, section 6, then existing
items in 4.5/5.4. Wardrobe authority/save/asset lanes proceed independently;
functional equipment needs sections 2/3 and compatible section 4 assets.
All engine execution belongs to the coordinator's single approved lane.

## 1. Authorization and integration baseline

- [x] 1.1 Record isolated UI/backend/asset source ownership and current authorization; verify no competing engine execution or main-checkout edits.
- [ ] 1.2 Reconcile source/API changes against the accepted environment before integration; verify delivery order remains environment then UI.
- [ ] 1.3 Register synthetic current-version save fixtures and explicit incompatible-test reset cases; verify they do not require historical migration infrastructure.
- [ ] 1.4 Register useful 720p/4K visual/controller/gameplay states and shared runtime slot; verify source-only evidence is not called runtime proof.

## 2. Portable item authority and conservation

- [x] 2.1 Add wearable definitions/instances, single owners, slot references and stable IDs; verify duplicate/invalid owner and definition rejection.
- [x] 2.2 Extend pack/chest capacity to count carried/stored garments as one and equipped as zero; verify 120-unit limits without slot economy.
- [x] 2.3 Implement atomic equip/replace/unequip and dependent apron rules; verify full-pack swap and failed displacement preserve possessions.
- [x] 2.4 Add garment and amount chest transfers with existing reach; verify full/unreachable/stale/duplicate requests do not lose items.
- [x] 2.5 Add stable ordered groups and split/merge/reorder with reconciliation for quantity mutations; verify exact count partition and no extra capacity cost.
- [x] 2.6 Add real-cost garment recipes and per-instance cosmetic dye; verify correct costs, prerequisites, output and no invented warmth effects.

## 3. Current-version saves and explicit test reset

- [x] 3.1 Encode/decode current portable wardrobe and layout state with integrity/bounds; verify valid roundtrip and corrupt/future schema rejection.
- [ ] 3.2 Update UE wrapper for current wardrobe data; verify incompatible disposable tests show explicit reset rather than fabricated migration.
- [ ] 3.3 Validate whole save before live mutation; verify invalid IDs/owners/slots/layout leave live state unchanged.
- [ ] 3.4 Keep current manual/auto/recovery/session snapshots coherent; verify same-world recovery replaces rather than merges possessions.
- [ ] 3.5 Preserve protected temp/backup/replacement and truthful failures; verify failed write/retry and separate-process current save reload.
- [ ] 3.6 Wire clear incompatible-test reset through the coordinated Controller adapter; verify deliberate fresh start and no silent load success or profile-preservation project.

## 4. Compatible modular clothing and icons

- [ ] 4.1 Reconstruct retained adult bases with permanent modest coverage/complete feet; verify supported presets have no deleted-geometry holes.
- [ ] 4.2 Export/import tunic, apron, shoes/socks and original footwraps with compatible fits; verify rig/material/scale and provenance.
- [ ] 4.3 Bind prepared garment components to real equipped IDs; verify supported combinations, dye/skin/tool separation and missing-content error before commit.
- [ ] 4.4 Add shared real character preview with explicit orbit focus; verify equipment parity and cleanup on close/load/recovery.
- [ ] 4.5 Supply original or verified-license icons for items/recipes/plans/tabs/slots; verify complete recognizable coverage and names at grid size.

## 5. Native shell, input and page integration

- [ ] 5.1 Add narrow native UMG/Slate shell and dependencies; verify shared build succeeds and no duplicate Canvas menu drawing.
- [ ] 5.2 Route accepted menu input once through existing prompt classifier; verify noise/held-stick stability, deliberate switching and test input isolation.
- [ ] 5.3 Implement labeled icon tabs, regions and four-way grid navigation; verify scroll/partial rows/empty states/focus restoration on controller and keyboard.
- [ ] 5.4 Build real carried/chest grids and hover/focus details, then equipment/preview; verify truthful counts/capacity and no hover mutation.
- [ ] 5.5 Wire Move/Split/Merge/amount/equip/recolor to typed real authority; verify cancel/failure/repeated confirm and no click-through.
- [ ] 5.6 Integrate Craft/Build/Guidebook/Credits/Settings/Appearance purpose-specific content; verify existing actions/settings and separate body/hair versus owned clothing.
- [ ] 5.7 Integrate pause/modal lifecycle, feedback and camera restoration; verify action costs remain, browsing pauses and feedback doesn't cover controls.

## 6. Discoverable reliable exit

- [ ] 6.1 Pin Settings exit with cancel-default confirmation; verify three-press gameplay route and labeled Settings path from every page.
- [ ] 6.2 Report actual save success/failure and config persistence status; verify no attempted save is reported durable.
- [ ] 6.3 Implement Saving/Failed/Retry handling; verify actual failure stays open/paused and successful save precedes exit.
- [ ] 6.4 Add explicit unsaved confirmation and independent recovery Settings/quit; verify no forced retry/checkpoint overwrite or accidental discard.
- [ ] 6.5 Keep invalid startup routes explicit and restart separately confirmed; verify cancel preserves state and reset clears stale focus.

## 7. Integrated evidence

- [ ] 7.1 Run permitted portable rules/current-save tests; verify ownership conservation and existing gameplay semantics.
- [ ] 7.2 Run approved focused menu/prompt/feedback/settings/lifecycle tests; verify real semantics rather than obsolete row-index assumptions.
- [ ] 7.3 Run current save/load/reset/quit tests and existing route guards in shared slot; verify truthful results independently of process exit code.
- [ ] 7.4 Run affected action/full-loop/hotkey cases with actual Lit/LightingOn/ShaderComplexityOff checks; verify unchanged rewards and materials.
- [ ] 7.5 Review matched baseline/candidate 720p/4K menu states and ordinary character movement; verify legible icons/details/feedback and coverage.
- [ ] 7.6 Batch demonstrated corrections and recheck them; verify scoped defects are resolved without arbitrary iteration/time/cost work-stop caps.
- [ ] 7.7 Measure meaningful menu/gameplay timing and repeated open/close cleanup; verify no material regression and report actual 60 FPS evidence/limits.

## 8. Integrated playable delivery

- [ ] 8.1 Update directly affected setup/design/playtest/test/provenance docs from actual implementation; verify controls and save-reset guidance are accurate.
- [ ] 8.2 Checkpoint/push only owned coherent source and integrate via coordinator after environment; verify exact commits and approved shared build.
- [ ] 8.3 Deliver latest playable UI/wardrobe candidate with actual basic visual/controller/gameplay/current-save evidence; verify no inferred human aesthetic approval.
- [ ] 8.4 Report completed scope and remaining issues truthfully, with no automatic PR/schedule; verify final OpenSpec progress matches implemented and tested behavior.

## Current UI lane evidence

September 20 source milestone: central policy revised before implementation;
UI/wardrobe/asset owners exchanged typed API and rendering paths. Native Slate
shell, original icons, current-item grid/details and explicit exit flow are
source work in the isolated UI branch. Python source-contract checks are not
C++ compilation or visual proof. Shared engine compile/controller/720p/4K review
remain pending; relevant mixed-source/runtime tasks remain unchecked.

Backend authority commits `e8068cb`/`c11317a`, integrated here as `1a972ce`/
`e2f55b4`, passed the owner's guarded portable compile/link/test run: 22 scenarios,
1714 checks. That evidence completes portable authority tasks 2.1-2.6 and 3.1,
not rendered equipment, Controller IO or native-menu runtime acceptance.
`cf4bc38` adds UI authority adapters and current-save/reset handling; its 11
Python source-contract checks are deliberately labeled source-only.
