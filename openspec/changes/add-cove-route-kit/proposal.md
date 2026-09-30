# Proposal

## Why

Water is laying a protected walking route from the manor's south front down to the cove: about 85 m of
descent over roughly 450 m. Most of it is graded path, with 3 to 6 short stair flights where it crosses
the cliff band. The route needs original 1850s Cornish estate furniture: steps she can climb, an edge that
stops her walking off the drop, and a sign that tells her where the path goes. Props authors the meshes;
Water owns the route and places them at runtime from its route data, as the road bridge is placed.

## What Changes

- Four Blender recipes under `Scripts/Blender/Recipes/`, all original procedural geometry and baked PBR
  materials:
  - `cove_steps.py`: granite treads `SM_CoveStep_A/B/C`, a landing slab `SM_CoveLanding` and a rubble
    side cheek `SM_CoveStepCheek`.
  - `cove_kerb.py`: granite edge kerbs `SM_CoveKerb_Straight`, `_Curve15` and `_End`.
  - `cove_handrail.py`: oak post-and-rail `SM_CoveRail_Level`, a raked bay `SM_CoveRail_Bay` for each
    flight pitch, `_EndPost` and `_CornerPost`.
  - `fingerpost.py`: an oak `SM_Fingerpost_ToTheCove`.
- Each export lands in `Assets/Props/<Name>/` with its report and review renders, and is imported as a prop.
- The pivots, sizes and collision in design.md are the interface Water's generator builds against.

## Impact

- New recipes, meshes, textures and imported props only. There's no simulation, save or C++ change on the
  Props side. Water adds the route data, a runtime builder and the Landscape cut.
- Blender builds, renders and imports need a Blender slot and an Unreal slot; nothing heavy runs before then.
