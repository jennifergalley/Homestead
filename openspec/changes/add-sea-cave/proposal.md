# Add Sea Cave

## Why
Jenny's Next-build card (jenny-mv376d6c-vmhkjy): "Just like in Poldark, please add a cave into the cliffs along the beach, that has a cool little water reservoir inside, or something else interesting to explore - it should be fairly sizable". Also answers the older cove-cave card (jenny-muuqa6l2-z1awzr).

## What
- A walkable granite sea cave in the cove bay's west cliff, entered from the sand: a mouth under a granite crag, a bending passage and a domed chamber (about 20 x 15 m) with a still freshwater pool, a low ledge, and a thread of sky through a roof fissure (faint moonlight at night).
- Scenery only: no save changes, no new placement ids (the Beach lane's two tideline nodes sit at the mouth inside its own budget). The pail fills at the pool. The field-book map labels it "Sea cave".
- Built from `Scripts/Blender/Recipes/sea_cave.py` (fitted to the heightfield under it) and placed at runtime by `AHomesteadSeaCave`; the landscape's Visibility layer is holed at the mouth (`Scripts/Terrain/build_landscape_holes.py`).
