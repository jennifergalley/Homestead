# Tasks

## 1. Compact Pack Grid

- [x] 1.1 Record selected-build 720p/4K item-cell geometry, icon/name/location/count content, Carried/Nearby/Wearing modes, split/merge/move/transfer action buttons, incoming-group behavior, empty/full/duplicate-stack scrolling, focus and pack/chest transactions; retain Coral Island/Minecraft reference links without copied screenshots
- [x] 1.2 Render pack stacks/wearables as compact square icon tiles with always-visible quantity counts and no name/location prose, using responsive columns and stable group IDs; centralize incoming ordinary-item insertion into the earliest compatible stack and verify gather/craft/pickup/transfer, food/material/tool/clothing, deliberate duplicate groups, selection, pointer targets and count readability
- [ ] 1.3 Keep item name, carried location, description and true subject actions in contextual details only while removing Split/Merge/Move/Transfer buttons; verify pointer hover plus keyboard/controller focus stay synchronized and smaller cells retain stale-revision/eat/equip/dye/Drop behavior
- [x] 1.4 Remove Carried/Nearby chest/Wearing mode controls and every proximity-derived chest count/take/store route from Inventory after paper-doll equipped-item inspection is available; verify Inventory is identical beside/away from storage and all equipped wearables remain reachable
- [x] 1.5 Implement Ctrl+click split-half with Ctrl+Enter/controller-X parity, placing the new group adjacent; verify x5 becomes x3+x2, quantity-one/wearables do not mutate, and exact totals/group IDs/save layout remain valid
- [ ] 1.6 Implement pointer drag plus Enter/A virtual drag with visual ghost, insertion and merge targets; verify different-item reorder, same-item merge, scroll/autoscroll, threshold versus click, invalid/capture/revision/page cancel and no mutation before a valid drop
- [x] 1.7 Add an icon-only upper-left Pack Sort that atomically collapses ordinary duplicates and applies Tools/Materials/Food/Farming/Clothing/Other order with deterministic tie-breaks; verify exact totals/identities/dyes/capacity/equipment and same result across repeated/save-reload calls

## 2. Dedicated Exact-Chest Storage

- [x] 2.1 Add an exact active-chest storage session opened only from valid focused E/A/mouse-secondary chest interaction; verify standing near a chest or opening Inventory cannot expose storage and two nearby chests never substitute for the focused ID
- [ ] 2.2 Build separate responsive Chest and Pack compact icon/count grids with concise container labels and one shared details/actions surface; verify empty/full/scrolling containers, duplicate stacks, wearables and 720p/4K side-by-side/fallback layout
- [ ] 2.3 Route same-grid reorder/merge and whole-stack cross-grid transfer through the shared drag/virtual-drag target model, auto-stacking destination quantities; verify partial transfer via split-then-drag, expected revision, capacity, ownership, exact active chest and atomic two-grid refresh without action buttons
- [x] 2.4 Close and clear storage safely on Back, load/recovery, new world, invalid chest/session state and UI destruction; verify no stale chest reference, replay, wrong-container transfer, world unpause, or click-through
- [ ] 2.5 Exercise pointer, keyboard and controller focus/direct manipulation between both grids, Sort, details, remaining actions and dialogs without a global controls legend; verify mouse secondary open, drag/virtual-drag cancel, hotbar/tool isolation, directional reverse paths and cancel-default behavior

## 3. Integrated Acceptance and Promotion

- [ ] 3.1 Run focused compact-grid/storage/menu/input/transaction contracts, full portable simulation/menu/full-loop/save suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [ ] 3.2 Run ordinary mouse/keyboard and controller pack/chest routes with acquisition auto-stack, split-half, reorder, merge, Sort, multiple physical chests, partial/full drag transfers, clothing, food, capacity rejection and save/reload at 720p/4K; inspect density, direct-manipulation feedback, exact storage ownership and cadence
- [ ] 3.3 Build one immutable Shipping candidate under the serialized engine slot, update UI/setup/playtest docs and promote only if Inventory is materially simpler and smaller while chest storage is accessible exclusively through the correct world chest
