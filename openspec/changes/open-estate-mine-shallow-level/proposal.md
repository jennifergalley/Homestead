# Open the estate mine: shallow level

Status: **decisions recorded** (Jenny, 2026-10-11; design §8). Awaiting her go to build.

## Why

Jenny (2026-10-11): "What I really want to unlock next is the ability to restore the manor, but in
my mind that looks like adding the mine and the ability to mine a lot of rock from the mine which I
can then use to restore the manor because the manor is made of stone."

The mine exists to feed the manor. Stone is its main output, and quantity matters more than variety.
Ore and gems are a sideline for selling and later crafting. Her planner cards set the shape:

- **Mine building** (card `jenny-mut6zr63`): a large stone building she can walk into. The main
  ground-floor room has rocks piled in a corner, a desk, an oil lamp on the desk that burns forever
  and can't be picked up, and a trapdoor in the floor.
- **Mining** (card `jenny-mutdvi4f`): the trapdoor leads down into dark tunnels where she needs her
  own oil lamp to see and her pickaxe to mine stone, ore and gems. She can sell what she mines in the
  village and use it in crafting, "most important is the stone building recipes for rebuilding the
  manor to its former glory."

## What changes

1. **The mine house.** A stone engine house replaces the "Mine ruin" marker on the clifftop
   (anchor `MineEntrance`). It has a walk-in ground-floor room furnished as she described, plus a
   trapdoor that opens onto a ladder shaft.
2. **The shallow level.** This is a new underground place: a small, fixed, hand-built tunnel network
   below the mine house, reached by climbing down the ladder. It is properly dark, so her held or
   set-down oil lamp is the only real light, apart from a little daylight at the foot of the ladder
   shaft.
3. **Mining.** She uses her pickaxe on rock faces for stone, which is plentiful and comes back each
   morning, and on seams and veins for coal, copper ore and the occasional quartz or amethyst. All
   of it stacks, and the general store buys it.
4. **Stone for the manor.** Stone becomes the main material of the manor restoration. This change
   delivers the stone supply and its numbers. The restoration itself, commissioned from a village
   mason, is the follow-up change `rebuild-manor-and-decorate` (Jenny's D1 and D2).

## Reuse research

| Need | Reused | Gap |
| --- | --- | --- |
| Walk-in enclosed place, darker inside | Sea cave (`HomesteadSeaCave.*`, `HomesteadWorldSeaCave.cpp`, the interior lighting in `HomesteadWorldLighting.cpp`, `Scripts/Blender/Recipes/sea_cave.py`) | Proper darkness: the cave is dim, while the mine must be dark without the lamp |
| Portable light that burns oil | Oil lamp (`openspec/specs/oil-lamp`): held or set down, 6 h of oil, flasks sold in the store | None. The mine is what the lamp was built for |
| Pickaxe swings at a node that yields stone | Rubble node and the clearing system (`HomesteadOvergrowth.cpp`, balance §6) | New rock-face and vein node kinds that come back instead of being cleared for good |
| Nodes that come back each morning | Beach gathers (mussels, laver and the rest, back at 6:00) | None |
| Selling | General store buyers (`ItemInfo::buyers`) | New catalogue rows |
| Building interior | The manor's standing room (interior lighting and leak fixes) | The new building and its furnishings |
| Map and travel | `Anchor::MineEntrance`, the "Mine ruin" map glyph, the existing woodland path from the manor | Relabel it "Mine" once the house exists |

No third-party assets are needed. The house and the tunnel kit are original Blender work following
the `blender-assets` skill, in the sea cave's granite look.

## Smallest useful result and first playable demonstration

She walks from the manor to the mine house, goes in, and sees the rock pile, the desk and the lit
lamp. She opens the trapdoor, climbs down, lights her lamp and mines a rock face for a pack of stone
plus some coal and copper. She climbs out and sells some of it in the village. The next morning the
rock faces are back.

## First delivery, full acceptance and deferred scope

- **First delivery (one build):** the mine house with its furnished room and a working trapdoor; the
  shallow level with rock faces, coal seams and copper veins (gems turn up at random); real darkness; and store
  sales.
- **Full acceptance:** Jenny can mine enough stone in a normal day to make visible progress on the
  manor once `rebuild-manor-and-decorate` ships, and the mine feels like a place: dark, timbered,
  quiet and dripping.
- **Deferred to later changes:** smelting ore into bars at a furnace; deeper levels behind a flooded
  shaft (`restore-mine-pumping-and-deep-levels`); iron, silver, gold and the rarer gems; hauling
  stone by handcart (`add-handcart-hauling-and-dynamic-prices`).
