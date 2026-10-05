# Build changelist

Integration updates this file for every delivery. Each entry uses a date/slot heading, SHA, status,
and short player-facing changelist; the planner reads it directly.

Delivery is work-driven: the planner's Next build ships as soon as its admitted work is verified and
reviewed, then Build after next moves up. Lanes implement only what Jenny has prioritized for a planner
slot, then go idle; they do not pick up unflagged queue items.

## 2026-10-04 — Next build

- Build ID: `20261004-next-01`
- Package source SHA: `7bbab1c5175b23fd012c8ad1f5a134d81c93d487`
- Status: delivered (Development)
- Ships: the in-room new-game/naming save guard; closer village, beach, route flowers, and old-save
  road recovery; legacy-only seed packet load merging; estate-tree felling/regrowth; flat meal ink
  glyphs; Mr. Josiah Trethewey's clean-shaven MetaHuman clerk; and the reviewed sleep, rename, and
  UI work already admitted on main.
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
