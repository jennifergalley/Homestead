# Farming Fishing measured build 02

Target: `20261003-measured-02`, Oct 3 9 PM local; fallback Oct 4 7:30 AM.
Authorization: `2026-10-04T01:55:51.513Z`, scope source `8b67ebc0`; coordinator's
registry update `5e00cc7f` observed on origin/main. Checkout was complete/clean
at `8b67ebc0`; no reset/restore was performed.

App session `d4518710-b508-4196-8598-1cf91d0edf7d`; runtime
`5be207bc-49b1-4a1b-811e-088ae565dc1b`; branch
`jennifergalley-farming-fishing-agent`; worktree `jennifergalley-cautious-robot`.
Actual model/effort `gpt-6.1-sol` / `high`, confirmed by local usage events
69327-69329. Launch context default; actual runtime context tier unknown.
No helpers, owned Unreal process or automation. Owned visible Blender PID 20420,
live port 9878. No game/save/shortcut touched. Integration reports its combined
editor/game compile and 21/21 native suites passed; original-art and
release-save-isolation admission gates remain.

## Boundaries and selected feedback
Latest authorization: Jenny directly requested continuation even overnight at
22:05 local Oct 3; coordinator explicitly restored hands-on slot2 after checking
other lanes parked. This supersedes the prior parking/no-overnight hold for the
existing fishing/food scope. Jenny subsequently permitted continued Blender work
while she plays. No own Unreal launch, UBT/UAT, automation, helpers, real-save
access or shortcut changes; Integration retains imports and release gates.

| Feedback | Change | State |
| --- | --- | --- |
| `jenny-mut56ggp-uijcta`, `jenny-mut57myz-9tfjci` | `fix-seed-planting-hint-and-harvest-balance` | source implemented; compile/acceptance pending |
| `jenny-mut5gsd4-oebvov` | `add-basic-crop-cookfire-recipes` | source implemented; compile/acceptance pending |
| `jenny-mut592z8-2od0d5`, `jenny-mut5legr-oba2rd` | existing `add-shore-and-river-fishing` | source/native verified; editor compile and acceptance pending |

First delivery: seed hint/balance and eight crop meals. Then fishing pole at exactly
1500 coins beside backpack upgrade, inventory/hotbar, original active fishing,
river/lake/ocean catches, General Store sale and fish preparations.
No carry/grip/hair/foliage/rucksack/manor relocation or speculative scope.

Jenny's Oct 3 20:04 correction requires all-new art, not reused pole/fish/meal
assets: one original pole, six distinct catches and sixteen distinct new dishes.
Existing shared code/material-building helpers are reusable, not existing meshes.
She selected a visible Blender window. Coordinator granted slot2 and the shared
Blender/GPU authoring window; Integration owns Unreal imports/save/builds.
No placeholder promotion or entire 23-item-set promise for 9 PM.

Travel Rest (`1b0e10a4-09b5-458e-b9de-098cce831b07`) owns bed/time/wait/discovery/travel.
Its Simulation State/declarations and optional travel save hook must survive integration.
Our shared edits are localized Recipe/crafting/HarvestCrop, ephemeral fishing reset/probe,
and shop goods/trade only; no wait edits. Fishing adds no save section. Integration/docs liaison:
`e528fd4a-5aed-4c95-9463-a37941afc00b`; coordinator:
`146ed534-2f78-48ba-b0fc-98436c1f3223`.

## Findings and reusable implementation context
- `HomesteadHUD.cpp::DrawInteractCue` retires a lone keyed cue after three uses.
  Fix the actual retirement policy, not only FocusActions' already-present plant string.
- `CheckSow` / `DescribeSow` share eligibility; input E plants, tool click works.
- `CropTable`: only cultivated legacy roots yield separate seeds (two guaranteed);
  period crops already yield no seed packets. Preserve wild forage and regrowing fruit.
- Implemented tuning: cultivated roots one seed at 25%, deterministic plot/day; eight new
  Meals use one kindling and existing fire/hearth checks. Recipe design lists dishes
  and round(12 + 0.6 * consumed sale coins) nominal Energy.
- `SellPrice` equals BasePrice, not a presumed discount. Item/Recipe append-only;
  counted stocks already load narrower saves with new items zero.
- `CraftChange`, `Cooking`, `RecipeName`, cookbook metadata and native fixtures
  iterating every Recipe need coherent wiring. Prefer a small shared recipe helper.
- Existing fishing proposal was a corrupted System.Object[] stub; repaired the same
  change and completed short design/spec/tasks before apply. No duplicate change,
  external assets, dependencies, new animations or editor launch.

## Checkpoint and next action
Coordinator supplied the separate apply instruction after the short plans; both
changes are implemented. Eligible selected seeds use the authoritative DescribeSow
cue and Plant Seeds alone bypasses three-use retirement. Other hints still retire.
Eight append-only Meals share ingredient/output data with crafting and assessment;
the cookbook shows output icons and nominal Energy. No serializer/tag/version/bake
changes; inventories widen through the existing counted-stock format.

The first native pass compiled all selected targets and passed Economy/Manor; its
simulation run exposed a directly coupled old full-pack fixture assuming six root
harvest items. The fixture now tests the actual four-root plus zero/one-seed yield,
including a carried seed for the later replant. The corrected Release native pass
passed Simulation, Economy and Manor (3/3, 210.16 seconds).
Focused new coverage exercises retirement, both seed outcomes/reload, a 1024-sample
20-30% frequency bound, unchanged wild seeds/bought crops, all eight meal ingredients,
kindling/station refusal atomicity, Energy/eating and current/narrower stock saves.
Both OpenSpec changes pass strict validation; risky source wiring reviewed directly.

Jenny's 20:00 local update says she is not playing and permits Blender/Unreal;
the earlier 19:12 playtime compilation hold is superseded. Slot2 now covers original
asset authoring rather than the prior conditional visual-QA assignment. This lane
does not launch/compile its editor; Integration owns the combined current-source
editor and eventual imports. No [ready] or art-acceptance claim before viewed
4K renders, asset wiring and integrated verification.
Never touch Jenny's game/saves/shortcut, no overnight automation.

Native command: CMake Release targets HomesteadSimulationTests,
HomesteadEconomyTests, HomesteadManorTests, then CTest selecting those same suites.
Both strict OpenSpec validations and git diff --check pass. The native Manor target
reported an existing C4456 shadowed-local warning at Tests/HomesteadManorTests.cpp:309;
that unrelated source was not changed. No editor compile has been attempted.

Player checks after Integration clears compilation: select seeds and plant at least
four eligible tilled plots without losing [E] Plant Seeds; harvest roots and see
occasional singles, not guaranteed pairs. Inspect the eight new recipes and their
Energy, cook a herb-seasoned dish beside the fueled fire/hearth, then eat it; each
batch uses one kindling. Jenny's acceptance tasks remain open.

## Fishing source checkpoint
The commit containing this handoff follows pushed seed/crop checkpoint
`249229b0d5c3f072d4fb6384e9ded0cb0f503d9c`. It adds the pole to ordinary General Store
goods at exactly 1500 coins regardless of markup, before or after the backpack upgrade.
Pack layout, hotbar selection, held-tool presentation and the generic cookbook are wired.
That source checkpoint uses the original wooden digging-stick mesh/pose with a
unique pole component. It is now explicitly rejected as final fishing art and
will be replaced by the newly authored FishingPole, not retained as a fallback.

Native fishing owns pole/Energy/space/water eligibility, the 1.5 Energy cast charge,
2-4 second wait, 0.9 second hook window and two 1.8 second landing passes (55-85% band).
E/LMB or A/RT casts/hooks/lands; Esc/B, walking away or changing tool/menu cancels.
Only the final successful beat awards one fish. River trout/salmon, lake perch/carp
and sea mackerel/bass are distinct pools. All sell to the General Store; raw catches
are prepared before eating. Eight fish preparations cover raw sushi-style slices,
three grills, salmon soup, fish/ember potatoes, herbed carp and mackerel/bean chowder.
Seven cooked dishes consume one kindling at the existing fire/hearth; raw mackerel
slices need no station/fuel. Sale-opportunity Energy uses the shared crop-meal curve.

The new scaled Slate card is 520x200 logical units, centred 180 units above the bottom.
Its timing target has contrasting boundary marks and a two-tone marker, not colour
alone. Habitat/readiness are cached on focus refresh, never probed in HUD paint; only
constant-size session state animates per frame. Gallery adds staged bite/landing and
pole-purchase entries. The gallery restores the real water probe immediately after
setting up its staged cast; these entries are not evidence of live-water verification.

Release native Simulation/Fishing/Economy passed 3/3 (177.87 seconds). The final extended
Fishing executable passed 1036 checks, including 1499-coin refusal, exactly-1500 purchase,
pack-row/save retention, all habitats, timing boundaries, bad/late/cancelled casts,
full pack, changed water, load/new-game resets, all eight preparations and actual
uncapped Energy gain. Initial fixture failures were corrected: the all-recipes stock
needed room for Split Firewood after adding six fish; estate load fixtures need the
matching placement context. Gameplay/serializer behavior was not changed for those fixes.
All three selected OpenSpec changes pass strict validation; git diff --check passes.
State-machine/input/recipe/save wiring was reviewed directly; no UBT/Live Coding attempted.

Integration must preserve HUD's removal of obsolete pickup mounting/root cleanup and
SelectHotbarSlot's redundant one-second name toast while retaining FishingRoot and
active fishing's selected-tool presentation. Preserve HUD's QuietActionSerial changes
in BeginHintUse/EndHintUse/NotifyResourceAction: fishing uses that helper for intermediate
success; its card owns progress, while terminal catch/cancel/refusal uses normal notices.
Travel Rest's State/discoveredTravel and independent optional travel save section remain
separate; preserve those localized hunks. Item/Recipe append-only, counted stocks,
SimulationSaveVersion 13 and placement bake unchanged; in-flight fishing is not saved.

This is a source checkpoint, NOT [ready]. Remaining gates: coordinated editor compile,
actual river/lake/sea reach and pole/card inspection, release-specific save isolation,
then Jenny's integrated manual acceptance. Older packages can reject wider stock saves
and fall back to a backup; append-only widening alone is not release rollback safety.
No gameplay admission/promotion until Integration verifies isolated save routing.
Player check: buy/select the pole for 1500, catch at each habitat using the timing card,
sell a catch, prepare/eat raw slices and a kindling-fired dish. No packaging, main merge,
shortcut retarget, live-game/save access, polling or overnight automation by this lane.

## Original-art work in progress
`Scripts/Blender/Recipes/fishing_pole.py` builds a new 1.95 m hazel blank,
tip-fixed wound flax line, carved float and forged hook. Grip pivot at the origin,
tip +Z; no borrowed mesh and no new motion. Geometry currently has 42,488 triangles.
It is built/exported under `Assets/Props/FishingPole`, with 4096px basecolor,
roughness, OpenGL normal, AO and metallic maps. Final 3840x2160 hero/detail
renders (192 samples, RTX5080 OPTIX) and the basecolor atlas were viewed.
Art checkpoint `befe70b52fc593247d55a90de42383293f80dfa3` is pushed. Integration
reports import/current-source held-path fixture at `0d05fa4c`; this is not full
fishing gameplay acceptance.
The recipe reserves four straight shaft UV islands and uses a new linen-cord
material helper. Float follows its leader's curve so it is attached rather than
floating off the line. Clamp the taper input at Blender's float32 endpoints.
For manual geometry-only probes, `build_prop.py` normally supplies `kit.mats`;
set that binding explicitly when calling a recipe without the builder.
Review-only Kloofendal HDRI fetched with the standard MD5-verified CC0 pipeline;
it is cached/ignored and is not a source of shipped geometry/material textures.

First 4K critique found a diagonal material stripe across the float: the shared
`assign_tube_uvs` helper mapped cap-centre vertices to the last body ring and its
fan triangles overwrote body UVs. Corrected the helper to reserve independent
cap disks; `Tests/BlenderTubeUVTests.py` passes capped/uncapped atlas bounds,
nonzero cap area and cap/body separation in the live window without clearing it.
Second viewed pass removes that stripe and widens the detail framing to show
the full float, line and hook. Wood remains matte with longitudinal grain;
thin line, modest natural bow and restrained wear are intentional. Remaining
weaknesses: static secured travel tackle (not a deployed-line animation), no LOD,
and unverified in-engine grip/body clearance. No existing assets were rebaked.

The live preservation comparison appended only owned generated snapshots, but
Blender retained their library records and refused to overwrite the source
`.blend`. Saved a corrected scene copy, removed only those two owned snapshot
library IDs, then rebuilt successfully. Do not remove unrelated library IDs.

Integration's bounded Travel/HUD-only editor window finished and its editor
exited before Cycles resumed. Final rod bake/render finished at ~20:31 local;
Integration was offered a GPU-safe import/viewport gap. Its current-source
compile result predates this checkpoint's new pole path, so it must compile that
localized C++ change and import the asset before grip verification. The old
digging-stick fallback is removed; missing original pole explicitly logs an error.
No native rules, serialization, Item/Recipe widths or versions changed in this
art checkpoint.

Jenny explicitly accepted the tip-line design at ~20:40 local; a reel may be
a later upgrade, not this batch. The supplied 1280-wide Integration side capture
was viewed: pole projects forward, without obvious idle leg/torso collision.
It is too small to establish palm seating/finger wrap/butt-to-forearm clearance;
close palm and front/three-quarter views were requested for the next owned
Integration pass, not an extra editor launch. No speculative axis correction.

Coordinator renewed slot2 for the six-fish family, then sixteen dishes. The new
`caught_fish.py` matches the six catalogue ids to brown trout, Atlantic salmon,
European perch, common carp, Atlantic mackerel and European sea bass. Text
references and representative (not gameplay) dimensions are recorded in the
recipe. Independently lofted bodies/heads/tails, integrated gill creases, lenticular eyes,
ray-supported fins, salmonid adipose fins, four carp barbels and paired mackerel
finlet rows distinguish anatomy, not merely recolouring a shared mesh.
First baked counts were 37,304/41,922/38,673/41,124/42,233/40,488 triangles,
lengths 34/62/30/42/36/46cm. Shared new material helpers author countershading,
species spots/bars/waves, scale relief, membranes and striated eyes.

Integration closed its rod viewport window and released GPU before the first
4096 PBR / 4K 192-sample fish-family bake/render began. The viewed first trout
hero/detail and atlas FAILED the art bar: floating rectangular gill patch,
polka-dot scale pattern, flat pennant fins and button-like eye/front lip.
Remaining owned headless renders were stopped; this is not accepted catch art.
Subsequent passes integrate gills into the body, replace circular scale rims with
staggered shingle seams, add fin striation/bow and unrayed adipose material,
round the muzzle and conform the eyes to the actual curved head surface.
Trout red spots are sparse and region-confined rather than uniform halo dots;
scale spacing uses circumference rather than a stretched sine coordinate.
Ray tubes now follow the membrane bow; a scene-preserving regression verifies
seven rays at eight stations within 2 microns, alongside all six mesh checks.
The final review roll is +78 degrees so the dorsal side appears above the flank;
the earlier negative roll made the belly fins look like dorsals.

Six original catch meshes, one source blend, six FBX files, 24 4096px PBR maps
and twelve viewed 3840x2160/192-sample OPTIX renders are persisted in
`Assets/Props/CaughtFish`. Recipe/FBX hashes, map/render dimensions, render
configuration and warning-free asset receipts pass; all six geometry checks
pass (dimensions, pivots, finite geometry/UVs, species, unique geometry, <=65k).
Final triangles, in catalogue order: 42,058 / 46,584 / 43,273 / 45,726 /
46,825 / 45,076. No native rules, save widths/versions, placements or Unreal
assets changed in this family.

This is a WIP preservation checkpoint, NOT accepted art or [ready]. Viewed
close-ups still look too smooth/graphic: mouths read as drawn smile lines,
heads lack convincing bony/skin detail, and perch/mackerel markings are too
regular. Do not import/admit these as final fishing art or promote them merely
because geometry/receipt checks pass. Next refinement should work on one
species' head, mouth and material realism before another whole-family render.
No dish geometry yet. Remaining art/presentation is at least several further
hours (roughly 4-6+, not a completion promise), before Integration/Jenny gates.

Owned visible Blender remains open with an EEVEE material-preview grid saved
at `E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\CaughtFish-live-review.blend`.
Grid transforms are display-only; never export that scene as production.
Owned obsolete geometry snapshot and exact blend backups were removed.
Viewport failure/fix: builder previews leave BLENDER_WORKBENCH, where MATERIAL
is not a valid shading enum. Switch scene engine to BLENDER_EEVEE first.
New item images and eating portions must still replace generic fish/food glyphs
and existing root/berry eating-prop reuse. This is several hours of remaining
work, not a credible full-set 9 PM delivery; no overnight automation or
continued work after a new explicit stop. The earlier parking instruction was
superseded by the direct 22:05 overnight authorization above.

Post-resume refinement is WIP, not a new art-admission claim. The full six-fish
visible review scene was copied to E: scratch before rebuilding. New source
replaces the external mouth-line tubes with separated upper/lower jaw skin and
recessed oral walls, strengthens cheek/opercular/orbital forms, enlarges the dark
pupil relative to its iris, and varies scale tones and perch/mackerel markings.
Two explicitly PD-self reference photographs (trout/perch, cited in the recipe)
were viewed for anatomy/material observation only; neither is shipped or used
as a texture. `caught_fish_head_review.py` is a draft-only trout/perch subset,
written to E: scratch, never a production import set.
The new source requests a fifth metallic map; committed `CaughtFish` outputs
at `c70b603a` are still the older four-map WIP and do not match these edits.
Do not import them or infer new source/render verification from old receipts.

Six bounded trout/perch passes were built and reviewed at 3840x2160 / 192
samples (OPTIX), first with five baked maps and then directly from source
shaders to avoid repeated draft baking. The latest `fish-head-review-wet`
scratch set pins both anatomy and material source hashes. Source anatomy now
has capped broad snouts, parted lip rims, two opercular/cheek creases, irregular
perch bands, reduced trout dorsal/tail proportions and gold perch pectorals.
Latest joined preview counts are 48,826 / 50,012 triangles. These remain drafts.
Four other species have not received current-source full bake/render review;
the sixteen dishes/portions and item-image/eating wiring remain pending.

The new open-jaw topology exposed a real root defect: volume-based normal
repair inverted the open skin sheets, and eye placement originally followed
the pre-jaw surface. Explicit outer/cavity winding and eyes conformed to the
actual jaw surface fix both. The scene-preserving regression verifies all six
raw jaw models, outward upper/lower skin, recessed noncollapsed oral walls,
closed posterior seams and all twelve pupil clearances against actual body
triangles. Both joined preview meshes also pass exported outward-skin, UV,
dimension/pivot, uniqueness and budget checks; fin-ray attachment still passes.

The plain one-lobe prop shader does not represent wet-skin film independently
of scale roughness. New fish source uses a .65-weight/.06-roughness coat;
`after_bake` restores that film on the texture-backed material. The opt-in
`wet_fish` report field routes only these meshes to a newly duplicated
`M_CaughtFishWet` ClearCoat parent. It never changes the common parent; repeat
fish imports do not recompile an unchanged fish parent. Four offline import
contract tests pass (isolation, idempotence, invalid-data refusal and explicit
conflict/save errors), plus syntax checks. They are NOT a UE shader compile:
Integration recorded the held material gate at `d3ce243e` and owns that check.

Viewed close-ups still read as procedural molded heads/regular scales and
opaque fin sheets rather than the requested naturalistic close-up finish.
Do not treat the wet-film change or passing geometry tests as art acceptance.
Another full-family bake/import would be premature before resolving this
visual-quality direction; the latest two-species review is the bounded evidence.
Current original-art work remains attributable only to build02; captured usage
is 278 calls through event70276, 1,617,143,930,000 recorded nano-AIU, provisional
and incomplete. No model/context change, helpers or reconciled billing claim.

### Autonomous structural proof after the 23:08 continuation
Jenny's direct continuation supersedes the brief art-direction pause. Slot2
and the overnight exception remain active; no approval wait, automation,
helper implementation or owned UE launch was added. Coordinator's one
read-only review of frozen `4148ab7c` supported anatomy-first corrections;
those four earlier views remain untouched in `Refinement`.

New source has crescent opercular/preopercular fields, flatter-sided cheeks,
localized maxillary/mandibular ridges, rounded snout caps, a full curved oral
roof/floor behind rolled lips, fleshy adipose fins and broad bowed pectoral
fans with narrower rooted attachments. Scale overlap now runs longitudinally
and follows local girth, with locally warped cells; pigment size, density,
cluster selection and halos vary. Film remains exactly .65/.06, without a
blanket roughness sweep. Thin-fin welding moved the lowest vertex by 35
microns after the shared kit set the pivot; this recipe now settles the
finished welded mesh and accumulates that offset into review metadata.

`StructuralProof/Source` and `StructuralProof/Baked` preserve ONE current-source
trout, not a regenerated/admissible family. Both are explicitly draft-only.
50,111 triangles; 4.85 x 34.0 x 9.52cm. Source hero/detail, neutral-pigment
geometry hero/detail and five-map baked hero/detail were all viewed at
3840x2160/192 samples/OPTIX with identical existing framing/lighting.
All five 4096px atlases were inspected. The baked film fixture passes;
current-source hashes, FBX/map hashes, sizes, warning-free receipts and
paired frames pass `BlenderFishBakeProofTests.py`. Mean absolute source/baked
differences are 0.071 hero and 0.421 detail in 8-bit levels; this measures
bake fidelity, NOT realism or UE shader acceptance.

All six raw jaw/cavity/outward-skin/eye fixtures and twelve projected-area
pectoral fan fixtures pass; the one joined/baked trout passes pivot, UV,
outward normals and budget checks. Four offline material-import contracts
still pass. No gameplay, saves, enum widths, versions, placements or Unreal
assets changed.

This remains WIP, not [ready]: the viewed jaw is still too wedge-like and the
front chin has an angular transition; the opercular margin and exposed scale
finish remain too regular for the requested close-up realism. Next correction
will use an anatomical lower-jaw hinge/retreat rather than simply inflating
head halves, then repeat the one-trout proof before the full six-fish bake.
The sixteen dishes/portions and image/eating wiring remain untouched and are
still several hours of work. Integration's actual ClearCoat compile/import,
held-pole close grip evidence and gameplay/release gates remain outstanding.
Visible Blender preserves the one-trout baked scene; earlier full-family
and source scenes have named E: preservation copies.

Current build02-only usage is 319 calls through event70353, 1,920,206,500,000
recorded nano-AIU, incomplete/provisional. Actual model remains
`gpt-6.1-sol`/high; launch context default, actual context tier unknown. No
new helper/model segment or reconciled billing claim; later calls need recapture.

Commands already run: git status/log (clean checkout), openspec list/context/spec
inventory, full relevant crop/crafting specs, new change/status/instructions.
Notion identity and Ledger read in full; no technical-project Notion writes.
Canonical startup sources: README.md, round-3.md, agent-lifecycle.md,
measured-build-02.json and full selected backlog feedback.

Usage reference: `accounting/farming-fishing-measured-02-usage.json`, exported from
metadata-only local usage via the adjacent lane allocation. Startup/planning is a
bounded overhead segment, implementation begins at 2026-10-04T02:04:27.787Z.
Fishing implementation begins at 2026-10-04T02:17:28.345Z. Interim coverage includes
source work; terminal response and later calls need recapture.
The adjacent `farming-fishing-measured-02-report.json` is the provisional attributed
aggregate; recorded nano-AIU is not a reconciled billing claim.
Original-art checkpoint snapshot: 140 calls through event 70004, recorded
737049830000 nano-AIU, status incomplete/provisional. Original-art attribution
starts at Jenny's correction `2026-10-04T03:04:19.556Z`; subsequent calls and
terminal response still need recapture. No helpers or model/configuration change.
Fish WIP usage has been refreshed in the adjacent metadata-only snapshot/report;
later calls and the terminal response require recapture. All usage belongs to
build 02, never the closed build 01 allocation. No actual-context
or reconciled-credit claim. TEMP/TMP for heavy tools must use
`E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\tmp`.
