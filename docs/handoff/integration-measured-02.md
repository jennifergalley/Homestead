# Measured build 02 integration

Build `20261003-measured-02`; Integration runtime
`e528fd4a-5aed-4c95-9463-a37941afc00b`, project session
`52572922-09fd-4287-85c5-840ceec6d895`, branch
`jennifergalley-integration-agent-68e`, worktree
`jennifergalley-studious-goggles`. Runtime model is GPT-5.6 Terra; reasoning effort and
actual/configured context tiers are unknown.

## Admission checkpoint

- **Gameplay-first Shipping decision (09:44 local, Oct 4):** Jenny superseded the earlier
  all-art-perfect/sixteen-meal hold. Admit the existing original pole, six original fish, and
  exactly eleven existing original meals through Farming checkpoint
  `843324c17b85b5432785fc7a6b9e333702dc26ec`: `BakedPotatoes`, `RoastedTurnips`,
  `StewedCarrots`, `HerbedBroadBeans`, `CabbagePotatoStew`, `BerryCompote`,
  `StrawberryCompote`, `RootVegetableHotpot`, `RawFishSlices`, `GrilledTrout`, and
  `GrilledPerch`. Their disclosed molded/baked/procedural
  appearance is accepted for mechanics playtesting only; do not claim final realism or substitute
  reused old art. `GrilledMackerel` (`0a1d1da8`), `FishSoup`, `FishAndPotatoes`,
  `HerbedCarp`, and `MackerelChowder` are deferred and preserved, not preparations represented
  as completed work.
- Integration owns hands-on slot 1 for the lean import, combined build, runtime, release-isolation,
  package, Shipping-check, and promotion path. Farming owns one ready checkpoint containing only
  the frozen eleven meals' minimum exports, bakes/maps, representative imagery, item-eating wiring,
  manifest, and deferred-meal availability gating. It stops optional modeling, probes, and polish;
  it does not import Unreal assets or alter catch/save/version contracts. Existing texture budgets
  are ceilings, not mandatory 4K rebakes: use the cheapest coherent provisional original-asset
  import/material path and preserve richer WIP for later. The no-waiver functional, save, rollback,
  and protected-release gates remain in force.
- Wiring gate: current source has fish item/recipe identities but no `CaughtFish` mesh-path
  reference. Farming's assigned imagery/eating delta must bind the six imported original fish and
  the eleven meal portions to actual gameplay presentation; otherwise the fish would be cooked
  assets only. This requires no enum, save, or version change.
- Initial critical path: Farming's ready eleven-meal handoff, then Integration import, merged
  Development/runtime checks, real candidate/rollback save containment evidence, package, and
  Shipping EstateSmoke/ToolRepeat. Once the handoff exists, the initial lean-path estimate is
  90–150 minutes excluding compile, runtime, save-containment, or cook failures; no safe delivery
  estimate exists before it.
- Pre-handoff baseline: `Scripts\Test-Native.ps1 -Configuration Release` passed all 21 tests in
  200.90 seconds after the hands-on grant. It validates the already integrated gameplay source only;
  it does not validate the pending asset/export/eating-wiring checkpoint and must be rerun where
  affected after that checkpoint lands.
- Frozen source admission: merged and published
  `843324c17b85b5432785fc7a6b9e333702dc26ec` at `c2d8b080783faf425c59351460caeab5aa6907d6`.
  The merge carries the authorized original fish and eleven meal source/provenance only; the five
  deferred meals are absent. It has no gameplay or save-code delta. Its sole implementation change
  adds the fish-only `wet_fish` report-metadata parent selection to `import_props.py`; ordinary
  props retain their existing parent selection.
- Fish import: in the owned editor at port 8768, `import_props.main(['CaughtFish'])` imported and
  saved all six authorized original fish to
  `/Game/SurvivalGame/Environment/Props/CaughtFish`. The saved folder has 36 assets, including
  `SM_RiverTrout`, `SM_RiverSalmon`, `SM_LakeCarp`, `SM_LakePerch`, `SM_SeaBass`, and
  `SM_SeaMackerel`; all six mesh lookups succeeded. The imported mesh extents match the report:
  trout 7.72 × 34.05 × 10.79 cm, salmon 13.54 × 62.09 × 19.08 cm, perch 7.30 × 30.05 ×
  12.11 cm, carp 11.89 × 42.07 × 16.78 cm, mackerel 8.12 × 36.05 × 10.44 cm, and bass
  11.23 × 46.07 × 15.13 cm. The editor then closed. This proves the asset import only, not
  material runtime appearance, gameplay, meal import, package, save containment, or promotion.
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
Risky-art reviewer `991baa12-1d79-4171-bcd6-5c653ce1d09a` is a single build02 read-only helper
(GPT-6.1 Sol, high, default launch) diagnosing the `4148` images and recipe. It uses no GPU, editor,
or implementation slot. Jenny directly authorized Farming to continue autonomously on structural jaw,
gill, and scale geometry, so this review does not pause that lane; export and UE admission remain held.
The reviewer completed in two read-only turns with no files, processes, or cleanup. It found anatomy
construction—not missing gloss—as the primary failure, then uniform scales/marks; its concrete jaw,
operculum, pectoral, and scale corrections are advisory to Farming's newer structural edits. Before a
full fish family, require one trout source-versus-baked 4K proof: the reviewed `BAKE=None` wrapper
proves the source shader only, not five-map translation. Preserve the opt-in wet film, jaw winding,
and eye-contact improvements. This is not final art or UE admission.
The same reviewer has one bounded same-task follow-up on the newest frozen `c0` proof/source: inspect
two to four views and choose one remaining causal correction among anatomy, shader, or bake. This is
read-only, uses no new helper or GPU/editor/implementation slot, and is not a waiting gate for
Farming's autonomous slot 2. The residual molded/procedural appearance remains, the art/wiring estimate
has grown to an uncertain 11–18+ hours, and the sixteen dishes are untouched.
The reviewer completed that follow-up in its third and final read-only turn. `c0` materially improved;
its one directed next action is tangent-continuous carp cheek/lip rostral interpolation in
`body/carp_lip`, verified with one head view—not further family or gloss sweeps. It viewed the
SpeciesProof SourceFamily carp hero/detail, SourceTrout+BakedTrout trout detail, and BakedTrout normal.
The baked trout shows local cheek-reflection patches absent from source and possible normal-map
blockiness, but the precise cause is unproven and global frame scores do not prove local fidelity.
Retain the prior anatomy, fin, and film fixes. This creates no waiting, admission, or UE claim.
At 03:57 local, the fish bake/normal reconstruction became a genuine blocker. Technical reviewer
`fc56f594-b380-45f1-9633-428ca29e5089` (GPT-6.1 Sol, high, default launch; actual context unknown)
has one read-only shader-to-bake-to-normal causal probe/fix. Same-mesh source-normal hybrid improved
`3.105` to `1.318`; 16-bit, bump, triangulation, and coat variants failed; 8K/head UV was about
`2.44`; an untouched repeat was `.007`. No root cause, admission, or budget increase is proven.
The reviewer performs no process, render, implementation, generic bake rewrite, family bake, or UE
work. Farming preserves/checkpoints carp/probe WIP and continues already-authorized sixteen original
dish source/geometry work in slot 2 without waiting or new scope.
Reviewer `fc56f594-b380-45f1-9633-428ca29e5089` completed in one read-only turn with no files or
processes. The root remains unproven: SourceBump/POINT sampling and differentials are strongest, with
a tangent-path alternative; bump-removal tests lacked matched source controls, and source roughness
hybrid `2.068` also contributes. Its one directed discriminating test is a 4096 frozen-trout
OBJECT-space diagnostic normal roundtrip retaining the same topology/UVs/transforms/smooth state,
Base, and Coat; `<=1.7` plus local improvement distinguishes tangent from the roughly `3.1/2.4`
procedural branch. It is not a production object-space UE map or art acceptance. Preserve budgets,
shaders, and parents; no shared-helper defect is established. Farming continues independent original
meal source work without waiting.
The directed OBJECT4096 test did not clear the defect: guarded source/bake states and shared OBJECT
Base+Coat NormalMap1/NonColor produced cheek RMS `3.062` versus original `3.105`, with viewed patches
persisting. Tangent conversion alone is insufficient; the exact procedural-normal/filtering defect
remains unresolved and creates no production/import acceptance. Correct the earlier bump-probe reading:
those probes used the original bumped source as reference rather than matched bump-free round trips,
so they do not exclude Bump evaluation. Fish probes now stop; original sixteen-dish source work begins
independently.
The same reviewer completed a second and final read-only follow-up with no files or processes. The
unchanged OBJECT result (`3.062`) confirms tangent conversion is insufficient and the cause remains
unproven. After the meal-source milestone only, its one advisory scratch-4096 discriminator is to
explicitly take the final Bump normal `WORLD->OBJECT`, set type `NORMAL`, normalize and encode
`.5n+.5` through `EMIT` strength `1` into a data image, then substitute the existing OBJECT
Base+Coat map. `<=1.7` with local improvement implicates NORMAL-pass extraction; roughly `3.062`
implicates field evaluation/filtering. It is not a production fix, UE object-space map, budget change,
admission, shared-helper claim, or meal pause.
The further authorized explicit-normal EMIT capture also failed: cheek RMS `3.060` versus OBJECT
`3.062`, with viewed patches persisting despite signed final Bump-socket transformation,
normalization, `.5n+.5` encoding, 4096 NonColor, and Base+Coat OBJECT reconstruction. The exact
UV-bake procedural evaluation/filtering cause remains unresolved; no production shader, map, helper,
or import change follows. Frozen fish evidence is unchanged. Original potato serving/portion source
work advances independently, but is not meal or art acceptance.
The original **Baked Potatoes** serving/portion source milestone is built and viewed twice. Its
nonradial crumb correction removes centre spokes; closed, UV, and scale fixtures pass at
`43,968/13,632` triangles. This remains art WIP/source evidence only, not meal acceptance; Farming
continues authorized meal refinement without waiting.
Held source-only checkpoint `24500d2a` preserves the original Baked Potatoes serving/portion and
four viewed 4K frames, with copied geometry, closed, scale, UV, and hash fixtures passing. It remains
synthetic and **not** art/import-ready, with no save or gameplay changes. Its
`NormalCaptureDiagnostics` retains the failed OBJECT (`3.062`) and explicit EMIT (`3.060`) cases plus
the unmatched-bump-control correction. Farming continues authorized independent meal realism/source
work in slot 2; fish proofs and budgets stay held, with no re-review, probe, or Integration hands-on
action assigned.
Correction: the isolated food module was cached in live Blender, so `24500d2a`'s on-disk material
hash did **not** prove the current graph. Its geometry/hash fixtures remain valid, but its skin/flesh
nodes were older parameters and it remains inadmissible. Farming is adding recipe-owned
`importlib.reload(food)`, shader source tags, and actual noise-scale fixtures before a fresh source
review. The canonical lesson is that `build_prop` reloads kit/material modules only; custom recipe
dependencies need explicit live reload.
Held checkpoint `1cc2911f` verifies the cache correction: it reproduces the stale graph fixture, then
passes recipe-owned reload, executed-source tags, and actual node-scale checks; copied source and four
fresh viewed 4K frames were separately verified. `PotatoesReloadedSourceProof` remains art/import WIP,
while old `24500d2a` retains the shader-cache audit. No fish, shared-helper, budget, gameplay, or save
edits are included.
The verified reload evidence replaces `24500d2a`'s invalid current-graph proof, not art acceptance:
uniform flesh and procedural wood remain. Original dish source work continues independently with no
fish budget/shared-helper, gameplay/save, or Integration hands-on change.
Held checkpoint `e6715c2e` preserves the original six-wedge Turnip1 serving, a new earthenware dish,
and separate portion, with four viewed 4K frames and passing closed-component, current-shader, and
copied-hash fixtures. It remains too molded and **not** art/import-ready. The closed-bevel winding fix
is recipe-local and does not alter open fish or any shared helper; Potato function ASTs are unchanged
from `1cc2911f`. Potato and turnip source work is preserved, but 14 dishes plus realism, baking,
imagery, and wiring remain. Farming continues authorized slot 2 with an uncertain forecast; no fish
helper/budget, gameplay/save, or Integration hands-on action changes.
Held WIP checkpoint `ae434be5` preserves original Herbed Beans: 24 seeds, 36 flakes, a new
`13.44` cm bowl/portion, and `62,928/2,432` triangles. Four viewed 4K frames plus source,
shader, coordinate, island, applied-transform, sampled-bowl, and edible-bean fixtures pass. The
completed OPTIX pass
leaves f/64 detail failures—waxy/molded beans and procedural clay—so this is **not** art/import
admission. There are now four source prototypes, 12 unstarted dishes, and zero approvals. Farming's
664-call build02-only snapshot is provisional; no fish/helper/budget/bake, UE, gameplay, or save
change is included. The visible EEVEE state is preserved after the completed GPU render.
Metadata-only follow-up `2612dcaa` checked one bean: translated Object coordinates match retained
`pcoord` within `1.88e-8` m, yet viewed 4K/native-pixel comparison remains `0.0284` full and
`0.0923` surface RMS and does not recover fine relief. There is no source-coordinate fix, fish/bake
diagnosis, or production change. The `ae434be5` art hold is unchanged; diagnostic replay/evidence
stays on owned E: scratch, its visible held bean scene was restored, and no render remains active.
Farming's build02 snapshot is now provisionally 679 calls; four prototypes, 12 unstarted dishes, and
zero approvals remain.
Held WIP checkpoint `70f03669` preserves original Cabbage Potato Stew and a new hollow maple
food-bearing eating spoon. Four viewed 4K, 192-sample frames and copied source/FBX/fixtures pass;
the source is `45,576/10,424` triangles and the sampled spoon hollow is `2.959` mm. There are now
five source prototypes, 11 unstarted dishes, and zero approvals. It remains stiff/molded/procedural
and **not** import/admission-ready; broth source transmission is not a UE material contract and the
spoon grip is unverified. Runtime spoon seating and liquid material are future Integration gates.
The existing eight shader ASTs are unchanged, and there are no fish/bake/animation/gameplay/save
changes. Farming's build02 snapshot is provisionally 698 calls (event `70962`).
Held WIP checkpoint `50f155b9` preserves `BerryCompoteSourceProof`: `58,972/10,528` triangles,
serving/portion dimensions `[11.8,11.8,3.26]/[2.25,14.6,1.12]` cm, copied source, four viewed 4K
frames, FBX, and passing fixtures. It remains molded/gel/procedural and **not** art/import-ready.
There are six source prototypes, 10 unstarted dishes, and zero approvals. Cabbage-spoon extraction
geometry, `pcoord`, and material-index fixtures remain identical/current; the prior 12 shader and four
geometry-function ASTs remain unchanged. There is no fish/bake/animation/gameplay/save change.
Farming's build02 snapshot is provisionally 726 calls (event `71003`), with more than 12 remaining
source hours plus unknown gates.
Held source-only checkpoint `c82c482d` preserves original Strawberry Compote, dish, and `15.2` cm
spoon with frozen copied fixtures and four viewed 4K frames. A local cooked-edge bevel collapsed one
triangle; recipe-local dissolve of its 10 nm degenerate edge restores unchanged closed/clearance
bounds. The meal remains molded/raw-looking and **not** art/import-ready. There are seven meal-source
prototypes, nine unstarted dishes, and zero approvals; old library definitions remain unchanged.
There is no fish/bake/gameplay/save/UE change. Farming's build02 snapshot is provisionally 759 calls
(event `71051`); more than 12 source/refinement hours plus unknown gates is not a delivery ETA.
Farming continues authorized hotpot source work with no Integration hands-on assignment.
Held source-only checkpoint `1bc234e1` preserves original Root/Turnip Hotpot, crock, and `17` cm
spoon with copied fixtures and four viewed 4K frames. It remains smooth/stump-like and **not**
art/import-ready. There are eight crop-meal source prototypes, eight fish preparations unstarted, and
zero approvals. The prior 16 shader and six geometry-function ASTs are unchanged. Farming continues
authorized raw-fish meal source work independently of unresolved fish baking; no fish/bake/gameplay/
save/UE change, Integration hands-on assignment, or release ETA follows.
Held source-only checkpoint `7737497a` preserves independent original raw-mackerel cuts, oval dish,
and edible slice with copied fixtures and four viewed 4K frames. It remains molded/graphic and **not**
art/import-ready. There are nine meal-source prototypes, seven unstarted dishes, and zero approvals.
The prior 17 food shaders and six geometry functions are unchanged, as is the catch recipe/source.
This is not a caught-fish probe or bake and creates no import, gameplay, save, UE, Integration
hands-on, or release-ETA change. Farming continues authorized cooked-food source work.
Held source-only checkpoint `f6464440` preserves original Grilled Trout, platter, and portion with
copied fixtures, four viewed final 4K frames, adaptive unequal muscle folds, and a cooked-skin edge.
It remains molded/rubbery with patterned-muscle appearance and is **not** art/import-ready. An original
portion-extent fixture caught `38.35` mm, so the actual piece was trimmed to `37.55` mm rather than
relaxing the fixture; bounds remain unchanged.
There are ten source prototypes, six unstarted dishes, and zero approvals. The prior 20 shaders and
six geometry definitions are unchanged. Farming's build02 snapshot is provisionally 818 calls (event
`71154`); no catch/bake/import/gameplay/save/UE change is included.
Held source-only checkpoint `843324c1` preserves original Perch fillet pair, new platter, and portion
with copied fixtures and four viewed final 4K frames. It remains smooth/molded with painted skin and
is **not** art/import-ready. There are 11 meal-source prototypes, five unstarted dishes, and zero
approvals. New shared cooked-loft/trout delegation has exact frozen-trout vertices, topology,
`pcoord`, UV, and material-index replay plus invalid-input tests; the prior 23 shaders and six
geometry functions are unchanged. There is no catch/bake/gameplay/save/UE change. Farming's build02
snapshot is provisionally 842 calls (event `71198`), not reconciled; no Integration hands-on
assignment or release ETA follows.
Held source-only checkpoint `0a1d1da8` preserves original Grilled Mackerel fillet, cut pieces, new
platter, and edible piece with copied fixtures and four viewed final 4K frames. Cut relief is confined
to interior caps after a viewed boundary crease; it remains smooth/rubbery with graphic skin and is
**not** art/import-ready. There are 12 meal-source prototypes, four unstarted dishes, and zero
approvals. The prior 26 shaders and eight geometry functions are unchanged. There is no catch
probe/bake/gameplay/save/UE change. Farming's build02 snapshot is provisionally 869 calls (event
`71246`), not reconciled; it continues the remaining four authorized meals with no Integration
hands-on assignment or release ETA.
Held WIP checkpoint `bc4126c3` adds original Stewed Carrots source/proof. It fixes the viewed bowl
penetration and rejects an `8.45` cm heap through largest-first bounded vertex seating, but does not
prove complete contact/intersection. Its 12-piece bowl/portion, `53,792/3,976` triangle source, four
fresh viewed 4K frames, and copied-source/executed-shader/island fixtures pass. It remains
slab-like/procedural and below the art/import realism bar: there are three meal-source prototypes,
13 unstarted dishes, and zero meal approvals. There are no fish
budget/helper/bake, UE, gameplay, save, or version changes. Farming continues authorized independent
meal source work.
Held WIP checkpoint `825fbe4b15cfb51ce583ab46f049afec954478c2` preserves carp tangent/source proof
(`55,008` triangles, `42.11` cm), passing raw and copied fixtures, and twelve-case local bake
evidence. It has no gameplay, save, or UE action and remains not import/admission-ready. The
coordinator directs only the one 4096 OBJECT-space diagnostic plus independent original sixteen-dish
source modeling; production budgets and shaders remain unchanged. Live carp WIP is saved on E:;
Farming's build02 accounting reports 521 calls provisionally.
Checkpoint `cf30e41d` supplies that single-trout structural proof under
`Assets\Props\CaughtFish\StructuralProof`: source, clay, and five-map 4K renders with hashes. It is
still WIP-only; its frozen `4148` views are unchanged and the full six-family root remains stale and
inadmissible. Its recipe-local ground re-seat compensates for thin-fin welding shifting the minimum Z
by 35 micrometres. Wet film remains `.65/.06`; there are no generic-parent changes or UE/API compile
claims. Farming continues anatomy work, and no import is requested.
Checkpoint `15000b47` is the later held WIP family proof: `CaughtFish\HingedProof` and
`CaughtFish\FamilyReview` preserve six anatomy fixtures, 30 maps, twelve viewed 4K frames, portable
textures, and four passing offline import contracts, with no save changes. It is **not**
import/admission-ready: molded heads, checker-like scales, and opaque fins remain visible failures;
sixteen dishes and their wiring are still pending. Farming estimates at least another 8–12 hours,
followed by engine gates. It continues the authorized refinement with no UE or build launch.

### Narrow overnight exception

At 22:05 local on Oct 3, Jenny directly authorized Farming Fishing to continue overnight if needed.
This supersedes the prior no-overnight parking instruction **only** for that lane's existing scope:
six-fish refinement, then sixteen original dishes, portions, export, and wiring, in autopilot slot 2.
It creates no automation and does not authorize WIP import/admission, Integration Unreal/build/package
work, or any change to the original-art, route-isolation, and release gates. Oct 4 7:30 AM remains a
target rather than a promise.
At 00:38 local on Oct 4, Farming estimated another 8–12+ hours for fish refinement, excluding
engine gates, with sixteen dishes and wiring still pending. The full 07:30 delivery is therefore
historically retained but at risk and no longer a realistic expectation. Do not invent a replacement
slot or alter Jenny's planner selections. The current release remains unchanged; WIP import/admission
is still forbidden while Farming's autonomous overnight slot 2 continues.
Checkpoint `411fbd34` is held WIP only. Its `MembraneProof` preserves six current fish, twelve viewed
4K frames, two material slots, 30 maps, and validated portable blend/receipts without gameplay, save,
or version changes. It adds isolated `M_CaughtFishMembrane` TwoSidedFoliage with `.35` opacity
dispatch and nine offline contracts, but these are neither UE compile nor appearance proof; do not
import or admit it. The canonical portability lesson is recorded: `relative_remap=True` can retain
absolute image paths, so explicitly repoint to `//Textures` paths and reopen the blend. Farming
continues anatomy refinement.
Farming also verified a live-render destination pitfall: `build_prop(copy=True)` can retain
`bpy.data.filepath` for the previous proof, causing `render_beauty` to write there. It restored only
its three overwritten frozen `411fbd34` files from preservation copies and confirmed that proof
folder clean; current carp renders now write to E: scratch. The canonical rule is to explicitly open
the newly exported blend before rendering.
Checkpoint `c0ec4e0c` is another held WIP source checkpoint. It adds species mouths, cheeks, and
fins plus shared base/coat relief; `SpeciesProof` preserves six source meshes, twelve viewed 4K
frames, and one portable five-map trout pair with nine passing offline contracts. This is neither an
engine nor an art gate: there is no baked six-fish family or dish delivery, and there are no UE,
build, save, or version changes. Molded heads, procedural rays, and localized baked head blotching
remain visible despite its whole-frame fidelity pass.

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
