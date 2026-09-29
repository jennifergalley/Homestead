# Tasks

## 1. Simulation

- [x] 1.1 `HomesteadCrops.{h,cpp}`: the crop table (seed, produce, bonus, grow and regrow hours, harvest style, mesh stem), day helpers, growth modifiers, stages and status text
- [x] 1.2 Plant, HarvestCrop and the growth tick read the table; planting and harvest messages state days
- [x] 1.3 Seed and produce Items for the six period crops, their CropKinds and table rows, and the store listing (after the v13 count-prefixed stock save is on main)
- [x] 1.4 Native tests: growing days, watered/dry/weedy rates, stages, focus text, regrow, yields, store listing, save round trip of the new kinds

## 2. Plants

- [x] 2.1 Blender recipes `crop_turnip`, `crop_carrot`, `crop_potato`, `crop_cabbage`, `crop_broad_bean` and `crop_strawberry`: five stages each, with LODs
- [x] 2.2 Import them to `Environment/Props/Crop*`

## 3. Game

- [x] 3.1 Plot visuals: stage meshes per crop and dry and wet soil at the 0.4 threshold
- [x] 3.2 Focus line from `PlotStatus`, with Harvest, Water and Weed actions
- [x] 3.3 Harvest animations (pull and pick) with the produce in her hand, hidden afterwards, and a lab command
- [x] 3.4 PIE on the Estate: till, sow, water, sleep through each stage, harvest; screenshots of every stage

## 4. Visible produce (Jenny's playtest)

- [x] 4.1 Stage meshes without produce; one SM_Crop<Name>_Produce per crop with per-stage anchors in the report
- [x] 4.2 crop_produce_anchors.py generates HomesteadCropProduceAnchors.inc; AddCropProduce draws produce per plot on one instanced mesh, sized and raised by growth
- [x] 4.3 M_CropProduce: per-instance ripeness tints produce from pale green to full colour
- [x] 4.4 Drop the ripe glint (Jenny): ripeness shows by the produce alone
- [x] 4.5 HomesteadGrowCrops <days> [tend] playtest exec (Simulation::PassDaysForPlaytest) with a native test
- [x] 4.6 Keyboard X does the secondary action outside build planning, matching controller X in the prompt
- [x] 4.7 PIE: every crop at 25/50/75/100% at the gameplay camera, in daylight, rain and dusk
