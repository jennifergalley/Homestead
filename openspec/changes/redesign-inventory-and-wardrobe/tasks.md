# Tasks

Planning-only handoff: all implementation tasks are intentionally unchecked.
Completed proposal/spec/design research is not implementation progress.
Do not apply this change during the environment run or infer tool permission
from OpenSpec's artifact-complete status.

Follow the dependency lanes below rather than treating heading order as a
global barrier. All lanes require completed section 1, including accepted
environment work and a fresh UI apply authorization. The coordinator, not this
planning session, assigns the later implementer/run. Keep environment progress
untouched.

| Milestone lane | Existing tasks and prerequisites | Verification/completion boundary |
| --- | --- | --- |
| Early discoverable exit | After section 1, develop 5.1/5.2, the Settings/navigation/pause portions of 5.3/5.6/5.7, then 6.1-6.5 using the existing protected save representation | Verify the pinned exit, save failure/retry, explicit unsaved exit and recovery route before wardrobe work finishes; recheck them against the upgraded saves later |
| Early existing-item inventory | After the shell/input foundation above, develop existing-item/tab icons from 4.5 and carried/chest grid, navigation and hover/focus details portions of 5.3/5.4/5.7 | Verify current item counts, 120-unit capacity, supported existing actions, device parity and pause at 720p/4K; retain existing authority/save semantics, not temporary wardrobe or persistent split/reorder data |
| Owned wardrobe and integration | Sections 2 and 3 establish ownership/persistence; 4.1/4.2 establish safe modular assets before functional equipment presentation in 4.3/4.4 and completion of 5.4/5.5/5.6 | Equip/craft/store/dye and persistent layout require their real transactions, migration and admitted rendering; early UI must not present cosmetic toggles or placeholders as owned clothing |
| Whole-change acceptance | Complete all remaining behavior and sections 7/8 after the lanes converge | All original tests, both save generations' exit checks and the existing two-set visual/performance limits still apply; no early milestone is whole-change acceptance or automatic promotion |

These lanes introduce no new tasks or authorization. Mixed-scope tasks remain
unchecked until their entire description and verification pass; record partial
milestone evidence separately. Early screenshots count within Set A's existing
budget, not an extra art-review cycle. Persistent grouping/splitting waits for
2.5 and section 3; clothing preview/equipment waits for the wardrobe lane.

## 1. Accepted prerequisite and frozen evidence

- [ ] 1.1 Record completed and coordinator-accepted `upgrade-woodland-environment-assets`, its accepted source/package, a new UI/wardrobe apply authorization and the exact approved/proven offline tool workflow; verify all gates are present or stop without implementation or engine/tool launches.
- [ ] 1.2 Reconcile this plan against that accepted baseline, including current save versions, input/pause hooks and authoring restrictions; verify no version collision, unrelated code merge or inherited environment deadline/permission.
- [ ] 1.3 Freeze synthetic baseline portable v2/v3 and UE wrapper v1-v4 fixtures with provenance, distinguishing historical bytes from emulation; verify old counts/IDs/dye/appearance/recovery metadata and preserve baseline-produced v4 bytes before serializer changes.
- [ ] 1.4 Register matched visual states, 720p/4K captures, normal-control clips and design section 9 budgets in a candidate-owned evidence record; verify test roots, baseline hashes and untouched personal saves, preview selection and active process.

## 2. Portable item authority and conservation

- [ ] 2.1 Add stable wearable definitions/instances, tagged single ownership, equipment slot references and separate ID allocator without renumbering the 14 existing items or world IDs; verify unique-owner, invalid-definition/dye and duplicate-reference rejection tests.
- [ ] 2.2 Extend pack/chest capacity calculations and existing mutations to count carried/stored wearables as one unit, equipped as zero; verify 119/120/121 boundaries and unchanged material/tool quantity semantics.
- [ ] 2.3 Implement atomic equip/replace/unequip with torso+legs tunic, dependent apron and footwear compatibility; verify full-pack one-for-one swap, two-item displacement rejection, cancel and stale revision cases preserve all IDs/owners.
- [ ] 2.4 Extend reachable chest transfer to garments and selected fungible amounts with stable chest identity and existing 280 cm reach; verify store/take, full storage, missing/out-of-range chest and repeated operation tokens conserve possessions.
- [ ] 2.5 Add persistent ordered layout groups, separate group IDs, split/merge/reorder and reconciliation for every existing quantity mutation including harvest/craft/eat/fill/build/transfer; verify group sums equal authoritative counts and split cells add no slot/capacity cost.
- [ ] 2.6 Add the three proposed fiber garment recipes, existing-knife prerequisite, one-result ownership and per-instance cosmetic dye; verify truthful requirements, exact ingredient debit, post-consumption capacity, failure atomicity and no craft recipe for legacy leather shoes or new warmth effect.

## 3. Versioned migration and coherent checkpoints

- [ ] 3.1 Implement portable v4 encoding/decoding and explicit legacy appearance conversion context while preserving v2 roots migration, old 14-count arrays and checksum/length bounds; verify golden legacy parse, new round trip and missing-context/future-version failures.
- [ ] 3.2 Implement wrapper v5 migration from supported v1-v4 with preserved `HOMESAV1`/CRC and deterministic equipped tunic/footwear/optional apron IDs; verify both outfits/all dyes and repeated legacy load without quantities, WorldId or appearance changing.
- [ ] 3.3 Decode/validate each save candidate once before whole-state application; verify incompatible wrapper/payload pairs, bad owners/chest IDs, duplicate IDs, layout mismatch, invalid counts and exhausted allocators fail without partial live mutation.
- [ ] 3.4 Extend manual/rotating-auto/recovery and in-memory session snapshots to coherent wardrobe, appearance, location/view and world identity; verify same-world healthy recovery and fallback replace rather than merge later items.
- [ ] 3.5 Preserve temp-readback/backup/replace IO and add fault/interruption fixtures for every stage; verify separate-process reload after failed/complete migration and retained good primary/backup data, including stored formerly equipped legacy garments.
- [ ] 3.6 Add schema-compatibility metadata/launcher safeguards and a consent-gated versioned profile snapshot path for eventual promotion; verify synthetic old/new profiles cannot silently downgrade/reset, both histories remain available and no personal profile is used for tests.

## 4. Compatible modular clothing and icons

- [ ] 4.1 Under the approved authoring workflow, reconstruct the retained adult body/hair bases with permanent modest coverage and complete feet from verified source; verify base-only states for all three bodies without copying deleted-body masks or changing face/hair direction.
- [ ] 4.2 Export/import independently owned tunic, apron, shoes-with-socks and original woven footwrap geometry on the existing rig with fit variants/material contracts; verify round-trip scale, bone/weight consistency, coverage and source/license/hash provenance.
- [ ] 4.3 Wire prepared modular components to authoritative equipment, retaining pose, tint, eye/skin/hair and held-tool behavior; verify every allowed body/hair/garment combination and rejection before ownership commit if required render data is missing.
- [ ] 4.4 Add the single cached on-demand character preview using the shared presentation model and design capture limits; verify matching equipped identities/dye, explicit orbit focus, no world relighting/camera mutation and release after close/load/recovery.
- [ ] 4.5 Produce original or verified-license baked icons and a cooked registry for all existing/new items, recipe outputs, building plans, tabs and slot cues; verify complete coverage, correct dye representation, legibility at grid size and no reference-image extraction or external uploads.

## 5. Native shell, input and page integration

- [ ] 5.1 Add only required UMG/Slate dependencies and the narrow C++ shell/model/widgets behind a candidate-only development switch; verify it builds under the approved pipeline, draws one menu only and preserves the normal Canvas HUD/planning path.
- [ ] 5.2 Route menu-owned accepted input exactly once through the existing prompt-intent classifier, with unchanged world mappings and test input isolation; verify noise, signed mouse accumulation, held-stick flip-back, deliberate device changes and no competing flag writers.
- [ ] 5.3 Implement seven labeled icon tabs, region navigation, four-way grid movement, scroll/partial-row edges and stable ID-based focus restoration; verify complete controller/keyboard paths, explicit menu-only Tab/arrow changes and empty/disabled focus escape.
- [ ] 5.4 Build Inventory equipment/preview, carried/nearby chest grids and hover/focus details with truthful quantities, capacity and valid actions; verify mouse click parity, hover never mutates, empty/full/unavailable storage and selection after source removal.
- [ ] 5.5 Wire explicit move/split/merge/quantity/equip/recolor actions to typed authority results; verify source/destination/amount display, One/All, cancel, duplicate confirm, rejected compatibility and no gameplay click-through.
- [ ] 5.6 Migrate Craft, Build, Guidebook, Credits, Settings and Appearance to purpose-specific content; verify real recipe costs and eat/take/store actions remain available, free outfit toggles are removed, body/hair editing stays separate, and video/audio settings and credits remain intact.
- [ ] 5.7 Integrate pause/modal ownership, action cancellation, feedback region and preview/camera restoration; verify browsing does not advance time, confirmed existing actions retain their costs, F5/F9 cancel drafts safely, and toasts do not obscure controls at either resolution.

## 6. Discoverable and reliable exit

- [ ] 6.1 Add the pinned Settings session bar and cancel-default exit confirmation; verify Escape/B -> Right -> activate reaches it in three presses at 720p/4K without scrolling and every page has a labeled Settings path.
- [ ] 6.2 Return structured save stage/message/revision and track truthful successful-save recency plus outstanding graphics-save errors; verify no attempted write is reported saved and world/config persistence failures are distinguished.
- [ ] 6.3 Implement the guarded Saving/Failed/Retry flow using the existing protected IO path; verify a dedicated synthetic process stays alive/paused after each injected failure and exits only after a successful retry/commit.
- [ ] 6.4 Add separate cancel-default Quit without saving confirmation and recovery Settings/quit route; verify no checkpoint overwrite, no forced retry, explicit loss warning, accessible no-usable-save exit and back returns to the correct parent screen.
- [ ] 6.5 Preserve startup/profile validation and separate Start a new clearing confirmation; verify invalid routes fail before save access, cancel does nothing, and restart does not delete existing saves or leave stale equipment/focus.

## 7. Integrated behavior, visual sets and performance

- [ ] 7.1 Run the portable native suite plus new conservation/migration fixtures through `Scripts\Test-Native.ps1`; verify existing rules and all new failure cases without engine headers or player-save access.
- [ ] 7.2 Extend and run approved native-engine/packaged menu, prompt, book-clarity, active-feedback, VSync and lifecycle fixtures; verify semantic behavior rather than preserving obsolete row-index/whole-mesh assertions.
- [ ] 7.3 Run separate-process migration/save-routing/quit coverage and existing launcher guards using only synthetic profiles; verify five slots/backups, recovery, original/other-profile hashes and explicit failure reports independently of process exit code.
- [ ] 7.4 Run affected existing action/full-loop/save-load/hotkey tests with actual Lit/LightingOn/ShaderComplexityOff guards under the approved offline workflow; verify F5/F9 do not restore inherited debug behavior, rewards are conserved and held tools/materials remain correct.
- [ ] 7.5 Complete matched visual Set A within the design's 40-still/two-short-clip ceiling, covering real 720p/native-4K menu states and ordinary garment movement; verify readable labels/icons/details, measured contrast, coverage and no toast overlap, recording limitations rather than human approval.
- [ ] 7.6 Apply at most one batched correction and complete matched Set B within the same limits; verify the original defects are resolved or mark the candidate incomplete and stop rather than starting a third polish pass.
- [ ] 7.7 Run screenshot-free warmup/three-sample timing and 50-open/close retention checks using the registered baseline; verify closed-menu <=5% p95 regression, menu <=2 ms added CPU/GPU p95 and <=256 MiB added resident memory or report a failed budget without silently widening it.

## 8. Documentation and reversible handoff

- [ ] 8.1 Update directly affected game/setup/playtest/design/test documentation and garment/icon provenance from the actual built result; verify all controls/recipes/migration instructions are accurate and no unsupported warmth/art-approval claims remain.
- [ ] 8.2 Remove the temporary old-menu switch/path only after parity, then create a fresh isolated candidate package and private checkpoint with receipt/proof hashes; verify accepted preview selection, original package and personal saves remain untouched.
- [ ] 8.3 Present the completed task evidence, two review sets, schema compatibility, proposed defaults and any remaining limitations to the coordinator; verify acceptance/promotion is explicit and not inferred from passing tests or OpenSpec status.
- [ ] 8.4 Record/test the synthetic rollback procedure with old and upgraded profiles preserved, stop the authorized work and hand off; verify no automatic push, merge, PR, schedule or follow-on coding run is created by this plan.
