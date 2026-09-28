# Tasks

## 1. Model

- [x] 1.1 Add the overgrowth `ResourceKind`s with a per-kind table (tool, minimum tier, energy, swings, yields). Add the new items and catalogue rows, and a tool-tier state per tool
- [x] 1.2 Implement the `ClearOvergrowth` transaction with a tier gate, multi-swing commit on the final swing, and yield exactly once. Add portable tests for right, wrong and under-tier tools, energy reserve, capacity, and save/reload by stable ID
- [x] 1.3 Remove warmth from Simulation, HUD, sleep and clothing. Retire the knife, machete, fibre, reed, deer and fur paths from new games. Bump the save version with a reset notice. Update or retire the affected tests (the version bump is the world lane's single round-1 bump at integration; packaged suites that still walk the survival loop are listed in the design's Status)

## 2. First playable clearing (first delivery)

- [ ] 2.1 Author the billhook mesh (a Blender recipe), wire it to the machete hack animation, and add the salvage-pile kind and Haft recipe. Coordinate the pile placement with the arrival lane
- [ ] 2.2 Present bramble thin, thicket and bank variants from placements, with the fruiting material switch off in spring. Verify hacking thin bramble around the manor doorway and the tier prompt on a thicket
- [ ] 2.3 Package, then from a new game haft the billhook and clear bramble around the doorway. Check save/reload, capture in-game views, commit and push

## 3. Full tool set

- [ ] 3.1 Rename and restyle the axe and hoe. Add stump and log clearing to the axe with multi-swing hit reactions. Verify felling is unchanged
- [ ] 3.2 Author the pickaxe mesh and strike animation, and add rubble, rock and boulder clearing with stone and scrap yields
- [ ] 3.3 Author the scythe mesh and two-handed sweep animation, add the arc clearing with aggregated feedback, and verify the arc against the visible sweep
- [ ] 3.4 Add the daily weed creep near remaining overgrowth, with no regrowth on tilled, built or road ground. Verify it across several slept days

## 4. Acceptance

- [ ] 4.1 Run mouse and controller routes through all five tools and every worn-tier kind, plus the tier prompts. Check hotbar slotting and Craft UI clarity at 720p and 4K
- [ ] 4.2 Jenny playtests the clearing feel and density. Tune swing counts, yields and overgrowth density from her feedback
