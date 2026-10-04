# Measured build 02 integration

Build `20261003-measured-02`; Integration runtime
`e528fd4a-5aed-4c95-9463-a37941afc00b`, project session
`52572922-09fd-4287-85c5-840ceec6d895`, branch
`jennifergalley-integration-agent-68e`, worktree
`jennifergalley-studious-goggles`. Runtime model is GPT-5.6 Terra; reasoning effort and
actual/configured context tiers are unknown.

## Admission checkpoint

- Authorized at `2026-10-04T01:55:51.513Z` for the Oct 3 9 PM slot. The original 9 PM scope is
  retained historically, but the complete delivery was deferred at 20:49 local; Oct 4 7:30 AM is a
  target rather than a promise, with no overnight/signoff work. Checkout was clean at preflight
  (`8b67ebc0`).
- This session owns measured02 manifest/allocation metadata and shared-doc liaison. Its fresh
  accounting segment begins at authorization; there are no helpers, reviews, failures, builds,
  merges, packages, or tests allocated yet.
- Three hands-on slots are held by Farming Fishing, Travel Rest, and Planner Editing. Do not begin
  integration hands-on work until the coordinator explicitly grants a slot and forwards an
  independently completed checkpoint.
- Current shortcut release remains protected in
  `jennifergalley-redesigned-couscous\Build\Releases\20261002-measured-01\shipping-candidate\Windows`;
  the sole protected rollback remains
  `jennifergalley-literate-eureka\Build\Releases\20261001-9pm-shipping\Windows`. Do not reopen
  build01 windows, touch Jenny's live game, remove saves, or archive either worktree.
- Jenny permits Editor and Blender source/asset work while she plays, but no UBT, UAT, Live Coding,
  game compilation, or packages. Keep memory checked and Unreal processes at two or fewer including
  her game; Farming Fishing alone has lane editor permission and Travel Rest has none. Preserve the
  active game, saves, shortcut, and protected releases. No automation is scheduled.
- Baseline planner coverage is not a clean full suite: `PlannerStatusTests.mjs` has a pre-existing
  final renderer assertion for removed `statusFilters`/`completedExpanded` controls. The lane reports
  its first four data tests pass; do not label the stale renderer assertion a measured02 regression.

## Delivery gate

Admit only forwarded lane commits that are complete and independently reviewed where gameplay or
save risk warrants it. Integration alone may merge, run native and valid Development FullLoop,
package via the documented clean-cook path, and run Shipping EstateSmoke/ToolRepeat after a slot
grant. Shipping FullLoop remains invalid until Estate-adapted.

Planner Editing admission additionally requires its new title, description, screenshot, and
identifier-preservation coverage to pass. The pre-existing removed-controls renderer assertion is
reported separately and does not justify restoring unrelated planner UI.

## Planner admission delivery

- Merged the forwarded planner commit `108bbe621723c74d51ac0b896fa3ee2c73cd1fbb`, merged current
  main registry metadata, and published the result at `fc83c40dd539f98f8cd877a47881efe740e7e693`.
- Re-ran the isolated Node admission command after the merge: 19 checks passed; the one browser
  proof skipped because `PLANNER_BROWSER_MODULE` was not configured to an E: `playwright-core`
  module. `npx openspec validate edit-planner-feedback --strict` passed. The stale renderer
  assertion remained intentionally excluded by its named skip pattern.
- The coordinator reloaded extensions and reopened the existing round-3 planner canvas with provider
  `60849`; Jenny still owns the player edit check. Planner admission is complete and slot 3 is
  released.
- This slot performed no Unreal, UBT, UAT, Live Coding, package, release, gameplay, or save work.
- **Separate delivered tooling receipt:** `3846476d` delivered live planner feedback editing. The
  planner fallback publication (`5ec7c7a8` on main) hides and unschedules that tooling delivery
  while preserving the original OpenSpec player checkbox; it is not gameplay admission.

## HUD selection

HUD owns selected chest-name hint enlargement and concise, non-overlapping resource/refusal/save
notices. Preserve Travel Rest's first-visit travel notice and Farming Fishing's Plant Seeds hint;
mine path and building work remain unselected. The lane is held to the active no-compilation rule.

- Final source checkpoint `f184744c0c2fc9927375ec560411d20b9517ab82` on
  `jennifergalley-hud-agent` is pushed but **not admitted**. Both selected source tasks are
  implemented; compile, Unreal, and UI gallery acceptance remain held. Jenny's manual player check
  is post-delivery. The lane used no
  helpers, editor, build, automation, main, shortcut, or save work. The hard-wrap completeness
  diagnostic is included. Preserve Farming's three-use seed-hint retirement and the fishing mount;
  meaningful notice clocks remain, while quiet success and hotbar selection no longer erase notices.
  The high-effort bounded review found no source-risk issue in notice serial/timer behavior,
  hint-learning, or the hard-wrap diagnostic; this is not compile or gallery acceptance.

## Travel Rest checkpoint

- Checkpoint `b37b0070` on `jennifergalley-travel-rest-agent` is pushed but **not admitted**:
  editor compile and independent review remain pending while game compilation is held. Jenny's
  manual player check is post-delivery. Do not merge it yet.
- Five regression suites and the new 286-check TravelRest suite passed after a test-fixture
  Saturday/Sunday correction and isolated native retry. The approved optional travel record,
  transactional malformed/duplicate refusal, and no-save/bake-bump boundary remain in force.
- Estate time advancement does not force a low-energy doze. Sunday wait retains normal awake energy
  drain and crop/season consequences, without granting rest.
- **Blocked by review:** `HomesteadControllerSaves.cpp:306–318` lets an old reader reject `travel`,
  load a compatible pre-upgrade `.bak`, and continue saving. Those saves can overwrite the upgraded
  slot and then its backup. A promoted Travel save requires separately routed, preserved pre-upgrade
  saves, or shared-directory write protection plus rejection coverage; colocated `.bak` files are
  insufficient.
- Read-only live routing proof: `Homestead Estate.lnk` targets the measured01 candidate with
  `-UserDir="<candidate>\\Windows\\SurvivalGame"`. `ResolveHomesteadSaveRoute` defaults to
  `ProjectSavedDir\\SaveGames`, yielding that candidate's
  `Windows\\SurvivalGame\\Saved\\SaveGames`; the protected rollback has a separate package-local
  root. The current release is therefore isolated, but every future promotion must preserve this
  exact release-local override, copy/hash pre-upgrade saves into the new candidate before first
  launch, and leave the old root untouched for rollback. `-HomesteadPreviewProfile` is unsuitable:
  it uses shared `UserSettingsDir` profile roots.

### Mandatory release-isolation gate

The release-local `-UserDir` design is conditionally clean; it requires no source save-root
migration, but neither gameplay checkpoint is ready until the following promotion/rollback evidence
exists:

1. Candidate and rollback launches each report the expected, canonical, physically distinct
   `SaveGames` root; reject junctions, symlinks, aliases, shared routes, missing/wrong `-UserDir`,
   and `-HomesteadPreviewProfile` before save access.
2. Copy every loadable `.sav` and `.bak` from a stable, non-writing pre-upgrade source, hash-check
   it into a fresh candidate root, and keep an immutable, old-reader-compatible rollback snapshot.
   The candidate root contains no unrelated prior saves.
3. In the authorized test window, load and save a synthetic candidate Travel save, then exercise
   candidate manual/autosave/backup replacement and rollback auto/manual/recovery writes across all
   slots and `.bak` files. Hashes in the *other* release root must remain unchanged.
4. Rejection coverage uses disposable fixtures only; it never injects an upgraded save into the
   protected rollback root. Any new `LoadLatest` guard is defense in depth, not a fix for deployed
   old binaries.

### Guard implementation and review

- `Assert-ReleaseSaveIsolation.ps1` now parses `UserDir=` with Unreal `FParse`-compatible
  boundaries, quote handling, ASCII whitespace, and unquoted delimiters before validating the
  sole effective route. It rejects competing route forms rather than attempting to choose one.
- `ReleaseSaveIsolationTests.ps1` passes 15 disposable E:-scratch fixtures, including conventional,
  slash, bare, double-dash, whitespace, quoted-text, Unicode-boundary, comma, and device-alias
  bypasses. A high-effort reviewer rechecked the final parser change with no significant issue.
- This proves only the pre-launch guard's fixture behavior. The mandatory real candidate/rollback
  route, snapshot/hash, and write-containment evidence above remains required before promotion.

Travel Rest may append an optional `travel <count> <id>...` section of sorted, unique destination
IDs 1..6; Manor (0) remains implicit. Missing legacy sections lock every non-Manor destination, and
malformed or duplicate sections refuse transactionally. This does not bump the save or bake version.
Admit it only after independent save review and non-destructive rejection coverage prove the
separate-save or write-protection boundary. Estate `AdvanceGameHours` must not force a zero-energy
doze; Sunday wait preserves authoritative energy drain and adds no rest grant.

## Farming Fishing checkpoint

- Final checkpoint `ff3c9de86c58efce4901b9d89de174003a85d0d4` is **not admitted**. It contains the
  prior seed/crop work plus pole 1500, timed river/lake/sea fishing, and eight fish preparations,
  cards, glyphs, and gallery work. The lane reports native Simulation/Economy/Manor 3/3 and 1,036
  focused fishing checks passing; its handoff is `docs\handoff\farming-fishing-measured-02.md`.
- Editor compile, automated visual acceptance, and the mandatory release-isolation proof remain
  required. Jenny's manual player check is post-delivery. No heavy launch or real saves were touched. Preserve HUD pickup/name-toast removals and
  `QuietActionSerial`, along with Travel save hunks, during any later integration.
- The local combined candidate `9fe8b263` compiled `SurvivalGameEditor` and `SurvivalGame`
  Development successfully in 176 seconds after a game-closed process check. This validates source
  integration only; it does not substitute for visual, player, or release proof.
- Jenny now requires original art for the pole, six caught fish, and sixteen new dishes. The reused
  pole/meal visuals in this source checkpoint block Fishing/Food promotion until original assets are
  authored, imported, and wired. Do not use a stale or current editor pass as art acceptance.
- Full integrated Release native coverage passed 21/21 in 199.74 seconds. The only warnings were
  pre-existing C4456 shadowed locals in `HomesteadManorTests.cpp` and
  `HomesteadPublicRoadTests.cpp`.
- Original-pole checkpoint `befe70b52fc593247d55a90de42383293f80dfa3` is integrated locally at
  `60da4466`. It adds the authored `FishingPole` source package, capped-tube UV regression, held-mesh
  wiring, and the lane's authored 4K maps. The Blender pipeline guide now records the exact,
  scope-limited cleanup for its appended-library save failure.
- A fresh combined `SurvivalGameEditor` + `SurvivalGame` Development build passed in 62 seconds.
  In a `measured02-rod` preview-profile PIE fixture, `hud-fishing-bite` reported
  `UI_GALLERY_READY`; `Held_FishingPole` resolved to
  `/Game/SurvivalGame/Environment/Props/FishingPole/SM_FishingPole`, was visible at unit scale, and
  its side capture showed its hand-mounted clearance. The editor closed immediately afterward.
  This proves the original pole import and held presentation only—not fish/dish art, fishing gameplay,
  full automated visual acceptance, package readiness, or promotion.

## Delivery decision

The local pre-Fishing boundary `5e40dc30` isolates Travel and HUD source, but it is not a clean
verified 9 PM delivery: Travel/HUD still lack runtime, gallery, and the mandatory
real promotion containment proof. Fishing/Food also lacks the required original assets. Defer rather
than package or promote a placeholder-art or incompletely verified slice.

At 20:49 local on Oct 3, the complete measured02 selected delivery is explicitly deferred: six fish
and sixteen dish assets remain outstanding, and neither a Fishing/Food candidate nor an alternate
Travel/HUD slice has all of its required evidence. Do not consume a speculative package cycle tonight.
The next verification step is Farming's asset-owner review of the imported pole's real held pose,
followed by original fish/dish delivery; only then may Integration schedule the remaining source,
runtime, save-isolation, and package gates.

Farming's later fish checkpoint `c70b603a` is explicitly WIP and must not be imported or admitted.
It preserves six original meshes, 24 PBR maps, 12 viewed 4K renders, and geometry/receipt checks,
but its head, mouth, and marking realism still needs correction; sixteen dishes and their portions
remain outstanding. The canonical Blender guide records the live preview remedy: switch from
`BLENDER_WORKBENCH` to `BLENDER_EEVEE` before requesting material viewport shading.
Refinement also established that the plain one-lobe prop shader cannot represent the required wet-skin
film. Farming will add an opt-in fish-only ClearCoat parent and import hook, without changing an
existing parent; its source bake must preserve the same film. Keep fish import/admission held until
that ready checkpoint arrives, then verify the new parent compiles during the owned Unreal import.
Later source-only checkpoint `4148ab7c94026f70b577ada0488069c8a22fb8aa` preserves anatomy/film
work and four viewed 4K drafts but is still WIP: coordinator art direction, full-family output, and
import are held. It changes no UE, gameplay, or saves. Its fish-only ClearCoat import contracts pass
offline; actual UE parent compile remains Integration's future gate. Reusable Blender fixes are
documented: explicitly wind open-jaw repair volumes, conform eyes to `jaw_surface`, and use the
active screen owned by a Blender window for `temp_override`.

### Narrow overnight exception

At 22:05 local on Oct 3, Jenny directly authorized Farming Fishing to continue overnight if needed.
This supersedes the prior no-overnight parking instruction **only** for that lane's existing scope:
six-fish refinement, then sixteen original dishes, portions, export, and wiring, in autopilot slot 2.
It creates no automation and does not authorize WIP import/admission, Integration Unreal/build/package
work, or any change to the original-art, route-isolation, and release gates. Oct 4 7:30 AM remains a
target rather than a promise.

At 22:07 local, Jenny additionally authorized the coordinator—not Farming—to schedule a single-use
30-minute fallback check, rearming it only after processing while work remains active. She also
directed that Fishing Blender/Unreal work may continue while she plays: do not apply a blanket
playtime pause. If a real resource conflict arises, report its exact editor/GPU/process constraint so
she can choose to stop playing. The two-Unreal-process cap and assigned import ownership remain
mandatory. Integration's overnight hands-on, build, and package hold is unchanged until separately
assigned.

## Bounded current-source runtime evidence

- With Farming's Blender window paused, Integration launched one owned editor at port 8768 with the
  `measured02-runtime` preview profile. Estate PIE reached `worldReady` with no map errors.
- `homestead.UIGallery focus-chest` reported `UI_GALLERY_READY focus-chest`; its capture shows the
  wrapped `Wide Winter Wool Storage` chest title and action within the HUD. This is one HUD gallery
  state, not full gallery acceptance. Jenny's manual player check is post-delivery. The editor was then closed before Farming resumed
  Cycles work.
- The merged candidate reran `ReleaseSaveIsolationTests.ps1`: 15 disposable E:-scratch checks passed.
  This remains pre-launch fixture evidence only. Travel runtime and real candidate/rollback
  route/hash/write-containment evidence are pending.
- Its counted-stock serialization widens to 23 Items without a save version, bake, or tagged-section
  change. Older readers can therefore reject these saves even with zero new quantities; Farming
  inherits the Travel separate-save/write-protection rollback gate. Fishing casts are ephemeral.
- A final high-effort review of `249229b0..ff3c9de` found no scoped fishing/item/economy/input/save
  blocker. It reaffirmed the HUD `QuietActionSerial` and pickup/name-toast preservation plus
  Travel discovery/save-section constraints. This is source-risk evidence only, not compile, visual,
  player, or release approval.
