# Build changelist

Integration updates this file for every delivery. Each entry uses a date/slot heading, SHA, status,
and short player-facing changelist; the planner reads it directly.

Timing (Jenny, 2026-10-01): start each build about an hour before its slot (7:30 AM, 4 PM, 9 PM).
Lanes implement only what Jenny has prioritized for a specific build, then go idle. They don't pick up
unflagged queue items. If little or nothing is prioritized, the orchestrator tells Jenny to schedule work.

## 2026-10-01 — 7:30 AM

- SHA: `20ad66e9`
- Status: delivered
- Ships:
- Zero Energy never kills her; low Energy warns, slows, and blocks exhausting tool work.
- One press at bed time sleeps until rested or morning.
- She can wade into the lake, fill a 15-portion pail in the water, and water from it.
- Hold to chop or clear; E interacts while click uses tools.
- Seed tiles show an outline and `[E] Plant`; one hint card lists the available actions.
- Dragging shows a ghost; the full-size hotbar row supports `R` rotation.
- Hotbar items can be stored in a chest, including with Shift-click.
- The Build tab shows a clear `Requires` list.
- Parchment UI, concise copy, and compact Settings are the new default.
- Night brightness Set A is in; both shops close on Sundays.
- New pickaxe and billhook sounds, a quieter scythe, and footsteps sit under the ambience.
- Manor interior light leaks are fixed.
- Cove steps, kerbs, and rails are ready to walk.

## 2026-10-01 — 4:00 PM

- SHA: `f8ec22dc`
- Status: delivered
- Ships:
- The manor-ruin signpost points `To town`; cove fingerposts point the way down.
- Lake-path wildflowers, berries, and root forage now line the walk.
- Rain can arrive at any hour, including at night; weeds appear once per day.
- Hair stays dark chestnut indoors by day.
- Hoe, axe, pickaxe, and watering wrists move more naturally; the scythe stays above ground.
- Audio is balanced against the forest ambience, with short lamp messages.

## 2026-10-01 — early evening

- SHA: `c2a589452bc05faec0f72f08758d84f5e5f92c7d`
- Status: delivered
- Ships:
- Two-handed swings and the scythe's left fist follow a more natural working plane.
- The hoe now holds its working grip with a more natural elbow and wrist.
- Her feet step into and out of kneels one at a time, with a corrected stick cradle.
- The animation inspector is harder to break in Development and the character lab; it is not a player feature.
- Verification: Native 19, Inspector 22, nine Development routes, and guarded Shipping EstateSmoke/ToolRepeat passed; the promoted Shipping SHA-256 is `6E2EC11A5CF44AA7945AC88559C7020D6371D7A8A071DDFB53DCE4D82344D27C` with 18 saves preserved.

## 2026-10-01 — 9 PM

- SHA: `3aa62ab0e8f7a03c4c0ab52f9dde764e710999be`
- Status: delivered
- Ships:
- Menu panels line up edge to edge, with one clear selection outline.
- Crafting recipes fill the page width.
- Map labels are crisp, with no text shadow.
- The wardrobe preview just shows her standing.
- A lighter accent keeps selected text readable.
- A dark parchment palette is available in Settings › Book colours.
- Weeds sprout in only some plots each day, at random.
- Verification: Native 19, eight Development smoke/FullLoop routes, and guarded Shipping EstateSmoke/ToolRepeat passed with 392 zero-network samples; Shipping SHA-256 `DB6B6D8BB4E7928E3CA5695EB77B3B2A3CE38E58BA2107AE863F9681E53FFA87`, with 18 current saves copied unchanged.

## 2026-10-02 — 11:46 PM

- SHA: `d2155333` (source/package checkpoint; delivery accounting handoff follows)
- Status: delivered
- Player acceptance: complete per Jenny, 2026-10-03; delivered to `Homestead Estate.lnk`.
- Build ID: `20261002-measured-01`
- Ships:
- Dark theme applies consistently across the book and HUD.
- Clicking a known map destination fast travels, with T and controller confirmation retained.
- The town signpost stands beside the road past the derelict farm and points toward town.
- Pack items stay in the exact selected squares and preserve their layout after reload.
- The General Store purchases grown crops while preserving its existing buy loop.
- Accounting: include new lanes, helpers, orchestration, review and integration from the kickoff;
  show model/effort/context and coverage in the planner. Earlier implementation already on main is
  inherited work; historical costs must be separately attributed or explicitly marked unavailable.
- Deferred: animation/tool carrying, rucksack fit, foliage flicker and hair corrections remain in
  the backlog. Existing main changes are not reverted merely because further fixes are deferred.
- Timing: Jenny authorized work at 21:32 local; the shortcut was promoted at 23:46:31 after the
  valid delivery gates. No overnight automation.
- Verification: reviewed Town crop/sign transaction work and reviewed/re-reviewed pack-slot
  migration/input work; Native 19, current-source Development FullLoop, and guarded Shipping
  EstateSmoke/ToolRepeat passed. Shipping FullLoop is intentionally excluded because it is an
  unadapted Woodland route while Shipping forces Estate. The promoted Shipping executable SHA-256
  is `272613AA2892C6B6A4A618A5277747A81ECA0AA72A997C14DCBB3CB5538E93D8`; 20 package-local
  save/config files were hash-verified on copy, and the prior 20261001 9 PM Shipping release is
  retained as rollback.

## 2026-10-03 — 9:00 PM

- SHA: not built
- Status: deferred at 20:49 local; this 9 PM scope is retained as the historical selected card set.
- Build ID: `20261003-measured-02`
- Plant Seeds hint appears when selected seeds target tilled ground.
- Crop harvests yield fewer seeds so buying seeds remains useful.
- Add 5-10 basic crop/forage cookfire recipes, with kindling and some herb seasoning.
- Sell a hotbar fishing pole at the General Store for 1500 coins.
- Catch location-specific river, lake and ocean fish; cook or sell them.
- Unlock marked fast-travel destinations by visiting, including Town before signpost/map travel.
- Evening sleep lasts until the earlier of dawn or 6 AM.
- Sunday General Store waiting advances to Monday opening.
- Edit feedback titles, descriptions and screenshots directly in the planner.
- Scope: the fishing feature and its two feedback cards are one implementation, not duplicate work.
- Timing: authorized 18:55 local. The fallback is a **target** for 2026-10-04 7:30 AM, not a
  promise; no overnight work resumes after signoff without Jenny's next message.
- Planner tooling receipt: the live feedback-editing delivery is `3846476d`; it is separately
  delivered tooling, hidden and unscheduled, with its OpenSpec player checkbox preserved.
- Safety: verified work only; no placeholder or unverified subset package.

## Later

- The player-driven deferred queue is in [backlog.md](backlog.md): river/ocean, map and town feedback, fishing, hauling, mine work, manor rebuilding, artisan goods, and later estate systems.
