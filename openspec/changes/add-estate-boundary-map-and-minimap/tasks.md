# Tasks

## 1. Parcels

- [x] 1.1 Add Simulation parcels from the landmark polygons, with point-in-parcel queries, ownership save fields and a new-game default. Add portable tests for polygon edge cases and save/reload
- [x] 1.2 Make build placement call `CanBuildAt` and reject placements outside the estate with an explicit reason. Verify that nothing is consumed and that foraging outside still works

## 2. First playable minimap (first delivery)

- [x] 2.1 Add the capture-and-stylise bake (`Scripts\Map\bake_estate_map.py` + `Homestead.ImportEstateMap`), and bake `T_EstateMap` plus its transform from the first Estate terrain (the LIDAR heightmap). The orthographic-capture fold-in (`--capture`) waits for the level's water and dressing
- [ ] 2.2 Add the `SHomesteadMinimap` overlay: a north-up crop, the player arrow, the dashed owned boundary and landmark glyphs, a protected HUD region, and hiding with the HUD. Verify it at 720p and 4K (verified in PIE on the Estate map at 4K and in the 720p editor -game Hotbar route; packaged 720p/4K check waits on the integration build)
- [ ] 2.3 Add the boundary-crossing toast with hysteresis, using the estate name. Package, walk off the estate along the road, capture in-game views, commit and push (built; in PIE one "Leaving"/"Entering the Trevennor estate" toast per crossing at the north edge and the road gateway, none while jittering ±1.5 m on the line; packaged walk pending)

## 3. Map tab and settings

- [x] 3.1 Add the field-book Map tab: fitted texture, parcel shading, named landmarks, a you-are-here marker, mouse pan and zoom, and controller stick pan and zoom with landmark focus stepping
- [x] 3.2 Add the "Minimap rotates with camera" setting with an N marker. Verify both modes in ordinary play
- [ ] 3.3 Run the native menu navigation tests with the new tab, plus mouse and controller parity checks, and verify no input leaks into the world (the editor -game NativeMenu route passes at 720p with the five Map steps, including no simulation change and LB/RB; real-mouse wheel and drag verified in PIE; packaged run waits on the integration build. Two 4K editor -game runs failed early on unrelated steps under heavy machine load)

## 4. Acceptance

- [ ] 4.1 Jenny playtests orientation along the estate-to-town route. Tune the minimap size, zoom and boundary line from her feedback
