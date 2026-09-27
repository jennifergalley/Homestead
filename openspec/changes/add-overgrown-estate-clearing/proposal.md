# Proposal

## Why

The pivot opens on an estate "massively overgrown with weeds and thick underbrush like
blackberry". Clearing it is the first gameplay loop (`pivot-to-cozy-estate-life-sim`). Jenny
chose layered, tier-gated overgrowth in the Coral Island manner, and a new starter tool set:

- The knife is retired.
- A scythe replaces the machete for grass and weeds.
- A billhook handles bramble.
- The axe also clears stumps and logs.
- A pickaxe breaks rubble.
- She crafts her first tools by hafting rusted heads salvaged from the ruin.

Today's woodland underbrush can be cleared with the machete, and felling uses the hatchet.
There are no tool tiers, and no weeds, stumps or rubble as estate overgrowth.

## What Changes

- **Overgrowth kinds** as authored, persistent Simulation resources on the estate, placed
  through `DA_EstatePlacements`:

  | Kind | Tool | Minimum tier | Yield |
  | --- | --- | --- | --- |
  | TallGrass, Weeds | Scythe | Worn | Hay (from grass), Weeds (compost later) |
  | BrambleThin, Sapling | Billhook | Worn | Bramble canes, Kindling |
  | BrambleThicket | Billhook | Iron | Bramble canes |
  | BrambleBank | Billhook | Steel | Bramble canes, a chance of scrap |
  | StumpSmall, FallenBranch | Axe | Worn | Firewood, Kindling |
  | StumpLarge, FallenLog | Axe | Iron | Timber, Firewood |
  | StumpAncient, GiantLog | Axe | Steel | Timber |
  | Rubble, SmallRock | Pickaxe | Worn | Stone, Scrap iron / Scrap lead |
  | Boulder | Pickaxe | Iron | Stone |

  Brambles show blackberry fruit from late summer into early autumn. That's cosmetic in round
  1, because the game starts in spring; picking blackberries arrives with round 2's seasons.
- **Tool tiers.** Each tool has a tier: Worn, Iron, Steel or Master-forged. Round 1 ships Worn
  only.
  - A target above the tool's tier doesn't change, costs no energy, and prompts "Needs an iron
    axe", for example.
  - Higher tiers clear faster, and the scythe and hoe also sweep more squares per swing.
  - The data model and the prompt are complete now. Blacksmith upgrades come with the smith's
    shop in round 3.
- **Starter tools:**
  - **Scythe:** new. Sweeps an arc of grass and weeds in front of her. Needs a new Blender
    mesh and a two-handed sweep animation.
  - **Billhook:** new. Reuses the machete's one-handed hack animation, with a new hooked-blade
    mesh.
  - **Pickaxe:** new. Needs a Blender mesh and an overhead strike animation, adapted from the
    axe swing.
  - **Axe:** the hatchet renamed and restyled in iron. It gains stumps and logs.
  - **Hoe:** the digging stick restyled in iron. Its behaviour is unchanged.
  - **Pail:** unchanged, and she starts with it.
- **Bootstrap by hafting.** Salvage piles in and around the ruin, placed by
  `add-ruined-manor-and-arrival`, yield rusted heads: an axe head, scythe blade, billhook
  head, pick head and hoe blade. The hand recipe "Haft a tool" combines a rusted head with
  two Branches, and later Timber, to make the worn tool. There's no station and no knife.
- **Retirements:**
  - The Knife, the Machete, the Fiber-gated recipes and the reed-cutting action leave the
    play path. Their items, icons and animations are hidden from new games rather than deleted
    in code.
  - Deer remains and fur leave placement.
  - Warmth, cold and insulation are removed from the simulation and HUD. That's the umbrella
    decision, implemented here because this change edits the same Simulation and HUD code.
- **Weed creep.** Cleared estate ground that hasn't been tilled or built on slowly regrows
  sparse Weeds, only near remaining overgrowth, at a gentle rate. Ground that's tilled or
  built on never regrows overgrowth.

## Reuse research

- **Project code:**
  - Underbrush clearing and edit persistence (`UnderbrushEdit`, woody and soft underbrush
    energy).
  - Felling presentation and landing, garden weeding, the gather animation and the pickup
    presentation.
  - The hotbar and its selected-tool dispatch.
  - Crafting and recipe assessment, and the Craft UI with its icon ingredients.
  - Energy costs in `Exertion`.
- **Project assets:**
  - The Blender `BlackberryBramble` and understory library, extended with thicket and bank
    variants and a fruiting-state material switch.
  - The granite rubble, spall and boulder set.
  - The hatchet, machete and stone-hoe meshes and animations.
  - `Scripts\Blender\Recipes` for the new tool meshes.
- **Comparative references (conventions only):**
  - Coral Island tool tiers gate stump and boulder sizes, and a clear prompt names the needed
    upgrade.
  - Stardew Valley's opening farm is covered in weeds, stumps, logs and rocks, with large
    stumps and logs needing upgraded tools.
- **Period verisimilitude:**
  - Billhooks, slashers and scythes were the standard English tools for clearing bramble and
    rough grass.
  - Mown grass dried as hay was a saleable good.
- **Custom gaps:**
  - The scythe and pickaxe meshes and their animations, and the billhook mesh.
  - The overgrowth kinds, the tier model, hafting, and weed creep.
  - The retirement of the warmth and knife paths.
- **No external assets or licences.**

## Smallest useful result and first playable demonstration

**Smallest result:** in the standing room she picks up the pail. She salvages a rusted
billhook head from a nearby pile, hafts it with two branches, and hacks thin bramble around
the manor's doorway. Canes drop and the bramble stays cleared after save and reload.

**Full acceptance:**

- All five tools hafted.
- Every worn-tier kind cleared with the correct tool, with tier-gated prompts on the
  thicket, large stump and boulder.
- The scythe sweep and pickaxe strike animations.
- Weed creep over several in-game days.
- Warmth fully removed.
- Mouse and controller parity.
- Stable-ID persistence across save and load.

**Deferred:**

- Blacksmith upgrades (round 3).
- Blackberry picking and seasons (round 2).
- Compost from weeds (round 2).
- Hay as livestock feed (round 6).
- Tool durability, which is not planned.

## Capabilities

### New Capabilities

- `estate-overgrowth-clearing`: Layered overgrowth kinds, the tool set and tiers, tier-gated
  clearing, hafting, weed creep, and the retirement of the knife, machete and warmth.

### Modified Capabilities

None. No main specs exist yet.

## Impact

- `Homestead::Simulation`:
  - `ResourceKind` gains the overgrowth kinds, with a minimum tool and tier.
  - `Item` gains Scythe, Billhook, Pickaxe, the RustedHead variants, Hay, Weeds,
    BrambleCanes, Kindling, ScrapIron and ScrapLead. Axe and Hoe replace the Hatchet and
    DiggingStick display names.
  - A tool-tier state per tool, a Haft recipe, and a weed-creep tick.
  - Warmth fields and logic removed.
  - The save version bumps, coordinated with the world lane.
- The item catalogue rows in the `add-dollars-and-general-store` catalogue file: names, icons,
  categories and base prices for every new item.
- `HomesteadController` and `HomesteadCharacter`: tool dispatch for the scythe arc, billhook
  hack and pickaxe strike, and the tier prompts.
- `HomesteadHUD`: the warmth meter removed.
- `HomesteadWorld`: presentation for the overgrowth kinds from placements.
- New Blender recipes for the scythe, billhook, pickaxe and rusted heads, and the bramble
  thicket and bank variants. New animation assets for the scythe sweep and pick strike.
- Tests: the clearing, hotbar, weeding and felling tests are updated. Warmth tests are
  retired, and tier-gate tests are added.
