# Playtest backlog

Only Jenny-directed work belongs here. Every line is one player-checkable slice, not a speculative implementation plan. Lanes create or revise an OpenSpec change only after Jenny gives feedback or explicitly selects a later item. `priority.json` is Jenny's scheduling order, not an autonomous pickup queue: lanes implement only the items she prioritized for their build, then go idle. The planner canvas can drag-reorder the list; **Top**, **Next build**, **Quote**, and **Remove** act on it, with **Remove** archiving the change.

<!-- jenny-inbox:start -->
- **test** — Jenny, 2026-10-02 (added from the planner canvas): just testing backlog feature
<!-- jenny-inbox:end -->

## Next measured build

Jenny selected this scope on 2026-10-02. No additional work is scheduled.

1. **Apply dark theme everywhere** — finish the chosen palette across the HUD and book, with readable text and correct focus after switching. **Lane:** UI.
2. **Fix map click travel** — click a known map destination to fast travel, retaining T and controller confirmation. **Lane:** UI.
3. **Move town signpost** — beside the road past the derelict farm, pointing toward town rather than outside the manor. **Lane:** Town.
4. **Place items in exact slots** — items remain in the square selected, including gaps and after save/reload. **Lane:** UI.
5. **Add dollars and general store** — **Urgent:** the General Store buys Jenny's grown crops; separate specialty stores come later. Preserve the existing buy loop. **Lane:** Town.

## Next candidates (deferred, not scheduled)

- **Fix tool grip contact** — tool carrying/contact and animation corrections remain in the backlog; no further work in this build.
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
