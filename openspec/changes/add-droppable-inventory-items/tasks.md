# Tasks

## 1. Atomic World-Drop Authority

- [x] 1.1 Record selected-build inventory action/group/wearable totals, quantity dialogs, pack capacity, focus/ground placement and current save format so the no-Drop baseline is reproducible
- [x] 1.2 Add strict bounded world-drop state, World wearable ownership and current-version serialization with stable IDs/payload/position invariants; verify malformed, duplicate, out-of-range, ownership mismatch, count overflow and disposable-old-save rejection cases
- [x] 1.3 Implement atomic carried-group quantity and unequipped-wearable Drop transactions with near-player/ground/structure/plot/water/revision/limit validation plus compatible ordinary-stack merge; verify every rejection leaves exact inventory/drop totals unchanged
- [x] 1.4 Implement whole-drop pickup with range, capacity, layout and wearable-identity/dye validation; verify success inserts/removes once, capacity failure remains unchanged, repeated pickup cannot duplicate, and merge totals remain exact

## 2. Grounded World Pickup

- [ ] 2.1 Add deterministic forward/nearby safe-ground candidate resolution and a small noncolliding nonnavigating original bundle/token visual with category tint/count accent; verify blocked placement rejects rather than burying or losing the item
- [ ] 2.2 Integrate world-drop focus/title/action and exact pickup feedback alongside resources/plots/structures with stable distance ties; verify drops are identifiable, nonrenewable, nonblocking, camera-safe and do not steal unrelated focus incorrectly
- [ ] 2.3 Integrate active-window visual create/remove, low-cover/build reservation, save/load and chunk churn; verify distant State persists, collected drops never reappear, merged drops remain one, and no stale component/focus/collision survives

## 3. Inventory, Hotbar, and Storage UI

- [ ] 3.1 Add `Drop...` only to carried item groups and unequipped carried wearables, using the shared amount stepper and stable subject/revision capture; verify pointer/keyboard/controller confirm/cancel, one-item garment flow and no mutation from focus/navigation
- [ ] 3.2 Exclude chest-owned and equipped possessions until transferred/unequipped, refresh compact inventory selection after commit, and verify dropped carried tools immediately ghost hotbar references then recover on pickup
- [ ] 3.3 Exercise every current item category plus dyed clothing, partial/full stacks, only-knife recovery, multiple nearby drops, merge/limit, obstructed ground and full-pack pickup in ordinary mouse/keyboard and controller play

## 4. Integrated Acceptance and Promotion

- [ ] 4.1 Run focused drop/pickup/save/inventory/focus/world contracts, full portable simulation/world/menu/full-loop suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [ ] 4.2 Run ordinary save/leave/reload/return and active-window routes at 720p/4K, inspect bundle/focus/readability/cadence, build one immutable Shipping candidate, update setup/playtest docs and promote only if possessions can be dropped/recovered without deletion, duplication, obstruction or clutter regression
