# Measured build 02 integration

Build `20261003-measured-02`; Integration runtime
`e528fd4a-5aed-4c95-9463-a37941afc00b`, project session
`52572922-09fd-4287-85c5-840ceec6d895`, branch
`jennifergalley-integration-agent-68e`, worktree
`jennifergalley-studious-goggles`. Runtime model is GPT-5.6 Terra; reasoning effort and
actual/configured context tiers are unknown.

## Admission checkpoint

- Authorized at `2026-10-04T01:55:51.513Z` for the Oct 3 9 PM slot; verified unfinished work
  falls back to Oct 4 7:30 AM. Checkout was clean at preflight (`8b67ebc0`).
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

## HUD selection

HUD owns selected chest-name hint enlargement and concise, non-overlapping resource/refusal/save
notices. Preserve Travel Rest's first-visit travel notice and Farming Fishing's Plant Seeds hint;
mine path and building work remain unselected. The lane is held to the active no-compilation rule.

- Final source checkpoint `f184744c0c2fc9927375ec560411d20b9517ab82` on
  `jennifergalley-hud-agent` is pushed but **not admitted**. Both selected source tasks are
  implemented; compile, Unreal, UI gallery, and Jenny acceptance remain held. The lane used no
  helpers, editor, build, automation, main, shortcut, or save work. The hard-wrap completeness
  diagnostic is included. Preserve Farming's three-use seed-hint retirement and the fishing mount;
  meaningful notice clocks remain, while quiet success and hotbar selection no longer erase notices.
  The high-effort bounded review found no source-risk issue in notice serial/timer behavior,
  hint-learning, or the hard-wrap diagnostic; this is not compile, gallery, or player acceptance.

## Travel Rest checkpoint

- Checkpoint `b37b0070` on `jennifergalley-travel-rest-agent` is pushed but **not admitted**:
  editor compile, independent review, and Jenny's player checks remain pending while game compilation
  is held. Do not merge it yet.
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

- Checkpoint `249229b0d5c3f072d4fb6384e9ded0cb0f503d9c` is also **not admitted** until editor
  compile and player acceptance. Independent review found no separate gameplay/economy source
  blocker in its appended Items/Recipes, Plant Seeds hint, single-seed return, harvest, or meal flow.
- Its existing counted-stock serialization widens when Items are appended, so older readers reject
  the save even if every new meal count is zero. Farming promotion inherits the Travel separate-save
  or write-protection rollback requirement.
