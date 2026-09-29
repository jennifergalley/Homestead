## Context

The MVP survival game builds its woodland per 24 m chunk at runtime. The Estate is one fixed map
whose decorative scenery is baked offline by `scatter.py` into `EstateScenery.bin` (one HISM per
kind) and whose interactables are baked into `HomesteadEstateWorldPlacements.inc`. This change ports
the MVP's woodland rules into that bake for one region.

## The MVP woodland, as built (the reference)

From `HomesteadWorldGeneration.cpp` and `HomesteadWorld.cpp` (unchanged on `mvp-survival`):

| Layer | MVP rule | Meshes |
| --- | --- | --- |
| Trees | 36 slots per 24 m chunk on a 4 m grid, jittered 0.5-3.5 m within the cell and pulled up to 1 m towards one of four cluster centres; kept with probability (62000 + woodland/20)/65536, about 95-99%; one chunk in four has a 2.8 m-radius opening; none on a granite knob (radius 5.4 m, one chunk in seven); none within 2.5 m of the creek | 60% `SM_TreeSmall02_Woodland` (scale 0.90-1.04), 35% `SM_MatureFir` (0.90-1.05), 5% `SM_Jacaranda` (0.90-1.06) |
| Young trees | Sapling forage, about 2.5 per chunk | 45% young broadleaf (`SM_TreeSmall02_Woodland` 0.90-1.08), 55% young conifer: `SM_FirPole` (variant 2 of 4) or `SM_FirSapling_a`/`_c` (0.92-1.08) |
| Underbrush | 11 clusters per chunk; the density field (Perlin at 34 m and 11 m) picks thicket (>= 0.64: 7+ plants over 4.2 m; brambles, bramble thickets, toyon), shrubs (>= 0.32: 3-12 plants over 3.4 m; hazel, deer brush, thimbleberry, fern, strawberry, yarrow) or meadow (4-9 over 2.4 m; strawberry, yarrow, fern); plus 30 scattered tries; plants keep 0.6 x the sum of their radii apart | `BlackberryBramble`, `BlackberryBrambleLarge`, `ToyonHedge`, `Hazel`, `DeerBrush`, `Thimbleberry`, `BrackenFern`, `WildStrawberry`, `GrassYarrowTuft` |
| Ground cover | 1200 tries per chunk (about 2 per m²) of grass: 1/16 mid, 2/16 small, 9/16 tall, 4/16 tiny; every 32nd try a fern (a-d in turn); every 64th a flower (scale 0.55-0.80). Grass culls at 35-50 m, flowers 30-48 m, ferns 50 m, low underbrush 22-38 m, shrubs 90 m | `SM_GrassMedium01_*`, `SM_Fern02_a-d`, `SM_FlowerEmpodium_a/b` |
| Granite | Outcrops in broad bands: an erratic or jointed boulder anchor with 2-3 boulders and 3-5 cobble clusters round it (12-52% of chunks); a dome or split boulder on each knob; lone erratics; 1-4 scattered cobbles per chunk | Granite cobbles, spalls, rubble, loaf, talus, low, erratic, jointed, dome, split |
| Forage | Per chunk: 1 branch pile, and a small patch each (anchor plus up to 3) of stones, berry bushes, roots, flowers and saplings | Estate visuals are the same as the MVP's |
| Floor | `M_GrassGroundBlend` (`T_GrassGround_*`) | |
| Light | The same `AHomesteadWorld` sun, sky and height fog as the Estate uses (fog 0.007 by day) | |

## Decisions

- **Region.** `Scripts/Terrain/mvp_woodland.json` holds the polygon (metres, +X north, +Y east)
  and the ecotone width. It lives outside `estate_layout.json` because `reshape.py` regenerates that
  file. The region is the western estate's gentle pasture slopes, clear of the manor grounds, the
  farm, the drive, the river valley and the mine entrance.
- **Faithful rules, estate coordinates.** The scatter reproduces the MVP rules on a 24 m chunk grid
  over the region with numpy, using its own deterministic noise, not the MVP's hashes (the MVP's
  exact trees depended on a world seed; the look comes from the rules).
- **Ecotone.** Within 25 m inside the edge the MVP layers thin out with a noisy falloff, and the
  estate's Cornish layers are removed only inside the region minus a noisy 12 m band, so the edge
  reads as a mixed fringe rather than a wall.
- **Interactables.** A share of the MVP's rates: about one tree in thirty is a fellable ForestTree
  (the rest are scenery, as the Estate's other woods are); branches, stones, berry bushes, roots and
  flowers at about a third of the MVP per-chunk rates, all clear of the drive and each other. Ids
  start at 560000 and are written into the same generated `.inc`. Forest trees with ids in 560000+
  pick the MVP palette in `ResolveGeneratedTreeVisual`.
- **Brambles are clearable overgrowth.** As in the MVP, a blackberry bramble blocks her until she cuts it. The region's brambles are interactable nodes, not scenery: `BrambleThin` (`SM_BlackberryBramble`, worn billhook) and `BrambleThicket` (`SM_BlackberryBrambleLarge`, iron billhook, the estate's rule). `BuildOvergrowth` draws the MVP meshes for ids in the region's range, and their base mesh blocks her (not the camera or traces). They take ids after the forage, so no earlier id moves. Toyon and the other shrubs stay walk-through scenery.
- **Performance.** Tree kinds keep cull distance 0 (as the other estate trees); every small kind gets
  the MVP's cull distances, no shadows on grass/ferns/flowers/low underbrush, and the existing 60 m
  WPO disable distance. Grass density is set after measuring the instance count and load time.

## Lanes and ownership

- This lane: `scatter.py` (the MVP section and the in-region masks), `mvp_woodland.json`, the
  generated `EstateScenery.bin` and `.inc`, `EstateSceneryKinds` rows 19+, the region palette in
  `ResolveGeneratedTreeVisual`.
- Trees lane (`65a2408b`): kinds 16-18 and the Cornish tree meshes.
- Ground lane (`89914e30`): `bake_ground.py`, `build_ground.py`, the landscape material: the MVP
  floor zone, no blade grass and the canopy kinds, from `mvp_woodland.json`.
- Re-bake order after this change: `scatter.py`, then the ground lane's `bake_ground.py` and
  `build_ground.py`, then the estate map (`docs/setup.md`).
