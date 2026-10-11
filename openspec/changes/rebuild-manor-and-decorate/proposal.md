# Rebuild the manor and decorate

Status: **direction recorded, not yet scheduled.** This is the follow-up build after
`open-estate-mine-shallow-level` (Jenny's D2, 2026-10-11).

## Why

Jenny (2026-10-11): "What I really want to unlock next is the ability to restore the manor ... the
manor is made of stone." The mine supplies the stone, and this change spends it.

## What changes

- **Restoration in five stages:** the great hall, kitchen, bedchamber, upper floor, then the roof
  and chimneys. The stone, timber and coin costs come from Balance (mine design §7, "Manor stone
  budget"): 1,500 stone, 215 timber and 7,000 coins in all.
- **Commissioned from a village mason, Coral Island-style** (Jenny's D1). She saves each stage's raw
  materials in chests anywhere on her estate, then commissions the work from the mason. The mason
  deducts the materials straight from her estate chests (she can't carry them all to the village),
  she pays the fee, and the stage is restored overnight. There are no part-deliveries and no
  progress bar that fills by itself.
- **The mason's yard:** a separate village business with its own keeper, not part of the General
  Store.
- **This replaces freeform rebuilding.** The building menu and the survival-era wooden buildings are
  being removed (card `jenny-mv48a218`), so the manor no longer uses the snapped building system.
- Interior decor and the dog companion remain in scope for later and get planned when this change
  is scheduled.

## Reuse research

To do when the change is scheduled. Starting points: estate chests and their saved contents, the
General Store's shop UI and keeper, the manor ruin and its standing room, and Coral Island's
commission flow as the reference.

## Smallest useful result and first playable demonstration

With 200 stone and 20 timber in her estate chests, she walks to the mason's yard in the village,
commissions the great hall and pays the fee. The next morning the great hall is restored.
