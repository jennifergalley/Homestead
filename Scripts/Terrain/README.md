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
python Scripts\Terrain\public_road.py          # -> Simulation\HomesteadEstatePublicRoad.inc
python Scripts\Terrain\preview_zoom.py game_reshaped_4033.npy overview.png -2016 2016 -2016 2016 4 Scripts\Terrain\estate_layout.json
```

`preview.py` and `preview_zoom.py` draw hillshades with 10 m contours and a labelled grid. The last
argument of `preview_zoom.py` overlays the road, river, estuary, polygons and anchors from the layout
JSON. Its arguments are `src out xmin xmax ymin ymax step [layout.json]`, in game metres.

**Re-bake the estate map afterwards.** The minimap and Map-tab texture is baked from
`Estate_Heightmap_4033.png`, `estate_layout.json` and the scenery scatter that `scatter.py` writes
(`Content\SurvivalGame\Estate\Runtime\EstateScenery.bin`). After re-running `reshape.py` or
`scatter.py`, re-bake and re-import it: see "Estate map (T_EstateMap)" in `docs\setup.md`.

After a road, `roadProfile`, route sign/stop, or terrain-height change, also run
`public_road.py`. It reads the layout plus `EstateHeightfield.r16` and generates the runtime 486-point
public-road centreline, chainage, safe travel endpoints, sign anchors and bridge keep-out in
`Simulation\HomesteadEstatePublicRoad.inc`. Verify its stops, signs and terrain heights before release.

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
   with 12 m verges. Its first profile was capped at 11%, which built a 10 m causeway and a 13 m
   cutting in the river valley; `road_grade.py` has replaced it (see "Road grade" below).
5. **River.** A least-cost path finds the floor of the wooded valley. The river is cut into it as a
   channel 0.9 m deep and about 4.5 m wide, with its bed forced to run downhill. At the end of the
   run `river_channel.py` grades it to its finished channel (see "River channel" below).
6. **Cove.** A sand beach is blended into the valley mouth.
7. **Waterline.** The waterline is softened.

The anchors, polygons and centrelines are written to `estate_layout.json`, in metres. The C++
layout in `Source/SurvivalGame/Simulation/HomesteadEstate.cpp` (`ProvisionalEstateLayout`) mirrors
it in centimetres. Update both together.

Later hand edits go on Landscape Edit Layers in the editor, so re-importing this base keeps them.

### Road grade (`road_grade.py`)

The road follows the ground. `python Scripts\Terrain\road_grade.py` re-grades its long profile once:
the ground it was graded over, lightly smoothed, held to 1 in 5 at most (the smallest-worst-deviation
profile within that grade), with a level bridge deck 0.9 m over the river at chainage 683-684 m and
1 in 12 approach ramps inside the 20 m bridge keep-out. The manor forecourt and town end keep their
levels, and the centreline, chainage and anchors don't move. Cut and fill against the natural ground
fell from +9.9 / -12.8 m to +1.6 / -1.4 m (p95 0.7 m).

It changes only the road corridor (reshape.py's grading is linear in the profile, so the change is
exact), re-seats the river under the bridge with `river_channel.py`, and records `roadProfile` and
`roadGrade` (with the previous profile) in `estate_layout.json`. Run again, it leaves the heightfield
alone and only brings a stale work npy into step. Afterwards: `public_road.py`, `weightmaps.py` (the
old cutting's banks lose their cliff paint), `bake_ground.py`, `bake_estate_map.py`; in the editor
`ApplyEstateHeightfield` over the rectangle it prints (r16 rows 1382-3111, columns 1513-2052),
`ApplyEstateWeightmaps` over the same rectangle with the seven layers from `<work>\weights`, then
`build_ground.py` and `ImportEstateMap`.

### Town square (`town_layout.py`)

`python Scripts\Terrain\town_layout.py` lays out the town round the TownSquare anchor. It makes an open
60 × 45 m square faced by terraces of two (sharing a party wall) and cottages, with 3–6 m side lanes
between the groups, and puts the general store in the middle of the east side. A curved 5.5 m
`townStreet` runs from the main road's last point into the square's north-west corner.

The script refuses to write a layout that breaks those rules. It writes `town` (square, street,
buildings, store footprint) and the GeneralStoreDoor/Counter anchors to `estate_layout.json`, and
`Tests/Data/HomesteadTownLayout.inc` for the native checks. Mirror the anchors in `HomesteadEstate.cpp`.

The town stands on reshape's plane pad, so the heightfield doesn't change. `weightmaps.py` paints the
street and square as packed earth, `bake_ground.py` wears them, and `bake_estate_map.py` draws them
with the buildings. In the editor, `Content/Python/homestead_agent/town_massing.py` places the
blockouts from the layout and deletes stale ones.

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
`EstateSea` and `EstateRiver` in place (their external actor files keep their names), and places the
spring's stones (`EstateSpringStone1-8`, folder `Water/Spring`). Neither the sea nor the river is
spatially loaded, so both are drawn and the pail can probe their loaded water geometry; the player
still must stand within the gameplay freshwater edge-distance reach to fill a carried pail.
The river runs over the cove beach into the shore wash; its `HomesteadWater` tag (the pail refill)
covers only the stream. The sea is tagged `HomesteadSea` only. Save the actors' packages afterwards
(`set_course` alone doesn't dirty the package; the script calls `modify()` first).

### Estate lake (`lake_basin.py`, `lake_features.py`)

An upland pool north-west of the farm (up-left on the north-up estate map), centred (-50, -745) m, about
76 x 44 m. `python Scripts\Terrain\lake_basin.py` grades it once:
- a bed shelving 1 in 4 to 1.8 m, a wet lip rising 0.3 per metre out of the water;
- a cut bank with granite on the uphill side, and a low turfed pond bay on the downhill side;
- a 1 in 10 landing on the farm side, and a footpath from the farm's north fence (-160, -688) to it.

The footpath is the dashed trail on the field-book map, and it shows on the ground: `lake_features.py` clears
the scatter 2.4 m either side (a 2 m track and its verges), and `bake_ground.py` wears a bare track about
2 m wide (grass cut, straw-short verges) and lifts the canopy mask along it, so the trodden soil shows
through the tree belt's leaf litter north of the farm instead of disappearing under it.

It writes the heightfield, PNG and work npy, and "lake" in `estate_layout.json` (shore, level, landing,
path, pathProfile, graded). Once "graded" is set it only reapplies the scenery: to regrade, restore the
heightfield and remove "lake". `lake_features.py` clears the scatter from the water, its lip and the path
and plants the margin (tussock grass, talus and boulders; fixed seed). `scatter.py` applies it after a
fresh scatter. Then, in the editor: `apply_estate_heightfield` over the rectangle it printed, save only the
proxies it touched, run `place_water.py` (`EstateLake`, an `AHomesteadWaterPool` with `MI_EstatePond`, not
spatially loaded), `bake_ground.py` + `build_ground.py`, and `bake_estate_map.py` + `ImportEstateMap`.

The pool's shoreline is a closed spline at the water level with scale Y 0: the controller's water probe
treats its inside as in the water and the pail aims 25 cm inside it. An invisible pawn-only wall 2.2 m in
from the shore (about knee deep) keeps her out of the deep water.
### River channel (`river_channel.py`)

reshape.py's cut only lowers ground, so where the river runs along a valley side the downhill bank
was missing and the water floated over grass. `python Scripts\Terrain\river_channel.py` grades the
channel to a designed section along the layout's river: a flat bed, banks rising 0.6 m per metre
to 0.25-0.5 m above the water, and a raised bank (a 0.8 m berm falling back at 0.5 per metre)
wherever the ground falls away. The stream grows from a 1.2 m spring rill to a 3.8 m river over its
first 300 m. It rises in a 3 m pool cut into the head of its valley, fords the drive between low,
gentle banks, and crosses the cove beach 0.3 m below the sand into the sea.

It rewrites `EstateHeightfield.r16` (the source of truth), `Estate_Heightmap_4033.png` and the work
npy, and adds `riverChannel`, `riverSurface` (m), `riverHalfWidth` (the waterline, m) and `riverEnd`
to `estate_layout.json`. Once they're stored it reuses them, so re-running it changes nothing;
reshape.py runs it last on fresh terrain. Afterwards, with the Estate level loaded and PIE stopped:

1. `unreal.HomesteadEstateAuthoringLibrary.apply_estate_heightfield(<absolute r16 path>, MinX, MinY,
   MaxX, MaxY, 63, True)` is a dry run over the vertex rectangle it printed (column = X, row = Y). Then
   pass `False` to write the tiles that differ into the Landscape's base edit layer.
2. Save only the landscape proxies over the changed vertices. Loading the region leaves every
   other proxy dirty with no real change, and saving those would rewrite 230 files. Find them by
   their actor bounds.
3. Run `place_water.py`, then `bake_ground.py` and `build_ground.py`. Don't re-run `scatter.py` for a local edit: a change in which cells pass its tests shifts its random draws, and about 3,900 cobbles and boulders move across the whole map. Scenery stores only X and Y (Z comes from the heightfield at runtime), so the committed scatter sits correctly on the regraded ground. Drop only the records that now fall in the water (this regrade dropped 10 plants within 30 cm of the waterline).

The Landscape matches the r16 everywhere: a full-map dry run reports no differences, and the rendered
heights (`editor_ground_height`) agree within 1.1 cm along the map edge and 0.15 cm at 4,000 random
interior vertices. The tool skips the outermost row and column, because the landscape data
interface misreads them there (GetData and GetDataFast both return a neighbour's value).

The ribbon (`AHomesteadWaterRibbon`) reads spline scale Y as the waterline half-width and runs
`BankOverlap` (45 cm) under each bank. It rounds off round the pool (`StartCap`, one pool radius,
with the spline starting that far upstream of the pool centre) and tapers over `EndCap` (6 m) into
the sea. Vertex colour B makes white water on grades over about 4 % and at the spring
(`M_EstateRiver`, the creek graph plus a white-water layer, through `MI_EstateRiver`: calmer ripples (RippleScale 0.12), Specular 0.9, Roughness 0.02, so at a grazing angle it reflects the banks and sky instead of reading as a streaked glass slab). V restarts every 10 m, because
half-precision UVs smear the ripples into stripes at 1 km.

Cost, from `ProfileGPU` in PIE at a 3054×1135 viewport with the sea filling the view from the western clifftop: `SLW::Draw` 0.23 ms, depth prepass 0.09 ms, and Lumen water reflections about 0.2 ms. The old one-plane creek-material sea cost 0.10, 0.07 and 0.2 ms. The mesh has about 89k vertices. Read the stable sub-passes: frame times and reflection spikes swing widely when other editors share the GPU.

The tuning parameters are on `MI_EstateOcean`, grouped Waves, Foam, Colour and Data. Bake the
values you settle on into the defaults in `build_ocean.py`.


The mouth is cut once (`cut_mouth`, recorded as `riverMouth` in the layout). Where the beach is lower than 0.5 m, the bed goes to at least -0.3 m, so the sea runs up into the channel. The stream carries on to the first point whose bed is 0.6 m under the sea. Its surface there is held 3 cm under the sea, so the river ribbon slides beneath the ocean instead of stopping on dry sand or fighting the ocean surface. Afterwards: `bake_ocean.py` + `build_ocean.py` (the shore texture sees the channel), `place_water.py`, `bake_ground.py`, `bake_estate_map.py`.

### Cove route (`cove_route.py`)

The on-foot way from the manor's south front door down to the sand at the head of the cove
(`openspec/changes/add-cove-route`): 509 m and 83 m of fall, a graded path at 1 in 7 or gentler through the
meadow's three switchbacks, then 191 granite steps in 18 flights down the valley side, and paths along the
bench and valley floor to the sand. `CONTROL` in the script holds its turning points and the kind of way
leaving each ("path" or "stairs"); `--search` re-runs the least-cost search they came from, and `--dry`
designs and writes only `Saved/CoveRoute/` (plan and profile, leg table).

```powershell
python Scripts\Terrain\cove_route.py            # grade once; then re-emits the route data
python Scripts\Terrain\bake_ground.py           # wears the path, no grass on the steps
python Scripts\Map\bake_estate_map.py           # the dashed footpath
```

It grades the heightfield once (`coveRoute.graded` in the layout): a path's bed over 1.2 m either side,
5 cm under every tread and landing over 2.2 m, blending back over 2.5 m, never into the river channel.
The run prints the `ApplyEstateHeightfield` rectangle (rows 1338-1541, columns 1457-1755). The full
design lives in `cove_route.json` (the layout keeps a summary and the map's `footpaths`), and
`Simulation\HomesteadEstateCoveRoute.inc` carries the centreline, flights, landings, kerbs, rail bays and
fingerposts at Props' kit pivots (`add-cove-route-kit`). A later run leaves the heightfield alone and
re-emits the data; to move the route, restore the heightfield from before it and remove `coveRoute`.
`scatter.py` clears a fresh scatter off it after the lake. Set `HOMESTEAD_TERRAIN_WORK` to your work folder
(the script keeps its `game_reshaped_4033.npy` in step round the route).

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

`build_ground.py` imports `Saved\Ground\` PNGs from its own checkout. Always run
`bake_ground.py` in that same checkout immediately before importing; otherwise it can import a
different worktree's stale ground bake. Then, in the editor with PIE stopped, run
`pyfile Scripts\Terrain\build_ground.py` and then
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
world tiling, easing into a 12.9 m copy past 60 m. Glades take the same floor, as the prototype's openings did. `ZoneTint` on `M_EstateLandscape` (0.93, 1.02, 1.12) cools the zone's floor toward the prototype's grey-green; the rest of the estate is untouched. Tree canopy kinds
and crown radii come from `SCENERY_TREES` in `Scripts/Map/bake_estate_map.py`, with a fallback in
`bake_ground.py` for kinds 13-16 and 19-22 until it lists them. Order after the woodland and trees
branches land: `scatter.py`, then `bake_ground.py` and `build_ground.py`.

Weather and night: `build_ground.py` also makes `MPC_EstateGround` (Wetness, Daylight), which
`AHomesteadWorld::UpdateLighting` sets every refresh. The ground wets through over the first half hour
of the rain and dries over four hours after it. Wet soil and stone darken and gloss (leaf litter under trees and the MVP woodland floor darken but stay matt, with no puddles, or they read as a sheet of water beside the river; turf
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
