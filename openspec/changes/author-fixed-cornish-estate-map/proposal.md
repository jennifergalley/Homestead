# Proposal

## Why

The pivot needs a fixed, hand-authored world (`pivot-to-cozy-estate-life-sim`,
`estate-life-sim-direction`). Today the play path is a seeded, procedural, chunk-streamed
woodland built with `ProceduralMeshComponent` terrain and `Homestead::Generation`. It can't
hold an authored estate, a coast, a town or a road, and every other round-1 change needs a
real place to integrate into. This change is the hub lane of round 1.

## What Changes

- **Author a fixed ~4 × 4 km Cornish map** as an Unreal Landscape in a new World Partition
  level `/Game/SurvivalGame/Maps/Estate`, which becomes the default game map.
- **Base the terrain on real elevation data.** Use the Environment Agency LIDAR Composite DTM
  (1 m; Open Government Licence v3.0; direct download, no account), centred on the St Agnes,
  Trevaunance Coombe and Chapel Porth coast. Its wooded coombe to a cove, its cliffs and its
  clifftop engine-house country are the closest real analogue to the agreed estate.
- **Reshape the terrain to the agreed layout**, using real Perranporth/Perran Sands forms as
  the reference for the dunes:
  - The estate on a south-facing slope down to its own cove and cliffs.
  - A clifftop mine site.
  - A small river through a wooded valley to the cove.
  - A dirt road about 1.5 km to an invented estuary-head town with a harbour.
  - Moorland and tors to the north.
  - Dunes and beaches along the far coast.

  No single real 4 km box holds all of these, so the result is a designed composite. We don't
  mosaic unrelated tiles.
- **Water**:
  - Ocean, the estate river and one lake use the Water plugin. The plugin is first proven on a
    cut-down copy of the real heightmap.
  - The fallback is hand-placed meshes with the Single Layer Water material, which the creek
    already uses.
  - The estuary is ocean plus a tidal inlet shape. There's no tide simulation.
- **Biomes**:
  - Landscape paint layers for woodland floor, pasture and plains, heather moorland, dune
    sand, beach, cliff rock, dirt road and farmland soil.
  - Core PCG graphs (our own, not the experimental Biome sample plugins) scatter the already
    admitted tree, understory, rock and wildflower palette from `upgrade-woodland-environment-assets`
    and the Blender library.
- **Dirt road** as a Landscape spline that deforms the terrain and carries rut and verge
  spline meshes. It runs from the estate gateway to the town edge.
- **Named landmark anchors** in one authored data asset, which the other round-1 lanes
  read: manor footprint, standing room spawn, mine entrance, mill site, cove, estate gateway,
  road ends, town square, and general-store door.
- **Authored resource placement with stable IDs.** Trees, rocks and forage come from
  editor-time PCG, baked into a versioned placement table with stable IDs. At a new game the
  Simulation seeds its resources from that table, and edits keep persisting by stable ID, as
  they do today.
- **Retire the procedural chunk path from play.** `AHomesteadWorld` gains a fixed-world mode:
  the Landscape provides the ground, heights come from Landscape traces, and resources come
  from the placement table. The chunk generation code stays in the tree, unused by the
  default map, until a later cleanup removes it.
- **Far view** through World Partition HLODs. This takes over the view-distance part of
  `polish-locomotion-view-distance-and-time-hud`.
- **BREAKING:** new games start on the fixed map. Existing woodland test saves are
  incompatible. They're reset with a plain notice, and unrelated data is left alone.

## Reuse research

See `docs\research\estate-pivot\first-slice-reuse.md` for the full report and sources.

- **Terrain data:** EA LIDAR Composite DTM 1 m (GeoTIFF, OGL v3.0, no account). OS Terrain 50
  covers far-field context only. OS Terrain 5 isn't fully open, so we don't use it. LIDAR
  stops at the tideline, so the seabed near the cliffs is hand-sculpted. EMODnet bathymetry is
  optional reference only, until its licence is verified.
- **Pipeline:** QGIS/GDAL (`gdalwarp` to EPSG:27700 → `gdal_translate -ot UInt16 -scale` to a
  4033×4033 16-bit PNG), then Landscape "Import from File". 4033² gives 63-quad sections with
  2×2 per component and 32×32 = 1024 components, at Epic's recommended maximum. Verify it
  against the 5.8 Landscape Technical Guide at import.
- **Unreal facilities:** Landscape plus World Partition streaming proxies and HLOD
  (production). Landscape splines (mature). Core PCG graphs (production). The Water plugin's
  status in 5.8 is unclear, and there are community reports of editor slowdowns with rivers
  on large landscapes, so it gets proven early with the Single Layer Water fallback.
- **Assets:** admitted woodland trees, FirSapling and understory; the Blender granite rocks,
  blackberry bramble and shrubs; the wildflower groundcover. There's no new asset purchase.
- **Custom gaps:** the reshaped composite terrain, the paint-layer landscape material,
  biome PCG graphs, the placement bake, and the fixed-world mode in `AHomesteadWorld`.

**Attribution** in `docs\asset-credits.md` and the in-game credits: "Contains Environment
Agency information © Environment Agency and/or database right 2026, licensed under the Open
Government Licence v3.0." If OS Terrain 50 is used, add "Contains OS data © Crown copyright
and database right 2026". Verify the wording against the live licence pages at import.

## Smallest useful result and first playable demonstration

**Smallest result:** start the packaged game and walk from the standing-room spawn, down
through the estate to the cove, along the dirt road and into the town-edge blockout, on the
real-terrain Landscape. The sea, the river and the first woodland dressing are visible.
Hotbar felling still works on placed trees.

**Full acceptance** adds all biome layers and PCG dressing, the lake, the dunes and
moorland, HLOD far views, save/load of edits on the fixed map, and 60 FPS at the target
settings on Jenny's PC.

**Deferred:** fine dressing of every area, detailed town art (that's
`add-dollars-and-general-store` plus later rounds), the mine interior (round 7), the mill
(round 9), tides, swimming and boats (round 13).

## Capabilities

### New Capabilities

- `fixed-estate-world`: The authored map, its terrain provenance, water, biomes, road,
  landmark anchors, stable authored resource placement and fixed-world play path.

### Modified Capabilities

None. No main specs exist yet.

## Impact

- New level `Content\SurvivalGame\Maps\Estate` (World Partition), landscape material, PCG
  graphs, Water bodies, road spline, and a landmark and placement data asset.
- `Config\DefaultEngine.ini`: `GameDefaultMap` and `EditorStartupMap` point to Estate. The
  `SurvivalGame.uproject` plugins gain Water, Landmass if needed, and PCG.
- `AHomesteadWorld`: a fixed-world mode that uses Landscape ground and trace heights and
  seeds resources from the placement table. Its creek code becomes the fallback water path.
- `Homestead::Simulation`: new-game resource seeding from the placement table. The save
  version bumps. Resource edits stay keyed by stable ID.
- Scripts: `Scripts\Terrain\` holds the GDAL/QGIS preparation script and notes. Raw LIDAR
  tiles stay out of git, but the derived 16-bit heightmap is committed.
- Docs: `docs\asset-credits.md` gets the OGL attribution, `docs\setup.md` gets terrain
  regeneration steps, and the playtest docs cover the new start location.
- Tests: the generated-world tests are retargeted or retired along with the path they cover.
  New fixed-world smoke and placement-persistence checks are added.
