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

- [x] 3.1 Rename and restyle the axe and hoe. Add stump and log clearing to the axe with multi-swing hit reactions. Verify felling is unchanged
  - Status: verified in PIE on the Estate. Stump clearing with the axe shows the swing countdown and the ground-strike clip. The restyled meshes are SM_EstateAxe (`estate_axe.py`) and SM_DrawHoe (`draw_hoe.py`), listed ahead of the flint fallbacks. With the EstateAxe, a forest tree still fells through the unchanged felling presentation. The DrawHoe tills one square with the hoe clip.
- [x] 3.2 Author the pickaxe mesh and strike animation, and add rubble, rock and boulder clearing with stone and scrap yields
- [x] 3.3 Author the scythe mesh and two-handed sweep animation, add the arc clearing with aggregated feedback, and verify the arc against the visible sweep
  - Status: verified in PIE on the Estate. The mow was re-authored from Jenny's feedback: a wide stance, knees bent into a forward lean, and both arms reaching forward on the nibs. A side view in slow motion shows the sweep crossing in front of her and mowing the tufts in the arc ahead, with the aggregated toast "Mowed 1 tuft: +2 Hay." The idle carry now holds the snath upright at her right side with the blade resting behind her. A swing when nothing is inside the arc's short reach says "Step closer to mow." Before, it silently did nothing. Jenny's re-review of the pose is part of 4.2.
- [ ] 3.4 Add the daily weed creep near remaining overgrowth, with no regrowth on tilled, built or road ground. Verify it across several slept days
  - Status: implemented. The native `WeedCreepNearOvergrowth` test covers 12 days: yard grass creeps back, while a distant patch and a tilled square never regrow. It is not yet checked across slept days in game.

## 4. Acceptance

- [ ] 4.1 Run mouse and controller routes through all five tools and every worn-tier kind, plus the tier prompts. Check hotbar slotting and Craft UI clarity at 720p and 4K
- [ ] 4.2 Jenny playtests the clearing feel and density. Tune swing counts, yields and overgrowth density from her feedback
