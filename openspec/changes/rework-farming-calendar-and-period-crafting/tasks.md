# Tasks

## 1. Calendar and hunger: lane A, which lands first

- [x] 1.1 Add `Homestead::Calendar`: day index, day of season, season, year, weekday and days left, derived from `hour` with 28-day seasons. Route `SeasonName`, `DayNumber`, the save label and rain days through it. Expose a season-rollover hook. Add native tests for the boundaries, including Winter 28 to Spring 1 of the next year
- [ ] 1.2 Default `dayMinutes` to 30 for new games and keep the 30/60/120 setting. Show "Mon, Spring 12" in the HUD calendar, plus "N days left" in a season's last three days. Verify at 720p and 4K
- [x] 1.3 Replace hunger failure with the Fed, Hungry and Famished penalties. `WorkCost` scales every work cost, sleep recovery follows the hunger state, and the threshold toasts fire once each. Add native tests covering days at 0 hunger without failure
- [x] 1.4 Add crop season masks, the out-of-season planting refusal, the too-late warning on planting and on the focus line, and withering at the season rollover with hoe clearing. Add native tests for the Spring-to-Summer potatoes-versus-carrots case
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
- [ ] 5.4 Add the four hearth dishes with the §3a energy values (all Meals, flat 3-hour Well fed), and the Craft categories with mouse and controller navigation

## 6. Energy, food and a kinder start: lane F, the Calendar Agent's follow-up after lane A integrates

Jenny decided on 2026-09-29 to replace task 1.3's interim Hungry/Famished work (design §3a and §3b). Start only after lane A (a3c7e04d or later) is on `main`.

- [x] 6.1 Remove hunger on the estate. Freeze `hunger` at 100 (at new game and on load, with no drain), make `GetHungerState()` always return Fed (done by removing `HungerState` and `GetHungerState()` outright, since nothing else read them; `Hunger::` keeps only the woodland drain rates), and remove a3c7e04d's Hungry/Famished factors and toasts. Drop the sleep hunger gate (`HomesteadController.cpp` around line 3495). Keep the woodland's legacy hunger. Keep `State::hunger` serialized. Native test: ten estate days without food bring no toast, no penalty and no failure
- [x] 6.2 (Props, `jennifergalley-energy-wellfed`: `Simulation/HomesteadFood.*`, `ItemInfo::food`, `Simulation::IsWellFed`; tests in `HomesteadCalendarTests.cpp`) Add the catalogue food class (Snack or Meal) and the final §3a table. On the estate `IsEdible` reads the class. Add Well fed: an absolute-hour `wellFedUntilHour`, a ×0.85 `WorkCost` factor, a flat 3 game hours for every Meal, and a refresh rule of `hour + 3` that never stacks. Eating rules: below full energy a Snack or Meal always restores its energy. At full energy a Snack is refused, and a Meal is eaten only if it starts Well fed or extends it by at least 1 hour, with the "energy was already full" toast; otherwise it's refused. Nothing is consumed on a refusal. Save the timer in an optional trailing `wellfed` section, written only while active, and no version bump. On load, an expiry at or before `hour` has expired and loads as not Well fed. A non-finite expiry, or one later than `hour + 3`, rejects the load explicitly with `ResultCode::CorruptSave` and is never clamped. Native tests:
  - a meal against a snack, and the 3-hour duration;
  - a second meal below full energy restores energy and resets the expiry to 3 hours from now;
  - at full energy: a snack is refused and kept; a meal that starts Well fed is eaten; a meal extending it by under 1 hour is refused and kept;
  - a pasty at 23:00 is still active at 01:30 and has expired by 02:00, and expiry works across sleep;
  - the ×0.85 cost;
  - save/load round-trip while Well fed; a missing section, or an expiry at or before `hour`, loads as not Well fed; a non-finite expiry, or one later than `hour + 3`, fails with `CorruptSave` and leaves the current game unchanged.
- [ ] 6.3 (Menu) The simulation already returns the toasts ("+12 Energy", "+40 Energy · Well fed until 2:30 PM", "Your energy was already full. Well fed until …"), and hotbar quick-eat shows the Meal toast. What remains is the HUD: hide the Food icon on the estate, and add the Well fed icon showing "until 2:30 PM" (clock time). Change the eat toasts and food descriptions to "+40 Energy · Well fed until 2:30 PM" or "+12 Energy". Verify at 720p and 4K with mouse and controller, including hotbar quick-eat
- [ ] 6.4 Starter chest (new games only): seed 3 Cornish pasties, 2 loaves of bread, and one of every finished outfit piece (every wearable with a garment mesh, currently the linen shirt, linen long shirt, trousers, fur coat, fur boots, woven sandals and turn shoes) in the standing-room chest via `Manor::SeedStandingRoom`. Skip any piece she already owns, never seed through `SeedStandingRoomAt` or on load, and use the normal capacity check. Native tests: the new-game chest contents, nothing duplicated after save/load or a second standing-room seed, and the worn tunic not duplicated
- [ ] 6.5 Hoe signposting: salvage `order[]` becomes billhook, hoe, axe, scythe, pickaxe; the no-hoe till refusal suggests the salvage; the arrival journal gets the west-rooms line. Native tests: the second search yields the hoe blade, and the refusal text
- [ ] 6.6 Package. Measure the real duration of one game hour at the default day length with a stopwatch against the HUD clock, and record it in this task. Then from a new game: eat a chest pasty and see Well fed, check there's no hunger icon, find the hoe blade in the second salvage pile, and till. Capture in-game views

## 7. Integration and acceptance

- [ ] 7.1 Do the single `SimulationSaveVersion` bump with the reset notice at final integration. It also drops the unused `hunger` field. Retarget the packaged FullLoop and Smoke suites to the seasonal year and the one-meter model
- [ ] 7.2 Run the full-acceptance route in a packaged build, one continuous run. Harvest one crop per season, pick blackberries, find mushrooms, sell grain to Tregear's, fence a plot, furnish the room, cook and eat each dish (checking Well fed), and find both shops closed on a Sunday. Check mouse and controller parity
- [ ] 7.3 Jenny playtests. Confirm the adopted 60-minute default day feel, Sunday closing, the shop and keeper names, and the energy and flat 3-hour Well fed values, and fold in her feedback
