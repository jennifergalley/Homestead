# Tasks

- [x] 1.1 `river_channel.py`: the designed channel, spring pool, ford, beach run; r16, PNG, npy, layout; run from reshape.py.
- [x] 1.2 `ApplyEstateHeightfield` editor function; patch the Landscape (29 tiles, 9 proxies).
- [x] 1.3 Ribbon: waterline scale, bank overlap, rounded caps, white water in vertex colour B, V restarting every 10 m.
- [x] 1.4 `M_EstateRiver` (the creek graph plus white water); `place_water.py` places the river at every layout point with the new surface and widths, not spatially loaded, and places the spring stones.
- [x] 1.5 Pail: aim inside the waterline, and step down the bank with her feet on the ground.
- [x] 1.6 Re-bake EstateGround (`bake_ground.py`) and re-import it (`build_ground.py`).
- [x] 1.7 Verify in PIE: from the manor, at the spring, along the banks, at the ford and the mouth, and a pail fill. _Captures in E:\CopilotScratch\89914e30-d8b6-4605-8635-5735406c97a2\captures\r2 (sheet.jpg). Pail: from 1.2 m back from the waterline she stepped 84 cm down the bank (27 cm lower) and the fill succeeded. The kneel itself wasn't captured._
- [x] 1.8 Placements and scenery: no interactable within 1 m of the new waterline. The committed scatter is kept; the 10 plants in the new water were dropped.
- [ ] 1.9 Re-bake and import the estate map (`bake_estate_map.py`, `Homestead.ImportEstateMap`) at the next editor pass. The channel is at most a couple of texels wide on the map.
- [x] 1.10 Playtest follow-up (Integration, integ-4pm): near the mouth in rain the wet woodland floor was a glossy sheet and the river a raised-looking slab. Litter and the MVP floor now stay matt when wet with no puddles, and `MI_EstateRiver` calms the ripples and brightens the reflection. New `HomesteadEmptyPail` exec (`Simulation::EmptyPail`, native-tested). _PIE, day 2 12:10 rain: E:\CopilotScratch\89914e30-d8b6-4605-8635-5735406c97a2\captures\r3 (m_tune.jpg, pail_kneel_s.jpg). Pail emptied, then filled from 1.2 m back: she stepped 84 cm down the bank and knelt with the pail at the waterline._
- [x] 1.11 River meets the sea (Jenny/Integration: the ribbon stopped a couple of metres short, with dry sand and the ocean's shore wash between). Implemented once in `river_channel.py` (`cut_mouth`, recorded as `riverMouth`):
  - It cuts the bed through the beach berm to at least -0.3 m wherever the beach is under 0.5 m, so the sea runs up into the channel.
  - It carries the stream on to the first point whose bed is 0.6 m under the sea (the end moves from point 318 to 319).
  - It holds the surface 3 cm under the sea there, so the ribbon slides beneath the ocean instead of z-fighting it.
  - 77 vertices changed; every raised cell stays at -0.74 m or lower. The ocean shore data, ground and map are re-baked.
- [ ] 1.12 Editor: `ApplyEstateHeightfield` (r16 rows 1503-1513, columns 1435-1447), `place_water.py` (the river ribbon), `build_ocean.py` (shore texture), `ImportEstateMap`. PIE at eye level from the beach and from the river bank: water continuous from the river into the sea, no sand strip, no z-fighting at the join; the pail still fills from the river.
