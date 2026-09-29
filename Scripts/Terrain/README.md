# Estate terrain pipeline

The fixed 4 km Trevennor estate map is built from real Environment Agency LIDAR of the St Agnes,
Trevaunance and Chapel Porth coast in north Cornwall. The LIDAR is turned, cropped and reshaped into
the agreed layout, then exported as one 16-bit heightmap for a single 4033² World Partition
Landscape.

## Source data

The source is the EA LIDAR Composite DTM, 2022, at 1 m resolution. It's downloaded through the Defra
Data Services Platform tile API. (The WCS endpoint returns zeros for this area.)

- Search: `POST https://environment.data.gov.uk/backend/catalog/api/tiles/collections/survey/search`
  with a GeoJSON polygon of the area.
- Download: `GET https://environment.data.gov.uk/tiles/collections/survey/lidar_composite_dtm/2022/1/<TILE>`.
  Each download is a zip containing a GeoTIFF.
- Tiles used: SW6545, SW6550, SW7045, SW7050, SW7545, SW7550. Together they cover OSGB
  E165000–180000, N45000–55000.
- The raw tiles are about 1 GB and aren't in git. Unzip them under `E:\TerrainSource\EA_LIDAR_DTM1m\`
  (or set `HOMESTEAD_LIDAR`). Intermediate grids go to `E:\TerrainSource\work\` (or set
  `HOMESTEAD_TERRAIN_WORK`).

### Attribution

The data is released under the Open Government Licence v3.0. Credit it in the game credits as:

> Contains Environment Agency information © Environment Agency and/or database right 2022. All rights reserved.

The seabed isn't in the DTM. It's generated below the tideline, and EMODnet bathymetry isn't used.

## Commands

Run these from the repo root with Python 3. The scripts need numpy, scipy, rasterio, Pillow and
scikit-image.

```powershell
python Scripts\Terrain\mosaic.py               # tiles -> work\mosaic_E165000_N45000_1m.npy (+ overview_5m.png)
python Scripts\Terrain\resample_game_frame.py  # -> work\game_raw_4033.npy in the game frame
python Scripts\Terrain\reshape.py              # -> Estate_Heightmap_4033.png, estate_layout.json
python Scripts\Terrain\preview_zoom.py game_reshaped_4033.npy overview.png -2016 2016 -2016 2016 4 Scripts\Terrain\estate_layout.json
```

`preview.py` and `preview_zoom.py` draw hillshades with 10 m contours and a labelled grid. The last
argument of `preview_zoom.py` overlays the road, river, estuary, polygons and anchors from the layout
JSON. Its arguments are `src out xmin xmax ymin ymax step [layout.json]`, in game metres.

**Re-bake the estate map afterwards.** The minimap and Map-tab texture is baked from
`Estate_Heightmap_4033.png`, `estate_layout.json` and the scenery scatter that `scatter.py` writes
(`Content\SurvivalGame\Estate\Runtime\EstateScenery.bin`). After re-running `reshape.py` or
`scatter.py`, re-bake and re-import it: see "Estate map (T_EstateMap)" in `docs\setup.md`.

## Game frame

- Unreal axes: +X is north and +Y is east, in metres from the map centre. One landscape quad is 1 m.
- The OSGB point E170300 N49000 sits at the origin. The frame is turned 90°, so game north is real
  east and the Atlantic, which is really to the west, lies to the south.
- `resample_game_frame.py` holds `CENTER` and `ROTATION`.

## Reshaping (`reshape.py`)

The script applies these steps in order:

1. **Sea.** Land below 2.5 m that connects to the southern edge becomes seabed. It deepens offshore
   to about −22 m.
2. **Estuary.** The south-east valley (the real Chapel Porth coombe) is sunk below sea level along
   its floor, keeping its natural walls. It forms a ria up to the town's harbour.
3. **Pads.**
   - The manor ruin: 18 × 30 m, flat, about 86.5 m above sea level.
   - The mine head on the western clifftop.
   - The mill by the ford.
   - The town: a plane fit on the spur above the estuary, about 91 m.
4. **Road.** The road runs from the manor forecourt, fords the river by the mill, climbs the east
   bank at the estate gateway, and continues about 1.7 km east to the town. It's graded to a 5 m bed
   with 12 m verges, and its gradient is capped at 11%.
5. **River.** A least-cost path finds the floor of the wooded valley. The river is cut into it as a
   channel 0.9 m deep and about 4.5 m wide, with its bed forced to run downhill.
6. **Cove.** A sand beach is blended into the valley mouth.
7. **Waterline.** The waterline is softened.

The anchors, polygons and centrelines are written to `estate_layout.json`, in metres. The C++
layout in `Source/SurvivalGame/Simulation/HomesteadEstate.cpp` (`ProvisionalEstateLayout`) mirrors
it in centimetres. Update both together.

Later hand edits go on Landscape Edit Layers in the editor, so re-importing this base keeps them.

## Heightmap export and import

- `Estate_Heightmap_4033.png` is 4033×4033, 16-bit greyscale.
- Pixel value = 32768 + metres × 128. That's Landscape **Z scale 100**: 1 unit is 100/128 cm, and
  the range is ±256 m.
- The PNG's orientation is Unreal's import convention: image column is +X (game north), and image
  row is +Y (game east).
- Import settings:
  - Section size 63 quads, 2×2 sections per component, 32×32 components.
  - Scale (100, 100, 100).
  - Location (−201600, −201600, 0), so the centre vertex lands on the origin and sea level is at
    Z = 0.
- The elevation range after reshaping is −22.6 m to 191.7 m. The top is St Agnes Beacon, on the
  north-west moor.

## Ocean and river (`bake_ocean.py`, `build_ocean.py`, `place_water.py`)

The sea is Single Layer Water, not the Water plugin (see the design's decision 3). It has three
parts, all generated here with no external textures. `python Scripts\Terrain\bake_ocean.py waves` re-bakes only the wind-sea volume.

- **Shore data.** `bake_ocean.py` reads `Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16`
  and, past the map edge, the outer land ring's heights (`ring()` in `bake_outer_land.py`). It
  writes two RGBA textures in the same layout: R = √(depth / 32 m), G = √(distance to the shore /
  512 m), B = exposure to the open sea (a wide blur of the sea mask). Only water connected to the
  southern (Atlantic) edge counts.
  - `Saved/Ocean/T_EstateOceanShore.png` (2048×1024) covers the map's coast at about 1.7 m per
    texel. It's baked with 640 m of ring around the map, so shore distances near the edge see the
    land beyond it.
  - `Saved/Ocean/T_EstateOceanShoreFar.png` (2048×1024) reaches 3 km past the map edge at about
    5 m per texel, so the ring's coast gets the same foam line, shallows and swell fade.
  The material uses the fine texture wherever it covers (blending over its last 30 m) and the far one
  outside it. The frames are in `Saved/Ocean/ocean_bake.json`: `u = (Y − ShoreY0) / ShoreSizeY`,
  `v = (ShoreX1 − X) / ShoreSizeX` in game metres, and likewise `Far*`. The material's `ShoreFrame`
  and `ShoreFarFrame` parameters hold the same numbers, so re-run `build_ocean.py` after the
  heightfield or the ring changes.
- **Mesh.** `SM_EstateOcean` is a tensor grid in world centimetres at the origin. Cells are 8 m
  wherever the ground is below 1.2 m, and they grow ×1.28 per step outside the map out to 60 km on
  every side, so land that reaches the map edge never shows void behind it and the horizon is sea all
  round. It's about 131k vertices, without collision, Nanite, lightmap UVs or
  a distance field. Interchange's OBJ reader maps OBJ (x, y, z) to Unreal (x, −y, z), so the bake
  writes y negated and swaps the face winding.
- **Material.** `M_EstateOcean` / `MI_EstateOcean` evaluate the waves analytically in two Custom
  HLSL nodes, in world metres (never mesh UVs), so nothing stretches with the mesh:
  - `Swell`: four long swells (78 m and shorter, 0.32 m) from the south-south-west move the
    vertices with world position offset. They follow deep-water dispersion (each wavelength travels
    at its own speed) and die away in water shallower than about 6 m, so the beaches never clip.
  - `WindSea`: the wind sea comes from `VT_OceanWaves`, a 128×128×64 volume texture baked by
    `bake_ocean.py` (`T_OceanWaves.png`, an 8×8 atlas of frames). It's a 48 m patch of a
    Phillips-spectrum FFT ocean for a 5.5 m/s breeze, with each frequency rounded so the patch loops
    seamlessly every 16 s. It stores slope, height and a whitecap mask from where the choppy
    surface folds. The material samples three copies at unrelated scales and headings, each with
    its clock scaled by 1/√scale so dispersion stays right, so neither the tile nor the loop shows.
    `Whitecaps` turns the folded crests into foam, with more of them in the gusts. `Gusts` adds
    drifting patches of rougher and glassier water (cat's paws). `Ripples` adds two faint rotated
    layers of the tiling capillary normal map. Detail finer than a pixel is averaged away by the
    mips, and its slope variance moves into roughness, so the distant sea softens into glitter
    instead of shimmering.
    Volume-texture import: `build_ocean.import_wave_volume()` imports the atlas, then sets it as
    the `VolumeTexture`'s `source2d_texture` before setting the tile sizes to 128. Setting the
    source resets the tile size to a default (102 for a 1024² atlas).  - `ShoreWaves`: crests that follow the baked shore distance and roll in every 9 s. Each one
    throws a sheet of swash foam up the sand that drains into lace, with a thin line that lingers
    at the waterline. Foam lace drifts shoreward on a two-phase flow map.
  - `MapEdgeDeepening`: the authored seabed stops at the map edge (about −22 m) and the water
    outside has no floor (except in the shallows along the ring's coast, which keep their colour), which showed as a step from teal to navy along the edge. Over the last
    400 m inside the map, absorption and scattering are scaled up together (×3.5 at the edge),
    which keeps the saturated colour and makes the water read as deep.
  - Colour comes from low scattering and red-first absorption: turquoise over the sand shallows,
    deep blue-green offshore. Facets are flattened just enough to keep every reflection above the
    horizon, because a reflection ray into the sea returns black.

Rebuild after changing the heightfield, and after changing any of the scripts:

```powershell
python Scripts\Terrain\bake_ocean.py            # ~45 s -> Saved\Ocean\*, Assets\Environment\Ocean\*.png
```

Then, in the editor with the Estate level loaded and PIE stopped, run `pyfile
Scripts\Terrain\build_ocean.py` (McpHelpers). It imports the three textures and the mesh, re-authors
the material graph in place and points `EstateSea` at the result. `place_water.py` updates
`EstateSea` and `EstateRiver` in place (their external actor files keep their names). The river ends
where its surface meets the cove beach (the stream soaks into the sand), so the ribbon and its
`HomesteadWater` pail-refill tag never lie over salt water. The sea is tagged `HomesteadSea` only.
Save both actors' packages afterwards: PIE streams spatially loaded actors such as `EstateRiver`
from their saved packages, so unsaved edits don't show in play, and `set_course` alone doesn't
dirty the package (the script calls `modify()` first).

Cost, from `ProfileGPU` in PIE at a 3054×1135 viewport with the sea filling the view from the western clifftop: `SLW::Draw` 0.23 ms, depth prepass 0.09 ms, and Lumen water reflections about 0.2 ms. The old one-plane creek-material sea cost 0.10, 0.07 and 0.2 ms. The mesh has about 89k vertices. Read the stable sub-passes: frame times and reflection spikes swing widely when other editors share the GPU.

The tuning parameters are on `MI_EstateOcean`, grouped Waves, Foam, Colour and Data. Bake the
values you settle on into the defaults in `build_ocean.py`.

## Ground and meadow (`bake_ground.py`, `build_ground.py`, `build_landscape_material.py`)

`bake_ground.py` (about 90 s) reads the heightfield, the paint-layer weights, `estate_layout.json` and
the tree records in `EstateScenery.bin`, and writes:

- `Saved/Ground/T_EstateGround.png` (2048², over the map like `T_EstateRoadSDF`): R = grass density,
  G = grass height, B = dryness, A = wear (trodden soil round the manor, the road shoulders, the mill,
  mine and gateway).
- `Saved/Ground/T_EstateCanopy.png`: R = tree canopy (every tree kind in `SCENERY_TREES` of
  `Scripts/Map/bake_estate_map.py`, each at its own crown radius times its scale, with a soft edge), G = stony soil on steep banks outside the cliff layer.
- `Content/SurvivalGame/Estate/Runtime/EstateGround.bin`: "HGD1", u16 size (1024), then per cell the
  most grass anywhere in it (u8) and the surface under it (u8: Soil, Grass, Road, Sand, Rock, Woodland,
  Moor, Water). The game reads it through `HomesteadEstateGround` for the meadow and footsteps.
- `Assets/Environment/Ground/T_GrassWind.png`: tiling gust noise.
- `Saved/Ground/SM_GrassPatch_LOD{0,1,2}.obj`: 2 × 2 m patches of 1100, 423 and 136 grass blades (5500, 1269 and 136 triangles).
  The LODs are nested by each blade's random rank (all, rank < 0.38, rank < 0.14). UV0 holds the
  blade's rank bucket (whole part) and its root in the patch (fraction), so the material can find the
  root; `build_ground.py` imports with full-precision UVs and detects Interchange's V flip.

No grass grows in the manor footprint, within 3.9 m of the river, near the beach, within 140 m of
the town square, or on the derelict farm's fence line. The farm's field itself is left overgrown.

Then, in the editor with PIE stopped, run `pyfile Scripts\Terrain\build_ground.py` and then
`pyfile Scripts\Terrain\build_landscape_material.py`. The first imports the data, the meshes and the
CC0 ground sets (downloaded from Poly Haven to `HOMESTEAD_GROUND_DOWNLOADS`, default
`Saved/Ground/Downloads`; not kept in git). It also authors `M_EstateGrass` / `MI_EstateGrass`. The
second rebuilds `M_EstateLandscape` with its ground-finish pass.

At runtime `UHomesteadGrassField` (`Source/SurvivalGame/HomesteadGrassField.*`) instances the patches
in 6 m chunks within 51 m of the camera. `AHomesteadWorld::Refresh` updates it every 0.25 s. It writes
three clear circles per patch into per-instance custom data, one for each nearby interactable, world
drop or plot, and skips patches under building pieces. A garden plot is a bare square instead of a circle (a negative radius in the custom data: the plot's half size plus 15 cm), so tilled beds and planted crops never have blades through them. Inside those circles the sward is grazed to a fifth of its height, with a ragged edge, rather than left bare. Blades near a low game camera, and along its line to the heroine, are grazed too (`MPC_CameraSafeFoliage`), so the camera never looks through a wall of grass. `M_EstateGrass` thins blades by rank with
distance (`GrassFade`: blades ranked below min(1, (12/d)^1.7) show). It also clears the road's wheel
tracks, bends the blades in gusts (`GrassWind`) and parts them round the heroine (`GrassPush`).
`HomesteadGrassField.h`'s LOD distances depend on `GrassFade` and the LOD keep fractions, so change
all three together.

MVP woodland zone: when `Scripts/Terrain/mvp_woodland.json` exists (the woodland-biome lane's
region: `polygon` in metres, optional `floor_edge_m` (default 15) and `glades`), no meadow blades grow
inside the polygon and footsteps are woodland floor there, since that lane scatters the survival
prototype's own grass clumps. `T_EstateCanopy.B` ramps the prototype's forest floor in from the edge
to `floor_edge_m` inside, along a wandering line. The floor is the prototype's `M_GrassGroundBlend` as the generated woodland drew it away from the
creek: `T_Ground*` (brown mud and leaves) mixed with 7-27 % `T_GrassGround*` in slow patches, at 3 m
world tiling, easing into a 12.9 m copy past 60 m. Glades take a lighter mix. Tree canopy kinds
and crown radii come from `SCENERY_TREES` in `Scripts/Map/bake_estate_map.py`, with a fallback in
`bake_ground.py` for kinds 13-16 and 19-22 until it lists them. Order after the woodland and trees
branches land: `scatter.py`, then `bake_ground.py` and `build_ground.py`.

Weather and night: `build_ground.py` also makes `MPC_EstateGround` (Wetness, Daylight), which
`AHomesteadWorld::UpdateLighting` sets every refresh. The ground wets through over the first half hour
of the rain and dries over four hours after it. Wet soil, litter and stone darken and gloss (turf
less), trodden ground holds a sheen of water, and grass blades darken and gloss. The blades' light
transmission and gust sheen fade out at night so the meadow doesn't glow under the moon
(`NightTransmission` on `MI_EstateGrass`: 0.3 keeps a little light through the blades so their shaded faces don't go black).

Re-run `bake_ground.py` and `build_ground.py` after changing the heightfield, the layout,
the paint layers or the scatter (the canopy comes from its trees).

## Rain and overcast (`build_weather.py`, `Scripts/prepare_rain_loop.py`)

`python Scripts\prepare_rain_loop.py` cuts `Assets/Audio/Ambience/RainLoop.wav` from a CC0 recording.
Then, in the editor with PIE stopped, run `pyfile Scripts\Terrain\build_weather.py`. It builds
`/Game/SurvivalGame/Estate/Weather`:
- `MPC_EstateWeather`: Rain, Overcast, Daylight, and Shelter0-7 (the roofs nearest the camera).
- `SM_RainStreaks`: 16,000 near and 9,000 far quads.
- `M_Rain`: world-anchored streaks wrapped round the camera, slanted by the wind and thinned by the rain.
- `M_RainClouds`: the cloud layer over sky pixels only.
It also imports `RainLoop`. At runtime `UHomesteadWeather` (`Source/SurvivalGame/HomesteadWeather.*`)
follows the camera. `AHomesteadWorld::UpdateLighting` greys the light from
`Homestead::Overcast(hour)`: the sun dims and softens, the sky light rises, exposure drops 0.8 EV and
saturation drops to 72%. The streaks and sound follow `Homestead::RainAmount(hour)`. The landscape
rings its standing water with ripples while it rains.
