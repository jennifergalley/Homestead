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
