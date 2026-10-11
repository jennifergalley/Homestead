# Build changelist

Integration updates this file for every delivery. Each entry uses a date/slot heading, SHA, status,
and short player-facing changelist; the planner reads it directly.

Delivery is work-driven: the planner's Next build ships as soon as its admitted work is verified and
reviewed, then Build after next moves up. Lanes implement only what Jenny has prioritized for a planner
slot, then go idle; they do not pick up unflagged queue items.

## 2026-10-10 — 5:18 PM

- Build ID: `20261010-next-01`
- Package source SHA: `5fb269035ccbbc772d47a84e2760d0bf8ef4ccc5`
- Status: delivered
- Configuration: Development
- Ships:
- Old saves don't load into the new map (they say so and stay untouched), so start a new game.
- A smaller map, with a new coast and cove with cliff steps down to the water; the mine moved up to
  the clifftop; the map cropped to a 1520 m square around PlayableBounds with walls at the edges;
  the Penvose gate; roadside cottages; neighbour and commons area outlines. 50 retired placement
  ids, 145 new ones (585000-585299).
- Well fed's HUD chip text no longer clips past the panel's right edge (chip height 38→60,
  word-wrapped) — fixes the cosmetic issue flagged in the prior build.
- Admitted: map shrink stage 2 (`1f8fdf647`, rebased), plus Balance docs
  (`b3beabaff`, `5a9e312c2`, `9397e1fb0`, `7460bc1c2` — stage-2 travel targets, layout review, and
  final OK, including confirming the cove heath fix `babfe357e` is included).
- Verification: Native 26/26 (bakeVersion bump included); editor/game Development compile and
  package succeeded via `pwsh` (Windows PowerShell 5.1 lacks the 2-arg `[IO.Path]::GetFullPath`
  overload `Build-Game.ps1` uses — must invoke with `pwsh`, not `powershell.exe`). An Opus 5 risky
  save-compatibility review found pre-shrink saves would otherwise pass the version gate and
  silently drop retired-parcel purchases and out-of-bounds placements with no refund; fixed by
  bumping `bakeVersion` 2→3 in `ProvisionalEstatePlacements()` only (the frozen placement-pin table
  used by `HomesteadManorTests` is untouched, still `bakeVersion = 2`). Fixed 3 stale FullLoop
  assertions uncovered while re-validating the suite against the new content: watering-toast
  quiet-success notices (`d46b291ff`), probabilistic seed-bonus roots-harvest count
  (`95285907f`), and crafting-row count after deferred fish recipes (`1e2f3d49b`). Packaged
  `EstateSmoke` (59.78fps, zero errors), `ToolRepeat` (59.97fps), and `FullLoop`/F5-F9 proof
  (54.70fps, full save/load round-trip, failure-recovery modal) all passed clean against the
  installed Development package. `Content/` was clean post-cook (no incidental resave noise this
  time). The installed executable SHA-256 is
  `5C5BF4B40B86857A283922A0542A3CD4104A182A2CB5EED3E5CF825E4E212EC6`.
- Promotion: `Homestead Estate.lnk` targets the Development `SurvivalGame.exe` at the unchanged
  `Build\Windows` path with the existing Homestead icon; no retargeting needed. The retained
  rollback is `E:\Repos\SurvivalGame\Build\Windows-20261010-morning` (current + one rollback; the
  prior `Windows-20261004-evening-03` rollback was pruned per the round's disk-space finding).
- Review: Balance OK on every `[ready]` (stage-2 travel targets, layout review, final OK, cove heath
  fix). Jenny approved stage-2 media and native 26/26 + beach 6/6 before this delivery.
- Known issues: none carried forward; the Well Fed HUD chip clipping from the prior build is fixed.
- Accounting: `docs\handoff\accounting\reports\20261010-next-01.json` (1426 calls, 11520274750000
  recorded nano-AIU; status incomplete — fully recorded/rated, no final capture yet). Segments:
  Map Agent (claude-opus-5.5/high), Balance Agent 2c8 (claude-opus-5.5/high), Integration/self
  (mixed claude-opus-5 and claude-sonnet-5).

## 2026-10-04 — 7:27 PM

- Build ID: `20261004-next-01`
- Package source SHA: `7bbab1c5175b23fd012c8ad1f5a134d81c93d487`
- Status: delivered
- Configuration: Development
- Ships:
- The village is about 63 seconds' sprint from the manor, with its square, beach, and route flowers.
- Estate trees can be felled and regrow away from home and protected landmarks.
- Sleep restores full energy.
- The clerk and store use the Trethewey name.
- UI polish and shared ornate frames are in.
- Meals use flat ink glyphs.
- Mr. Josiah Trethewey is a clean-shaven MetaHuman clerk.
- New games spawn in the manor and retain the name form before a save can be made.
- The installed Development build keeps the developer console and crash reports.
- Verification: Native 24/24; Development editor/game compile and cook/package; 53 installed package
  hashes; 41 preserved package-local Saved/config hashes; release-save isolation; installed Development
  EstateSmoke and ToolRepeat passed. Development acceptance uses ordinary `Test-Game` routes
  (Shipping QA guard N/A). The F5/F9 probe recorded one save, one load, an identical saved/loaded
  state MD5, Lit rendering, and profile isolation. Its intentional MetaHuman mesh assertion and the
  Development-only TraceControl TCP `0.0.0.0:1985` listener were explicitly waived; all other owned
  endpoints still fail the probe. The installed executable SHA-256 is
  `6C4933A229423DC31DEE711ADF1C725FF8883C3B162B3E77A9C89F09BA0ACCD3`.
- Promotion: `Homestead Estate.lnk` now targets the Development `SurvivalGame.exe` with the existing
  Estate map argument, package-local `-UserDir`, and unchanged Homestead icon. The prior release
  remains at `E:\Repos\SurvivalGame\Build\Windows-20261004-evening-03`.
- Review: Clerk Balance OK (`81a547cd`); Jenny's asset-review gate was waived while AFK. Final clerk
  media: `E:\CopilotScratch\fa19573e-88a5-4c54-ada3-3f9a07de5fc2\review\clerk-01\06-front-counter-clean-shaven.png`.
  Player acceptance remains pending Jenny's playtest.
- Accounting: per-build report unavailable: `assistant_usage_events` is missing from the session DBs.
  No estimated or zero-cost report was fabricated.

## 2026-10-09 — 11:02 PM

- Build ID: `20261009-next-01`
- Package source SHA: `75b7307fa2767609d76b5ce3244a4e93c34d6efa`
- Status: delivered
- Configuration: Development
- Ships:
- Brambles, weeds and branches regrow around the manor and farm.
- More berry bushes and roots in the estate woods, with a distinct bush look and `[E] Pick Berries`.
- Near-home stumps can be cleared with the axe.
- Growing crops always show their type and day.
- Iron tool upgrades at the General Store (2,000 coins each).
- Bigger landmark icons on the minimap and compass.
- Fishing is harder (about 50% escape at the first two strikes, random waits) and plays out on a
  float on the water with a closing ring and high fishing camera; LMB only, no "Caught" toast.
- Clothes swap on equip ("Wear X"), chests get an Equip action, and the General Store sells 7
  garments plus 8 modern garments with icons (15 total), fitted closely.
- A smoother, graded village street, plus village dressing: cobbled square, well, benches, hedges,
  cottage gardens, flowers and a noticeboard placeholder.
- No Pascoe or Cornwall in-game; the store is Trethewey's General Store, Mr. Trethewey the clerk,
  and the Cornish pasty is now the Meat pasty.
- Well fed now states its benefit (work costs 15% less Energy for 3 hours).
- Admitted lanes: Gameplay (`8fa4e3cc8`), Wardrobe + modern clothing (`b11107ccb`, `25ff298c8`),
  Upkeep (`caf5eb525`), Fishing (`cfefbcc44`), Village (`8bdc73636`), plus orphaned Balance docs
  (`04a57e644`, `d2211552c`, and the three files cherry-picked from `origin/jennifergalley-balance-agent`
  after its archival: `docs/design/balance.md`, `docs/design/cohesion.md`,
  `docs/handoff/balance-agent.md`).
- Verification: Native 25/25 Release (post all merges); editor and game-target compile (two
  non-fatal pre-existing/incidental warnings: `C4701` in `HomesteadControllerFocus.cpp:138`,
  `C4457` in `HomesteadTreeFelling.cpp:135`); 3 Opus 5 risky reviews clean (Gameplay's iron-purchase
  path and the merged shop rows surviving the Gameplay/Wardrobe conflict resolution; Upkeep's
  save/serialize diff and placement-id ranges 583000-583119/584000-584179 against old-save
  compatibility and id collisions; Village's berry-540012 placement move and the updated
  `HomesteadManorTests` pin). `EstateSmoke`, `ToolRepeat`, and `-NativeSaveRetry` (the Development
  F5/F9-equivalent proof: `Test-OfflineStartup.ps1` has a known pre-existing defect) all passed
  against the installed Development package; `Assert-ReleaseSaveIsolation.ps1` passed against the
  installed copy and its rollback. A UI-gallery PIE pass
  (`Test-Game.ps1 -UIGallery -UIGalleryIds shop-fishing-pole,hud-wellfed,hud-fishing-bite,
  hud-fishing-landing,focus-shopkeeper`) confirmed the Trethewey naming is live everywhere and the
  "no Caught toast" fishing copy; it found the Well Fed HUD chip's text ("work costs 15% less")
  clipping against the chip's right border at the default string length — a minor, non-blocking
  cosmetic issue, reported below, not fixed this round. Esc-mid-cast was trusted to Fishing's own
  native coverage rather than independently re-verified in a live PIE pass. The staged
  bite/landing float-and-ring visuals did not render in the sandboxed `-UIGallery` route (a gallery
  capture limitation, not a product defect — those ids need live water-proximity context). The
  package cook incidentally re-saved 974 `Content/` files; all were discarded with
  `git restore Content/` before verifying the main checkout was clean (a larger-scope recurrence of
  the known "Ground bakes re-save byte-different uassets" issue — see the editor skill). The
  installed executable SHA-256 is
  `DC9B58F325BB621D3369618C96F8B41A6F8983CCF743E8ECEAD9BD9F1CCFE41F`. The install was packaged
  in place at the main checkout's `Build\Windows` (no separate worktree-staging copy existed to
  hash against); only the executable hash was recorded for traceability.
- Promotion: `Homestead Estate.lnk` targets the Development `SurvivalGame.exe` with the existing
  Estate map argument, package-local `-UserDir`, and unchanged Homestead icon. The retained rollback
  is `E:\Repos\SurvivalGame\Build\Windows-20261004-evening-03` (current + one rollback, per policy;
  no other dated releases present to prune).
- Review: Balance OK recorded on each `[ready]` (Gameplay, Wardrobe x2, Upkeep, Village, Fishing).
  Jenny approved review media for the new currant bush asset (Upkeep), the village dressing
  (Village), the 7 garments plus all 8 modern garments (Wardrobe), and the fishing float/camera
  rework (Fishing).
- Known issues: Well Fed HUD chip text clips at the right border at the default string length
  (new `[Balance UI]`-style finding, not blocking; backlog candidate). Fishing's "one catch
  message" backlog card is closed by this delivery.
- Accounting: `docs\handoff\accounting\reports\20261009-next-01.json` — 2,741 calls, 53,025.876335
  AIU recorded across Orchestrator, Integration, the 5 lanes, and Balance, allocated from each
  session's creation through capture; no missing sessions or excluded calls; one known limit (some
  configured context tiers unrecorded). `session-store.db` does carry `assistant_usage_events` this
  round, unlike 2026-10-04's build.

## Next build

- A coast of mixed cliffs and gentle slopes, with a gentler path to the cove.
- Map-edge cliffs, no inescapable drops, a smooth river mouth, and fixed cliff, grass, path, stair-edge, beach-corner and boulder artifacts.
- A sea cave in the beach cliffs with a pool inside.
- Beach shells, stones, driftwood and mussels to gather daily, cook and sell.
- A beach campfire she lights with driftwood; it burns orange for a few hours.
- Wooden directional signs replace the teleporting village and manor signs.
- Tilling clears the derelict farm's rows and sticks; half as many meadow herbs, now sellable.
- Neutral, non-Cornish names; the town is called the village.
- Darker, moonlit nights without a second sun.
- The title screen says Start a new game.

## Build after next

- Unassigned; awaiting Jenny's next backlog priorities.

## 2026-10-04 — 3:13 PM

- Build ID: `20261004-evening-03`
- SHA: `4de52b4ff02a680ef7c25fc7cc76fcb2e877330b`
- Status: delivered
- Ships: fishing pole and six fish icons, authored cast/bite/catch presentation, and a randomized
  bite/strike minigame.
- Verification: Native 23/23; Development cook and Shipping reuse stage; package and package-local
  Saved hashes; release-save isolation; guarded Shipping EstateSmoke and ToolRepeat all passed.
  The StartupProbe recorded F5=1, F9=1, an error-free same-world state-MD5 roundtrip, normal Lit
  1280x720 rendering, and MetaHuman readiness. Its final legacy `Base.Mesh` identity comparison
  was waived because the active 2:05 PM release fails it identically while intentionally rendering
  MetaHuman BodyFull. The promoted Shipping executable SHA-256 is
  `4786A724D43E3DBB60D45AF3D7762D432946A21AEDAB934ABD2C4614C73EA24C`.
- Promotion: 3:13:39 PM local to `Homestead Estate.lnk`, preserving its icon and package-local
  `-UserDir`; `Windows-20261004-afternoon-03` remains the rollback. Player acceptance remains
  pending Jenny's playtest.
- Accounting: `accounting/reports/20261004-evening-03.json` records 611 observed calls /
  4,266.46243 AIU through the delivery closure; it is not billing-reconciled and retains unknown
  tails and earlier mixed/shared work as explicit limitations.

## 2026-10-04 — 2:05 PM

- Build ID: `20261004-afternoon-03`
- SHA: `c1d5351f23f69e9f12bc8f274fb1ac101fd45286`
- Status: delivered
- Ships:
- Quit is compact and matches Settings.
- The Back slot shows Leather Rucksack or None without changing capacity.
- Crop-named one-slot seed packets have their own icons.
- Ripe crops always show their named Harvest hint unless weeding comes first.
- Wildflowers cover more field, wood, and water edges while excluding farm, manor, and tilled ground.
- FarmFence posts remain visible until the complete fence is dismantled.
- Fishing code and assets are excluded; fishing remains planned for 9 PM.
- Verification: Development cook/reuse-stage Shipping package, 31 package hashes, 29 copied
  package-local Saved hashes, release-save isolation, guarded Shipping EstateSmoke and ToolRepeat,
  and normal package-local F5/F9 all passed. The promoted Shipping executable SHA-256 is
  `98DD0BD5D90038F110F2442E64FE11292BA3C02C84533193DD8ADB965DE3E943`; the prior recovery package
  remains at `E:\Repos\SurvivalGame\Build\Windows-20261004-recovery`.
- Promotion: 2:05:09 PM local to `Homestead Estate.lnk`, preserving its icon and package-local
  `-UserDir`. Player acceptance remains pending Jenny's playtest.
- Accounting: `accounting/reports/20261004-afternoon-03.json` retains observed local runtime AIU
  and its explicit mixed-work, context, tail, and billing limitations. Measured02 remains closed.

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

- Dedicated Development QA adapter for `Invoke-ShippingQA` parity.

- Launch Development builds with trace control disabled or bound to localhost, if UE supports a flag
  (avoids a LAN-exposed 1985 listener and firewall prompts).

- Rebuild `Export-BuildUsage` from `~/.copilot/session-store.db` (the source the planner's
  account-usage tracker already reads) and backfill the 2026-10-04 evening build report.

- Fix `HomesteadStartupProbe`'s legacy mesh-identity assertion to be MetaHuman-aware; the Fishing
  promotion waiver covers only that pre-existing false invariant.

- The player-driven deferred queue is in [backlog.md](backlog.md): river/ocean, map and town feedback, fishing, hauling, mine work, manor rebuilding, artisan goods, and later estate systems.
- Tooling deferred: extend `Scripts\Build-Game.ps1`'s Shipping `-ReuseCooked -Package` archive
  validation so a future build can stage directly under `E:\HomesteadReleases\<build-id>`. Until a
  tooling slot changes that script, package under the worktree's `Build\Releases`, then use the
  durable-copy promotion procedure recorded in the delivery checklist.
