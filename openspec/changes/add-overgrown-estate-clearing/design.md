# Design

## Context

- **Simulation today:**
  - `Item`, `ResourceKind` and `Recipe` are C++ enums, and `Inventory` is a count per item.
  - Underbrush is generated per chunk, and its clears are recorded as `UnderbrushEdit`.
  - Trees are `ResourceNode`s that the hatchet fells.
  - The machete clears woody underbrush and the knife cuts reeds.
  - `Exertion` constants price the work. There's warmth state with insulation.
- **Presentation and input:**
  - The hotbar selects a carried tool, and left-click or RT dispatches it through the
    controller to Simulation.
- **Dependencies:**
  - `author-fixed-cornish-estate-map` replaces generated placement with `DA_EstatePlacements`,
    records carrying stable IDs and minimum tiers.
  - `add-ruined-manor-and-arrival` places the salvage piles.

## Goals / Non-Goals

**Goals:**

- Satisfying, legible clearing whose tool tiers promise future upgrades.
- Keep existing, proven presentation paths: felling, gather and pickup, and the hack.

**Non-Goals:**

- Upgrade purchasing, durability, seasons and berry picking, compost, or livestock feed.

## Decisions

### 1. Overgrowth kinds are resources with a minimum tool and tier

- Each placement record declares its kind, and a per-kind table gives:
  - the required tool (Scythe, Billhook, Axe or Pickaxe);
  - the minimum tier;
  - the energy cost;
  - the swing count at each tier;
  - the yield range.
- Clearing uses one Simulation transaction, `ClearOvergrowth(id, tool)`. It validates range,
  the carried tool, the tier and the energy reserve, then marks the resource cleared and grants
  the yield exactly once.
- The existing `ResourceEdit` keyed by stable ID stores the cleared state. There's no
  separate underbrush edit list on the fixed map.
- **Alternative considered:** keep the per-chunk underbrush index. Rejected, because the fixed
  map has no chunks and stable placement IDs already exist.

### 2. Tool tiers as per-tool state

- `State.toolTier[ToolKind]` holds Worn, Iron, Steel or Master. It exists once per tool type,
  because the game carries one of each tool.
- Owning the tool is still an inventory count. The tier belongs to the tool type, so dropping
  a tool and picking it back up keeps its tier.
- The gate check is `tier >= kindMinTier`. If the check fails, Simulation returns a
  `Result` code `ToolTier` with the message "Needs a {tier} {tool}". Presentation plays a
  short "bounce" reaction, and nothing mutates.
- Higher tiers reduce the swing count and energy per clear. The scythe's arc radius, and the
  hoe's and watering can's squares, scale with the tier.
- **Alternative considered:** per-instance tool items. Rejected as needless while each tool is
  unique.

### 3. Multi-swing clears

- Stumps, logs and boulders take several swings, for example two to four at Worn.
- The swings in progress live on the controller, not in the save, and they reset if she walks
  away.
- The transaction commits on the final swing, so a half-cleared stump never persists.
  Presentation plays a per-swing hit reaction: chips, a thud, a crack.

### 4. Scythe sweep

- One swing clears every TallGrass and Weeds target whose centre lies in a forward arc: about
  140° and 1.6 m at Worn.
- Each target is cleared through its own transaction, and the yields are aggregated into one
  feedback toast.
- The animation is a new two-handed sweep authored on `metahuman_base_skel`, using the
  existing work-animation pipeline in `improve-homestead-work-animations`. It includes a
  wind-up, a hip-driven sweep and a recover.

### 5. Billhook and pickaxe presentation

- The billhook reuses the machete's hack animation with a new mesh. The grip transform is
  adjusted, following the existing held-tool geometry table.
- The pickaxe adapts the axe's overhead chop into a downward strike at ground or boulder
  height.
- Both meshes, and the scythe, are procedural Blender recipes in `Scripts\Blender\Recipes`:
  ash hafts, and a patinated iron head or blade. Their provenance is recorded in
  `docs\asset-credits.md`.

### 6. Hafting bootstrap

- Salvage piles are resources that yield specific rusted heads once each.
  `add-ruined-manor-and-arrival` places them, and this lane defines the kind.
- The hand recipe `Haft` consumes one rusted head plus two Branches and produces the worn
  tool, which the hotbar slots automatically.
- The pail starts in the standing room's chest.
- Branches come from FallenBranch clears (by hand gather or axe) and from existing loose-branch
  forage near the manor. That avoids a deadlock before the first axe.

### 7. Weed creep

- Once per in-game day (at the 6 AM rollover):
  - Each cleared-grass or cleared-weed placement that isn't covered by a plot, structure or
    road has a small chance, about 3%, of respawning as Weeds.
  - Only placements within about 6 m of any uncleared overgrowth qualify, so a fully cleared
    area stays clean.
- Respawn clears the edit, and the record returns to its authored kind at worn tier.
- Tilled plots keep their existing plot-weed mechanic unchanged.

### 8. Retirements

- Knife, Machete, Fiber and the reed-cutting recipes and action are removed from new-game
  seeding, placement, recipes and the hotbar defaults.
- Deer remains and fur leave placement. Their code stays until a cleanup change removes it.
- Warmth is removed: the `warmth` and `warmOutfit` state, insulation, cold drain, the warmth
  HUD meter, and warmth effects from sleep. `WearableDefinitionInfo::insulation` is ignored,
  and clothing becomes cosmetic.
- The save version bumps, and incompatible test saves reset with the standard notice.

## Lanes and ownership

This lane owns:

- The overgrowth kinds and tier table, and tool tiers.
- The Haft recipe, weed creep and the warmth removal.
- Tool dispatch for the scythe, billhook and pickaxe.
- The new tool meshes and animations, and the bramble variants.

It shares:

- `ResourceKind`/placement records with the world lane: it adds kinds, and the world lane
  places them.
- Item catalogue rows with the store lane: the file belongs to the store lane, and this lane
  adds rows.
- Salvage piles with the arrival lane.

## Risks / Trade-offs

- **New animations are the long pole.** → Ship the billhook first on the reused hack, then add
  the pickaxe strike, and the scythe sweep last. The scythe can temporarily reuse the
  hack at reduced reach, labelled as a stand-in.
- **Too many overgrowth instances on 1 km².** → Instanced static meshes per kind. Simulation
  holds records only for interactive placements, and decorative grass stays as PCG foliage.
- **Tier gates feel like walls without the blacksmith.** → Round 1 seeds mostly worn-tier
  targets. Higher-tier ones are visible teases, placed along edges rather than blocking the
  road or the manor.

## Open Questions

- Stardew-like clutter density against Jenny's sense of "massively overgrown". This gets tuned
  in the first playtest.

## Status and implementation notes

- **Model** (`Simulation/HomesteadOvergrowth.*`): the per-kind table, tiers in `State::toolTiers`
  (saved as a tagged trailing `tools` section), `CheckOvergrowth`/`ClearOvergrowth`, the scythe
  arc, salvage and weed creep. Yields roll deterministically per node, so a clear never depends on
  hidden RNG state. Yield that doesn't fit the pack is left as a world drop rather than refusing
  the clear. Uncleared overgrowth blocks tilling and building on the estate.
- **Salvage**: one `SalvagePile` kind, searched by hand. Each pile gives the next head she lacks
  (as a head or a hafted tool), billhook first, then axe, scythe, pickaxe and hoe, plus one scrap
  iron; once she has all five, piles give scrap only. So the arrival lane places piles without
  saying which head is where.
- **Retirements**: new games start without the knife; gathering and hafting need none. Reeds and
  deer remains stay in woodland generation as inert scenery. Build pieces take bramble canes in
  place of fibre (wattle); garment crafting leaves the Craft page (the dressmaker, round 3).
  Knife, machete, fibre and fur are `hiddenFromNewGames` catalogue rows so older saves still parse.
  Pre-pivot vitals lines (with warmth and the warm-outfit flag) still load.
- **Presentation**: bramble thin/thicket/bank are spring meshes from `bramble_overgrowth.py`
  (fruitless; the fruiting meshes wait for round 2's seasons). Grass, weeds, saplings, rubble,
  rocks and boulders reuse admitted meshes; salvage piles and the spring flowers use labelled
  stand-ins. Overgrowth has no collision yet, so the doorway bramble can't trap her before she has
  a billhook.
- **Swings**: the billhook uses the machete hack with its own prop; the axe uses the felling clip
  for stumps and logs, one blow per press; the pickaxe has its own downward strike
  (`AN_HeroineMH_GroundStrike`) and the scythe its own two-handed mow (`AN_HeroineMH_ScytheMow`).
- **Woodland suites** (the non-estate Homestead map) were retargeted to the estate kit with
  labelled stand-in salvage grants, and the non-packaged editor runs pass: Smoke (82 steps),
  Hotbar (62, now run with the MetaHuman heroine and the billhook in the knife's role),
  NativeMenu (195; bramble canes replace fibre, a stand-in knife and fibre keep garment
  coverage), Clearing (195; the default route fells mature trees with the axe because saplings
  are billhook overgrowth now) and FullLoop (355). The creek suite's knife/reed section is
  retired. Known gaps: the Clearing suite's `-CameraLifecycle` route still harvests a sapling
  for produce and hasn't been retargeted; BookClarity fails at its second step because Start now
  opens Settings (older than this change; only its knife references were retargeted); the
  watering visual playtest's can stage hasn't been rerun. The mature-tree clear probe checks a
  fixed 0.45 s pose sample, so a long frame hitch can fail it (seen twice in six runs, on a loaded PC).