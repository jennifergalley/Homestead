# Tasks

## 1. Model

- [x] 1.1 Add the overgrowth `ResourceKind`s with a per-kind table (tool, minimum tier, energy, swings, yields). Add the new items and catalogue rows, and a tool-tier state per tool
- [x] 1.2 Implement the `ClearOvergrowth` transaction with a tier gate, multi-swing commit on the final swing, and yield exactly once. Add portable tests for right, wrong and under-tier tools, energy reserve, capacity, and save/reload by stable ID
- [x] 1.3 Remove warmth from Simulation, HUD, sleep and clothing. Retire the knife, machete, fibre, reed, deer and fur paths from new games. Bump the save version with a reset notice. Update or retire the affected tests (the version bump is the world lane's single round-1 bump at integration; packaged suites that still walk the survival loop are listed in the design's Status)

## 2. First playable clearing (first delivery)

- [x] 2.1 Author the billhook mesh (a Blender recipe), wire it to the machete hack animation, and add the salvage-pile kind and Haft recipe. Coordinate the pile placement with the arrival lane
- [x] 2.2 Present bramble thin, thicket and bank variants from placements, with the fruiting material switch off in spring. Verify hacking thin bramble around the manor doorway and the tier prompt on a thicket
- [ ] 2.3 Package, then from a new game haft the billhook and clear bramble around the doorway. Check save/reload, capture in-game views, commit and push
  - Status: the new-game route (salvage, haft, doorway bramble, thicket prompt) is verified in PIE on the Estate map. Packaging now belongs to the orchestrator. In-game save/reload is not yet checked; native tests cover persistence by stable ID.

## 3. Full tool set

- [ ] 3.1 Rename and restyle the axe and hoe. Add stump and log clearing to the axe with multi-swing hit reactions. Verify felling is unchanged
  - Status: stump clearing with the axe (swing countdown, the ground-strike clip) is verified in PIE. The restyled meshes are authored (`estate_axe.py` SM_EstateAxe, `draw_hoe.py` SM_DrawHoe), imported and wired ahead of the flint fallbacks; the EstateAxe is seen in hand on a stump in PIE. Tree felling with it and tilling with the DrawHoe are not yet re-checked in game.
- [x] 3.2 Author the pickaxe mesh and strike animation, and add rubble, rock and boulder clearing with stone and scrap yields
- [ ] 3.3 Author the scythe mesh and two-handed sweep animation, add the arc clearing with aggregated feedback, and verify the arc against the visible sweep
  - Status: the mesh, the mow clip and the arc clearing with an aggregated toast are verified in PIE. The mow pose (arms forward, knees bent into the lean) and the idle carry (snath upright at her right side, blade resting behind) are re-authored from Jenny's feedback; the idle carry is checked in PIE, the new mow pose is not yet captured in game. A mow out of the arc's short reach now says "Step closer to mow." instead of doing nothing.
- [ ] 3.4 Add the daily weed creep near remaining overgrowth, with no regrowth on tilled, built or road ground. Verify it across several slept days

## 4. Acceptance

- [ ] 4.1 Run mouse and controller routes through all five tools and every worn-tier kind, plus the tier prompts. Check hotbar slotting and Craft UI clarity at 720p and 4K
- [ ] 4.2 Jenny playtests the clearing feel and density. Tune swing counts, yields and overgrowth density from her feedback
