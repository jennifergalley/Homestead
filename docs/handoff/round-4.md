# Round 4: Next build after `20261004-next-01`

Started 2026-10-09 13:30 local. Rules carry over from [round-3.md](round-3.md) and
[README.md](README.md): work-driven Next build / Build after next, Development packaging by
Integration only, two-Unreal-process limit (one reserved for Integration), Jenny reviews new assets
and animations before `[ready]`, Balance OK before `[ready]`, work only while Jenny is around.

Jenny's Oct 5-7 planner edits had been saved only in the retired orchestrator worktree
(`jennifergalley-expert-fiesta`); they reached `main` in `e696a013f`. Map-shrink stage 2 moved to
Build after next (Jenny, 2026-10-09) so it doesn't hold back the rest.

## Registry

| Name | Session | Model | MCP port | Scope (planner cards) |
| --- | --- | --- | --- | --- |
| Orchestrator Agent | `cfe8292c-5320-4ab3-a942-1b64c2cc956b` | Claude Opus 5.5 / high | — | coordinates only |
| Integration Agent | `09ae4063-4f60-475e-bb88-7c6d6a7bbc7c` | Claude Sonnet 5 / medium | reserved slot | merge, package, promote; docs liaison |
| Balance Agent | `81a547cd-0a72-4415-a617-460b5ade5f3c` (archived 2026-10-09 19:33; spawn fresh when needed) | Claude Opus 5.5 / high | — | review only |
| Upkeep Agent | `0b009706-dca7-491b-8a2f-be986c0ba59e` | Claude Sonnet 5.5 / high | 8771 | bramble/weed/branch regrowth; more berry bushes and roots; clearable home stumps |
| Gameplay Agent | `1f460f04-3e05-46de-95a7-0d45fac84cfd` | Claude Sonnet 5.5 / high | 8772 | crop type/day tooltip; iron tools at store; bigger landmark icons |
| Fishing Agent | `c630ec78-24ce-43d9-929f-0b354f08a801` | Claude Opus 5.5 / high | 8773 | harder fishing; bob-on-water presentation |
| Wardrobe Agent | `d5e13a60-fa8b-48fe-9d83-cea4fedfdf8e` | Claude Opus 5.5 / high | 8774 | swap-equip clothes; clothes sold at store; refit garments |
| Village Agent | `47fbb074-06a2-46f0-a579-48bb068884e2` | Claude Sonnet 5.5 / high | 8775 | smoother town path; village dressing |

Gameplay and Wardrobe both add General Store stock; they coordinate the append-only catalogue edit.

## Build after next

- Map-shrink stage 2: half-scale terrain/water/scatter rebake (`openspec/changes/shrink-estate-map`,
  [map-shrink-planning.md](map-shrink-planning.md)). Resets saves.
  Village carry-over (Village Agent, 8bdc73636): rerun town_pad.py, town_path.py, town_layout.py, public_road.py, weightmaps.py, route_sights.py --bake, village_dress.py, bake_ground.py, then Scripts\Map\bake_estate_map.py. `berries.py` has no village-street clearance, so add one before regenerating, or a berry can land back on the street (540012 was moved off it by hand).

## Placement ids claimed this round

(Lanes add a line here when they claim new estate placement ids.)
