# Proposal

## Why

Homestead has a solid playable foundation: a MetaHuman heroine, third-person movement and work
animations, a ten-slot tool hotbar, pointer-first inventory and chests, crafting, freeform
snapped building, hand-scale gardening with watering and weeding, felling, foraging, day/night,
energy and hunger, save/sleep, and a Blender asset pipeline. Its design, though, is an
open-world survival game on a seeded procedural woodland.

On 2026-09-27 Jenny chose a different game. Homestead becomes a **cozy life sim in the Coral
Island tradition**, set on the **early-Victorian Cornish coast (1840s–1860s)** and inspired by
*Poldark*. The daughter of a noble English family comes home to a derelict, massively overgrown
estate. She clears it, farms, ranches, fishes, reopens the family mine, hauls her goods to
town and sells them. With the money she rebuilds and decorates the manor, hires help, and
restores the family's name. The goal is a fortune and standing, not survival. Romance and
family come later.

Without one recorded direction, agents would keep building on the procedural world, cold
survival and primitive crafting. This umbrella change makes the new direction authoritative,
records every interview decision in one place, orders the delivery rounds, and states what
happens to every existing open change.

## What Changes

- **Replace the product direction.** `docs\game-plan.md`, `PRODUCT.md` and
  `openspec\config.yaml` describe the cozy estate life sim. The survival-era plan is kept in
  `docs\archive\` for history. The working-policy and lessons sections carry over unchanged.
- **Record the confirmed design** in `design.md`: setting, fixed world and layout, opening
  state, meters, calendar, weather, tools and tiers, loops, economy and sinks, town, manor,
  workers, reputation, companion, and what's retained or dropped.
- **Fix the round order**: fourteen rounds, starting with the first playable slice "Walk your
  estate". Each round is its own OpenSpec change.
- **Scaffold the rounds.** The five first-slice changes are fully specified. Rounds 2–14 are
  proposal-only and get fleshed out when reached, with fresh reuse research and Jenny's
  latest playtest feedback.
- **Disposition every open change** (see `design.md`): continue character, animation, UI and
  music work; fold reusable environment assets into the fixed map; supersede procedural
  world generation, warmth/cold, hunting/scavenging and the flint-and-fibre bootstrap.
- **BREAKING (gameplay):** the warmth/cold meter, clothing insulation, cold nights, the flint
  knife, the machete, fibre recipes, deer remains/fur and the procedural woodland all leave
  the design. Existing test saves reset when the fixed map lands. Saves are disposable test
  data under the current working policy, so this is reported rather than migrated.

## Reuse research

Most of the reuse comes from inside the project:

- **Keep as-is:** heroine, animation set, hotbar, inventory, chest, field-book menu, crafting
  UI, the snapped foundation/wall build system (the basis for the manor rebuild), gardening
  (hoe, plant, water, weed), felling, the stream-refill pail, day/night, energy/hunger,
  save/sleep, music, the Blender procedural asset library (granite rocks, blackberry bramble
  and understory, stone hoe, tilled bed, seeds, clothing tools), and the editor MCP playtest
  skill.
- **Retire from the play path:** the procedural chunk terrain and generation
  (`Homestead::Generation`, `HomesteadWorld` chunk streaming) once the fixed map is playable.
  The code stays until removing it is safe.
- **External reuse** belongs to each round's change. For the first slice it covers UK Open
  Government Licence terrain data, Unreal Landscape, World Partition, the Water plugin, PCG
  and landscape splines, free town and building kits on Fab/Megascans, and MetaHuman for
  NPCs. It is summarised in `author-fixed-cornish-estate-map` and
  `add-dollars-and-general-store`.

## Smallest useful result and first playable demonstration

This change produces no gameplay itself. Its first deliverable is the updated direction
documents and OpenSpec set, reviewed by Jenny. The first playable demonstration of the new
direction is round 1, "Walk your estate":

1. Start a new game and name the heroine, her family and the estate.
2. Wake in the manor's one standing room and see the estate boundary on the minimap and the
   world map.
3. Clear bramble, weeds, small stumps and rubble with found and hafted tools.
4. Walk the dirt road to the town and sell cut hay, salvaged scrap and spring flowers to a
   MetaHuman general-store keeper for dollars.
5. See those goods in the shop's stock.

## Capabilities

### New Capabilities

- `estate-life-sim-direction`: The product-level rules every later round must honour. These
  cover the cozy, never-lethal pressure model; energy and hunger without cold; the fixed
  authored world; physical hauling to town; money with meaningful sinks; tiered tools that
  gate what can be cleared; and no slaughter.

### Modified Capabilities

None. No main specs exist yet in `openspec\specs`.

## Impact

- Docs: `docs\game-plan.md` (rewritten), `docs\archive\survival-prototype-game-plan.md` (moved
  history), `PRODUCT.md`, `openspec\config.yaml`, `README.md` (summary line).
- OpenSpec: this change plus eighteen new round changes. Existing open changes are annotated
  by the disposition table rather than edited.
- Code: none in this change. Later rounds touch `Simulation`, `HomesteadWorld`, the
  controller, the HUD and the menus, as each change describes.
- Sessions: the coordinator finishes its in-flight character work, and all other Homestead
  work is paused until pointed at these changes.
