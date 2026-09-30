# Heroine replacement trial: baseline and reuse decision

## Selected-build baseline (2026-09-23)

The immutable `inventory-drop-v18` preview selects
`Build\Releases\20260923-150721-drop\drop-02-shipping`. Its Shipping executable
SHA-256 is `326E4E5C41197468E5EC97AED2717F21F581F622D7134B94EE3A020FBBAD247F`.
The selected save is **not** used by any of the captures below: each Shipping QA
route created its own fresh isolated `EngineUser`/`SmokeSave` under its named
`Saved\Automation` output directory. No personal save or selected binary changed.

| Captures | 720p | 4K | What the route proves |
|---|---|---|---|
| Normal woodland, full-body motion/portrait/gather | `Saved\Automation\heroine-quality-baseline-720\Frames`, `telemetry.csv`, `observations.txt` | `Saved\Automation\heroine-quality-baseline-4k\Frames`, `telemetry.csv`, `observations.txt` | Ordinary mapped walk, stop, turn, camera orbit, Appearance, berry gather, and tree contact without teleport/state/time edits. 412 and 118 chronological frames respectively; visual-only sampling, not an FPS benchmark. |
| Fresh ten-slot hotbar and Knife clearing | `Saved\Automation\heroine-hotbar-baseline-720\hotbar-gameplay.png`, `smoke-result.txt` | `Saved\Automation\heroine-hotbar-baseline-4k\hotbar-gameplay.png`, `smoke-result.txt` | Carried Knife only; three preassigned unavailable ghost tool pictures. Later isolated steps use a **controlled teleport/supply fixture** to prove selected Knife low-growth Clear removes exactly one patch. This is not normal-play clearing footage. |
| Hatchet requirement pane | `Saved\Automation\heroine-craft-baseline-720\craft-requirements-ready.png`, `craft-requirements-blocked.png` | `Saved\Automation\heroine-craft-baseline-4k\craft-requirements-ready.png`, `craft-requirements-blocked.png` | Controlled inventory route shows Branch 10/4, Stone 8/3, Fiber 5/2 and retained Knife, with stacked three-line text and a scrollbar even at 720p. Hold-to-craft checks passed separately; this is not yet a largest-recipe capture. |

The 720p and 4K visual routes start at Spring, Day 1, 06:00, Clear, fresh
Preferred/long-wave/tunic/shoes; use the same route and generated tree/resource
IDs. **They are not frame-perfect matched views**: screenshot readback changes
phase wall time and camera position. The 720p contact sheet is in the session
artifacts (`heroine-baseline-720-contact.png`); the full-resolution frames and
telemetry are the evidence. `presentation-settings.txt` records D3D12, camera
resolution/settings, mesh/material paths and process memory at each resolution.
For later A/B comparison keep the route, outfit, new-world seed, resolution,
graphics policy and light/time fixed; compare corresponding phases, camera
angles and in-game timestamps, not arbitrary frame numbers.

In the baseline telemetry the Knife is the only carried tool; no held Knife is
rendered. Ordinary berry gathering reaches its authoritative result and recovers
to idle. A **normal mapped Knife low-growth/reed sequence and largest-recipe
capture are still missing**, so OpenSpec task 1.1 remains open. The baseline is
not visual approval. In particular the photographed walking loop, face and
wave edges still need a materially better playable trial.

## Reuse decision

| Route | Rights and access | Compatibility and coverage | Decision |
|---|---|---|---|
| [City Sample Crowds](https://www.fab.com/listings/903037e9-e1ac-4f41-96e8-1683c6fa7ad4), Epic Games | The **item's own public metadata** advertises USD 0, stock availability, and "UE-Only Content - Licensed for Use Only with Unreal Engine-based Products". Unreal use is plausible, but neither a direct package/download without an account nor actual entitlement was established. No account was created and nothing was downloaded. | The description says fully rigged MetaHuman-adapted heads/bodies, six body types and a mix-and-match city wardrobe. This does **not** establish UE 5.8 import, our 53-bone clip retarget, existing tunic/apron/shoe fit, wave/bob/ponytail and dyes, or per-character in-woodland render cost. Epic's [modular-character documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/working-with-modular-characters-in-unreal-engine) warns Leader Pose can be expensive on the render thread; it is not a performance measurement of this pack. | **Reject for this trial**: account/access and wardrobe/rig/performance compatibility are unverified. A $0 listing or attractive thumbnail is not an imported character. |
| Already acquired MPFB 2.0.17 + MakeHuman system assets | [Bundled license](../../../Assets/Characters/Source/Licenses/LICENSE.md) sections C-D states mesh, targets, textures, rigs and output are CC0; [provenance](../../../Assets/Characters/provenance.json) pins the exact official archive, hashes and local tool installation. No new account or asset acquisition. | Existing UE 5.8.2 imported 54-bone wrapper/53-bone authored skeleton, three adult bodies, three hairstyles, saved material/color selectors and modular wardrobe already run in Shipping. Substantive **new face/body geometry** can retain bind, slots and garment IDs; material/geometry/render cost still need actual validation. The current appearance is *not* acceptable merely because its assets are licensed. | **Accept as isolated re-authoring trial**, not as completed visual replacement. Preserve the old meshes and `inventory-drop-v18` while creating new content paths and ordinary gameplay A/B. |

No purchased assets, live Editor integration, new account, or external model
download is authorized by this decision. City Sample's listing may be revisited
only after access, exact license terms, UE version, retarget, garment coverage
and game-camera cost are established.

## Ownership of overlapping OpenSpec work

`prioritize-heroine-quality-and-tool-clarity` owns the **first character-first
playable slice** and its integrated build: one trial heroine, planted walk/stop,
distinct sprint, held Knife and truthful Knife action, then Craft/hotbar
presentation. This is the implementing owner for overlapping code and assets.
It keeps the final sprint contract (toggle Shift/L3, 300 cm/s, zero
sprint-specific Energy cost, refuse/auto-off at Energy <=10 and no auto-resume)
from `improve-contextual-feedback-and-sprint`; it does not claim that plan's
contextual HUD or full regression/promote tasks. Its sprint tasks 3.1-3.4
remain unchecked pending their own exact evidence.

`polish-locomotion-view-distance-and-time-hud` retains broader directional
turn/slope, time card and distant-world obligations. Its combined first-slice
task 1.2 and walk tasks 4.1-4.4 remain open until their **entire** stated
acceptance passes; world streaming/time card are not prerequisites to this
character-first slice. `refine-equipment-preview-and-idle` retains transparent
paper doll, slot UI and long breathing idle/portrait work; its idle tasks 3.1-3.3
remain open. `refine-playable-heroine-hairstyles` retains rejected wave-end
and complete visual-selector acceptance; its already-completed Bob work is
not undone. None of those checkboxes is inferred complete from a plan or a
partial animation/mesh import. Their design and spec contracts are reused,
not forked into competing implementations.

## September 24 original movement and replacement trials

The existing admitted CC0 MPFB bind now has two additional animation-only
sources: `Assets\Characters\Heroine\Sprinting\AN_Heroine_Sprint.fbx`
(SHA-256 `CD16F467A48397AFD3572EB73D3942B3B2E85465F196268A9E21434A386A34C0`)
and `Assets\Characters\Heroine\KnifeCut\AN_Heroine_KnifeCut.fbx`
(SHA-256 `F44EB0FF193429422526F8A7525FEADB49196B4E4E3228AAD08FA35CBD4F59B0`).
Blender 5.2.1 round-trip evidence under `Build\CharacterPreview` confirms the
unchanged 53-bone bind, closed loops, planted feet, zero net sprint root
travel, 300 cm/s authored sprint stance, 0.9-second cut and 37 cm wrist
travel. Separate Unreal import/reload receipts confirm the same skeleton,
no root-motion extraction and zero notifies in isolated trial namespaces.
These are technical motion checks, not aesthetic approval.

The ordinary mapped Editor `sprint-editor-route-01` held Shift and L3,
covered 297 cm on the sampled walk pass and 759 cm during the three-second
keyboard sprint/diagonal pass, showed the distinct clip in both sprint input
paths, and spent 1.233 Energy (including ordinary survival time). It recorded
79 chronological in-world frames; its screenshots are not an FPS benchmark.
The ordinary creek walk `knife-editor-creek-03` gathered exactly five Fiber,
played one visible held-Knife cut and prevented duplicate gathering. The
controlled hotbar authority fixture `knife-editor-hotbar-01` also cleared
one low patch with the Knife rather than the Hatchet gesture. That second
fixture teleports to prepare its target and is not normal walking footage.

Sculpt01 was rejected because the CC0 face changes were too subtle. A second
isolated `Assets\Characters\HeroineTrials\Sculpt02` source moved 16,704
facial-skin and 37,899 hair vertices (max 1.84 and 8.12 cm respectively)
without changing the bind, material roles or unrelated geometry. It imported
on the admitted original skeleton and clothed Preferred/Bob presentation,
but matched `hero-bob-matched-editor-{720,4k}` versus
`hero-sculpt02-matched-editor-{720,4k}` captures at the same controlled
(-1000,0) portrait site, 280 cm camera arm, 180-degree yaw and daylight
were **not materially different**. Sculpt02 is therefore a rejected
game-camera trial, not a selected heroine.

The separately admitted CC0 MakeHuman `bob02` OBJ and texture were authored
into `HeroineTrials\Bob02`, with original nonhair faces and rig preserved.
Its neutral/tintable hair texture, nine-role mesh and shared skeleton imported
into `/Game/Trials/HeroineBob02_20260924_03`. Matched 720p and 4K Editor
motion/portrait routes passed functionally, but the 4K close comparison
(`files\bob02-4k-face-comparison.jpg` in this session's private artifacts;
raw frames under `Saved\Automation\hero-bob-matched-editor-4k` and
`hero-bob02-matched-editor-4k`) shows the new sweep covering an eye and
bright hair-card gaps. That candidate also fails the more attractive
straight-bob gate and remains unselected. The current Bob actually exposes
the face more clearly.

The Sprint/Knife source pair passed a genuine offline `-SkipZenStore` Cook
through the existing native commandlet and root-only guard; exact cooked
packages, clean exit and separate loose UAT stage are recorded at
`Saved\Automation\character-sprint-knife-cook-01` and
`Build\Releases\20260924-character-first-03`. A packaged mapped 720p
Sprint route and a 4K screenshot-independent travel route passed held
Shift/L3, distinct clip weight, stationary non-drain, menu cancellation and
save/reload. The latter captured portrait/walk/sprint illustrations only
after measuring movement, so screenshot stalls were not misreported as
walk speed; no GPU/Present/VRR timing is claimed. **Neither the liked
replacement heroine nor complete outfit/body parity is accepted yet.**
`inventory-drop-v18` remains the selected fallback.

## September 24 integrated character-first Shipping candidate

`Build\Releases\20260924-character-first-07\Windows` combines the cooked
Sprint/Knife clips, icon-led Craft requirements, earned hotbar and staged
woodland changes. Its staging receipt pins the native Shipping executable to
SHA-256 `8B756CFA326C120A5E743D799A5CF31D1F9DBAA9D01116860B16B5CBD4B8A870`;
it is an immutable, unselected candidate, not a replacement heroine.

The fresh isolated `Saved\Automation\character-first-07-*` routes passed
720p/4K Craft, hotbar, creek, native menu, mapped Sprint, and generated-world
crossing. Both hotbar routes confirmed that an active Knife cut is stopped
immediately by a *real menu press*, without later replay; the 4K QA fixture
does not perform screenshot readback between the action and that input.
The ordinary creek routes gathered exactly one Fiber yield with a held
Knife-cut gesture and preserved depletion through save/reload. Both Craft
routes completed six hold-to-craft recipes and the controlled full-pack
capacity blocker, while focus revealed Fiber sourcing. The full 720p
survival loop and independent-process generated-world and native wardrobe
save consumers also passed. The screenshot-independent mapped Sprint
comparison covered 324/323 cm walking and 722/694 cm sprinting in the
720p/4K captures respectively; its later 4K illustrative frames are not
cadence evidence. Generated-world crossing sampled 29.644/29.616 ms longest
game ticks, with no tick above 33.3 ms on those two routes. These are
offscreen game measurements, not measured display Present or universal
hitch freedom.

None of the above shows a materially more appealing heroine or repairs the
old walk, and Sculpt02 and bob02 failed the appearance gate. The selected
`inventory-drop-v18` remains intact. Tasks 3.1-3.3 are checked on exact
Craft/hotbar behavior; the trial-character, gait, full art parity, aesthetic
review and promotion tasks remain open.

## September 24 fitted CC0 re-authoring (rejected)

An additional `HeroineTrials\Reauthored03` pass used the admitted MPFB adult
target stack rather than deforming only the skin vertices. Its eyes, brows,
hair, modest base and all four modular garment pieces were fitted against the
same original 53-bone bind; the largest regenerated joint displacement was
1.86 mm. A separate clean Unreal import and reload verified seven trial
components under `/Game/Trials/HeroineReauthored_20260924_06`, without
overwriting the normal heroine. The trial is opt-in through
`-HomesteadHeroineTrialReauthored03` in the **Editor only** and supports the
Preferred body; switching to another body while the flag is active explicitly
rejects the unsupported fit instead of silently showing the old character.
`Preview.json` and the `character-first-07` Shipping package do not contain
or select it.

The first `hero-reauthored03-editor-{720,4k}` comparison exposed the
underlying older bob mesh covering one eye; that was not a valid
same-hairstyle face comparison. A bounded correction reused the admitted
selected-Bob geometry and neutral texture, changing no non-hair polygons
(maximum FBX round-trip difference `6.34e-8` m). The corrected
`hero-reauthored03-bob-parity-4k` matched the earlier
`hero-bob-matched-editor-4k` Preferred/Bob/tunic/shoes, woodland portrait
site, camera distance and lighting. Its walk/sprint, menu cancellation and
save/reload worked, but the face and clothed silhouette were **still too
similar** to be a meaningful appearance replacement: re-authoring moved
face vertices only 2.02 mm RMS and at most 6.68 mm. It retains the same
unconvincing underlying skin/eye treatment and old walk. This is a
**rejected appearance trial**, not proof of task 1.4, 2.1, 4.1 or Jenny's
approval. Another small CC0 morph pass is unlikely to clear the face/body
gate; a genuinely different licensed base or materially reworked modeling,
skin, hair and gait is needed before promotion.

## Replacement access and grounded-walk follow-up

The public Sketchfab API reports [Biviyt's Realistic Female](https://sketchfab.com/3d-models/realistic-female-b0cc2a6c26114da184252e433cf75d23)
as CC BY-SA and downloadable, but the official unauthenticated download
endpoint returned HTTP 401. Its store image also shows a T-pose in a modern
cropped tee and shorts, not our wardrobe coverage or proven quality in game.
[The MetaHuman-rigged base](https://sketchfab.com/3d-models/metahuman-female-rigged-be92d09cb91b421babf7cb7f4dc26af1)
is CC BY but explicitly has no textures. Neither is an approved accessible
replacement. [Blender Studio's Storm rig](https://studio.blender.org/characters/storm/v1/)
requires Blender 5.0 and its page gates downloadable assets behind login;
the asset embeds auto-executing Python. Do not source Epic City Sample or
MetaHuman files from an unlicensed mirror. The public
[Ready Player Me animation library license](https://github.com/readyplayerme/animation-library/blob/master/LICENSE.md)
explicitly restricts animations to Ready Player Me avatars and forbids
redistribution, so its walk clips cannot be used for this heroine. Search
summaries claiming account-free downloads or unrestricted reuse did not pass
the actual source terms and endpoints.

An original `AN_Heroine_GroundedWalk02` was authored in isolation at
`Assets\Characters\Heroine\WalkRevision` with a longer 0.833-second cycle,
180 cm/s planted stance, greater arm/torso motion, and unchanged 53-bone
bind. Blender FBX round-trip measured 0.0021 cm/s maximum stance-velocity
error, 0.000041 cm vertical stance drift and a zero-error loop; the new
Unreal clip was imported and reloaded under
`/Game/Trials/HeroineWalk_20260924_07`. `-WalkRevision` on the Editor visual
route opts in without changing the selected or staged Shipping clip. The
`hero-walkrevision-editor-720` and
`hero-walkrevision-reauthored03-720` ordinary mapped woodland routes
confirmed walk/sprint, Energy, menu and save compatibility. Further
chronological `hero-living-idle02-editor-720` telemetry exposed an
unacceptable stop: the longer-step walk's support toe slid almost
30 cm over approximately 0.19 seconds on release while blending to idle,
versus roughly 24 cm for the already unsatisfactory incumbent.
It also retained a stiff upper-body impression. Walk02 was **rejected**;
its runtime opt-in and UE imported package were removed, retaining the
original Blender/FBX source and test record as diagnostics. Task 2.1
is not complete.

The default Editor 720p hotbar route now exercises 20 cm-scale
noncolliding hand attachment, hover/selection, controller LB/RB, chest
storage/retrieval, exact drop/pickup, save/reload, and actual hair/body
asset replacement with the Knife hidden in the book and rebound once on
return. The real mapped creek route at 720p and native 4K captures
`creek-knife-gesture.png` after the authoritative Reed yield while a
Knife-specific cut is active. Mapped Ctrl+wheel briefly changes normal
gameplay camera distance for this close view, then returns it; neither
player position nor time is faked. At 4K the held small Knife and complete
character are visible in the ordinary woodland frame. The target is
depleted exactly once; invalid follow-up and canceled menu actions have
no extra yield or replay. Tasks 2.3 and 2.4 now have their own exact
presentation/authority evidence, but this does **not** approve the face,
the walk, or the overall heroine trial.

## Further September 24 art gates

`WaveEnds02` retained the original CC0 crown and replaced the old
high-frequency jagged lower clipping contour with a smoother, staggered
end. Its original bind, non-hair mesh/materials and real neutral texture
survived Blender and fresh Unreal import/reload. Preferred/LongWave
ordinary woodland routes passed at 720p/4K, but matched moving 4K views
still showed a broad solid wedge rather than the requested loose
mid-back waves. A second isolated `WaveStrands03` experiment preserved
the same crown and added 320 tapered lengths; it loaded and moved in
ordinary 720p gameplay, but became a bright, wiry mass against the
backlit woodland. **Both are rejected visual trials**, not approved
hairstyles or all-body wardrobe parity. The selected hair is untouched.

`Face04` moved 12,036 admitted face/brow vertices (maximum skin
displacement 1.92 cm) on the original bind and current selected Bob;
the saved original materials and non-hair wardrobe remained compatible
through clean UE 5.8 import/reload. Its real 720p/4K mapped motion and
same-position 4K woodland portrait matched the baseline. The more
pointed chin and changed brows made the eye/skin treatment less
convincing, not more attractive. **Face04 is also rejected**. These
results rule out another trivial morph, crop or alpha-card-only pass as
evidence of a suitable replacement. Tasks 1.4, 2.1, 2.5, 4.1-4.3 and
5.2-5.3 remain open: import, rig fit and numeric vertex differences
are not Jenny's visual approval.

The official [3D Scan Store free-head page](https://www.3dscanstore.com/blog/Free-3D-Head-Model)
states *personal use only; contact the vendor for commercial use*,
so its realism does not make it admissible to a distributable game.
Other account-free search claims failed their actual access/license
checks above. A separately licensed game-ready realistic head/body,
or substantial artist-quality original modeling with matching
skin/eye/hair work, is now the prerequisite to the appearance gate.

An isolated skin-material reuse pass sourced TextureCan
[Skin 0001](https://www.texturecan.com/details/574/), whose
[official terms](https://www.texturecan.com/terms/) grant CC0 commercial
and project redistribution. The exact 2K archive/normal/roughness hashes
and license are pinned in
`Assets\Characters\HeroineTrials\Skin04\source-license.json`; only decoded
image/text files were extracted, not executable scripts. A duplicate
Default Lit skin material combines a faint DirectX pore normal and bounded
roughness on the admitted Bob mesh; the imported isolated Unreal mesh
`/Game/Trials/HeroineSkin_20260924_14` reloaded with the same rig and
untouched eyes/hair/wardrobe. Matched 720p/4K actual-world views looked
slightly less glossy, but retained the same unappealing face and hairstyle.
It is a **material experiment**, not a heroine acceptance. Earlier incomplete
imports were discarded during testing. Following the failed art comparison,
the imported Skin04, Face04,
Reauthored03, WaveEnds02 and WaveStrands03 Unreal packages and their runtime
opt-in flags were removed too; the Blender/FBX source recipes, import
receipts and raw test captures remain as local diagnostic evidence. No
rejected look is selectable in normal gameplay. The unmodified selected
preview remains the rollback.

The original, opt-in `AN_Heroine_LivingIdle02` is a five-second CC0-bind
breathing/weight-shift loop. Blender export/reimport measured zero root
travel and exact planted toes, 1.32 cm head and 2.76 cm hand travel;
ordinary mapped Editor gameplay at 720p recorded **zero sampled
world-space toe drift** across both six-second idle passes, before and
after walk/sprint and save/reload. The first authored version had 1.6 cm
world-space foot glide despite a Blender-only zero-drift result; it was
replaced on a fresh clip path after eliminating root sway. That earlier
failure remains in `Saved\Automation\hero-living-idle-editor-720-r2`.
The native portrait now plays its own idle instead of following the
paused gameplay pose, rebinds garments, preserves its animation phase on
refresh, and captures only while Inventory/Appearance is visible. The
normal and five-second trial clips completed 720p and 4K native-menu
routes while Simulation state remained byte-identical during a timed
portrait loop; 4K Editor menu p99 measured about 22 ms versus about
21 ms in an earlier matched Editor run. This is a real but unselected
motion/preview improvement, not a visually accepted replacement,
transparent portrait or completed walk/idle OpenSpec gate.

That motion is now genuinely cooked and staged with the previously
validated Sprint and Knife clips in
`Build\Releases\20260924-character-first-08\Windows` (native Shipping
SHA-256 `3813F16FA504175FEABE1A2160C30AD8F7AB15CE8225473600D2549306D40FDF`).
The new clip is part of the default character and independent paused-menu
portrait in this **unselected** candidate; the rejected Walk02 and art
trials are not present. Mapped 720p/4K Shipping idle/sprint plus full
five-second Inventory phase, native wardrobe, exact Knife/chest/drop and
airborne/reserve checks, ordinary creek Knife/Fiber, generated-world
producer and independent consumer, six-recipe Craft, full survival loop,
wardrobe consumer and normal non-QA startup passed in fresh isolated
outputs under `Saved\Automation\character-first-08-*`. At native 4K
Shipping-menu p95/p99 sampled 17.52/17.92 ms against 17.52/17.93 ms in
the package07 4K menu route, with screenshot readback excluded. The
woodland crossing stayed under 33.3 ms in the mapped 720p/4K routes.
This proves gameplay integration, not a prettier face, natural accepted
walk, transparent portrait or Jenny's visual approval. `Preview.json`
continues to select `inventory-drop-v18`.

## September 24 licensed Vitruvian face trial

The public official [CharMorph Vitruvian character](https://github.com/Upliner/CharMorph-Vitruvian)
declares its model data CC0 in `config.yaml`, with the original author's
relicensing permission recorded in its README. Exact revision, source hashes
and handling are pinned in
`Assets\Characters\HeroineTrials\Vitruvian01\source-license.json`.
Only the public model and its official texture tiles were used: the separate
CharMorph add-on was not installed or executed. Blender loaded the model with
auto-execution disabled, removed its two embedded Python rig-UI texts, and
fitted a different face, eyes and neck to the admitted 53-bone rig while
retaining the existing clothed Preferred/Bob body. The trial is opt-in,
currently `/Game/Trials/HeroineVitruvian_20260924_25`; it is **not in
character-first-08** or the selected preview.

The initial imported neck tiles left conspicuous brown/pale shoulder
patches. Narrowing those surfaces exposed holes instead. The latest
fit retains all 318 licensed adjoining skin faces, projects their UVs
onto the actual admitted MPFB skin texture, and fits 196 outside neck
vertices up to 2.22 cm back onto the existing body contour. It neither
relights the defect nor swaps to a featureless flat neck material. The saved
blue/green/hazel/grey eye selector now colors the new irises without changing
the incumbent eye material. A fresh UE 5.8 import and post-cleanup reload
verified the original
bind, eleven used material roles and 161.86 cm bounds. Its ordinary mapped
4K gameplay and controlled in-world portrait are under
`Saved\Automation\hero-vitruvian01-fitted-neck-editor-4k`;
`files\vitruvian25-vs-original-matched.jpg` in this session's private
artifacts compares the face against the same original-Bob woodland
portrait. The later packaged 720p run below exercises the same
geometry chronologically. Mapped Shift/L3 movement, save and reload
passed, but the routes are **not** display-Present measurements.

This face is now in the **separate unselected Shipping playtest**
`Build\Releases\20260924-character-first-09\Windows`, executable
SHA-256 `B8437A227401C01E1D90A5F76A3B5CABBA1708E4D67A61F625F9688DF1866289`.
The guarded Cook included all 12 trial-specific assets. Its fresh
`Saved\Automation\character-first-09-face-sprint-{720,4k}` captures exercise
the actual packaged face with two equipped garments, normal mapped
Shift/L3, save/reload and portrait at 720p/4K. A separate normal,
non-QA Shipping process reloaded the Bob/face save in
`character-first-09-normal-face-consumer`. That first Shipping face
stage is retained as an immutable comparison, but `Try-HeroineTrial.cmd`
now selects the newer character-first-10 below. Its separate test profile
defaults a new world to the only supported Preferred/straight-brown-Bob
combination; the older character-first-08 trial and selected Preview
remain separate.

For a bounded cost comparison, the **same Shipping executable** ran the
original Preferred/Bob and trial25 Preferred/Bob in four fresh 720p/4K
`character-first-09-performance-{original,face}-{720,4k}` outputs.
Only their `timing-*` phases, before any screenshot request, contribute
to the measured actor-tick wall intervals:

| Resolution | Original Bob p95 / p99 | Trial25 p95 / p99 | Samples |
| --- | --- | --- | --- |
| 720p | 17.494 / 17.956 ms | 17.505 / 17.930 ms | 1197 / 1199 |
| 4K | 17.593 / 17.978 ms | 17.653 / 18.097 ms | 1199 / 1199 |

All four runs had exactly one tick above 33.3 ms; their longest ticks
were 92.601/69.430 ms (720p original/trial) and 73.549/78.672 ms
(4K original/trial). The route starts at the same point, ends within
about 2 cm, and uses the same graphics policy. These offscreen game
timings are *not* GPU cost, physical scanout or evidence of universal
cadence; they isolate the current face's measurable same-package cost
better than comparing an old Editor build with a new Shipping package.

At ordinary distance the new face is materially different and the
large shoulder patches are gone. A faint transition remains just under
the neck in close 4K daylight, and the original rounded Bob, unsupported
body/hairstyle combinations, and skidding incumbent walk remain.
The measured old-to-new open boundary was up to 1.27 cm in trial 21;
restoring licensed skin coverage alone caused pale patches until the
outer neck was fitted to the actual old body. Trials 16-24 were
removed after their evidence was saved; only 25 remains installed.
No import, passing gameplay check or still frame proves Jenny likes
the look. This is **not** an accepted heroine, completed OpenSpec
art task, or reason to change `Preview.json`.

## Isolated planted-stop investigation

The default one-second walk still blends directly to a different resting
foot stance, causing visible stop skate. An **Editor-only, opt-in** animation
proxy trial (`-HomesteadTrialFootLock`, or the isolated face flag in
the current Editor build) now uses Unreal's existing
component-space two-bone IK and foot-rotation controls on the current
53-bone skeleton. It traces actual ground below both feet on movement
release, captures grounded ankle positions/orientations and holds the
planted foot (or both planted feet) while a raised foot finishes its
step and the ordinary locomotion-to-idle
blend completes; it does not move the actor capsule, award resources or
change the default/Shipping character. A teleport, lost ground contact,
or resumed travel clears the anchors.

Same-geometry 720p Editor routes with and without that flag are under
`Saved\Automation\heroine-footlock{01,02,control}-editor-720`.
In the `footlock02` route the two toes moved 1.36/1.52 cm horizontally
and 0.23/0.32 cm vertically from walk release through stopped idle;
the unmodified route moved 16.71/14.30 cm horizontally. After sprint
release they moved 1.37/1.44 cm horizontally and 0.30/0.00 cm
vertically, versus 20.08/22.85 cm horizontally without the trial.
The capsule naturally traveled about 1.44 cm during each measured
braking segment in both runs. A mapped 4K woodland trial also passed.
Earlier position-only IK (`footlock01`) lifted a toe 9-11 cm. Rotation
preservation in `footlock02` stopped that particular snap but could
still *hold the other toe in midair* at a later stop (about 12 cm
above its usual idle height); it was **not** an accepted fix. The
later `heroine-contact-stop-editor-{720,4k}` and
`heroine-dual-support-editor-720` runs instead distinguish swing
from grounded feet using actual world collision. On the 720p contact
route both toe XY displacements were 0.34/2.87 cm on walk release
and 3.50/0.65 cm after sprint release; the capsule traveled normally,
and one airborne foot was allowed to finish its step. Do not compare
toe Z against the *capsule* alone on a slope: feet occupy different
world XY locations, so local terrain clearance is the contact test.
A chronological first-iteration sheet remains at
`files\footlock-stop01-comparison.jpg`.
This still needs start/turn/slope and close-foot visual review and a
better walk cycle before task 2.1 can be marked complete. It is not in
the playable `character-first-09` package.

The current opt-in manual build is instead the newly cooked, immutable
`Build\Releases\20260924-character-first-10\Windows`, executable SHA-256
`70FFC16FC08B3B01194F1172BC563CC63409F574F1A952B56EEE9C9D34D935C9`.
`Try-HeroineTrial.cmd` hash-checks this stage and isolates its test save
and settings from the selected Preview. The native 720p/4K
`character-first-10-face-footstop-*` routes again completed mapped
Shift/L3, menu cancellation, the clothed portrait and normal save/reload;
`character-first-10-normal-face-consumer` separately loaded a real
face/Bob save in a normal non-QA Shipping process. On the sampled 720p
walk-stop interval the unselected package09's two feet moved 19.44 and
16.64 cm horizontally, while package10 held its planted foot within
1.48 cm and let the other descend and complete a 14.08 cm step.
Following sprint release, package10 moved the two toes 1.13/3.15 cm
horizontally versus package09's 20.56/12.43 cm on its sampled route.
Foot clearance must be interpreted against each toe's local terrain,
not the capsule center on a slope. These different chronological
passes are informative, not perfectly phase-matched visual approval.
The old walk pose and rounded Bob still need substantive re-authoring.

The **combined ordinary input sequence** now exercises one genuine
fresh-world Preferred/Bob face25 route without player teleport or
world-inventory injection: walk to the creek, hold mapped gamepad L3
through a grounded crossing, release, approach naturally spawned
Flowers, select the carried Knife by slot one, clear that low growth
once, approach naturally spawned Reeds, gather exactly five Fiber with
one distinct Knife cut, stop, save and reload the depleted bank. A
later Hatchet recipe feasibility check uses an isolated Simulation
clone; it is **not** an injected item or an actual-world craft.
The full 720p/4K Editor routes are
`Saved\Automation\heroine-sequence-evidence-{720,4k}`. Their original
`heroine-sequence-clear.png` and `creek-knife-gesture.png` images are
ordinary in-world full-body captures, not Blender stills. The selected
`inventory-drop-v18` baseline had no distinct held Knife, sprint or
this Reed-specific gesture; current route output cannot be treated
as a phase-for-phase old-build performance comparison.

| Integrated observation | 720p | 4K |
| --- | --- | --- |
| Crossing X after held L3; Energy spent | 1783.50 cm; 0.502 | 1784.31 cm; 0.502 |
| Real Clear cut phase / weight | 0.300 s / 1.000 | 0.302 s / 1.000 |
| Real Reed cut phase / weight | 0.400 s / 1.000 | 0.412 s / 1.000 |
| Right hand to actual held Knife | 5.54 cm | 5.54 cm |
| Left/right toe clearance at Clear | 3.74 / 1.00 cm | 3.72 / 1.00 cm |
| Left/right toe clearance at Reeds | 1.00 / 3.06 cm | 1.00 / 3.02 cm |
| Actual camera distance Clear / Reeds | 470 / 310 cm | 470 / 310 cm |

The mapped route also verified that Flowers were removed only once,
the cut did not replay, Reed stubble and exactly five Fiber persisted
through save/reload, the crossing retained grounded collision, and
the heroine/garments shared the admitted rig. This is task **2.5**
evidence, not Jenny's approval of the face, walk, hair, all-body
parity or selected-preview promotion.

The same 720p/4K screenshot-free `PresentationDiagnostics` route was
run again against immutable package10's face/foot-stop build:

| Resolution | Package09 face p95 / p99 | Package10 face + foot stop p95 / p99 | Timing samples |
| --- | --- | --- | --- |
| 720p | 17.505 / 17.930 ms | 17.505 / 18.036 ms | 1199 each |
| 4K | 17.653 / 18.097 ms | 17.507 / 17.929 ms | 1199 each |

All four routes have one game tick over 33.3 ms; this limited
offscreen actor-tick comparison does not measure physical Present,
GPU duration, or guarantee hitch-free play elsewhere.

Walk02 was retested on the fitted head **after** the stop-foot correction
and with its proper `speed / 180` play rate (the incumbent's authored
stance is 120 cm/s, Walk02's is 180 cm/s). The isolated
`Saved\Automation\heroine-longstride-footstop-editor-720` route passed
normal mapped walk/sprint/save; two genuinely full-speed planted-stance
samples moved a toe about 9-12 cm/s in world space, rather than the
much larger misleading low-toe figures that included swing and braking.
Its 720p chronological comparison
(`files\walk02-fixedrate-vs-original-720.jpg`) still looks much like
the incumbent with the same upright/wooden upper body. Correcting the
stop and stride rate did not meet the visibly more human walk gate;
Walk02 remains rejected, with no packaged selection or task 2.1 claim.

## Licensed human-motion alternative (still an art trial)

The official [CMU Graphics Lab mocap FAQ](https://mocap.cs.cmu.edu/faqs.php)
explicitly permits the motion capture data to be copied, modified or
redistributed without permission. Subject 7's official walk index
describes motion 01 as `walk`, 04 as `slow walk` and 12 as `brisk walk`.
We downloaded only their text `.amc` files and the associated `.asf`
directly from `mocap.cs.cmu.edu/subjects/07`, without an account,
mirror, downloaded script or external converter. Exact text hashes and
URLs are in
`Assets\Characters\HeroineTrials\CMUWalk01\source-license.json`.
The initial copy of motion 12 left both toes aloft during part of a
brisk stride, so **it was not chosen** for the real-game trial.

A small project-authored ASF forward-kinematics/retarget recipe at
`Scripts\Characters\trial_cmu_walk.py` maps the complete official 01
and 04 walk cycles to the admitted in-place 53-bone heroine, preserves
the existing animation/wardrobe skeleton, and normalizes ground contact
against its heel or toe. The normal capture moves about 138 cm in one
second at its native recording rate; its authored low-contact median
was about 112 cm/s on the first retarget; adding the actual recorded
heel-to-toe roll raised the authored median to about **129 cm/s**, so the
**isolated** full-speed
Editor trial uses the corresponding velocity-scaled play rate rather
than pretending it matches the incumbent 120 cm/s stance. Its looping
source segment closes in the authored FBX with zero sampled toe seam;
the latest recipe uses actual pelvis orientation, sideways sway and
foot pitch from the recording. Its maximum 5.3 cm vertical root
correction and approximately 9.6 cm lifted-toe range are measurable
visual-review concerns, not quality passes.

Both resulting clips imported on the existing UE skeleton into
`/Game/Trials/HeroineCMUWalk_20260924_03/Animations` and independently
reloaded at the expected 1.067 and 1.433 seconds with root-motion
extraction disabled and no gameplay notifies. Mapped 720p/4K
`Saved\Automation\heroine-cmu-walk03-editor-*` passes exercise a real
clothed Preferred/Bob walk, Sprint, stop, portrait and save/reload.
The earlier `_01` and `_02` source/visual comparisons were retained,
but their imported Unreal packages were removed; only `_03` is current.
The captured gait
has different leg, arm and pelvis timing, but at normal camera distance
it has **not** yet demonstrated the visibly more human start/turn/slope
and garment/foot behavior needed for OpenSpec task 2.1. This motion
is **not** in `character-first-10`, the selected preview or any
default launch. It was separately cooked and staged into opt-in
`Build\Releases\20260924-character-first-11\Windows`, native Shipping
SHA-256 `13EA68169B0A90163B1E4F152C4D9E3D8EDD2EB2466D878D891E793FA1FECFB5`.
`Try-MotionTrial.cmd` is a hash-checked launcher with independent
`Saved\PlaytestProfiles\cmu-walk03`; the earlier face-and-original-walk
comparison remains accessible through `Try-HeroineTrial.cmd`.

Fresh `Saved\Automation\character-first-11-combined-{720,4k}` real
Shipping routes walked, held L3 to sprint, cleared actual low growth
with one attached-Knife cut, reached Reeds for exactly five Fiber,
saved and reloaded. Both used the cooked CMU normal walk; sampled
phase, 5.54 cm right-hand-to-Knife spacing, 0.502 Energy spent on the
crossing, foot/ground clearance and camera distances were recorded
at each work gesture. The 720p/4K `character-first-11-cmu-motion-*`
and same-binary `...original-motion-*` routes also exercised normal
walking, sprint, stopping and portraits. A **separate normal non-QA**
process loaded the actual face/CMU test-world save.

| Same Shipping11 route, before screenshot readback | Original walk p95 / p99 | CMU walk p95 / p99 |
| --- | --- | --- |
| 720p | 17.697 / 17.931 ms | 17.670 / 17.948 ms |
| 4K | 17.621 / 17.987 ms | 17.558 / 17.908 ms |

All four timing passes have approximately 1,199 samples and exactly
one sampled tick above 33.3 ms. These are offscreen actor-tick wall
intervals; they do **not** measure actual display Present, GPU work or
personal play feel. Import, cook, gameplay and near-equal timing do
not mark the walk aesthetically accepted, complete task 2.1, or
permit promotion.

### Slow/full blend and terrain-aware stop (Editor review)

The official subject-7 **slow** clip is now a separate in-place pose
at half-stick speed, phase-linked to the normal walk rather than
starting an unrelated cycle during speed changes. Its measured
low-contact stride is about 88.8 cm/s versus 129 cm/s for normal
walking; both use actor-velocity-calibrated playback. This affects
only the opt-in CMU trial, not the original-gait comparison or
`inventory-drop-v18`.

An initial chronological slow/stop test exposed a serious error:
a root/tree Visibility trace could be mistaken for the ground,
pulling a planted toe roughly 55 cm upward. The stop now references
the generated terrain height under each foot and admits the current
walkable movement floor only when it is genuinely elevated. The
focused mapped `Saved\Automation\heroine-cmu-slowblend-floor-720`
route records 25 slow, 17 full, 18 moving-turn, and 16 stop frames;
zero-speed toe movement after the slow stop is 0.56/5.24 cm
including the swing foot's landing. A sparse actual 3840x2160
review (`heroine-cmu-slowblend-floor-late-4k`) confirms slow/full
weights of 1.0/0.0 at 67.5/179.9 cm/s, and captures both stops
without imposing screenshot readback on every travel frame.
The opt-in original-gait mapped Sprint regression
(`heroine-original-floor-control-720`) and CMU Sprint regression
(`heroine-cmu-slowblend-floor-sprint-720`) both passed mapped travel,
menu cancellation, portrait, and save/reload. The unmodified-gait
creek route (`heroine-slowblend-floor-creek-720`) still gathered
exactly five Fiber and reloaded its depleted bank.

The 4K sampled stop was near a tree that shortened the camera arm,
and sparse stills cannot validate chronological quality or physical
frame presentation. A raised swing foot still needs its short
landing step. **Task 2.1 remains open** pending side-by-side
moving/turning/slope visual judgment against the selected build.

## Jenny's trial review: reject face, preserve motion and UI separately

After playing the face and motion trials, Jenny rejected the Vitruvian
face/neck seam **outright**. The old `Try-HeroineTrial.cmd`,
`Try-MotionTrial.cmd`, and `Try-SlowMotionTrial.cmd` remain immutable
historical A/B paths, not recommendations or appearance candidates.
(2026-09-25: the MetaHuman heroine replaced these trials, so the four `Try-*Trial.cmd`
launchers and their `Start-/Stage-/Cook-CharacterTrial.ps1` scripts were removed. Recover them
from git history if an A/B comparison is ever needed again.)
The CMU walk's arms and legs feel better to her, but the head tilts
back when moving. She also preferred the trials' menu scaling. The
appearance and UI judgments are independent: keep the better menu
layout when building the next playable movement trial; do not infer
acceptance of the rejected face or of the entire walk.

Offline measurement of the admitted slow/normal retarget showed its
head pointing **27.3-31.3 degrees upward** across both cycles. The
isolated `CMUWalk02` re-authoring corrects the neck/child-head pitch
to approximately **-2 degrees** while retaining the same 88.8/129.2
cm/s stance speed, zero toe loop seam and untouched licensed source.
Two new clips were freshly imported/reloaded on the existing
animation skeleton under
`/Game/Trials/HeroineCMUWalk_20260924_04/Animations`;
the `_03` clips and older Shipping binaries were not overwritten.
The original face, all three body/hair/owned clothing selectors, and
the opt-in stop are separate from this motion asset. Fresh normal
720p/4K mapped Editor slow/full/turn captures are under
`Saved\Automation\heroine-original-face-headlevel-{720,4k}`. Their
side view no longer shows the exaggerated backward pitch, but a
technical correction and sparse stills cannot establish Jenny's
in-motion aesthetic approval. The 720p original-face integrated
Knife/reed/save route
(`heroine-original-face-headlevel-sequence2-720`) passed. The first
attempt failed for a **test assertion** that assumed every combined
route had Vitruvian materials; it now checks the original skin/eye
contract when the rejected face flag is absent.

This is a reversible *movement-only* experiment while replacement
face/body sourcing remains open. Do not mark OpenSpec 2.1, 4.1, 4.2,
4.3, 5.2 or 5.3 complete solely from the head-angle measurement.

## Body, hairstyle and facial-color fit checks

The fitted head source now accepts a specific original modular body/hair
FBX without overwriting the playable Preferred/Bob trial. Four
additional **Editor-only** meshes imported and independently reloaded
under `/Game/Trials/HeroineVitruvianVariants_20260924_01`:
Preferred/LongWave, Willow/Bob, Hazel/Bob and Preferred/Ponytail.
All retain the original shared bind, four corresponding original
body/underlayer/hair material roles and seven licensed face roles;
the 720p/4K `heroine-vitruvian25-*-editor-*` mapped routes exercised
those exact clothed combinations, Sprint and save/reload. The front
face holds up, but Willow/Hazel collars still show a slight transition.
LongWave's lower edge is the same broad sheet Jenny already rejected;
the original Bob still looks round and the Ponytail exposes the
mostly bare brow area. These source/import paths do **not** make all
body/hair/outfit combinations acceptable and are absent from the
immutable `character-first-10` package.
Their runtime selection additionally requires the separate
`-HomesteadHeroineVariantTrials` opt-in; the human-facing face25
launcher does not advertise uncooked body/hair packages or silently
fall back to the old face if a combination has not been fitted.

To address the featureless brow, original physical strand geometry
was fitted to the licensed face in separate trials 26/27. Matched 4K
game portraits exposed thin, ink-like fragmented eyebrow marks,
especially near the old hairline. Both imported meshes were
**rejected and removed**; their source recipes and diagnostic captures
remain. A lower, softly feathered **painted** alternative is now
projected onto the actual licensed facial UV tile by the first-party
`project_vitruvian_brow_mask.py` and
`draw_vitruvian_brow_mask.py` recipes. Its digest-pinned 4K mask
`Assets\Characters\HeroineTrials\Vitruvian01\Brows28\T_VitruvianBrowMask.png`
and an isolated alternative face material have passed a fresh-process
import/reload. Opt-in `-HomesteadTrialPaintedBrows` reuses the *same*
face25 mesh and original eye/neck/skin geometry, blending a natural
hair-color-dependent brow mask into the skin rather than putting flat
cards above it. A matched actual-woodland 4K portrait
(`files\vitruvian-painted-brows25-vs-28-4k.jpg`) is markedly more
coherent than the rejected strands; a 720p dark-hair/Skin 2/Eye 1
motion and save route also passed.

The explicit 4K natural/blue, deeper/green and light/hazel palette
capture (`files\vitruvian-painted-brows-palette-4k.jpg`) demonstrates
that the new brows, skin and eyes truly change. It also exposed
**blocking defects**: the deeper preset has a hard color/shading break
at the old-body/new-neck boundary, green irises are too bright, and
the blonde brows become very faint. These are real in-game quality
problems, not fixed by passing selector/import checks. No eyebrow
trial or additional body/hair fit was cooked or added to the
playable `character-first-10` build, and tasks 4.1/4.2 remain open.

The **whole-body** route was also tested rather than assuming a face
graft is the only option. The official pinned CC0 model's 4K skin tiles
1003/1004 were obtained as EXR images directly from the original GitHub
blob API; their digests, upstream object IDs and sizes are recorded in
`Assets\Characters\HeroineTrials\VitruvianFull01\source-license.json`.
No add-on code or mirrored art ran. An original authoring pass preserved
the existing 53-bone bind, transferred at most four weighted
influences per vertex from nearby original skin (maximum measured
projection 23.71 cm at the differently posed hands), and retained
owned underwear, Bob, tunic and shoes. Full01 omitted a separate
licensed upper-torso skin role and produced an obvious transparent
chest opening in the actual 4K woodland. Full02 included that role
and revealed the new eyes, but still showed an exposed armpit/strap
gap and strong tile-color/shading breaks. The exact 4K comparison is
in `files\vitruvian-fullbody01-vs-02-4k.jpg`. Both imported full-body
packages and their opt-in runtime path were **rejected and removed**;
the licensed source FBX, report and diagnostic frames remain. These
defects cannot be turned into accepted clothing parity by an import
receipt or by hiding them with lighting. The safer head-graft trial
remains the only playable replacement source for now.

A separate neck-normal diagnosis verified why a simple "smooth the
mesh" claim is inadequate. Unreal's default skeletal FBX policy
**recomputes normals**, so a new authoring trial transferred original
body normals onto 196 projected neck vertices and explicitly imported
the FBX with `FBXNIM_IMPORT_NORMALS` into an isolated path. Both the
Blender round-trip and fresh UE import confirmed that policy and the
adjusted corner normals survived. Yet a matched 4K deeper-skin
comparison (`files\vitruvian-neck-normal-deep-25-vs-29.jpg`) still
showed the hard collarbone edge. Normal interpolation alone cannot
solve the material/geometric junction; the trial29 imported package
and runtime flag were removed while its source/evidence remain.
Neither the selected preview nor the cooked manual trials were
changed by this negative result.
