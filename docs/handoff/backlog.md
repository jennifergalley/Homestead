# Playtest backlog

Only Jenny-directed work belongs here. Every line is one player-checkable slice, not a speculative implementation plan. Lanes create or revise an OpenSpec change only after Jenny gives feedback or explicitly selects a later item. `priority.json` is Jenny's scheduling order, not an autonomous pickup queue: lanes implement only the items she prioritized for their build, then go idle. The planner canvas can drag-reorder the list; **Top**, **Next build**, **Quote**, and **Remove** act on it, with **Remove** archiving the change.

<!-- jenny-inbox:start -->
- **Edit items in the planner backlog** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to edit items I've added to the backlog from my feedback form, where clicking a pencil icon somewhere on the card opens it back up in the form so I can edit the title, description, and remove or add new screenshots for it.
- **Sleeping in evening sleeps until dawn** — Jenny, 2026-10-04 (added from the planner canvas): Starting to sleep in the evening (post 6 PM or sunset, whichever is earlier) should have me sleep through the night and wake up at dawn (6 AM, or whenever the sun rises, whichever is earlier).
- **Fishing with the pole at a river, lake, or ocean yields cookable / sellable fish** — Jenny, 2026-10-04 (added from the planner canvas): Fishing mechanics should work much the same way as Coral Island or Hades. I should catch different fish depending on where I fish, so river fish vs freshwater fish vs saltwater fish. I should be able to prepare the fish into edible dishes, from raw sushi to grilled fish, fish soup, fish n chips and a variety of seafood dishes. I should also be able to sell these fish to the General Store.
- **Basic Recipes from Crops** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to cook a selection of 5-10 additional basic recipes using the crops I can buy as seeds from the General Store and grow on my farm, as well as berries and roots. These require the cookfire and some kindling to prepare, and some should require Meadow Herbs for seasoning. The amount of energy they restore should be proportional to the opportunity cost of selling the crops for coins.
- **Unlock fast travel via map by visiting a location once** — Jenny, 2026-10-04 (added from the planner canvas): When I visit a new location marked on the map or minimap for the first time, a notice should appear that says "Fast Travel Destination Unlocked: <Location>". From then on, I should be able to fast travel to that location from the map the same way I can the manor or the Town. This includes the Town - I should need to walk there once before I can fast travel there, either by signpost or by map.
- **Buy a fishing pole at the General Store for 1500 coins** — Jenny, 2026-10-04 (added from the planner canvas): I should be able to buy a fishing pole at the General Store for 1500 coins at the same time as I can buy the backpack upgrade, which unlocks access to fishing. It should come into my inventory as an item I can place in my hotbar.
- **Reduce the rate and quantity of receiving seeds from harvesting crops** — Jenny, 2026-10-04 (added from the planner canvas): As it stands, I might never need to buy seeds because I am getting so many when I harvest. We need to rebalance things so that the player is incentivized to buy seeds from the General Store.
- **When I have seeds selected in my hotbar and am pointing at tilled ground, the [E] Plant Seeds interaction hint should always show** — Jenny, 2026-10-04 (added from the planner canvas): (no description)
- **Improve tool carry holds** — Jenny, 2026-10-03 (added from the planner canvas): Further tool-carry polish deferred from measured build 01, separate from the shipped grip/contact fixes. Refine idle and moving holds so grips look natural and tools do not clip through her hands or body.
- **Improve work animations** — Jenny, 2026-10-03 (added from the planner canvas): Further heroine action/work animation improvements deferred from measured build 01, separate from already shipped fixes. Polish natural movement, tool contact and transitions when Jenny schedules this work.
- **Teleporting back to the manor should deposit me at the new sign location beside the farm and road** — Jenny, 2026-10-03 (added from the planner canvas): (no description)
- **Wait at the General Store on Sunday until it opens** — Jenny, 2026-10-03 (added from the planner canvas): When I get to the General Store on Sunday, I should have the option to wait until they open on Monday morning so I can enter.
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
- **Add shore and river fishing** — Jenny's cozy-estate direction, 2026-09-27: catch and sell or cook one fish. **Lane:** Water. **Size:** L.
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
- **Unify HUD notices** — Jenny, 2026-09-30: trigger resource, refusal, and save notices without overlap or verbosity. **Lane:** Menu. **Size:** S.
- **Widen estate beach** — Jenny, 2026-09-30: walk 12–20 m of dry navigable shore below the cliffs. **Lane:** Water. **Size:** M.

Build status lives in [builds.md](builds.md).
