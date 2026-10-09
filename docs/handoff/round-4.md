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
| Balance Agent | `81a547cd-0a72-4415-a617-460b5ade5f3c` | existing | — | review only |
| Upkeep Agent | `80fd2cd9-165c-43ca-a748-2c92d87dce6e` | Claude Sonnet 5.5 / high | 8771 | bramble/weed/branch regrowth; more berry bushes and roots; clearable home stumps |
| Gameplay Agent | `020b4924-7cda-4b6e-8356-74c831e30440` | Claude Sonnet 5.5 / high | 8772 | crop type/day tooltip; iron tools at store; bigger landmark icons |
| Fishing Agent | `17ee0d4f-fe3e-4072-84b9-9df5b24f010c` | Claude Opus 5.5 / high | 8773 | harder fishing; bob-on-water presentation |
| Wardrobe Agent | `f0625f3a-1e58-4ab0-a1f0-b2dc260fb1a3` | Claude Opus 5.5 / high | 8774 | swap-equip clothes; clothes sold at store; refit garments |
| Village Agent | `31b68f14-5ecb-4a66-8c81-e1f0b900e8c1` | Claude Sonnet 5.5 / high | 8775 | smoother town path; village dressing |

Gameplay and Wardrobe both add General Store stock; they coordinate the append-only catalogue edit.

## Build after next

- Map-shrink stage 2: half-scale terrain/water/scatter rebake (`openspec/changes/shrink-estate-map`,
  [map-shrink-planning.md](map-shrink-planning.md)). Resets saves.

## Placement ids claimed this round

(Lanes add a line here when they claim new estate placement ids.)
