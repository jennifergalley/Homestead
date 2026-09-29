# Tasks

## 1. Model

- [x] 1.1 Add the overgrowth `ResourceKind`s with a per-kind table (tool, minimum tier, energy, swings, yields). Add the new items and catalogue rows, and a tool-tier state per tool
- [x] 1.2 Implement the `ClearOvergrowth` transaction with a tier gate, multi-swing commit on the final swing, and yield exactly once. Add portable tests for right, wrong and under-tier tools, energy reserve, capacity, and save/reload by stable ID
- [x] 1.3 Remove warmth from Simulation, HUD, sleep and clothing. Retire the knife, machete, fibre, reed, deer and fur paths from new games. Bump the save version with a reset notice. Update or retire the affected tests (the version bump is the world lane's single round-1 bump at integration; packaged suites that still walk the survival loop are listed in the design's Status)

## 2. First playable clearing (first delivery)

- [x] 2.1 Author the billhook mesh (a Blender recipe), wire it to the machete hack animation, and add the salvage-pile kind and Haft recipe. Coordinate the pile placement with the arrival lane
- [x] 2.2 Present bramble thin, thicket and bank variants from placements, with the fruiting material switch off in spring. Verify hacking thin bramble around the manor doorway and the tier prompt on a thicket
- [x] 2.3 Package, then from a new game haft the billhook and clear bramble around the doorway. Check save/reload, capture in-game views, commit and push
  - Status: verified in PIE on the Estate map (main 17a64854). Lanes don't package; the orchestrator packages the integrated build. From a new game (look, names, begin), she searched the salvage piles, hafted a worn billhook with the controller, cleared the thin bramble by the doorway and got the thicket's "Needs an iron billhook". Quick-save and load (F5/F9) kept every cleared tuft and bramble by id. A clear made after the save was reverted, and she was back at the saved spot. Some piles inside the ruin courtyard were reached by teleport, because `walk_to` steers in a straight line, so on-foot reachability of every pile is unchecked.

## 3. Full tool set

- [x] 3.1 Rename and restyle the axe and hoe. Add stump and log clearing to the axe with multi-swing hit reactions. Verify felling is unchanged
  - Status: verified in PIE on the Estate. Stump clearing with the axe shows the swing countdown and the ground-strike clip. The restyled meshes are SM_EstateAxe (`estate_axe.py`) and SM_DrawHoe (`draw_hoe.py`), listed ahead of the flint fallbacks. With the EstateAxe, a forest tree still fells through the unchanged felling presentation. The DrawHoe tills one square with the hoe clip.
- [x] 3.2 Author the pickaxe mesh and strike animation, and add rubble, rock and boulder clearing with stone and scrap yields
- [x] 3.3 Author the scythe mesh and two-handed sweep animation, add the arc clearing with aggregated feedback, and verify the arc against the visible sweep
  - Status: verified in PIE on the Estate. The mow was re-authored from Jenny's feedback: a wide stance, knees bent into a forward lean, and both arms reaching forward on the nibs. A side view in slow motion shows the sweep crossing in front of her and mowing the tufts in the arc ahead, with the aggregated toast "Mowed 1 tuft: +2 Hay." The idle carry now holds the snath upright at her right side with the blade resting behind her. A swing when nothing is inside the arc's short reach says "Step closer to mow." Before, it silently did nothing. Jenny's re-review of the pose is part of 4.2.
- [x] 3.4 Add the daily weed creep near remaining overgrowth, with no regrowth on tilled, built or road ground. Verify it across several slept days
  - Status: the native `WeedCreepNearOvergrowth` test covers 12 days: yard grass creeps back, while a distant patch and a tilled square never regrow. In PIE on the Estate, she mowed 17 forecourt tufts and slept six 8-hour nights in the bedroll, across two 6 AM rollovers. Exactly the tufts the deterministic roll predicts came back: 510031 and 510036 on day 1, then 510021 on day 2. Every other cleared tuft stayed cleared. Road ground is excluded by placement. Creep only revives authored grass and weed placements, never new positions. The Simulation has no road geometry, so the world lane's bake must keep grass and weed placements off the road.

## 4. Acceptance

- [x] 4.1 Run mouse and controller routes through all five tools and every worn-tier kind, plus the tier prompts. Check hotbar slotting and Craft UI clarity at 720p and 4K
  - Status: verified in PIE on the Estate, with each tool used by both mouse and controller. All five heads were hafted from salvage, three with the controller and two with the keyboard. Each auto-slotted to hotbar slots 1-5 in estate order. Worn-tier checks, each showing the countdown toast and the yield:
    - Scythe on tall grass and weeds: LMB and RT, with the aggregated toast.
    - Billhook on thin bramble (LMB) and on a two-swing sapling (RT).
    - Axe on a three-swing small stump (RT) and a fallen bough (LMB).
    - Pickaxe on two-swing rubble (LMB, stone plus scrap) and on a small rock (RT).
    - Hoe tilling with LMB and RT.
  - Tier prompts: "Needs an iron billhook" (thicket), "Needs an iron pickaxe" (boulder) and "Needs an iron axe" (large stump, fallen log). The on-screen prompts switch between [LMB] and [RT] with the last device used.
  - The Craft page was captured at 1280x720 and 3840x2160 by the NativeMenu suite, and both pass. Recipe tiles, the haft detail and the have/need rows are legible at both sizes.
  - Found: a press during the axe's swing recovery (about 3 s) is dropped without feedback. The rusted-axe-head icon is a stand-in that reads like a horn.
- [ ] 4.2 Jenny playtests the clearing feel and density. Tune swing counts, yields and overgrowth density from her feedback
  - Round-1 polish, from her first playtest of the packaged build:
    - Searching a salvage pile now plays the stone-gather kneel. A fallen bough gathered by hand plays the stick gather.
    - Kneel props (sticks, stones, pouch) are hidden whenever no kneel gather is active. Before, a finished or cancelled stone gather left the rocks on her arm, because `CancelAction` hid only the sticks.
    - A pickup requested while the previous gather is still playing or blending out is queued (`PendingKneel`) until her hands are free, instead of switching `KneelKind` under the running clip. That switch was what made a stick pickup sometimes fall back to the generic kneel.
    - Tool prompts say "Requires a <tool>" when she owns none, "Take the <tool> from storage" when it's only in a chest, and "Select the <tool>" when it's in her pack but not in hand.
    - Meadow herbs are a new authored clump, SM_WildMarjoram (`wild_marjoram.py`): about 50 cm of rosy-purple flowering tops over a leafy base, so a herb prompt always has a visible plant. Before, the plant was a tiny tuft with two small flowers.
    - Berry bushes: 42 fruiting blackberry brambles, ids 540000-540041, generated by `Scripts\Terrain\berries.py` from `EstateScenery.bin` into `HomesteadEstateBerryPlacements.inc`. They follow wood edges, hedgerows and the drive, and several are within 20 m of the front door. Rerun berries.py after any woods rebake. On the Estate a berry bush is SM_BlackberryBramble with three ripe clusters.
