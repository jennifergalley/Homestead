## Context

The overgrowth system (add-overgrown-estate-clearing) already has most of what this needs:
- a per-kind table of tool, minimum tier, energy, swings and yields;
- `ClearOvergrowth` as one transaction, committing only on the final swing;
- fixed-estate placements whose cleared state persists as ResourceEdits.

The clear-out adds content and one rule on top of that.

## Decisions

### Placement: a baked field, not runtime scatter
`Scripts/Terrain/clearout.py` samples the reshaped heightmap, the road, the baked scenery and every
earlier lane's placements, then writes `clearout(id, kind, x, y)` rows. Baking keeps ids stable and
lets native tests check the field without Unreal.

`ProvisionalEstatePlacements` appends the rows last and skips any row within 1.5 m of an earlier
placement, or 3 m of a blackberry bramble. So when the manor lane's grounds ring or the world
lane's forage moves, the result is a gap rather than an overlap, and the other ids never shift.

Density: about 380 rows plus the manor lane's grounds ring (550000+), spaced 2.2 m apart. Big
things are spaced 2.6-4 m apart. Kinds are drawn with a per-zone weight, and a kind that doesn't
fit where it was drawn waits for the zone's next spot, so big stumps and boulders keep their share.

### Zones
The zones are split by position relative to the ruin's walls:
- **Yard** (north, toward the farm): nettles thrive on middens, so this zone is rubbish, rubble and
  nettles.
- **Kitchen garden** (east, by the standing room): weeds, nettles and rank grass.
- **Grounds** (everything else): stumps, saplings, bramble and rocks.

Within 5 m of a wall, rubble is three times as likely, standing for masonry shed from the ruin.

### Tiers
Tool upgrades are round 3's blacksmith. Until then, about 5% of the field (large stumps, boulders
and thickets) stays as visible teases, as on Coral Island's first farm. Everything else clears with
the worn tools she hafts in the first hour.

StumpMedium fills the gap between the small stump (3 swings) and the iron-tier large stump.

### Spoiled ground (tilling and building refusal)
Point obstacles only blocked a square that contained their centre, so a field of hundreds still
left most squares free. Each kind now has a spoil radius, for example:

| Kind | Spoil radius |
|---|---|
| Weeds | 50 cm |
| Medium stump | 80 cm |
| Boulder | 140 cm |
| Giant log | 280 cm |

`OvergrowthSpoiling(state, footprint)` finds the nearest uncleared obstacle whose circle overlaps
the footprint. It applies only to new actions (Till, CheckSite), not to save validation, so an old
save with a plot beside a later-regrown tuft still loads. Weed creep uses the same radius to keep
regrowth off plots and buildings.

### Rubbish
The rubbish kinds are cleared by hand, with no tool.
- Crates, barrels and planks play the stick kneel: she breaks the rotten boards into kindling.
- Middens play the stone kneel, the same as salvage.

Yields come from the node id, so they're deterministic and never re-rolled. Finds are small and
realistic: twine, a little seed, scrap lead.

### Feedback
`AHomesteadWorld` notices when a node's visual goes from uncleared to cleared during play, not
while loading or streaming a region. When it does:
- the old components are handed to a 0.4 s pop, a 10% swell, then a shrink and sink;
- 7-10 chips are spawned, from the grass tuft, fallen bough or granite spalls mesh, and tumble
  ballistically for 0.75 s.

None of this is saved.

## Risks

- **Performance:** about 380 more per-node static mesh components on the estate. They're walk-through
  with no collision, and their meshes have LODs. Watch GPU time in the packaged build.
- **No collision** means she can walk through barrels and stumps. Physically blocking obstacles
  would need the strike approach to handle convex hulls first. That's a follow-up.
- **Manor lane dependency:** the EstateDebris meshes come from the manor lane. Until they're on
  main, the store crate and barrel stand in, and a midden stand-in covers the heap.
