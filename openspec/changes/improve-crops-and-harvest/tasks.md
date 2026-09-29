# Tasks

## 1. Simulation

- [x] 1.1 `HomesteadCrops.{h,cpp}`: the crop table (seed, produce, bonus, grow and regrow hours, harvest style, mesh stem), day helpers, growth modifiers, stages and status text
- [x] 1.2 Plant, HarvestCrop and the growth tick read the table; planting and harvest messages state days
- [ ] 1.3 Seed and produce Items for the six period crops, their CropKinds and table rows, and the store listing (after the v13 count-prefixed stock save is on main)
- [ ] 1.4 Native tests: growing days, watered/dry/weedy rates, stages, focus text, regrow, yields, store listing, save round trip of the new kinds

## 2. Plants

- [ ] 2.1 Blender recipes `crop_turnip`, `crop_carrot`, `crop_potato`, `crop_cabbage`, `crop_broad_bean` and `crop_strawberry`: five stages each, with LODs
- [ ] 2.2 Import them to `Environment/Props/Crop*`

## 3. Game

- [ ] 3.1 Plot visuals: stage meshes per crop, a ripe glint, and dry and wet soil at the 0.4 threshold
- [ ] 3.2 Focus line from `PlotStatus`, with Harvest, Water and Weed actions
- [ ] 3.3 Harvest animations (pull and pick) with the produce in her hand, hidden afterwards, and a lab command
- [ ] 3.4 PIE on the Estate: till, sow, water, sleep through each stage, harvest; screenshots of every stage
