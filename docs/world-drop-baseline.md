# World drop baseline

Selected build: `crafting-04-shipping / crafting-v17`.

Before `add-droppable-inventory-items`, carried ordinary groups and wearables
have no `Drop` action and Simulation schema 6 has no player-owned world payload.
Inventory quantities can change through gather, craft, eat, equipment, and
exact chest transfer only. World focus resolves renewable resources, plots,
structures, and water; no focusable nonrenewable pickup record exists.

The compact-inventory work established:

- stable carried group IDs and unique wearable IDs/dyes;
- a 120-unit pack and chest capacity;
- atomic expected-revision transactions;
- icon/count cells and contextual details/actions;
- exact Chest/Pack storage sessions and persistent layout serialization;
- hotbar availability derived from currently carried tools.

This is the controlling no-Drop baseline for schema 7. Adding Drop must move
ownership rather than delete quantities, must reject invalid placement
atomically, and must preserve the selected `crafting-v17` preview until a
replacement passes ordinary save/reload and recovery.

## Implemented checkpoint

The schema 7 authority is now integrated through gameplay and Inventory:

- `Drop...` appears only for carried ordinary stacks and unequipped carried
  garments. Ordinary quantities use the shared amount stepper; garments use a
  one-item confirmation. Both capture stable subject IDs and revisions.
- Deterministic candidates search clear dry ground close to the heroine and
  reject structures, plots, water, or solid world collision before the single
  authoritative transaction.
- Nearby drops render as low original category-tinted bundle tokens with a
  restrained count accent. They have no collision, overlap, navigation, physics,
  or camera-blocking role, and reserve their footprint from low ground cover.
- Exact item/count or garment identity is focusable and recoverable. Successful
  pickup removes its record and visual once; current-version save/load retains
  distant records and active-window refresh removes stale components.
- Dropping the only carried Knife immediately ghosts its assigned hotbar slot;
  exact pickup restores availability. Equipped and chest-owned possessions must
  first be unequipped or transferred.

Final acceptance covers pointer/keyboard/controller modal paths, every ordinary
item category, partial/full stacks, only-Knife hotbar recovery, dyed-garment
identity, merge/limit/obstruction/capacity rejection, low-cover reservation,
active-window hide/return, exact visual removal, and current-schema persistence.
Portable Simulation/World/Regional suites, source contracts, Development and
Shipping native routes at 720p/4K, the complete Shipping full loop, and a fresh
producer/consumer resume all pass.

Accepted unselected candidate:

- package: `Build\Releases\20260923-150721-drop\drop-02-shipping`
- executable SHA-256:
  `326E4E5C41197468E5EC97AED2717F21F581F622D7134B94EE3A020FBBAD247F`
- final full loop: 45.83 mean FPS, p95 17.51 ms, p99 18.10 ms over
  11,878 measured frames
- visual proof:
  `Build\Validation\drop02-shipping-native-720p\native-world-drop.png`

`crafting-v17` remains selected because preview promotion requires an acceptance
receipt tied to a committed source checkpoint. This candidate was built from
verified working-tree changes on HEAD `dbb6e37`; it must not be represented as
checkpointed or promoted until those changes are committed.
