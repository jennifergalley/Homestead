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
