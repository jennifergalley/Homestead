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
