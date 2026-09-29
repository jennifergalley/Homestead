# Proposal

## Why

Jenny's round-1 playtest: "The terrain looks a little simplistic, can we improve the textures for the
ground, maybe give it some 3D wavy grass? Also I think the footsteps sound needs to be a little softer
on this grass." The estate's pasture is one flat green texture with a handful of tufts, so the fields
read as a lawn rendered from a distance; and every surface plays the same bare-foot step.

## What Changes

- **Ground detail.** M_EstateLandscape gains a finish pass after its paint layers: the near texture
  fades into its macro sample with distance (no visible tiling), broad colour variation, dry grass on
  south slopes, trodden soil round the manor, the road shoulders and working sites (CC0
  `grass_path_2`), leaf litter and moss under the trees (from the scenery's tree records), stony soil
  on steep banks (CC0 `rocky_trail`), and the sward's colour where the 3D meadow grows and past it.
- **3D wavy grass.** A camera-centred meadow of real grass blades (`UHomesteadGrassField`): 2 x 2 m
  blade patches instanced in 6 m chunks out to about 50 m, three nested LODs, thinned by distance in
  the material with no popping, swaying in rolling gusts, parting round the heroine. Density, height
  and dryness come from a baked ground map. It stays out of the manor, off the road's wheel tracks,
  off water and the beach, round the town, off the derelict farm's fence line, and clears a circle
  round every interactable, world drop, plot and building piece.
- **Softer steps on grass.** On grass, moor and woodland floor her footsteps are quieter (-4 to -5 dB)
  with a gentle low-pass; other surfaces keep today's step.
- LandscapeGrass stays off (it comes out empty in 5.8 PIE); the meadow doesn't depend on it.

## Capabilities

### New Capabilities

- `estate-ground-presentation`: estate ground detail, the near-camera meadow, and surface-aware
  footstep softening.

### Modified Capabilities

None.

## Impact

- New: `Scripts/Terrain/bake_ground.py`, `Scripts/Terrain/build_ground.py`,
  `Source/SurvivalGame/HomesteadEstateGround.*`, `Source/SurvivalGame/HomesteadGrassField.*`,
  `Content/SurvivalGame/Estate/Ground/*`, `Content/SurvivalGame/Estate/Runtime/EstateGround.bin`.
- Changed: `Scripts/Terrain/build_landscape_material.py` (and `M_EstateLandscape`),
  `AHomesteadWorld::Refresh` (one call), `AHomesteadController::PlayFootstep`.
- No save change (SimulationSaveVersion stays 12). No interactable moves.
