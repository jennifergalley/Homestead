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
parts, all generated here with no external textures:

- **Shore data.** `bake_ocean.py` reads `Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16`
  and writes `Saved/Ocean/T_EstateOceanShore.png` (2048×1024 RGBA): R = √(depth / 32 m),
  G = √(distance to the shore / 512 m), B = exposure to the open sea (a wide blur of the sea mask).
  Only water connected to the southern (Atlantic) edge counts. The frame is in
  `Saved/Ocean/ocean_bake.json`: `u = (Y − ShoreY0) / ShoreSizeY`, `v = (ShoreX1 − X) / ShoreSizeX`,
  in game metres. The material's `ShoreFrame` parameter holds the same four numbers, so update it
  when the heightfield changes.
- **Mesh.** `SM_EstateOcean` is a tensor grid in world centimetres at the origin. Cells are 8 m
  wherever the ground is below 1.2 m, and they grow ×1.28 per step outside the map out to 60 km to the
  south, west and east, so the horizon is sea. Everything south of the map is sea, including off
  headlands that reach the edge. It's about 89k vertices, without collision, Nanite, lightmap UVs or
  a distance field. Interchange's OBJ reader maps OBJ (x, y, z) to Unreal (x, −y, z), so the bake
  writes y negated and swaps the face winding.
- **Material.** `M_EstateOcean` / `MI_EstateOcean` evaluate the waves analytically in two Custom
  HLSL nodes, in world metres (never mesh UVs), so nothing stretches with the mesh:
  - `Swell`: four long swells (78 m and shorter, 0.32 m) from the south-south-west move the
    vertices with world position offset. They follow deep-water dispersion (each wavelength travels
    at its own speed) and die away in water shallower than about 6 m, so the beaches never clip.
  - `WindSea`: 28 short-crested wind waves from 11 m down to 0.55 m, widely spread about the wind,
    with random amplitudes. `Gusts` adds drifting patches of rougher and glassier water (cat's paws).
    `Ripples` adds two rotated layers of the tiling capillary normal map. Waves shorter than a pixel
    fade out and their slope moves into roughness, so the distant sea softens into glitter instead
    of shimmering.
  - `ShoreWaves`: crests that follow the baked shore distance and roll in every 9 s. Each one
    throws a sheet of swash foam up the sand that drains into lace, with a thin line that lingers
    at the waterline. Foam lace drifts shoreward on a two-phase flow map.
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

The tuning parameters are on `MI_EstateOcean`, grouped Waves, Foam, Colour and Data. Bake the
values you settle on into the defaults in `build_ocean.py`.
