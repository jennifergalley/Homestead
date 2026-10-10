# Village Path And Dressing

## Why

Jenny, 2026-10-04: "The path down to town from the main road could be smoother. The terrain looks a little choppy, and the path doesn't seem as well-worn as it should be."
Balance (docs/design/cohesion.md section 4, 2026-10-04): the relocated village should feel lived-in (packed or cobbled square, cottage gardens, a well, a bench), and its flower beds should read as colour from about 10 m.

## What

- Re-grade the village street corridor (`Scripts/Terrain/town_path.py`): a longitudinal smoothing with a 1:30 cross-fall, a 4 m dirt width and a worn centre strip with trodden shoulders. Flower beds along the village front use the bloom-heavy kinds (bluebell, primrose, red campion, foxglove).
- Dress the village from a generated table (`Scripts/Terrain/village_dress.py` writes `HomesteadVillageDressing.inc`, placed at runtime by `HomesteadWorldVillage.cpp`), all relative to the layout's square, buildings and street so the half-scale rebake carries it over: a well, benches, a granite sett apron, cottage gardens (mixed crop stages and flowers behind a fence and hazel hedge), store clutter and daffodils at the square.
- Reuse-first: granite setts, farm fence, tilled beds, crops, store barrels and sacks, hazel and the wild flower clumps are existing meshes. Only the well, bench and sett patch are new original Blender recipes.
- Player-facing text says "village".

## Out of scope

Stage 2 of the map shrink (half-scale terrain rebake), new save fields, new simulation rules.
