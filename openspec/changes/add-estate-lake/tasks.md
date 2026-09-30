# Tasks

- [x] 1.1 `lake_basin.py` and `lake_features.py`: basin, banks, landing, footpath, margin; heightfield, PNG, npy, layout, scenery.
- [x] 1.2 `AHomesteadWaterPool` (surface, wading limit) and the closed-shoreline water probe.
- [x] 1.3 `place_water.py`: `EstateLake` with `MI_EstatePond`; `bake_ground.py`, `bake_estate_map.py` and `scatter.py` know the lake.
- [x] 1.4 Patch the Landscape (ApplyEstateHeightfield: 6 tiles, 2 proxies) and save only the proxies over the lake; place the lake; build_ground; import the map.
- [x] 1.5 PIE, no teleports: walk from the farm along the path to the landing, fill the pail, try to wade in; eye-level views from the landing, the far shore and the farm; the map. _Walked from the standing room out through the ruin's north gap, round the farm, along the path to the landing (100 m of path, 12% at most); emptied the pail, filled it kneeling at the water; the wading wall stopped her about 36 cm deep, 18 m short of the centre; the field-book map has no "Dirt road" label. The pond's absorption was retuned from rust-orange to neutral. Morning and dusk views not captured. Captures: E:\CopilotScratch\89914e30-d8b6-4605-8635-5735406c97a2\lake\ (lake_sheet.jpg, landing_kneel_s.jpg, wade_tuned_s.jpg, map_book_ui.png)._
- [ ] 1.7 Visible farm-to-lake trail (Jenny, 2026-09-29: the dashed map trail was hidden under canopy litter): scenery cleared 2.4 m either side, a bare worn track with its canopy mask lifted, ground and map re-baked. Source done; needs `build_ground.py` + `ImportEstateMap` in the editor and an on-foot PIE walk from the farm to the landing.
- [ ] 1.6 Tell Jenny about the cardinal correction (up-left on her map is north-west).
