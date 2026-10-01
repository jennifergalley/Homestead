# Proposal

## Why

Jenny (2026-09-30): "The path through the woods is fantastic by the way. The one that leads to the lake? Chef's
kiss. Now we just need to add berry bushes and wild roots in the forest around it for me to forage, and
wildflowers lining the path and the forest floor (even if just decorative)."

## What Changes

- **Forage:** 23 live placements (582300-582322: 13 brambles, 10 wild roots) in clusters on both sides of the
  lake trail: at its edges (within about 9 m, reachable from the path), in canopy gaps and clearings, and a few
  12-30 m in for exploring. Existing kinds, so gathering, regrowth, seasons and saves follow the existing rules.
  They're appended after every other section (after the tool rack) with the 3 m rule, so nothing earlier moves.
- **Wildflowers:** 548 decorative clumps (EstateScenery kinds 42-48) in drifts along both verges and patches on the
  woodland floor where light comes through: bluebells, primroses, wild garlic towards the lake, wood anemones, red
  campion, foxgloves and cow parsley. No collision or shadow, culled at 45-65 m, batched in the near cells.
  Bluebells, primroses and wild garlic reuse the existing clump meshes; anemone, campion, foxglove and cow parsley
  wait on Props' Blender recipes (a missing mesh logs once and its records are skipped).
- `Scripts/Terrain/lake_path_plants.py` bakes both, deterministically and idempotently; `scatter.py` reapplies
  the flowers after a fresh scatter. Ids claimed: placements 582300-582399, scenery kinds 42-48.
