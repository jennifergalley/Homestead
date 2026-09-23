# Tasks

## 1. Compact Pack Grid

- [ ] 1.1 Record selected-build 720p/4K item-cell geometry, icon/name/location/count content, Carried/Nearby/Wearing modes, empty/full/duplicate-stack scrolling, focus and pack/chest transactions; retain Coral Island/Minecraft reference links without copied screenshots
- [ ] 1.2 Render pack stacks/wearables as compact square icon tiles with always-visible quantity counts and no name/location prose, using responsive columns and existing stable group IDs; verify food/material/tool/clothing, duplicate groups, selection, pointer hit targets and count readability
- [ ] 1.3 Keep item name, carried location, description and valid actions in contextual details/actions only; verify pointer hover plus keyboard/controller focus stay synchronized and smaller cells do not lose stale-revision, split/merge/reorder/eat/equip/dye behavior
- [ ] 1.4 Remove Carried/Nearby chest/Wearing mode controls and every proximity-derived chest count/take/store route from Inventory after paper-doll equipped-item inspection is available; verify Inventory is identical beside/away from storage and all equipped wearables remain reachable

## 2. Dedicated Exact-Chest Storage

- [ ] 2.1 Add an exact active-chest storage session opened only from valid focused E/A/mouse-secondary chest interaction; verify standing near a chest or opening Inventory cannot expose storage and two nearby chests never substitute for the focused ID
- [ ] 2.2 Build separate responsive Chest and Pack compact icon/count grids with concise container labels and one shared details/actions surface; verify empty/full/scrolling containers, duplicate stacks, wearables and 720p/4K side-by-side/fallback layout
- [ ] 2.3 Route transfer/split/merge/reorder through existing amount stepper, expected revision, capacity, ownership and exact active-chest transactions; verify selection/open/close is nonmutating and confirmed changes refresh both grids atomically once
- [ ] 2.4 Close and clear storage safely on Back, load/recovery, new world, invalid chest/session state and UI destruction; verify no stale chest reference, replay, wrong-container transfer, world unpause, or click-through
- [ ] 2.5 Exercise pointer, keyboard and controller focus between both grids, details, actions and dialogs without a global controls legend; verify mouse secondary open, hotbar/tool input isolation, directional reverse paths and cancel-default behavior

## 3. Integrated Acceptance and Promotion

- [ ] 3.1 Run focused compact-grid/storage/menu/input/transaction contracts, full portable simulation/menu/full-loop/save suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [ ] 3.2 Run ordinary mouse/keyboard and controller pack/chest routes with multiple physical chests, partial/full transfers, clothing, food, capacity rejection and save/reload at 720p/4K; inspect density, count/name separation, exact storage ownership and cadence
- [ ] 3.3 Build one immutable Shipping candidate under the serialized engine slot, update UI/setup/playtest docs and promote only if Inventory is materially simpler and smaller while chest storage is accessible exclusively through the correct world chest
