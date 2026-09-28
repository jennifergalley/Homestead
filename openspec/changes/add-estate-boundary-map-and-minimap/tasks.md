# Tasks

## 1. Parcels

- [x] 1.1 Add Simulation parcels from the landmark polygons, with point-in-parcel queries, ownership save fields and a new-game default. Add portable tests for polygon edge cases and save/reload
- [x] 1.2 Make build placement call `CanBuildAt` and reject placements outside the estate with an explicit reason. Verify that nothing is consumed and that foraging outside still works

## 2. First playable minimap (first delivery)

- [x] 2.1 Add the capture-and-stylise bake (`Scripts\Map\bake_estate_map.py` + `Homestead.ImportEstateMap`), and bake `T_EstateMap` plus its transform from the first Estate terrain (the LIDAR heightmap). Re-baked with the orthographic capture fold-in (`Homestead.CaptureEstateMap` → `--capture`) on main 3b8172ba with the ocean v2 in the level. The Estate's woods are runtime HISMs from `EstateScenery.bin`, which an editor-world capture never sees, so the bake now reads that scatter directly and stamps each tree and hazel as an inked crown over a woodland wash (re-baked on b07d4a82 and checked in PIE in the minimap and the Map tab). Re-run the bake whenever `scatter.py` or the terrain changes
- [x] 2.2 Add the `SHomesteadMinimap` overlay: a north-up crop, the player arrow, the dashed owned boundary and landmark glyphs, a protected HUD region, and hiding with the HUD. Verify it at 720p and 4K (verified in PIE on the Estate map at 4K, and in the packaged integration build f2e504c5 on the Estate map at 720p windowed and 4K fullscreen; hidden during new-game setup, Names and the shop from 9a7d6e16; the bottom-right placement re-checked in the packaged b07d4a82 build at 720p and 4K, with the rim, N marker and hotbar clear, and the interact cue sliding left of the minimap verified in PIE)
- [x] 2.3 Add the boundary-crossing toast with hysteresis, using the estate name. Package, walk off the estate along the road, capture in-game views, commit and push (in PIE one "Leaving"/"Entering" toast per crossing at the north edge and the road gateway, none while jittering ±1.5 m on the line; from 9a7d6e16 it uses the name chosen in the Names step, verified in PIE as "Leaving/Entering the Penrose estate". Packaged walk-off on bbab1de7 at 720p: walking out along the road past the gateway shows "Leaving the Trevennor estate" about 10 m past the line, and walking back shows "Entering the Trevennor estate" once. That walk also exposed a packaged-only terrain collision fault just past the gateway, where she sinks under the landscape and is recovered repeatedly; PIE walks the same route cleanly. It was reported to the world lane)

## 3. Map tab and settings

- [x] 3.1 Add the field-book Map tab: fitted texture, parcel shading, named landmarks, a you-are-here marker, mouse pan and zoom, and controller stick pan and zoom with landmark focus stepping
- [x] 3.2 Add the "Minimap rotates with camera" setting with an N marker. Verify both modes in ordinary play
- [x] 3.3 Run the native menu navigation tests with the new tab, plus mouse and controller parity checks, and verify no input leaks into the world (the editor -game NativeMenu route passes at 720p with the five Map steps, including no simulation change and LB/RB; real-mouse wheel and drag verified in PIE; the Map tab renders correctly in the packaged build f2e504c5 at 720p and 4K, opened with M. The packaged NativeMenu suite on bbab1de7 passes all steps, including the Map steps 92-97. Two 4K editor -game runs failed early on unrelated steps under heavy machine load)

## 4. Acceptance

- [ ] 4.1 Jenny playtests orientation along the estate-to-town route. Tune the minimap size, zoom and boundary line from her feedback
