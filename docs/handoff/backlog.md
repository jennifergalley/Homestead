# Playtest backlog

Only Jenny-directed work belongs here. Every line is one player-checkable slice, not a speculative implementation plan. Lanes create or revise an OpenSpec change only after Jenny gives feedback or explicitly selects a later item. `priority.json` is Jenny's scheduling order, not an autonomous pickup queue: lanes implement only the items she prioritized for their build, then go idle. The planner canvas can drag-reorder the list; **Top**, **Next build**, **Quote**, and **Remove** act on it, with **Remove** archiving the change.

<!-- jenny-inbox:start -->
- **I should be able to go down into the mine and mine for rock, ore and gems** — Jenny, 2026-10-04 (added from the planner canvas): The trapdoor in the Mine building asset should be usable, and I should be able to desend into dark tunnels, where I need to use my oil lamp for light, and my pickaxe to mine for ores, stone, and gems, as laid out in our plans. I should be able to sell these items in town and use them in crafting ingredients. Most important is the stone building recipes for rebuilding the manor to its former glory.
- **I'm not able to chop down trees** — Jenny, 2026-10-04 (added from the planner canvas): It seems that in the shift from the survival MVP to the estate version of the game I lost the ability to chop down trees. We should still have the animation and sounds - please give me the ability to chop down any tree in the map, and depending on where it is (say, in a forest or far away from the manor or farm), it should have the capability to grow back (we can say it comes back as a fully formed tree for now and defer tree growth rates for later)
- **I should be able to Disassemble, Replace Fence around the Farm** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to craft new wooden fence parts (sections of fence and gates) in the Build menu and place them around the farm, and I should be able to disassemble the broken down fence that currently exists around the farm first so I can replace it
- **Pointing at a harvestable crop should always display "[E] Harvest <crop>"** — Jenny, 2026-10-04 (added from the planner canvas): Unless I also need to weed the square, right now when I point at a harvestable crop, it does not give me an interaction hint that I can harvest. It should always show [E] Harvest <crop> when the crop is ready to harvest and I point at the square.
- **Differentiate seeds by plant type in the inventory** — Jenny, 2026-10-04 (added from the planner canvas): instead of just saying Seeds, it should say Roots seeds, Carrot seeds, Turnip seeds, etc., they should take up individual inventory slots and not be stackable, and they should all have distinct icons with a picture of their end product on the seed packet. There shouldn't be any generic "Seeds" that doesn't tell me what I'm growing
- **Broken down fenceposts are invisible** — Jenny, 2026-10-04 (added from the planner canvas): The broken down fence posts are invisible, leaving horizontal pieces of fence floating in midair. The fenceposts should be visible and not disappear unless I disassemble the whole thing [Screenshot](attachments/backlog/jenny-mutdjl7e-nzl7lh-c4147f18-37f2-4828-8a52-6b0ceb13f266.png)
- **Wildflowers everywhere** — Jenny, 2026-10-04 (added from the planner canvas): I really enjoy the wildflowers along my walk to the lake, but I think we need them to be more widespread throughout the map. I'd like wildflowers to be essentially everywhere, along lakes and rivers, under the trees of the woods, and in open fields. The only place I really don't want them is in the farm (or anywhere I start tilling soil), or in the ruins of the broken down manor.
- **Toggle Backpack visibility** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to toggle the visibility of the backpack in my character's Equipped slots in my inventory. For a new "Back" slot under the "Equipped Slots", I should be able to select Leather Rucksack or None, with a hint making it clear that toggling off the pack's visibility doesn't remove the inventory space upgrade
- **Quit Game menu text is too large** — Jenny, 2026-10-04 (added from the planner canvas): Please make the size of the font and buttons of the Quit Game / Save and Quit / Quit without saving menu slightly smaller, to match the font size of the settings menu would be good, so it feels cohesive
- **Fishing pole and fish icons need refinement** — Jenny, 2026-10-04 (added from the planner canvas): The fishing pole and fish icons in the inventory / hotbar need refinement to match the quality and style of the other tools and finished crops. Fishing also needs an animation of her casting the line out, and the animations should drive the fishing / catching mechanics, not a text box with a progress bar that disappears too fast to read. Draw inspiration from how Coral Island does it. There should also be more randomness to the fishing animation, so it takes a random amount of time for a fish to bite, and the exact moment to click to catch the fish should involve randomness too so it becomes a minigame.
- **Build an asset for the Mine - a large stone building I can enter, with a trapdoor in the floor that leads into the Mine** — Jenny, 2026-10-04 (added from the planner canvas): Even if the trapdoor is just a placeholder for now, the building for the mine should exist in the world. I should be able to go inside the main room of the first floor, see rocks piled in a corner, a desk, an oil lamp on the desk that let's say burns infinitely but can't be picked up, and a trapdoor in the floor that will later on lead down into the mine.
- **Path to the Mine from the Manor** — Jenny, 2026-10-04 (added from the planner canvas): There should be a dedicated path through the woods to the mine from the manor, just as there is to the lake through the trees
- **Edit items in the planner backlog** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to edit items I've added to the backlog from my feedback form, where clicking a pencil icon somewhere on the card opens it back up in the form so I can edit the title, description, and remove or add new screenshots for it.
- **Improve tool carry holds** — Jenny, 2026-10-03 (added from the planner canvas): Further tool-carry polish deferred from measured build 01, separate from the shipped grip/contact fixes. Refine idle and moving holds so grips look natural and tools do not clip through her hands or body.
- **Improve work animations** — Jenny, 2026-10-03 (added from the planner canvas): Further heroine action/work animation improvements deferred from measured build 01, separate from already shipped fixes. Polish natural movement, tool contact and transitions when Jenny schedules this work.
- **Teleporting back to the manor should deposit me at the new sign location beside the farm and road** — Jenny, 2026-10-03 (added from the planner canvas): (no description)
- **test** — Jenny, 2026-10-02 (added from the planner canvas): just testing backlog feature
<!-- jenny-inbox:end -->

## Current delivered build

The five items Jenny selected on 2026-10-02 shipped in `20261002-measured-01` and are no longer planned improvements. The changelist is in [builds.md](builds.md). Jenny considers the player checks complete as of 2026-10-03; no next build work is scheduled.

## Next candidates (deferred, not scheduled)

- **Improve work animations** — further action/work animation polish is a separate Planned improvements card; deferred and not scheduled.
- **Improve tool carry holds** — further idle/moving tool holds are a separate Planned improvements card; earlier grip/contact fixes remain shipped.
- **Fit rucksack straps** — shoulder/chest fit remains deferred with the character work.
- **Refine playable heroine hairstyles** — Bob/updo motion and near/far artifacts remain deferred.
- **Reduce foliage shadow motion** — foliage flicker/shadow corrections remain deferred.
- **Speed up pickup animation** — pickup animation improvements remain deferred.

## Later

- **Replace heroine with MetaHuman** — retain the next player-reported character regression for a later selection.
- **Flexible sleep** — retain Jenny's one-press tired/rested/dawn bed-sleep feedback.
- **Improve contextual feedback and sprint** — retain flagged tired-sprint/refusal/recovery controls and copy.
- **Add rain weather** — retain flagged seasonal rain, shelter, wet ground, audio and lighting issues.
- **Fix estate river source** — retain the river-to-ocean on-foot check.
- **Improve estate frame rate** — retain the next player-visible hitch/performance regression.
- **Prioritize heroine quality and tool clarity** — Jenny, 2026-09-29: play an ordinary tool/wardrobe loop; fix the next visible pose, contact, or hair issue. **Lane:** Props. **Size:** M.
- **Rework farming calendar and period crafting** — Jenny, 2026-09-29: play a 60-minute day and Sunday shop loop; tune only flagged timing, closure, Energy, or Well fed behavior. **Lane:** Simulation. **Size:** M.
- **Enrich estate ground and meadow** — Jenny, 2026-10-01: walk farm, manor, lake, and coast; address the next ground-presentation issue. **Lane:** Water. **Size:** M.
- **Add handcart hauling and dynamic prices** — Jenny's cozy-estate direction, 2026-09-27: haul goods to town and see stock-sensitive pricing. **Lane:** Props/Menu. **Size:** L.
- **Restore mine pumping and deep levels** — Jenny's cozy-estate direction, 2026-09-27: operate a pump, open a deeper level, and bring back ore. **Lane:** Water/Props. **Size:** L.
- **Author fixed Cornish estate map** — Jenny, 2026-10-01: fix the next player-reported Estate route, coast, town, or landmark issue. **Lane:** Water. **Size:** M.
- **Rebuild manor and decorate** — Jenny's cozy-estate direction, 2026-09-27: rebuild one manor room and place decor. **Lane:** Props/Menu. **Size:** L.
- **Polish locomotion, view distance, and time HUD** — Jenny, 2026-09-29–30: fix the next movement, camera, time/weather, or foliage-readability issue. **Lane:** Props/Menu. **Size:** M.
- **Open estate mine shallow level** — Jenny's cozy-estate direction, 2026-09-27: clear the entrance, explore, and smelt ore. **Lane:** Water/Props. **Size:** L.
- **Expand randomized music playlist** — Jenny, 2026-09-29: play without immediate repeats and tune ambience only from feedback. **Lane:** Audio. **Size:** S.
- **Add watermill and artisan goods** — Jenny's cozy-estate direction, 2026-09-27: operate a station and sell one processed good. **Lane:** Props/Menu. **Size:** L.
- **Add ranching and livestock** — Jenny's cozy-estate direction, 2026-09-27: care for an animal and collect one product. **Lane:** Future round. **Size:** L.
- **Add horse and wagon** — Jenny's cozy-estate direction, 2026-09-27: stable a horse, load a wagon, and deliver goods. **Lane:** Future round. **Size:** L.
- **Add hired workers and estate upkeep** — Jenny's cozy-estate direction, 2026-09-27: hire one worker, assign work, and pay upkeep. **Lane:** Future round. **Size:** L.
- **Add family reputation and land purchase** — Jenny's cozy-estate direction, 2026-09-27: complete a request and purchase one adjacent parcel. **Lane:** Future round. **Size:** L.
- **Add courtship and family** — Jenny's cozy-estate direction, 2026-09-27: meet a town candidate and complete the first courtship step. **Lane:** Future round. **Size:** L.
- **Add boat fishing** — Jenny's cozy-estate direction, 2026-09-27: take a boat out and catch pilchard or crab. **Lane:** Future round. **Size:** L.
- **Add appearance camera controls** — Jenny, 2026-09-30: use Appearance camera controls and keep the heroine readable. **Lane:** Menu. **Size:** S.
- **Add chest auto store** — Jenny, 2026-09-30: auto-store matching items without loss or duplication. **Lane:** Menu. **Size:** S.
- **Add chest names** — Jenny, 2026-09-30: rename a chest and confirm persistence. **Lane:** Menu. **Size:** S.
- **Add cove route** — Jenny, 2026-09-30: walk the full cove route and address the next visible route issue. **Lane:** Water. **Size:** M.
- **Add cove route kit** — Jenny, 2026-09-30: walk cove steps, kerbs, and rails without clipping or blocked progress. **Lane:** Water/Props. **Size:** M.
- **Add dye chooser** — Jenny, 2026-09-30: dye one owned garment and confirm appearance persistence. **Lane:** Menu. **Size:** S.
- **Add estate lake** — Jenny, 2026-09-29: walk farm-to-lake, use the landing, and report the next issue. **Lane:** Water. **Size:** S.
- **Add field-book notice card** — Jenny, 2026-09-30: trigger a brief, readable, non-duplicated field-book notice. **Lane:** Menu. **Size:** S.
- **Add hotbar eat and meal gains** — Jenny, 2026-09-30: eat from the first inventory row and confirm Food/Energy result. **Lane:** Menu. **Size:** S.
- **Add HUD compass trial** — Jenny, 2026-09-30: navigate one Estate landmark and keep the compass only if useful. **Lane:** Menu. **Size:** S.
- **Add lake-trail forage** — Jenny, 2026-10-01: harvest lake-trail forage and preserve its saved identity through reload. **Lane:** Water. **Size:** S.
- **Add leather backpack** — Jenny, 2026-09-30: equip it, use extra storage, and confirm save/restore. **Lane:** Menu/Props. **Size:** M.
- **Add pickup gain lines** — Jenny, 2026-09-30: show one concise resource gain only when useful. **Lane:** Menu. **Size:** S.
- **Add road-sign travel** — Jenny, 2026-09-30: follow signed manor/coast/town routes without losing the path. **Lane:** Water/Props. **Size:** S.
- **Add sprint toggle** — Jenny, 2026-09-30: toggle sprint with both devices while respecting tired Energy. **Lane:** Menu. **Size:** S.
- **Add standing-room door and room audio** — Jenny, 2026-09-30: enter/leave the room with natural door and ambience behavior. **Lane:** Props/Audio. **Size:** S.
- **Add swimming** — Jenny, 2026-10-01: when selected, wade/swim with appropriate tool restrictions. **Lane:** Water. **Size:** L.
- **Add terrain-following road and bridge** — Jenny, 2026-09-30: walk the road and bridge without slope or collision trouble. **Lane:** Water. **Size:** M.
- **Add UI gallery** — Jenny, 2026-09-30: review representative UI at 720p/4K and fix the next readability failure. **Lane:** Menu. **Size:** S.
- **Add wait at closed shop** — Jenny, 2026-09-30: wait at a closed shop and confirm clean opening/time update. **Lane:** Menu. **Size:** S.
- **Audio loudness standard** — Jenny, 2026-09-30: compare every changed cue against ambience during ordinary play. **Lane:** Audio. **Size:** S.
- **Close shops on Sundays** — Jenny, 2026-09-30: visit both shops on Sunday and receive concise guidance. **Lane:** Water/Menu. **Size:** S.
- **Daily weed pass** — Jenny, 2026-09-30: advance a day, find a weed, and clear it with the expected tool. **Lane:** Simulation. **Size:** S.
- **Fix live sound sliders** — Jenny, 2026-09-30: hear live music, ambience, and effects preview and confirm saved values. **Lane:** Menu/Audio. **Size:** S.
- **Fix night brightness** — Jenny, 2026-09-30: walk at 19:00, 21:00, and midnight with readable but not daylight-bright night. **Lane:** Water. **Size:** M.
- **Reduce foliage shadow motion** — Jenny, 2026-09-30: walk under canopy without distracting or hitching shadow motion. **Lane:** Water/Performance. **Size:** M.
- **Space out town square** — Jenny, 2026-09-30: walk comfortable gaps, side lanes, and a clear town route. **Lane:** Water. **Size:** M.
- **Tune tool strike sounds** — Jenny, 2026-10-01: use each changed tool against a real target and keep its cue under ambience. **Lane:** Props/Audio. **Size:** S.
- **Widen estate beach** — Jenny, 2026-09-30: walk 12–20 m of dry navigable shore below the cliffs. **Lane:** Water. **Size:** M.

Build status lives in [builds.md](builds.md).
