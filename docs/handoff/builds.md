# Build changelist

Integration updates this file for every delivery. Each entry uses a date/slot heading, SHA, status,
and short player-facing changelist; the planner reads it directly.

Timing (Jenny, 2026-10-01): start each build about an hour before its slot (7:30 AM, 4 PM, 9 PM).
Lanes implement only what Jenny has prioritized for a specific build, then go idle. They don't pick up
unflagged queue items. If little or nothing is prioritized, the orchestrator tells Jenny to schedule work.

## 2026-10-04 — 9:00 PM

- Status: planned
- Selected feedback: `backlog:jenny-muu6k1g1-qr5dx2` fishing pole/fish icons and original
  cast/bite/catch presentation with randomized timing minigame.
- This is the only fishing code or asset admission for the 9 PM slot; it is excluded from the 4 PM
  build.

## 2026-10-04 — 4:00 PM

- Build ID: `20261004-afternoon-03`
- Status: planned
- Authorization: Jenny's 11:59:23 local message; target 4:00 PM local.
- Selected scope: compact Quit dialog; Back-slot rucksack visibility without capacity loss; wider
  wildflowers outside farm, manor ruins, and tilled ground; visible broken fenceposts until the
  complete fence is dismantled; crop-specific non-stackable seed packets/end-product icons; and a
  persistent ripe-crop `[E] Harvest <crop>` hint unless weeding takes precedence. Fishing code and
  assets are excluded.
- Accounting: starts at `2026-10-04T18:59:23Z` in
  `accounting/afternoon-03-allocation.json`. Measured02 remains closed at 2,477 calls /
  11,439.97802 observed AIU; its unknown post-capture tail does not transfer to this build.
- Runtime recovery: a fresh main-checkout Shipping package was installed at
  `E:\Repos\SurvivalGame\Build\Windows` after Development UBT, a fresh cook, staged-package and
  copied-save hash checks, release-save isolation, guarded EstateSmoke/ToolRepeat, and package-local
  F5/F9 verification. `Homestead Estate.lnk` now targets that package with its existing icon. This
  restores the lost active release only; it does not ship or remove any selected 4 PM feedback.
- Player acceptance: no player checks are marked accepted by this scheduling record.

## 2026-10-04 — 11:29 AM

- Build ID: `20261003-measured-02`
- SHA: `b073cebe` persists the exact manifest-approved 127-asset bootstrap resave set; `fa3265fd`
  records the fresh-cook/candidate evidence.
- Status: delivered
- Ships:
- Eleven crop meals shipped with provisional original art: Baked Potatoes, Roasted Turnips, Stewed
  Carrots, Herbed Broad Beans, Cabbage Potato Stew, Berry Compote, Strawberry Compote, Root
  Vegetable Hotpot, Raw Fish Slices, Grilled Trout, and Grilled Perch.
- A 1,500-coin fishing pole catches location-specific fish that she can sell or cook.
- Harvests yield fewer seeds, keeping seed purchases useful.
- Tilled ground with selected seeds shows the Plant Seeds hint.
- Evening sleep ends at dawn.
- Fast travel unlocks by visiting marked destinations.
- Sunday General Store waiting advances to Monday opening.
- Chest names are larger.
- HUD notices are unified, concise, and non-overlapping.
- Player acceptance: **pending Jenny's playtest**. Delivery and guarded functional/save acceptance
  do not close her unchecked planner checks.
- Planner/accounting: `accounting/reports/20261003-measured-02.json` is the loader-compatible,
  deduplicated observed-usage report. It covers all allocated lanes, Integration, orchestration, and
  reviewers through the delivery-closure capture; `11,439.97802` recorded AIU is not
  billing-reconciled credits, and any later tail/context remains unknown.
- Fresh cook: the first Development cook wrote 1,019 tracked assets and was stopped. Jenny approved
  surgical restoration of 892 verified incidental writes while retaining exactly the 127 admitted
  CaughtFish/PreparedFood/FishingPole/`M_CaughtFishWet` paths. A 2m31s reconciled recook then
  staged the current Shipping candidate without protected containers.
- Verification: 18 active saves copied/hash-checked into the candidate; release-route assertion
  passed; a real normal-candidate F5 manual write replaced only the candidate manual save, its
  `.bak` matched the pre-write payload, F9 reload remained alive, and the protected rollback root
  remained unchanged. Guarded Shipping EstateSmoke and ToolRepeat passed. Shipping executable SHA-256:
  `0D33088B19607142D1A5EB3FC410A499024DE71A3209E8601993ED47ED9171FA`.
- The active measured01 release, its sole rollback, shortcut icon, and all protected player saves
  remain retained and untouched.

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

## 2026-10-01 — 9:00 PM

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

## 2026-10-01 — 6:08 PM

- SHA: `c2a589452bc05faec0f72f08758d84f5e5f92c7d`
- Status: delivered
- Ships:
- Two-handed swings and the scythe's left fist follow a more natural working plane.
- The hoe now holds its working grip with a more natural elbow and wrist.
- Her feet step into and out of kneels one at a time, with a corrected stick cradle.
- The animation inspector is harder to break in Development and the character lab; it is not a player feature.
- Verification: Native 19, Inspector 22, nine Development routes, and guarded Shipping EstateSmoke/ToolRepeat passed; the promoted Shipping SHA-256 is `6E2EC11A5CF44AA7945AC88559C7020D6371D7A8A071DDFB53DCE4D82344D27C` with 18 saves preserved.

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

## Later

- The player-driven deferred queue is in [backlog.md](backlog.md): river/ocean, map and town feedback, fishing, hauling, mine work, manor rebuilding, artisan goods, and later estate systems.
- Tooling deferred: extend `Scripts\Build-Game.ps1`'s Shipping `-ReuseCooked -Package` archive
  validation so a future build can stage directly under `E:\HomesteadReleases\<build-id>`. Until a
  tooling slot changes that script, package under the worktree's `Build\Releases`, then use the
  durable-copy promotion procedure recorded in the delivery checklist.
