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

Current source checkpoint: original potatoes, roasted turnips, stewed carrots,
herbed broad beans, cabbage/potato stew, berry compote, strawberry compote,
root vegetable hotpot, raw mackerel slices and grilled trout have separate
serving/edible-portion WIP, NOT art/import acceptance. Six fish preparations, all meal
baking/imagery/eating wiring and the held fish realism/bake gate remain. Current
visible review is original grilled trout/portion WIP; recipe/source receipts below are frozen,
not automatically valid against a later edited material library.

## Boundaries and selected feedback
Latest authorization: Jenny directly requested continuation even overnight at
22:05 local Oct 3; coordinator explicitly restored hands-on slot2 after checking
other lanes parked. This supersedes the prior parking/no-overnight hold for the
existing fishing/food scope. Jenny subsequently permitted continued Blender work
while she plays. No own Unreal launch, UBT/UAT, automation, helpers, real-save
access or shortcut changes; Integration retains imports and release gates.

| Feedback | Change | State |
| --- | --- | --- |
| `jenny-mut56ggp-uijcta`, `jenny-mut57myz-9tfjci` | `fix-seed-planting-hint-and-harvest-balance` | native/source implemented; Integration reports compilation passed; manual acceptance not claimed |
| `jenny-mut5gsd4-oebvov` | `add-basic-crop-cookfire-recipes` | native/source implemented; Integration reports compilation passed; original meal art remains |
| `jenny-mut592z8-2od0d5`, `jenny-mut5legr-oba2rd` | existing `add-shore-and-river-fishing` | native/source verified and Integration compilation reported; fish/meal art and integrated acceptance remain |

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
That checkpoint preserved the one-trout baked scene; earlier full-family
and source scenes have named E: preservation copies.

Current build02-only usage is 319 calls through event70353, 1,920,206,500,000
recorded nano-AIU, incomplete/provisional. Actual model remains
`gpt-6.1-sol`/high; launch context default, actual context tier unknown. No
new helper/model segment or reconciled billing claim; later calls need recapture.

### Current held hinge/landmark family checkpoint

Still WIP, NOT [ready] or import/admission approval. The lower jaw now rotates
about its posterior hinge and retreats instead of dropping vertically.
Five predatory species have 160 seated curved teeth in total; carp retains
four surface-seated barbels. Adaptive sampling keeps the 201 body-ring budget
but follows the curved opercular landmark. Denser sampling exposed an inverted
mackerel oral roof: the lining inset now clamps to 45% of local mouth half-width.
Mirrored lateral-line scale coordinates, overlapping relief and granular
head pigmentation are present; the wet film remains exactly .65/.06.

Held evidence: `Assets\Props\CaughtFish\HingedProof\{Source,Baked}` has matching
one-trout source/clay/baked views, five 4096 maps and `comparison.json`.
Mean/RMS 8-bit differences are .084/.579 hero and .544/1.259 detail.
`Assets\Props\CaughtFish\FamilyReview` has the current-source six-family blend,
six FBX, thirty 4096 maps, twelve viewed 3840x2160/192-sample OPTIX views,
atlas overview, report and `verification.json`. Earlier Refinement/StructuralProof
evidence and the stale c70 production-root assets remain untouched.

All six geometry, pivot, finite-UV/outward-skin, hinge/vestibule, pupil,
tooth/barbel-root and pectoral fixtures pass. Triangle counts: trout 51,548;
salmon 57,534; perch 51,674; carp 53,666; mackerel 55,364; bass 54,751.
Both copied receipt validators pass against current source. The portable family
blend was reopened and its 24 material-map dependencies checked; AO is exported
and independently hashed, not a Blender material image node. A first dependency
probe incorrectly parsed Blender's relative prefix as a Windows UNC path;
the new explicit fixture strips that prefix before checking local Textures.
Four offline import contracts and diff whitespace checks pass.

The art bar is NOT met: viewed heads remain too molded/wedge-like, scale
reflectance shows rectangular/checker-like regularity, and fins remain too
opaque/graphic. Correct continuous substrate versus exposed-scale variation
and species head proportions before another family bake; no placeholder
promotion. Sixteen dishes/portions and image/eating wiring have not started.
Budget another 8-12+ hours for remaining art/wiring, not a release promise;
Integration's actual engine/material and playtest gates are additional.

Visible Blender PID20420/port9878 now holds the portable baked six-family
material-preview grid, with a named E: preservation copy. Grid transforms are
review-only and do not affect the saved FBX/production blend. No Unreal launch,
compile, import, gameplay/save/enum/version/placement change, helper or automation.
Build02-only provisional usage: 356 calls through event70403,
2,162,869,550,000 recorded nano-AIU; later calls require recapture.
Actual model/effort remains `gpt-6.1-sol`/high; launch context default,
actual context tier unknown.

### Held thin-fin and dentary checkpoint after 15000b47

WIP only, NOT [ready], import authorization or art acceptance. Trout/salmon/
mackerel skull stations are shortened smoothly without moving posterior fins.
Slender anterior dentaries now blend into the deeper throat; the oral roof
arches upward instead of depressing into the vestibule. Continuous clustered
substrate variation replaces rectangular diffuse/roughness/metallic blocks;
discrete scale variation is confined to exposed arches with a zero-start crown.
The fixed wet film remains .65/.06.

Thin ray-supported membranes now scatter light at source SSS .32 with effective
200/100/50-micron radii. A face-domain role mask restores TWO baked materials:
opaque anatomy at SSS0 and the exact M_<Item>Membrane slot at SSS .32, sharing
the same four material images. AO remains a fifth exported map. This avoids
the generic baker's maximum-source-SSS spreading through the head/body/eyes.
The opt-in importer isolates M_CaughtFishMembrane from the common parent and
uses TwoSidedFoliage/textured subsurface/.35 opacity as an engine approximation;
it does not claim Blender/UE shading equivalence or actual shader compatibility.
Nine offline dispatch/isolation/idempotence/error contracts pass; Integration
still owns the real API/shader/import/appearance gate for both fish parents.

Held evidence is Assets\Props\CaughtFish\MembraneProof\{Source,Baked,Family}.
The paired trout source/bake has mean/RMS 8-bit differences .084/.583 hero
and .529/1.558 detail; five 4096 maps and matching 4K views pass fidelity/
current-source receipts. The family has six FBX, thirty 4096 maps and twelve
viewed 3840x2160/192-sample OPTIX frames plus a viewed thirty-map overview.
Copied receipts pass. Triangles: trout 51,539; salmon 57,510; perch 51,596;
carp 53,610; mackerel 55,319; bass 54,655. Raw jaw/hinge/roof/station,
160 tooth/four barbel roots, twelve pupils/pectoral fans, fin attachment,
joined finite geometry/UV/pivot/normals and material-role fixtures pass.
No gameplay, saves, versions, enum widths, placements or Unreal assets changed.

Absolute Blender image paths survived save_as_mainfile(relative_remap=True)
and failed two portability probes. Explicit //Textures-relative paths, saved
without automatic remapping, then reopened, resolve all 24 family material
references locally. This is a verified preservation fix, not a claim that the
generic exporter already guarantees portability. Earlier held evidence is frozen.

Viewed improvement: no rectangular substrate blocks, shorter selected skulls,
thinner dentaries, more open palatal space and warmer light through the fins.
The remaining art defect is concrete: carp still has a predatory wedge/gape,
head ridges/opercular edges look carved and fins retain leaf-like regular ribs.
Next correction targets species-specific muzzle/gape and jaw-relative bony
landmarks before the sixteen dishes; no operational blocker or new approval
wait. Another 8-12+ hours of art/wiring remains an estimate, not a release promise.

Visible Blender PID20420/port9878 holds the current six baked meshes at their
export origins, not the previous review grid. Named pre-pass/source/baked
preservation copies remain on E:. No UE, UBT/UAT, new helpers, automation,
shortcut or real-save access. Overnight/slot2 authorization remains in force.
Build02-only provisional usage: 398 calls through event70463,
2,445,055,050,000 recorded nano-AIU. Actual gpt-6.1-sol/high; launch context
default, actual context tier unknown; later calls need recapture.

### Held species-mouth and surface checkpoint after 411fbd34

WIP only, NOT [ready] or fish import/admission authorization. The carp now has
a broad short muzzle, a rounded terminal opening with fleshy annular lips,
skin-covered lateral cheeks and a connected oral lining. Its smaller/forward
eye, shorter gape/tail, four forward/downward barbels and larger bronze scales
follow observation of George Chernilevsky's Cyprinus_carpio_2008_G1.jpg
(verified CC BY-SA 3.0 on Wikimedia Commons). The photo is observation-only in
E: scratch, not shipped or used as a texture. The long carp dorsal is no longer
treated as an entirely spiny comb.

All six share softer orbital/opercular/preopercular fields, jaw-relative
maxillary/dentary bands, a smooth mandibular hinge blend, 21-column vestibules
and a curved asymmetric pectoral fan rather than the earlier leaf/straight
edge. Skin adds clustered pigment/scale variation, smaller non-carved crowns,
silver/bronze reflectance and irregular salmon marking orientation. Fin shader
rays gain finer distal branches/segmentation and less graphic contrast.
Base Normal and Coat Normal now share the same authored/baked normal socket;
the film remains .65/.06 and the opaque/membrane role separation is unchanged.

Raw all-six stations/hinges/vestibules, 160 seated teeth/four barbels, twelve
pupils/pectoral fans and seven-ray/eight-station attachment fixtures pass.
The barbel clearance fixture casts against material-zero exterior skin only:
nearest-face signed distance against the open inner cavity gave false failures.
This checks sampled centre clearance, not general watertightness or a complete
self-intersection proof. Joined geometry/UV/pivot/outward/unique-mesh fixtures
pass, with trout 53,242; salmon 59,414; perch 53,004; carp 52,979; mackerel
56,908; bass 56,586 triangles, all below the 65,000 budget.

Held evidence is Assets\Props\CaughtFish\SpeciesProof:
SourceFamily has the current six source-shader meshes and twelve viewed
3840x2160/192-sample OPTIX frames. SourceTrout/BakedTrout retain a fresh paired
five-map/4096 proof exercising both baked coat-normal sockets and the membrane
role. Mean/RMS 8-bit differences are .099/.659 hero and .599/1.446 detail;
copied fidelity receipts and four local texture dependencies pass after
reopening. The basecolor and normal atlases were viewed. Nine offline fish
import contracts still pass; there is no current baked six-fish set and no
real Unreal API/shader/import/appearance proof for this source.

Viewed weaknesses remain: heads still read too molded, the carp front lip
has visible plane transitions, scale arches/rays retain procedural regularity,
and the trout bake adds locally blotchy head reflections despite passing the
whole-frame fidelity threshold. A sampled cheek crop had mean/RMS 2.39/3.11
8-bit difference; full-frame numbers are not close-up art acceptance. Diagnose
that actual bake/normal-detail loss and organic head/fin surfaces next, before
starting sixteen dishes/edible portions and image/eating wiring. Estimated
remaining art/wiring is 11-18+ hours (fish 3-6+, dishes/wiring 8-12+), uncertain
and not a release ETA; Integration's engine and player gates remain additional.

Live-builder copy=True does not update bpy.data.filepath. One initial carp
render therefore read an older report and accidentally wrote into frozen
MembraneProof/Family; only our three overwritten files were restored from
preservation copies, and that subtree was verified clean. Every subsequent
pass explicitly opened its exported blend before rendering. Integration
published the canonical remedy at 43bcced5. Older held receipts stay frozen.

Visible Blender PID20420/port9878 now holds six current source fish in an
EEVEE material-preview grid, preserved at
E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\CaughtFish-species-source-visible-grid.blend.
Review-only transforms do not alter saved export-origin meshes. Named E:
preservation copies remain; superseded task scratch is being reduced after
this checkpoint. No new helpers, automation, Unreal/UBT/UAT, production asset
promotion, gameplay/save/enum/version/placement changes, shortcuts or real saves.
Overnight/slot2 continuation remains authorized; no new approval wait.
Actual gpt-6.1-sol/high, launch context default, actual context tier unknown.
Build02-only provisional usage: 463 calls through event70551,
2,834,389,040,000 recorded nano-AIU; later calls require recapture.

### Held carp continuity and one-specimen bake isolation after c0ec4e0c

WIP only, NOT [ready], fish import/admission or a solved realism gate.
The carp rostrum now follows sixteen cubic bands from the posterior cheek
tangent around the lip into the oral lining; the lateral front cheek shares
the same curved transition. Opening, lining, four barbels and other five
species' geometry are preserved. The first .55/.45 control reaches exceeded
the existing 2mm length tolerance (42.25cm); .36/.26 passes at 42.11cm without
weakening that guard. All-six raw mouth/hinge/vestibule/tooth/barbel/pupil,
twelve pectoral and seven-ray attachment fixtures pass, with new sampled
outer/inner tangent and non-straight midpoint fixtures. These are not complete
watertightness or self-intersection guarantees.

Assets\Props\CaughtFish\CarpContinuityProof preserves the copied source blend,
FBX, report, two viewed 3840x2160/192-sample OPTIX frames and verification.
Carp is 55,008 triangles, [8.53,42.11,17.09]cm. Current source/adapter/material
hashes, copied FBX, joined geometry/UV/pivot/budget and unchanged .65/.06
film/source-relief linkage pass. The curved shoulder reads less angular in
the viewed close-up, but the family still looks too molded/procedural.
Source shader hash remains a0bf6a16; recipe hash is 444c5f44. Frozen
SpeciesProof, MembraneProof and production root are unchanged.

The bake investigation holds frozen c0 trout geometry/fields/framing constant.
Assets\Props\CaughtFish\BakeDiagnostics retains four selected detail frames,
input/frame hashes and generated fixed-crop measurements for twelve cases.
The cheek rectangle is x2050-2700,y800-1250, in the 3840x2160 detail frame.
Original RGB8 RMS 3.105 falls to 1.318 with source Normal/Coat Normal and to
2.068 with source roughness. Sixteen-bit normal, matched triangulation and zero coat only during normal
baking do not fix it. Correction: two/all microbump-removal variants were
compared against the original bumped source, not matched bump-free source/bake
pairs. They do NOT exclude Bump evaluation as a causal branch.
An 8192 normal reaches 2.439; a head-focused UV allocation with all five maps
still 4096 reaches 2.432, only partial improvements. All production budgets
and shader fields remain unchanged. Source/baked corner normals and transforms
match; sampled cheek UV overlap checks found none. Untouched-source re-render
RMS .00696 detail (.00677 hero, max difference1) is far below the observed
artifact. Normal translation dominates, roughness contributes, and the exact
reconstruction defect remains unproven. Hybrids are local input-isolation
diagnostics, not whole-model production materials or export receipts.

Coordinator decision at 2026-10-04T11:02Z: preserve/checkpoint this bounded
carp correction and probes, keep fish held, and proceed independently with
already-authorized original sixteen-dish source geometry/materials. Read-only
reviewer fc56f594-b380-45f1-9633-428ca29e5089 traces a causal one-specimen fix;
do not park for that review or run another family/gloss sweep. No generic bake
rewrites, family baking, new helper agents, UE/build/save/automation work.
The previous no-dishes-until-fish sequencing is superseded by this explicit
parallel source-modeling instruction; overnight/slot2 remains authorized.

Owned visible Blender PID20420/port9878 is saved as
E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\Carp-continuity-held-visible.blend,
labelled HELD WIP with EEVEE material preview. The earlier six-source grid is
also preserved. Open exported proof blends explicitly before any rendering;
the visible review copy is not an export receipt. Diagnostic input blends,
normal maps and scripts remain in named E: probe folders for the reviewer.
Blender lacks Pillow; copied-proof PNG dimensions were checked with its
dependency-free signature/IHDR parser instead of installing packages. Offline
Pillow/NumPy measurement decoded all twelve frames. No engine or art acceptance
claim, gameplay/save/version/enum/placement changes, shortcuts or real saves.

Actual gpt-6.1-sol/high, launch context default, actual tier unknown; no lane
helpers. Build02-only provisional snapshot: 521 calls through event70642,
3,182,641,350,000 recorded nano-AIU; terminal/later calls require recapture.
The prior 11-18+ hour art/wiring estimate is no longer a reliable completion
forecast after unresolved bake diagnosis; sixteen dishes/portions and image/
eating wiring have not started at this checkpoint. First independent source
slice is original Baked Potatoes serving and handheld portion, exactly potato
and fuel as catalogued (no invented butter/oil/herb ingredient).

### Two failed normal discriminators and first meal source milestone

Both reviewer-directed one-specimen discriminators are complete and stopped.
Frozen c0 trout topology/positions/matrices/corner normals/smooth/sharp/custom
normal state, active/render UV and POINT fields were guarded. One 4096 OBJECT
NORMAL round trip gives cheek RGB8 RMS 3.062 (original 3.105); one explicit
final-source-normal EMIT capture gives 3.060. The latter captures the actual
socket with WORLD->OBJECT, normalization and .5*n+.5 encoding; unlinked eye/
oral slots use explicit shading-normal equivalents. Both miss the predeclared
<=1.7 plus viewed-patch-reduction discriminator; patches persist. Neither
tangent conversion alone nor NORMAL-pass extraction alone explains the issue.
UV-bake procedural normal-field evaluation/filtering remains unproven. No
shared-helper correction, shader/budget change or further family bake authorized.

Assets\Props\CaughtFish\NormalCaptureDiagnostics preserves two viewed detail
frames, measurements, diagnostic reports and copied-frame/hash verification.
Reports retain inherited geometry/framing metadata: NOT fresh FBX/export/import
receipts. Replay scripts, blends and normal images remain in the named E:
fish-object-normal-probe and fish-emission-normal-probe folders. Earlier
bump-removal controls were unmatched source pairs and do not exclude Bump
evaluation. Integration recorded these results at 2f4e844e/f94a8399/166f8d2a.

Original Baked Potatoes serving/edible-half source is now built, not accepted.
New coal_baked_potatoes.py and isolated homestead_food_materials.py reuse only
generic Graph/wood infrastructure, no existing meal/crop/root mesh or photo
texture. The observed O'Dea CC BY-SA4.0 potato photograph stays on E: and is
not shipped; no pictured kale/toppings or uncatalogued oil/butter/herbs were
added. Exact ingredients: Potato2 and Kindling1. Original 23.2cm elm platter
holds one whole potato and both halves of a second; separate edible half is
7.8cm long, with no plate attached to the hand.

Rejected radial crumb spokes and rim noise leaking across the entire flesh
were replaced by a nonradial grid and perimeter-confined torn-rim noise.
Upper/lower halves share the same authored tuber but are not duplicated lower
halves. Serving/portion have 53,056/18,176 triangles and sizes
[23.2,23.2,6.87]/[5.4,7.8,2.74]cm. Four 3840x2160/192-sample OPTIX frames were
viewed; the current skin/wood remain procedurally mottled and flesh too uniform/
synthetic. This is NOT a completed meal or one of sixteen art approvals.

Assets\Props\PreparedFood\PotatoesSourceProof preserves the reopened copied
source blend, two FBX, four frames, report and verification. Source/food hashes
346ff854/6b2248b6 match; finite/closed/positive-volume/noncollapsed geometry,
UV/bottom-pivot/scale/budget/role fixtures and copied FBX/frame receipts pass.
Tests do not certify intersections, contact everywhere, grip or artistic/UE
suitability. Source report material socket defaults are not evaluated albedo.
No meal baking/import/wiring or gameplay/save/version/placement edits.

Owned Blender PID20420/port9878 is preserved as labelled EEVEE material WIP:
E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\Potatoes-current-source-held-visible.blend.
That replaces the stale second-pass visible potato copy, not frozen proofs.
Continue independent meal source refinement, then fifteen remaining dishes/
portions and original imagery/eating presentation; do not park waiting for fish
review. Overnight slot2 authorization remains; Integration owns all engine gates.
Actual gpt-6.1-sol/high, launch default/actual context unknown; no helpers or
automation. Build02 provisional snapshot: 568 calls through event70757,
3,418,866,540,000 recorded nano-AIU; later calls/terminal response need recapture.
Prior hour estimates are obsolete; completion forecast remains uncertain.

### Corrected live food dependency provenance

24500d2a stays held. Audit found that build_prop reloads kit/mats but the new
isolated food module remained cached between live requests. Its on-disk hash
did NOT certify the executed shader graph; cached skin had eleven nodes with
older ramp stops and flesh had only the 410-scale noise. Geometry/export hash
fixtures remain valid; old four frames are not current-shader proof. Historical
PotatoesSourceProof is unchanged except for explicit shader-cache-audit.json.
Integration published this correction at f2ed9100, not a solved-cache claim.

The owning recipe now explicitly reloads food before reading its report hash.
Materials tag the executed module's loaded SOURCE_SHA256; fixtures independently
check actual skin noise scales {170,65,1600,850} and flesh {230,1300}. The new
fixture reproduced the prior scene's stale-source failure, then passed after
fresh build/reopen. Two subsequent four-frame 4K/192-sample passes were viewed.
Reduced camouflage, region-confined dry scorch, finer irregular starch relief
and restrained natural moisture read less graphic, but flesh still looks too
uniform and the elm grain remains procedural. No artistic acceptance.

PotatoesReloadedSourceProof preserves a separate copied current blend/two FBX/
four frames/report and fresh geometry/executed-shader/hash verification.
Counts remain 53,056/18,176; portion now [5.4,7.8,2.71]cm. No food baking,
import, engine verification, imagery/eating wiring, gameplay/save or fish
material/helper/budget changes. Older proofs must not be rerendered in place.
Build02-only provisional usage now 589 calls through event70794,
3,473,105,220,000 recorded nano-AIU; later calls require recapture.

### Original roasted turnip source milestone

New roasted_turnips.py authors one trimmed 7.6cm purple-top root in six
sector-loft wedges, original 20cm hand-thrown earthenware, and a separate edible
wedge. Exact Turnip1/Kindling1; no butter/oil/herbs or reused crop/meal mesh.
Text anatomy reference is cited in the recipe; no external texture. Independently
modeled cut planes, subtle vascular relief, 0.55mm cooked-edge rounding and
analytic dish-floor seating preserve closed components. The six source sectors
represent one root, not six inventory units.

First viewed pass was raw-looking with repeated grey spots. Later passes reduce
that pattern to broader dry-heat browning and slightly irregular placement.
The edge modifier exposed negative closed volume on the portion; owning-recipe
normal repair now requires a closed shell and explicitly orients positive volume.
Fixtures check all seven serving islands/one portion island individually, so
the dish cannot mask an inverted wedge. This does NOT apply to open fish sheets
or alter shared normal helpers.

Current serving/portion: 42,526/6,638 triangles,
[20.0,20.0,4.69]/[4.01,7.6,3.3]cm. All finite/closed/positive-component-volume/
noncollapsed/UV/pivot/scale/current-shader fixtures pass. Four latest viewed
3840x2160/192-sample OPTIX frames remain too smooth/molded with procedural clay;
not art/import ready. RoastedTurnipsSourceProof preserves reopened source, two
FBX, four frames, report and copied hash/executed-graph verification. New food
functions leave potato_skin/potato_flesh ASTs unchanged from 1cc2911f.

Owned visible turnip WIP is saved on E: as Turnips-source-held-visible.blend;
display transforms never replace export-origin source. No food baking/import/
engine/wiring/gameplay/save/fish-helper/budget changes. Fourteen original dishes
still unstarted; potato/turnip art approval also remains. Continue independent
source work under existing overnight slot2, no automation or approval wait.
Build02-only provisional snapshot: 611 calls through event70829,
3,637,828,390,000 recorded nano-AIU; later calls/terminal response need recapture.

### Original stewed carrot source milestone

New stewed_carrots.py authors two trimmed 13.8cm tapered roots, each cut into
three lengths and split into halves (twelve pieces), original 18.4cm earthenware
bowl, small cooking-water volume and an independent edible half segment.
Exact Carrot2/Kindling1; no oil/butter/herbs/milk or reused mesh/photo texture.
New food geometry tools require closed positive-volume components and reload
explicitly; they do not touch open fish sheets or shared kit/bake helpers.
Previous potato/turnip shader function ASTs remain unchanged from e6715c2e.

First viewed source pass showed carrot/bowl penetration and overly sharp cuts.
Own surface-BVH vertex seating now supports pieces against the actual bowl and
earlier pieces, within the inner aperture, with 50-micron clearance. The first
supported arrangement created an 8.45cm tower and failed its bound; heel-first
two-layer placement removes the tower and retains the original 5.3-5.8cm height
fixture. Cooked edges round by 0.35mm; a later pass strengthens fine vascular
relief and gently varies the core boundary. No geometry is promoted on checks
alone; sampled support is not a complete face-intersection/contact guarantee.

Latest serving/portion: 53,792/3,976 triangles,
[18.4,18.4,5.41]/[2.51,4.6,1.26]cm. All fourteen serving islands/one portion
island are closed, finite, positive-volume and noncollapsed; UV/bottom-pivot/
scale/current shader/geometry hashes pass. Four latest 3840x2160/192-sample
OPTIX frames were viewed. Penetration/tower symptoms are gone, but the pieces
remain too slab-like and clay too procedural; NOT art/import acceptance.
StewedCarrotsSourceProof preserves the reopened copied source, two FBX, four
frames, report and hash/executed-graph verification. Pixel crops inspected the
earlier full-resolution smoothness rather than trusting downscaled previews.

Owned Blender is preserved as Carrots-source-held-visible.blend on the same
E: runtime scratch root, labelled EEVEE source WIP. Thirteen meals are unstarted;
all three modeled meals still need artistic refinement, with baking/imagery/
eating wiring and Integration gates untouched. No new agent helpers/automation/UE/
builds/gameplay/save/enum/version/placement/fish budget or shared bake changes.
Actual gpt-6.1-sol/high, launch default/actual context unknown. Build02-only
provisional snapshot: 632 calls through event70861, 3,838,758,080,000 recorded
nano-AIU; subsequent calls/terminal response need recapture.

### Original herbed broad bean source milestone

New herbed_broad_beans.py authors 24 individual 1.9-2.5cm cooked seeds with
asymmetric outlines, cotyledon groove, lateral hilum field/indentation, 36
original thin chopped Meadow Herb flakes and new 13.44cm earthenware bowl.
Separate single-bean edible portion has no bowl attached. Exact BroadBeans3/
Flowers1/Kindling1; no oil, butter, salt or invented garnish. Three crop units
are visually represented by the serving, not a new inventory conversion.
No existing crop/fish/food mesh or photographic texture is reused.

Viewed first pass was too sparse, smooth and polygonal; herbs read as cardboard
rectangles. A tighter bowl/layout, denser bean silhouettes, stronger irregular
hilum/fine-coat fields and thinner, curved six-sided herb fragments improve the
source. Native-pixel crops still exposed uniform smoothness. Final detail views
use recipe-owned f/64 (instead of f/22) and nearer portion focus so shallow focus
cannot hide it; no shared renderer/helper or fish shader/budget change.
All four final 3840x2160/192-sample OPTIX frames were viewed. Seeds still read too
waxy/molded, hila insufficiently convincing and clay too procedural: NOT art/
import acceptance. No completed-family or remaining-hours claim.

Serving/portion: 62,928/2,432 triangles,
[13.44,13.44,3.93]/[1.36,2.47,0.8]cm. Source fixtures pass all 61 serving islands/
one portion, positive closed volume/noncollapsed geometry, original roles,
finite part-local shader coordinates, applied transforms, bounded UVs/scale,
executed shader hashes/noise scales and sampled food/bowl nonpenetration.
These are not full face-intersection/contact/gravity/grip proof. Reopened
HerbedBroadBeansSourceProof preserves source, two FBX, four frames, report and
copied-source hash verification. All six existing food shader function ASTs
remain unchanged from bc4126c3; earlier frozen source proofs are not overwritten
or presented as current-code receipts after the new library functions.

Owned Blender preserved as Beans-source-held-visible.blend on E:, separated
and framed around the selected serving/portion in EEVEE Material view.
Four meal source prototypes remain below artistic acceptance; twelve dishes
unstarted. Baking, original imagery, eating wiring and Integration gates remain.
No agent helpers/automation/UE/UBT/UAT/gameplay/save/version/enum/placement change.
Actual gpt-6.1-sol/high; launch default/actual context unknown. Build02-only
provisional snapshot: 664 calls through event70905, 4,024,406,690,000 recorded
nano-AIU; subsequent calls/terminal response remain to capture.

Bounded food-only coordinate check after this checkpoint: on the single bean
portion, translated Object coordinates match pcoord at every vertex within
1.88e-8m. One matched 4K/192-sample f/64 detail frame and native-pixel crops were
viewed. RGB8 RMS is 0.0284 full-frame/0.0923 at [1650,1050,2400,1650]; the
replacement does not recover viewed fine coat relief. No coordinate-source fix
is justified by this check, and it establishes no fish or bake diagnosis.
Replay script, diagnostic blend/report/frame, exact field check and pixel
comparison remain in E: runtime scratch beans-coordinate-source. Frozen
ae434be5 source/proof and all production code/shaders/helpers remain unchanged.
Updated build02-only provisional snapshot: 679 calls through event70935,
4,068,096,370,000 recorded nano-AIU; later calls/terminal response still excluded.
Visible held source restored after the diagnostic; no active render or shell.

### Original cabbage/potato stew and eating-spoon source milestone

New cabbage_potato_stew.py authors twelve closed cooked cabbage lamina/rib grids,
six original softened potato cuts, eighteen Meadow Herb fragments, new 17.6cm
earthenware bowl and bounded cooking-broth volume. Exact Cabbage1/Potato1/
Flowers1/Kindling1; cooking water is presentation, with no new milk/oil/meat/
garnish requirement. Observational reference: https://en.wikipedia.org/wiki/Cabbage;
dimensions and serving/utensil design are authored, not inventory conversions.
No prior crop/food/fish/utensil mesh or photographic texture is reused.

The eating portion is a newly carved 18.2cm maple spoon, 2.8cm oval hollow head/
9mm precision-grip handle, carrying one cooked potato bite and cabbage curl.
It is not a bowl attached to the generic berry grip. Source -Y points to its
tip and the FBX Y mirror remains explicit. Realistic-animation grip guidance
was loaded; NO new action/carry/grip animation or runtime attachment was made.
Item-specific seating, finger/wrist/mouth clearance and imported axes remain
Integration gates; the bottom-centre export pivot is not an accepted grip anchor.

Viewed first pass was too chip-like, potatoes too cubical and the spoon neck
abrupt. Later passes reduce cooked-leaf cupping/bend, add branch/midrib thickness,
finer face relief, rounded cuts and a cubic neck transition. A hard liquid
support initially held every leaf above broth; liquid is now a presentation
volume, with mean-height partial immersion constrained by solid supports.
Two potato contacts keep the ingredient visible without lifting unsupported
pieces. These are authored/sampled placements, not a gravity/contact solver.
Broth transmission .45/IOR1.333 is Blender source only, NOT an Unreal material
contract or a successful bake. No shared kit/renderer/fish/bake helper changes.

Latest serving/portion: 45,576/10,424 triangles,
[17.6,17.6,6.01]/[2.8,18.2,1.46]cm. All 38 serving islands/3 spoon-portion islands
pass closed positive-volume/noncollapsed/finite geometry, retained part-local
coordinates, applied transforms, UV/scale/budget/source-shader fixtures and
sampled container clearance. Spoon-head centre is 2.959mm below sampled side
surfaces (fixture requires >2.5mm), so a flat paddle cannot pass as a hollow spoon.
No full face-intersection/contact/gravity/grip or artistic acceptance is implied.

Four final 3840x2160/192-sample OPTIX frames were viewed. Cooked leaves still
look too stiff, potato too molded/cubical and wood/clay/broth too procedural:
NOT art/import acceptance. CabbagePotatoStewSourceProof preserves reopened source,
two FBX, four frames, report and copied-source verification. Owned Blender is
preserved as Cabbage-stew-source-held-visible.blend on E:, separated/framed EEVEE
Material view. Five meal prototypes, zero approvals; eleven dishes unstarted.
Original imagery, baking, item-specific eating and engine gates remain pending.

Actual gpt-6.1-sol/high, launch default/actual context unknown; no agent helpers/
automation/UE/UBT/UAT/gameplay/save/enum/version/placement changes. Build02-only
provisional snapshot: 698 calls through event70962, 4,191,701,880,000 recorded
nano-AIU; subsequent calls/terminal response still need recapture.

### Original unsweetened berry compote source milestone

New berry_compote.py authors twelve softened bramble fruits (eight complete,
four ragged closed pulp exposures), original reduced-juice volume, new 11.8cm
glazed shallow dish and a separately generated 14.6cm maple eating spoon with
one exposed fruit bite. Exact Berries3/Kindling1, no sugar/cream/spices/leaves/
invented garnish. References read: https://en.wikipedia.org/wiki/Compote and
https://en.wikipedia.org/wiki/Blackberry; dimensions/design are authored, not
an inventory-to-weight conversion. No reused game mesh or photographic texture.

One closed fruit loft per fruit carries seeded angular drupelet relief, not
intersecting spheres. Initial evenly spaced fresh-looking berries in a flat
purple pool were viewed and rejected. Denser varied collapse/tilt, lower fluid,
geometric pulp-height variation, four ragged exposures and constrained
nonradial pulp triangulation replace that pass. A radial fan looked like a
flower/cut-plastic cap in 4K and was replaced, not declared anatomical evidence.
First liquid/wall sampling failure was corrected by authored radial clearance.
The first half-fruit eating bite failed its original height fixture; increased
the actual bite rather than weakening the bound. No fixture relaxation.

Serving/portion: 58,972/10,528 triangles,
[11.8,11.8,3.26]/[2.25,14.6,1.12]cm. All 14 serving/2 portion islands pass
closed/positive/finite/noncollapsed coordinates, applied transforms, bottom
pivot, UV/scale/budget/current executed-shader and sampled container clearance
fixtures. Spoon hollow is 2.374mm below sampled side surfaces (>2.006mm scaled
fixture); this is not a hand/finger/wrist/mouth-seating or contact/gravity proof.
Four final 3840x2160/192-sample OPTIX frames viewed and copied with source/two FBX/
report/reopened-source verification into BerryCompoteSourceProof.

Still too molded/gel-like and regular, pulp/wood/glaze too procedural: HELD,
not art/import admission. Six meal prototypes, zero approvals; ten unstarted.
Estimate >12 further hands-on hours for unstarted sources/refinement alone;
fish diagnosis, baking, imagery/eating wiring and UE gates add unbounded time.
Not a morning-build candidate or a release ETA.

Shared original spoon construction extracted to homestead_food_geometry;
cabbage default output preserves identical vertex/topology/part-local coordinates.
Container/hollow sampled fixtures extracted to BlenderPreparedFoodChecks with
the same cabbage thresholds. New compote's smaller utensil is generated before
coordinates/UVs, not a scaled reused static mesh. Twelve existing food shader
functions and four existing food geometry functions remain AST-identical.
Historical frozen proofs remain receipts at their original commit/library hash,
NOT current-code recaptures; none were rerendered/overwritten or promoted.

Actual gpt-6.1-sol/high, launch default/actual context unknown, no agents/
automation/fish/bake/render-helper/gameplay/save/enum/version/placement/UE/UBT/
UAT changes. Preserved separated/framed EEVEE Material scene:
Berry-compote-source-held-visible.blend on E:. Build02-only provisional snapshot:
726 calls through event71003, 4,336,968,510,000 recorded nano-AIU; later calls/
terminal response still need recapture.

### Original unsweetened strawberry compote source milestone

New strawberry_compote.py authors eight stemless softened halves, shoulder-to-tip
lofts with skin pits and 160 separate intrinsic achenes, closed nonradial flesh
sections, cooking juice, a new 13.2cm glazed dish and a generated 15.2cm carved
maple eating spoon with a near-full half-fruit bite. Exact Strawberries2/Kindling1;
no sugar, cream, spice, leaves or invented garnish. Strawberry/compote references
inform receptacle/achene anatomy; dimensions/count are authored presentation,
not an inventory-to-weight conversion. No existing crop/game mesh/photo reused.

Intermediate fruit assemblies use centre pivots before rotation; bottom pivots
had swung them through the dish wall. A supported mean-height constraint exposes
partially immersed fruit above the juice without treating liquid as a hard
support. A larger actual spoon bite satisfies the original height fixture; no
bounds were weakened. Softer 0.35mm cut-edge rounding produced one fully collapsed
bevel face; a local 10nm degenerate-edge dissolve removes it before closed-volume
validation. Reduced, varied pink axial flesh field replaces the stark white stripe.

Current serving/portion: 52,734/12,252 triangles,
[13.2,13.2,3.66]/[2.34,15.2,1.14]cm; 170/22 closed islands. Finite/positive volume,
noncollapsed coordinates, UV/bottom pivot/unit transforms/current executed-shader,
dimension/budget and sampled dish/spoon clearance fixtures pass. Sampled spoon
hollow 2.472mm exceeds the original scaled 2.088mm threshold; this is not hand,
finger, wrist, mouth, contact, stability or full face-intersection proof.

Four final 3840x2160/192-sample OPTIX frames viewed; original source, two FBX,
report, four frames and reopened copied-source verification are frozen in
StrawberryCompoteSourceProof. Still raw/molded/candy-like; flesh/juice lack convincing
cooked tissue/viscosity, wood/glaze remain procedural. HELD, not art/import ready.
Seven meal prototypes, nine unstarted, zero art approvals. Estimate >12 further
hands-on source/refinement hours; fish/bake/imagery/eating/UE gates remain unbounded.
Not a morning-build candidate or release ETA.

The new cut_food_patch helper accepts an arbitrary cut normal axis; berry's older
cap implementation remains in its recipe, not retroactively extracted or replaced.
All fourteen previous shader and five previous geometry definitions are
AST-identical to 50f155b9. Older frozen receipts retain their original source/hash;
none overwritten or promoted. Separate framed EEVEE Material review preserved
on E: as Strawberry-compote-source-held-visible.blend.

Actual gpt-6.1-sol/high, launch default/actual context unknown. No helper,
automation, fish probe, bake/budget/render-helper, gameplay/save/enum/version/
placement, UE/UBT/UAT or shortcut changes. Build02-only provisional snapshot:
759 calls through event71051, 4,492,791,490,000 recorded nano-AIU; subsequent calls
and terminal response still need recapture. Not reconciled billing credits.

### Original root vegetable hotpot source milestone

New root_vegetable_hotpot.py authors ten fibrous brown-skinned root segments,
six curved white-turnip quarter sectors, twenty-four Meadow Herb flecks, cooking
broth and a new 16.6cm two-lug earthenware crock. Exact Roots2/Turnip1/Flowers1/
Kindling1; no potato/carrot/meat/oil/milk. Read root-vegetable and turnip reference
pages; generic wild-root anatomy deliberately does not assign an uncatalogued
species. Authored piece counts/size are presentation, not stock-to-weight rules.
New 17cm generated maple spoon carries a separate root/turnip bite with seasoning.
No prior crop/game/meal mesh or photographic texture reused.

Root end sections use the nonradial cut helper; turnip cap boundaries retain the
exact subdivided knife-cut vertices. Cooked 0.8mm edge rounding, shallow real
longitudinal root ridges and curved turnip profiles replace sharp cylindrical
forms. Partially immerse pieces in 32mm broth; solid supports remain the crock
and other food, not liquid. Repositioning spoon bites prevents the turnip being
forced onto the root, reducing its assembly height from 20mm to 14.2mm without
weakening fixtures. The first herb fixture mistakenly expected unrelated noise
scales; corrected it to the actual unchanged chopped-herb graph (650/2200).

Current serving/portion: 54,400/12,106 triangles,
[19.36,16.6,5.71]/[2.62,17.0,1.42]cm; 44/5 closed islands. Closed/positive-volume,
finite/noncollapsed coordinates, UV/pivot/unit transforms, dimension/budget,
current executed-shader and sampled crock/spoon clearance checks pass. Sampled
spoon hollow is 2.764mm, above the original scaled 2.335mm threshold. This is not
full face-intersection, gravity/contact/stability, hand/finger/wrist/mouth proof.
Four final 3840x2160/192-sample OPTIX frames viewed; source/two FBX/report/four
frames and reopened copied-source receipt frozen in RootVegetableHotpotSourceProof.

Still regular stump/stone-like cuts, overly smooth tissue and flat/cloudy broth;
clay/wood too procedural. HELD, not art/import ready. Eight meal source prototypes
(all eight crop recipes), eight fish preparations unstarted, zero art approvals.
Estimate >12 further hands-on source/refinement hours, plus unknown held fish,
bake/imagery/eating/UE gates; not a morning candidate or release ETA.

Sixteen prior shader and six geometry definitions remain AST-identical to
c82c482d. Earlier frozen proofs remain tied to their original commit/library SHA,
not recaptured or promoted. Separate framed EEVEE Material review preserved on
E: as Root-hotpot-source-held-visible.blend. Actual gpt-6.1-sol/high, launch
default/actual context unknown; no agents/automation/fish probe/bake/budget/
render-helper/gameplay/save/enum/version/placement/UE/UBT/UAT/shortcut changes.
Build02-only provisional snapshot: 775 calls through event71078,
4,549,225,690,000 recorded nano-AIU; later calls/terminal response need recapture.
No reconciled billing-credit claim.

### Original raw mackerel meal source milestone

New raw_fish_slices.py authors six independent boneless skin-on fillet cuts, a
new 22x16.4cm oval ceramic dish and a separate 3.23cm edible slice. Exact
SeaMackerel1; no fire/fuel/rice/vinegar/soy/citrus/herbs/garnish. Myomere and mackerel
food references informed pale pink-white muscle, lateral dark muscle, myoseptal
partitions and skin markings; authored dimensions/count are presentation, not
stock-weight or real food-safety instructions. No caught-fish/crop/game mesh reused.

New closed asymmetric fillet lofts have oblique knife ends, tapered thickness,
shallow real myoseptal relief and finer soft fascia fields. Correct interior cap
coordinates back onto their oblique knife planes without altering boundaries.
First bars-of-soap silhouettes, stark muscle stripe and blue racing-stripe skin
were rejected in viewed 4K passes; taper, narrower varied dark muscle and thin
irregular dark skin marks improve them. Two slices are skin-up; edible slice
remains flesh-up. New prepared-food shaders do not call/modify caught-fish shaders.

Current serving/portion: 46,520/7,384 triangles,
[22.0,16.4,1.5]/[3.23,1.62,0.64]cm; 7/1 closed islands. Closed positive volume,
finite/noncollapsed coordinates, UV/bottom pivot/unit transforms, dimension/budget,
executed shader/source graph and sampled dish-clearance fixtures pass with no
bound relaxation. Four final 3840x2160/192-sample OPTIX frames viewed; original
source/two FBX/report/four frames/reopened copied receipt frozen in
RawFishSlicesSourceProof. No full self-intersection/contact/stability, eating
grip/hand/mouth or engine material/animation acceptance.

Still too smooth/molded/slab-like, muscle/skin markings too graphic and ceramic
too procedural: HELD, not art/import ready. Nine source prototypes, seven
preparations unstarted, zero art approvals. More than twelve hands-on hours still
estimated for unstarted sources/refinement, plus unknown fish/bake/imagery/eating/
UE gates; not a release ETA. Prior seventeen shader/six geometry definitions are
AST-identical to 1bc234e1; held catch recipe is byte-identical after newline
normalization. Older frozen proofs remain at their original SHA, not recaptured.

Separate framed EEVEE Material scene preserved on E: as
Raw-mackerel-source-held-visible.blend. Actual gpt-6.1-sol/high, launch default/
actual context unknown; no agents/automation/caught-fish probe/bake/budget/
render-helper/gameplay/save/enum/version/placement/UE/UBT/UAT/shortcut changes.
Build02-only provisional snapshot: 792 calls through event71109,
4,616,358,250,000 recorded nano-AIU; later calls/terminal response need recapture.
No reconciled billing-credit claim.

### Original grilled trout meal source milestone

New grilled_trout.py authors one independent boneless 14cm cooked trout fillet,
three loose muscle flakes, a new 22x15.6cm oval ceramic platter and a separate
2.5x3.76cm edible broken piece. Exact RiverTrout1/Kindling1; no oil/butter/herbs/
lemon/batter/garnish. Trout and fish-as-food references read; the earlier myomere
study informs muscle folds. No catch/raw-food/crop mesh reused. New food-only
flesh/skin/platter shaders do not call or modify the held catch library.

First viewed 4K pass was a smooth white soap bar with detached-looking top flakes.
Recipe-local adaptive rows now sample the narrow muscle troughs instead of missing
them; closed curved end caps retain their original boundaries and account for
their nonplanar fold. Loose flakes are seated beside the fillet rather than
perched on its crown. Third pass adds seeded unequal muscle partitions/depths,
rounded central folds, stronger head-to-tail taper, asymmetric thickness, more
visible cooked skin and subdued uneven browning. A portion extent of 38.35mm
failed the original 38mm fixture; actual axial length was reduced, not the bound.

Final serving/portion: 49,520/9,646 triangles,
[22.0,15.6,1.76]/[2.5,3.76,.92]cm; 5/1 closed positive-volume islands.
Finite/noncollapsed coordinates, UV/bottom pivot/unit transforms, original
dimension/budget/current-executed-shader graphs and sampled container clearance
pass. Four final 3840x2160/192-sample OPTIX frames viewed. Copied source/two FBX/
report/four frames/reopened receipt frozen in GrilledTroutSourceProof.
Still molded/rubbery with conspicuous patterned muscle, smooth olive skin,
insufficient broken-fibre/crisp surface and procedural ceramic: HELD, not
art/import ready. No full intersection/contact/grip/eating/engine acceptance.

Ten source prototypes/six unstarted preparations/zero art approvals. More than
twelve additional hands-on hours remain estimated for source/refinement; fish/
bake/imagery/eating/UE gates unknown, not a release ETA. Twenty prior shader and
six geometry definitions are AST-identical to 7737497a; previous frozen proofs
stay at their original commit, not recaptured against this new library.
Separate framed EEVEE Material review saved on E: as
Grilled-trout-source-held-visible.blend. Actual gpt-6.1-sol/high; launch default/
actual context unknown, no agents/automation/catch probe/bake/budget/shared
render-helper/gameplay/save/enum/version/placement/UE/UBT/UAT/shortcut change.
Build02-only provisional snapshot: 818 calls through event71154,
4,739,561,790,000 recorded nano-AIU; later calls/terminal response need recapture.
No reconciled billing-credit claim.

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
