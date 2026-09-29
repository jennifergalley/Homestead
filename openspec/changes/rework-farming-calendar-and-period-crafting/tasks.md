# Tasks

## 1. Calendar and hunger: lane A, which lands first

- [ ] 1.1 Add `Homestead::Calendar`: day index, day of season, season, year, weekday and days left, derived from `hour` with 28-day seasons. Route `SeasonName`, `DayNumber`, the save label and rain days through it. Expose a season-rollover hook. Add native tests for the boundaries, including Winter 28 to Spring 1 of the next year
- [ ] 1.2 Default `dayMinutes` to 30 for new games and keep the 30/60/120 setting. Show "Mon, Spring 12" in the HUD calendar, plus "N days left" in a season's last three days. Verify at 720p and 4K
- [ ] 1.3 Replace hunger failure with the Fed, Hungry and Famished penalties. `WorkCost` scales every work cost, sleep recovery follows the hunger state, and the threshold toasts fire once each. Add native tests covering days at 0 hunger without failure
- [ ] 1.4 Add crop season masks, the out-of-season planting refusal, the too-late warning on planting and on the focus line, and withering at the season rollover with hoe clearing. Add native tests for the Spring-to-Summer potatoes-versus-carrots case
- [ ] 1.5 Package, then play the first demonstration: buy and plant spring seed, advance to the end of Spring, harvest, and watch an out-of-season plot wither. Check that hunger only slows her. Capture in-game views, commit and push

## 2. New crops: lane B

- [ ] 2.1 Add the peas, wheat, barley, leeks and winter broccoli rows and catalogue rows, using the seasons, days, regrow and harvest style from the design table
- [ ] 2.2 Author the five Blender plant sets (Sprout, Young, Growing, Mature and Ripe, with LODs and ripening produce) and the four withered silhouette sets. Import them and verify each stage and the withered state from the gameplay camera
- [ ] 2.3 Verify planting, growth, harvest and sale of each new crop with `HomesteadGrowCrops`

## 3. Seedsman: lane C

- [ ] 3.1 Add the `Seedsman` shop record: in-season seed filtered daily, the tin watering can, grain buying, and Sunday closing for both shops. Move seed out of the general store. Add native tests
- [ ] 3.2 Add the tin watering can: twice the pail's water, the same fill and pour behaviour. Verify it with the hotbar
- [ ] 3.3 Build Tregear's shopfront, interior (seed drawers, grain sacks, scale, counter), `SeedsmanDoor` anchor and stand-in keeper with greetings. Check the closed-door notices in ordinary play

## 4. Seasonal forage and look: lane D

- [ ] 4.1 Add the forage season table, blackberry picking on the 42 brambles from Summer 15 to Autumn 28, and the `Blackberries` item. Add the field mushroom kind, prop and about 40 placements, available in Autumn
- [ ] 4.2 Drive `MPC_Season` from `AHomesteadWorld`, and tint the landscape grass, PCG grass, deciduous leaves and bracken. Add the winter bare canopy and morning frost, blended over the first three days of each season
- [ ] 4.3 Capture in-game views of the same woodland and meadow in all four seasons. The Performance agent compares winter and summer frame times

## 5. Period crafting: lane E

- [ ] 5.1 Add the Workbench and Sawhorse pieces and recipes, and the saw-planks action with a labelled stand-in motion until the saw stroke lands. Add native tests for the station requirement
- [ ] 5.2 Add the FenceRail and FenceGate pieces: snapping runs, 15° rotation, the owned-land check, gate open and close state saved, and collision. Verify fencing a garden plot in ordinary play
- [ ] 5.3 Add the stool, table, chair and shelf, with placement on foundations including the standing room
- [ ] 5.4 Add the four hearth dishes with tiered restores, and the Craft categories with mouse and controller navigation

## 6. Integration and acceptance

- [ ] 6.1 Do the single `SimulationSaveVersion` bump with the reset notice at final integration. Retarget the packaged FullLoop and Smoke suites to the seasonal year
- [ ] 6.2 Run the full-acceptance route in a packaged build, one continuous run. Harvest one crop per season, pick blackberries, find mushrooms, sell grain to Tregear's, fence a plot, furnish the room, cook each dish, and find both shops closed on a Sunday. Check mouse and controller parity
- [ ] 6.3 Jenny playtests. Confirm the day length (30 or 60), Sunday closing, and the shop and keeper names, and fold in her feedback
