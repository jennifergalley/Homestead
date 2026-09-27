# Tasks

## 1. Standing room (first delivery)

- [ ] 1.1 Add the Hearth piece (Fire cooking, a light, ambience) and the `StoneWall` skin. Seed the heritage standing-room building at `StandingRoomSpawn` with its bed, hearth and chest (pail and branches)
- [ ] 1.2 Spawn a new game in the room facing the door. Verify sleep and save, chest access and hearth cooking on the Estate map with mouse and controller
- [ ] 1.3 Author the coarse ruin massing (wall runs, chimney stack, rubble) as Blender recipes and place it on `ManorFootprint`. Reserve the footprint in `CanBuildAt`
- [ ] 1.4 Place the salvage piles, with the billhook head near the door, together with the clearing lane. Package, walk out of the room into the ruin, capture in-game views, commit and push

## 2. Names and arrival

- [ ] 2.1 Add the Names step after Appearance: three fields with defaults, validation, and a virtual keyboard for the controller, with a letter-grid fallback if needed. Store the names in Simulation and the save header
- [ ] 2.2 Show "{First} {Surname} — {Estate}, Spring 1" in the save list. Pass the estate name to the boundary toast and the HUD
- [ ] 2.3 Add the arrival title card and the Journal list with the arrival note

## 3. Ruin detail and acceptance

- [ ] 3.1 Add the detail pass: window openings with lintels, quoins, fallen timbers, slate scatter, lichen and soot, and ivy and bramble cling. Check its silhouette from the road approach and the cove in ordinary play
- [ ] 3.2 Run the whole new-game flow with mouse and controller, name save/reload, footprint rejection, and 720p/4K checks
- [ ] 3.3 Jenny playtests the arrival. Collect her default names, the year and her ruin feedback, and fold them in
