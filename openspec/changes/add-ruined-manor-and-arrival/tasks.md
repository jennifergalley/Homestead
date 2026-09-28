# Tasks

## 1. Standing room (first delivery)

- [x] 1.1 Merge the stone kit from `jennifergalley-stone-building-and-torch-assets` (d0a2a73c) and import StoneWall, StoneDoorway, StoneFoundation and StoneRoof. Add the Hearth piece (Fire cooking, a light, ambience) and the `StoneWall` skin using the kit. Seed the heritage standing-room building at `StandingRoomSpawn` with its bed, hearth and chest (pail and branches)
- [x] 1.2 Spawn a new game in the room facing the door. Verify sleep and save, chest access and hearth cooking on the Estate map with mouse and controller
  - Verified in PIE on the Estate with the controller: she spawns facing the door, sleeps and saves, the chest holds the pail and four branches, and she cooks Roasted roots at the hearth. The keyboard and mouse pass is also verified in PIE: Escape leaves Appearance, the Names rows are clicked and edited, Begin is clicked, E opens the chest (pail and four branches), E at the hearth then holding Enter makes Roasted roots, and E at the bedroll sleeps and writes the autosave. Under the roof the chase camera shortens to 300 cm; pinned against a wall, it lifts to an over-the-shoulder view.
- [x] 1.3 Author the coarse ruin massing (wall runs, chimney stack, rubble) as broken-top variants of the stone-kit recipes, and place it on `ManorFootprint`. Reserve the footprint in `CanBuildAt`
  - `AHomesteadManorRuin` is in the Estate level's `Manor` folder. The reservation is in `CheckSite`: previewing a foundation in the ruin shows "The old manor stands here."
- [x] 1.4 Place the salvage piles, with the billhook head near the door, together with the clearing lane. Package, walk out of the room into the ruin, capture in-game views, commit and push
  - Five piles (520001-520005) lie in and around the ruin. The first is in the south range, 2.5 m from the standing room's door. In PIE on the Estate she walks out of the door and searches it for a rusted billhook head and scrap iron; the pile is then spent. The piles still use the clearing lane's stand-in mesh (a small cobble heap), to be dressed with the 3.1 props. Packaging is the orchestrator's.

## 2. Names and arrival

- [x] 2.1 Add the Names step after Appearance: three fields with defaults, validation, and a virtual keyboard for the controller, with a letter-grid fallback if needed. Store the names in Simulation and the save header
  - Controller letter grid and validation are verified in PIE. With the mouse, clicking a row selects it. Backspace, Tab and Shift+Tab stay on that row, and typed characters go into it (the Slate inspector's key-char events, sent before the rows became non-focusable; key characters take the same focus path as Backspace). Empty fields are rejected in PIE. The prompt wording "Enter a surname." / "Enter an estate name." is covered by the native test. The clicked names reach the save label.
- [x] 2.2 Show "{First} {Surname} — {Estate}, Spring 1" in the save list. Pass the estate name to the boundary toast and the HUD
  - The label is saved ("Eleanor Trelawney — Trevennor, Spring 2" after a night's sleep). `Simulation::EstateName()` is exposed for the boundary toast and the store.
- [x] 2.3 Add the arrival title card and the Journal list with the arrival note

## 3. Ruin detail and acceptance

- [ ] 3.1 Add the detail pass: window openings with lintels, quoins, fallen timbers, slate scatter, lichen and soot, and ivy and bramble cling. Check its silhouette from the road approach and the cove in ordinary play
- [x] 3.2 Run the whole new-game flow with mouse and controller, name save/reload, footprint rejection, and 720p/4K checks
  - Done: the flow with the controller and with the keyboard and mouse (1.2, 2.1), the save label, and footprint rejection (1.3). In standalone 1280x720 and 3840x2160 windows with real keyboard input, the Names panel, its two-line hints and the arrival card fit and read cleanly; at 4K the panel is scaled once, by the engine's DPI curve.
  - Named save/reload: a standalone new game with the estate typed as "Polgrean" saved the label "Eleanor Trelawney — Polgrean, Spring 1". Relaunching resumed that save in the standing room on Spring day 1, with no setup screens. The store's "From {Estate}" header now reads `Simulation::EstateName()`, but I haven't seen it on screen: the console store command didn't open in the slow standalone run.
- [ ] 3.3 Jenny playtests the arrival. Collect her default names, the year and her ruin feedback, and fold them in
