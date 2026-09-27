# Tasks

## 1. Terrain source and import

- [x] 1.1 Download EA LIDAR Composite DTM 1 m tiles for the St Agnes, Trevaunance and Chapel Porth coast. Record tile IDs, URLs and verified OGL attribution wording. Keep raw tiles out of git
- [x] 1.2 Clip, reshape and export a 4033² 16-bit heightmap via `Scripts\Terrain`, a QGIS/GDAL script plus a README with the exact commands and Z scale. Verify the size against the 5.8 Landscape guide. _Done with numpy/scipy/rasterio scripts rather than QGIS; see `Scripts/Terrain/README.md`._
- [ ] 1.3 Create the World Partition `Estate` level, import the Landscape, and set it as the game and editor default map. Verify walkable slopes, the cove and cliffs in ordinary play. _Level and Landscape exist (257 WP proxies, 7 paint layers); the default-map switch waits for integration and the walk is unverified._

## 2. First playable walk (first delivery)

- [ ] 2.1 Publish `DA_EstateLandmarks` v1 with rough anchors for the standing room, gateway, cove, mine, mill, both road ends, the town square and the store door. Share the anchor names with the other lanes. _Interim: the v2 layout lives in `ProvisionalEstateLayout()` and `Scripts/Terrain/estate_layout.json`; lanes are using it. The data asset is still to do._
- [ ] 2.2 Add the fixed-world mode to `AHomesteadWorld`: Landscape ground, trace heights and no chunk streaming. Verify spawning, movement, camera collision and the hotbar on the new map. _Mode landed; spawn in the standing room verified in PIE, movement and camera still to verify._
- [ ] 2.3 Prove the ocean and estate river on a cut-down heightmap with the Water plugin, and record editor and runtime cost. Choose Water or the Single Layer Water fallback and implement the ocean, river and pail refill. _Chose the Single Layer Water fallback without a plugin trial: the plugin is experimental in 5.8, its Landmass brushes would regenerate the authored heightmap, and concurrent agent editors have already reset the GPU driver once. The sea is a datum plane (`MI_EstateSea`); the river is an `AHomesteadWaterRibbon` spline (`Scripts/Terrain/place_water.py`) that the pail probe reads. Refill is prompted at the river in PIE; she cannot wade past 70 cm. Costs are not yet measured._
- [ ] 2.4 Lay the dirt-road Landscape spline with rut and verge meshes from the gateway to the town edge. Walk it end to end in the game
- [ ] 2.5 Package and playtest the walk from the standing room to the cove, then along the road to the town edge. Capture in-game views, commit and push

## 3. Biomes and authored resources

- [x] 3.1 Paint the WoodlandFloor, Pasture, Moorland, DuneSand, Beach, CliffRock, DirtRoad and Farmland layers in a landscape material with distance blending. _Seven layers derived by `Scripts/Terrain/weightmaps.py`; Farmland is deferred to round 2 tilling._
- [ ] 3.2 Author PCG graphs that scatter the admitted tree, understory, rock and wildflower palette by layer, slope and water distance. Keep decorative grass as runtime PCG or foliage. _Interim: a deterministic Python scatter (`Scripts/Terrain/scatter.py`) bakes ~79k decorative instances into `EstateScenery.bin`, drawn as runtime HISMs. PCG graphs are not authored._
- [ ] 3.3 Bake interactive trees, rocks and forage into a versioned `DA_EstatePlacements` with stable IDs and minimum tiers. Seed new-game Simulation resources from it. _Interim: 352 world-lane placements (500100+) are baked into `HomesteadEstateWorldPlacements.inc`, bake version 2. No data asset yet._
- [ ] 3.4 Verify felling, forage and pickup on baked resources, and check that save/reload preserves edits by stable ID across two new games with an identical layout
- [ ] 3.5 Add the lake, dunes and beach, and the moorland and tors at first-pass quality

## 4. Integrated acceptance

- [ ] 4.1 Configure World Partition HLOD and foliage culling. Profile a packaged build on Jenny's PC at the target settings along the estate-to-town route, and record the frame times
- [ ] 4.2 Bump the save version with a plain incompatible-save reset notice. Retarget the smoke, full-loop, felling and watering tests to the fixed map, and retire the generated-world-only tests
- [ ] 4.3 Add the OGL attribution to `docs\asset-credits.md` and the in-game credits. Update `docs\setup.md` and the playtest docs
- [ ] 4.4 Jenny playtests the terrain and layout. Fold her reshaping feedback into the Landscape edit layers and the landmarks
