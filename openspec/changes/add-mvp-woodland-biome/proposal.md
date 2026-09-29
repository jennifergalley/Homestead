## Why

Jenny asked for part of the Estate to look and feel like the woodland from the MVP survival game:
"we did a really good job with that forest ... I still want parts of it to feel like CA." The
Estate's woods are Cornish oak, beech, sycamore and hazel with bracken. The MVP forest is a Sierra
Nevada / Californian mixed woodland: small broadleaves, tall firs and the odd jacaranda over
brambles, toyon, deer brush and thimbleberry, ferns and grass clumps on a grassy forest floor, with
granite outcrops. Walking into one clearly bounded part of the estate should feel like walking back
into the MVP.

## What changes

- A fixed **MVP woodland** region of about 12 ha in the west of the estate, west/north-west of the
  manor (`Scripts/Terrain/mvp_woodland.json`), about a minute's walk from the standing room. The
  river-valley oak/beech woods, the moor, the pasture and the manor grounds keep their Cornish
  character.
- `Scripts/Terrain/scatter.py` stops placing the Cornish woods inside the region (a 25 m ecotone
  mixes the two at the edge) and scatters the MVP biome there with the MVP's own rules: the tree grid,
  species mix and scales, clusters and small openings; the underbrush clusters, thickets and
  scattered shrubs; fern, grass and flower cover; and granite outcrops, knobs and lone erratics.
- New `EstateSceneryKinds` rows (19 onward) for the MVP meshes the estate didn't have yet:
  jacaranda, fir pole, fir saplings, blackberry bramble and thicket, toyon, deer brush,
  thimbleberry, wild strawberry, the other fern and grass variants, the Empodium flowers and the
  other granite kinds.
- Interactable MVP forage in the region (ids 560000+): fellable forest trees, branches, roots, berry
  bushes, stones and flowers at a share of the MVP's per-chunk rates, so gathering there feels like
  the MVP. Forest trees placed there resolve to the MVP palette (60% broadleaf, 35% fir, 5%
  jacaranda) instead of the estate's 80/20.
- The MVP forest floor under the region: requested from the ground lane (`bake_ground.py`): an MVP
  woodland zone using the MVP floor material textures, no blade-field grass inside the region (the
  MVP clumps replace it), woodland footsteps, and the new tree kinds in its canopy mask.

## Reuse research

- The MVP forest is still on `main`: `AHomesteadWorld`'s generated-chunk path
  (`HomesteadWorld.cpp` `BuildDecorations`, `GenerateUnderbrush`, `GenerateRocks`,
  `ResolveGeneratedTreeVisual`) and `Simulation/HomesteadWorldGeneration.cpp` (`TreeCandidate`,
  `AssignTreePalette`, `Candidate`). `mvp-survival` changed only night lighting and building
  previews since the fork (`git diff 750a4397 origin/mvp-survival`), so these rules are the MVP
  look. Every mesh is already in the project (`/Game/Trials/*`, `Environment/Props/*`), with its
  camera-safe foliage materials; nothing new to author or license.
- The Estate already draws baked scenery as one HISM per kind from `EstateScenery.bin`, hides it
  round every interactable, and bakes interactables into `HomesteadEstateWorldPlacements.inc`. The
  region needs only new kind rows, scatter rules and placements.
- Custom work is limited to porting the MVP's scatter rules to `scatter.py` and choosing the MVP
  palette for trees placed in the region.

## Smallest useful result and first playable demonstration

Walk west from the standing room for about a minute: the rough lawns and oak/hazel scrub give way,
over about 25 m, to the MVP forest: small broadleaves and firs every few metres with the odd
jacaranda, bramble thickets, toyon and deer brush, ferns and grass clumps, granite knobs, and trees,
branches, roots and berry bushes she can gather as in the MVP. Side-by-side screenshots of the MVP
woodland and the estate region from the same PIE session setup show the match.

First delivery: the region's scenery and interactables (this lane). Full acceptance also needs the
ground lane's MVP floor zone and the integration session's packaged build with ray tracing, where
the canopy light is judged. Deferred: clearable (colliding) brambles as in the MVP, MVP creek, and
regional fog or lighting tweaks.

## Impact

- `Scripts/Terrain/scatter.py`, `Scripts/Terrain/mvp_woodland.json` (new),
  `Content/SurvivalGame/Estate/Runtime/EstateScenery.bin`,
  `Source/SurvivalGame/Simulation/HomesteadEstateWorldPlacements.inc` (generated)
- `Source/SurvivalGame/HomesteadWorld.cpp` (`EstateSceneryKinds` rows, forest-tree palette in the
  region)
- `Source/SurvivalGame/Simulation/HomesteadEstate.h` (id-range comment)
- Ground lane: `Scripts/Terrain/bake_ground.py` and the landscape material (requested, not owned)
