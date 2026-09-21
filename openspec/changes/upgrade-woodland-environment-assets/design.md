# Design

## Context

See `proposal.md` for motivation and the two delta specifications for behavior.
The inspected source is `af0c90707c4e868fa693572563f987812813c318`.
`docs\research\environment-assets\integration-evidence.md` records exact helpers
and contracts; `2026-09-20-decision.md` in that folder is the asset decision and
source index entry point. Those are evidence, not competing execution plans.

The 80 m world uses analytic ground, procedural collision, seeded HISM
decoration, separate ID-keyed resource base/produce visuals and one lighting
owner. `AddDecoration` currently overwrites every material slot and normalizes
all three mesh dimensions; `AddPart` overwrites slot zero and assumes 100 cm
primitive dimensions. The FBX bootstrap disables material/texture import and
combines meshes. These are the core integration obstacles, not missing polygons.

Design is required because external asset admission, material/import behavior,
rendering cost and gameplay-preserving placement cross several surfaces.
Parent-owned offline-startup/fullscreen changes are not in this checkpoint.

## Goals / Non-Goals

**Goals:** Adapt the smallest existing paths to an admitted asset palette; retain
authored PBR and deterministic gameplay-safe placement; discover feasibility with
one representative spike before broad placement; isolate foliage and cloud cost.

**Non-Goals:** New terrain/biome framework, PCG/Geometry Nodes runtime, experimental
Nanite Foliage/Dynamic Wind pipeline, imported showcase maps, whole weather
plugins, character work or changes to the audio Jenny likes. No automatic
acceptance from a passing screenshot metric.

## Decisions

### Current delivery and next visible milestone

The qualified `grass-ground-01` preview is selected at `415878c3`, with the
samev5 profile and previous builds retained. Coordinator review accepted its
textured, sparse, dry/earthy clearing, not lush woodland. The large foreground
occluder in tree430 also exists in grove03; it is unfinished camera behavior,
not a new grass regression. The original ground diffuse is itself patchy
earth/grass, so a uniform-green result is not the contract.

The next visible milestone is clearing readability in actual dawn/day/night
game contexts and the known tree-encounter camera obstruction. Diagnose those
conditions before changing illumination or material response; do not assume
the observed warm dawn proves an asset bug. Interactive resource silhouettes
remain a subsequent product gap, with base/produce/cleared semantics preserved.
Do not repeat isolated asset polishing or certify unchanged historical tooling.
`tasks.md` distinguishes that work from already delivered placement, rendering,
credits, staging and selection.

### Historical tree increment and qualification

The fern clearing and subsequent usable wardrobe preview have shipped as
qualified increments. Keep wardrobe-ui-02 selected while integrating the frozen
TreeSmall02Prepared/v3 handoff. Its LOD2 and thirteen unchanged 2048x2048 maps
are hash-verified; do not repeat Blender reduction or import SourceLOD0.

Reserve `/Game/Trials/TreeSmall02_20260921_01` for exactly seventeen packages:
`Meshes/SM_TreeSmall02_LOD2`, three `Materials/M_TreeSmall02_{Branches,Leaves,Trunk}`,
and thirteen explicit `Textures/T_TreeSmall02_{Branch,Leaves,Trunk}_{Diff,NormalDX,Roughness,AO}`
plus `Textures/T_TreeSmall02_Leaves_Alpha`. This is one 231785-triangle mesh,
not an automatic LOD chain. Preserve three source slot identities and measured
section totals (23702 branches, 193938 leaves, 14145 trunk), both UV channels,
zero root, valid imported normals and one scene/unit conversion at scale1.

Correction01, following failed-before-save tree-import-01: the frozen blanket
UV0 statement is incorrect. Raw LOD2 attribution proves branches use UV1
(all23702 branch triangles have constant UV0), while leaves/trunk use UV0.
On newly imported data only, swap branch vertex-instance UV0/UV1 reversibly
before standard MikkTSpace tangent construction; preserve both channels and
reject cross-role shared instances or unexpected occupancy. Materials then
consistently sample runtimeUV0, including their tangent-space DirectX normals.
Do not mutate or re-export frozen v3. Its zero normal is genuinely used by1379
corners on1303 triangles (338 branch and1041 leaf corners), not unused metadata.
Use supported UE ComputeTriangleTangentsAndNormals followed by
ComputeTangentsAndNormals(Tangents|UseMikkTSpace), without the Normals flag.
The installed implementation fills invalid normals while preserving valid
custom normals; compare every nonzero input normal exactly and reject any
change. Record actual zero-normal counts/repairs and reject remaining invalid
normals. This is a specific native adaptation, not blanket normal recomputation.
Inspect actual game shading; numerical readiness alone is not visual acceptance.

Correction02 follows the separate failed-before-save tree-import-02: native
pre-state confirmed exactly338 branch/1041 leaf zero-normal corners, but the
engine's grouped repair left an invalid normal and admission stopped. Retain
that failure. Only a remaining exact-zero source vertex instance with exactly
one connected triangle may use its valid engine-computed face normal, followed
by standard Mikk tangent regeneration. Reject ambiguous/shared instances and
invalid face normals; preserve all nonzero source normals exactly. Export
per-role grouped/intermediate/final measurements and actual imported bounds
before adaptation. Distinct import03 reuses only the unchanged identified empty
trial directory, not a reset or overwrite.

Observed import03 outcome: the native inventory/save completed seventeen
packages, but the external acceptance gate failed on twelve referenced branch
tangent/binormal corners. All1379 source zero normals were repaired, including
322 branch single-face fallbacks; no orphan vertex instances exist. Leaves and
trunk have zero invalid basis corners. Actual UE bounds match the referenced
source prediction. These packages remain unaccepted and retained, not inputs
cleared for cook or selected gameplay. The separately retained failed result
must not become a pass. A read-only source calculation found no zero float32 UV
determinants or predicted float32 geometric cross products, so it does not
explain the twelve native corners. Exact native-corner/render-buffer attribution
is needed before another repair; no blind reimport, warning suppression or
global normal/tangent recomputation.

Final coordinator proportionality decision supersedes the optional additional
native diagnostic: preserve that unbuilt diff separately and restore only those
unbuilt edits to the exact receipt03 source/binaries, verified by hashes. Do not
rebuild or delay actual viewing for another stored-versus-render-buffer study.
The existing native read verification may inspect the exact retained seventeen
packages under `TreeDiagnostic`, followed by existing cook and ordinary game
viewing/collision. Only exactly twelve branch source-description tangent and
binormal corners are admitted as a disclosed diagnostic qualification; normals,
other roles, geometry, bounds, maps, UV routing and identities stay strict.
Use distinct diagnostic outputs/reservations and `passed-diagnostic-only`, never
rewrite failed import03 or turn the clean gate into a permissive default. Hash
all seventeen packages before/after, with unchanged authoring/privacy controls.
No source asset mutation, reimport or selected-preview change. Actual shaded
appearance, grounding, collision and gameplay determine the next decision;
neither the editor-description count nor diagnostic admission proves GPU damage,
visual acceptance or promotion.

Qualified read verification01 completed with unchanged packages and cooperative
exit. Diagnostic cook01 then failed the real-output gate: the runtime-loaded
tree namespace was missing from the existing explicit DirectoriesToAlwaysCook
list. Add only that exact namespace and retain the failed cook; use distinct
diagnostic cook02. Do not substitute old cooked files or enable broad CookAll.

Cook02 passed qualified, followed by genuine Shipping compiles/link/manifest/
metadata and fresh loose stage `tree-diagnostic-01`. Its first ordinary route
recorded521 real1280x720 frames over74.20s. Forage, shader/material readiness and
approach passed; trunk-block and dependent retreat flags remained zero. Actual
player radius settled near72.59cm with tangential motion around21.7cm/s; the
observer requires total speed below5cm/s, so the gate may reject sliding contact.
Physical retreat increased separation to357.55cm, but that is not the missing
specific-component collision proof. All19 sampled near-trunk positions had
heading error14.86-16.51degrees; the observer only performs its tree sweep below
10degrees. All four stages ran; this is not an omitted contact/retreat stage.
Do not weaken the assertion or call this
passed. The OS exit was zero despite the native failed-completion exit request;
the old wrapper checked forage only. Its production outcome assertion now also
rejects missing/failed tree outcomes, with the retained real failure as an offline
regression. Preserve the original capture/guard result; no retroactive pass.
Frame520 was directly inspected as recognizable textured tree geometry; quota
blocked full-view424/contact480 inspection. These originals are retained for
coordinator review. No second launch or selected-preview change at this checkpoint.

The coordinator then reviewed actual contact frame480 and authorized correcting
the observer, not gameplay physics: use actual consumed inward movement input,
small radial velocity/progress and a supported sweep against the specific tree
component. Permit tangential CharacterMovement sliding. Record contact samples
and retreat displacement independently from contact success; retain every
planned stage and the existing guard deadlines. A distinct genuine Shipping
build/stage02 may reuse unchanged cook02, then run one corrected ordinary route.
Original diagnostic01 remains incomplete; no promotion before real outcome.

Actual diagnostic02 included the whole crown (projected bounds entirely inside
1280x720), independent289.86cm retreat and all23 stages, but contact still failed.
The retained per-tick CSV identified the exact tree with near-zero radial motion.
Installed `FPhysInterface_Chaos::Sweep_Geom` uses `BuildOverlapAll` and returns
geometric-hit success; its false `bBlockingHit` is not channel-blocking policy.
That unsupported observer predicate was corrected under explicit coordinator
approval: require geometric hit and exact component, actor/component query
enablement and reciprocal Pawn/tree `ECR_Block` responses. Keep the raw
overlap-classified flag in evidence, never assign it true. No physics, asset or
tolerance change; preserve failed01/02.

Diagnostic03 then passed the actual ordinary tree scenario:501 real720p frames,
71.80s route, all23 stages, genuine forage, crown framing, inward trunk contact
and independent286.64cm retreat. Six successive contact samples accumulated
0.832s with all actual query/response conditions satisfied while tangential slide
continued. Guard exit/release completed without hard stop or cleanup errors.
`docs/research/environment-assets/tree-diagnostic-03/receipt.json` seals the
actual images and evidence. This is qualified ordinary-play evidence, not clean
source-basis acceptance, full woodland/performance approval or promotion.
wardrobe-ui-02 remains selected pending coordinator disposition. Future Editor
authoring must genuinely refresh changed native source/product pins; this
observer correction rebuilt Shipping only and reused unchanged cook02.

Bounds compare polygon-referenced LOD2 extrema, not all FBX control points:
40624 of424817 are unused, changing Y extent by0.4882cm. Transform all eight
referenced-bound corners through the recorded model rotation, axis/handedness
and metre-to-cm conversion; retain0.2cm tolerance and export actual min/max
before rejection. Import03 and qualified reload01 confirmed those numeric bounds.

Use explicit existing FBX/texture factory patterns, reference-free frozen inputs,
no automatic materials/textures/collision, no Nanite or wind. Branch/trunk are
opaque and single-sided; leaves are masked/two-sided with clip0.5. All use
DefaultLit, authored diffuse/DirectX normal/roughness/AO; only diffuse is sRGB.
Reject missing or extra slots, objects, references, maps and package files.
Measure bounds (about456.4cm tall), source settings and material connections on
import and again in a separate process before accepting persisted packages.

Create only simple lower-trunk collision from measured trunk-section vertices
in the bottom200cm, not canopy bounds or hidden primitive-tree dimensions.
Record the actual shape and reject nonfinite/implausible dimensions. The world
component uses that simple collision; foliage has no separate collision.
Replace just one eligible deterministic primitive decoration near the clearing,
keeping authored scale/materials, root placement and the existing reserved
home/resource/structure/plot/stream corridors. Other trees remain unchanged.

Carry existing completion-driven native controls, inherited marker lifetime,
Python execution-disabled snapshots, root-only job, local DDC and endpoint
policy forward. Rebuild genuine affected native products and refresh pins;
the later Shipping UI build did not update the older Editor modules. Import,
persisted verification, fresh cook and genuine Shipping stage are separate
observed gates. No generic bootstrap, SaveAll, unguarded helper or copied old
cooked-container shortcut. Inspect an actual game-camera tree silhouette,
grounding and walking collision before selecting a newer usable candidate.
One-tree cost is not broad-forest performance or full woodland acceptance.

### Current delivery order: visible fern clearing first

Use the successful Fern02 assets in a small real-world patch now, before the
rest of the palette. Preserve materials, uniform authored scale and an explicit
ground anchor; leave mesh vertices, terrain and simulation untouched. Keep
deterministic home/resource/structure/plot/stream exclusions and no collision.

A modest static-component patch is acceptable before broad HISM conversion:
the admitted material was proven on static meshes, not cooked instancing
permutations. Keep counts explicit; do not build a new material/import framework
to place the first patch. Evaluate dark undersides/detail in homestead lighting,
not another isolated cosmetic scene.

Then produce a clearing view and usable candidate, handling remaining cook/
package tool boundaries at use. Add tree/grass/ground incrementally. Current
policy permits necessary test-save resets and preview promotion after basic
verified usability, with honest known defects. Full-palette/4K/performance gates
are distinct from first delivery. Earlier serial narratives below are historical
where superseded; security/licensing and owned-process controls remain.

For this slice, cook the real new assets rather than reusing containers that
lack them. Reuse the admitted native/settings supervisor and standard engine
Cook commandlet with source-supported `RunAsCookCommandlet`, Windows target,
one cook process and a fresh candidate `OutputDir`. Retain shader-worker
denial, local cache, disabled Python, inherited marker lifetime and stop
controls; preserve failed partial cook output. Do not invoke the existing
unconditional Python/bootstrap/character reimport path. Package/build tools
are handled directly when needed. User-facing images must come from ordinary
gameplay of the resulting candidate, not another SceneCapture project.
For first delivery, prefer supported loose-file staging of genuine new cooked
data and Shipping code: omit the optional Pak request and explicitly skip
IoStore. Installed `ProjectParams.cs:812-815` and
`CopyBuildToStagingDirectory.Automation.cs:6243-6295` separate these choices;
Windows inherits the optional-Pak platform policy. Verify the actual stage
parameters/layout and runtime loading, not merely command success. An optional
future container build must not block this usable first increment. Do not copy
old containers or an entire engine/source tree, invent metadata, or admit
otherwise excluded UAT helpers.

The existing Shipping build deliberately excludes automated actor spawning and
ticks. The coordinator approved a narrow opt-in on 2026-09-21: reuse the existing
smoke/ordinary-control routes behind explicit `HomesteadShippingQA`, exactly one
route, fresh explicit output and isolated graphics/user/save routing. Reject an
invalid request before loading/saving/resetting a world; do not fall back to a
normal human game. Keep ordinary Shipping QA-off, external-input isolation,
console/debug/trace settings and current helper/network controls unchanged.
Resolve the actual packaged executable/configuration instead of assuming the
Development filename. Main owns this common gate; the UI lane retains its
NativeMenu route. Implement only after the current cook releases source pins,
then rebuild genuine Shipping products. Reuse that cook only if its content,
config and reflected dependencies still match; code gating is not a reason to
recook unchanged assets. First delivery needs basic actual controls and the
ordinary capture, not every historical fixture suite.

The first real cook selected Zen **output storage**, despite the separately
verified filesystem DDC. Its denied service launches and loopback8558
SYN_SENT sockets are retained as a failed, owned hard-stop operation with
observed death and clean protection release. The corrected invocation asserts
and exports `-SkipZenStore` in the actual inner Cook `Main` arguments before
calling it. Installed `CookCommandlet.cpp:425-427` supports that switch directly;
this is not a UAT-only flag or a new endpoint allowance. Require genuine loose
cooked files from the fresh corrected attempt; no old-container substitution.

### 1. Admission before import, and a separate offline execution gate

Current18:08 ordering clarification: the coordinator accepted the corrected
Editor settings/startup/cooperative-stop subgate in `native-settings-success.md`.
A separately approved isolated representative import may follow this subgate
plus completed source admission, without first completing cook/Pak/Shipping
or all of1.3. `fern-spike-plan.md` is the bounded first proposal, not execution
approval. All per-operation privacy/network/containment/output requirements and
all later visual/performance/gameplay gates remain. Full1.3 and2.3 stay unchecked.
The earlier all-engine-work dependency below is superseded only for this
explicitly admitted representative lane; it is not permission for integration.

Use the exact CC0 bundle and fallback in the decision dossier. Fab Black Alder
remains excluded unless a later explicit refresh resolves price/license/source
access; the 2024 Megascans promotion is not evidence of entitlement.

The distinct fallback is small Poly Haven Fir Sapling + Fern 02 + incumbent
ground/rocks/sky, initially omitting the separate grass asset. It reduces declared
source complexity, not proven render cost. Its young conifers cannot be scaled
arbitrarily into mature canopy; if open young woodland fails the composition
gate, retain the accepted baseline and report the upgrade incomplete.

After a new implementation request, the coordinator must identify the accepted
offline/fullscreen baseline and approve the actual editor/commandlet/import
executables and launch procedure. The reported listener-free Shipping result
does not clear new content authoring. No firewall rule, elevation workaround,
account action or use of a Development executable as a shortcut is permitted.
If that gate remains shut, stop before engine execution and report the blocker.

Current preflight status (2026-09-20,14:05 Arizona onward): explicit approval
now covers the reviewed bounded TraceControl exception, not additional services.
Read-only inspection identified pre-main CrashReportClientEditor startup outside
the proposed INI controls. Task1.3 is still unproven; no new engine was launched.
See `authoring-preflight-result.md`. This updates the current blocker, not the
normative privacy, runtime QA or quality requirements below.

The14:41 separately authorized feasibility study subsequently proved synthetic
pre-resume one-process/no-breakaway Job containment and disposable read-sharing
semantics. `containment-feasibility.md` records exact tests, source-supported
in-process shader compilation and the separate UAT/Pak boundary. This is a
possible direct-leaf control, not a filesystem/network sandbox or permission to
launch Unreal/lock the global marker. Task1.3 and all runtime gates stay open.

The15:02 continuation conditionally permits one120-second no-write Editor-Cmd
probe only after the direct-leaf guard, inherited marker lifetime, exact
admission/settings and observation prerequisites pass. The implemented primitive
and disposable tests are documented in `leaf-guard-result.md`; the production
admission/monitor and effective-settings export are unfinished, so no real lock
or engine launch occurred. The probe's job admits no children and uses supported
in-process shader compilation. Its inherited read handle is the lifetime
mechanism, not a filesystem/network sandbox. Task1.3 remains unchecked; these
results do not clear UAT/cook/Pak/import/Shipping-QA or weaken their requirements.

At15:32 the coordinator approved the safer native probe route after the Python
startup/pip trace. Add only an Editor-only UCommandlet module and necessary
descriptor/Editor-target wiring; do not add a native asset importer. Disable
Python completely rather than relying on late script settings. Direct local
UBT/UHT/compiler orchestration is separately observed with NoRemote flags and
reviewed outputs, never put inside the leaf's one-process job. Its eventual
Editor-Cmd launch retains every conditional guard requirement above and the
same single120-second attempt limit. Native configuration/DDC/privacy exports
are settings evidence, not shader workload, rendering or gameplay proof.

The16:00 implementation checkpoint is blocked before runtime: the installed
compiler spawned an unadmitted VCTIP telemetry uploader. The observed owned
build was stopped; the native module has not compiled and no real marker lock
or settings-probe attempt occurred. `native-preflight-result.md` records the
watchdog/control/policy tests and distinct executable decision. Ordinary
diagnostic/cache consent does not authorize telemetry uploaders.

Dependency refinement authorized by the coordinator on 2026-09-20: completion
of tasks1.1/1.2 permits a separately assigned no-engine preparation lane for
tasks2.1/2.2 and the source-only portion of2.3 while1.3 remains blocked. This
changes ordering, not the offline/privacy requirements or import authorization.
That refinement revised planning only; the subsequent separately authorized
preparation continuation is recorded in tasks.md and does not clear task1.3.

That lane is limited to the selected CC0 publisher manifests/files, exact
admission and acquisition receipts, and data-only source inspection. No
accounts, claims, paid content or broader research expansion. Never execute
untrusted blend files or supplied Python. Prefer data-only FBX parsing; any
later Blender inspection requires separate coordinator tool approval of the
official existing binary with factory startup, auto-execution disabled and
offline mode, without external texture paths or scripts. No Blender launch is
authorized by this refinement.

Source/exported-mesh counts, hierarchy, units, slots and maps must be labeled as
such; they are not imported Unreal triangles, renderability, material correctness
or runtime cost. Task2.3 retains its runtime-dependent evidence requirement and
stays unchecked until all of it is met. All engine/import/cook/pak/build/test
execution and environment game/material implementation otherwise remain blocked
until task1.3 is approved and proven. The separately admitted isolated
representative lane above is the only additional ordering exception.

Keep acquisition in a candidate-owned source folder, with exact publisher/license
links, date, chosen source version, bytes, SHA-256 and conversion notes. Extend
the existing manifest/receipt conventions only after files are acquired. Never
guess bytes or hashes from this dossier. Separate source archives, selected
extracted files, authored UE assets and package receipts. An unchanged manifest/
source pair must import repeatably without corrupting existing content.

Alternative rejected: import an entire showcase project and its lighting. That
adds unrelated actors, dependencies and uncertain redistribution obligations.

### 2. Representative static-mesh/material spike, not blind substitution

Import one tree variant, one fern clump and one grass clump plus the new ground
surface into an isolated candidate namespace. Inspect source hierarchy before
choosing whether to split objects; do not apply `combine_meshes=True` blindly.
Use FBX plus explicitly wired PBR channels as the preferred interchange route;
do not assume a Blender Geometry Nodes system transfers to Unreal.
Resolve selected filenames/channels from the admitted asset inventory: the new
ground choices are PNG while `textured_material` currently hardcodes JPG names.
Retain the incumbent JPG path rather than renaming files to disguise formats.

Retain each bark/branch/leaf slot and its intended map set. Use sRGB for base
color and linear data for roughness/alpha/AO; configure normal compression and
the verified DirectX normal convention. Validate masked leaves, opacity mips,
two-sided foliage response and normals in sun/backlight/night. Reconstruct only
the material features actually supported by acquired data; do not invent leaf
subsurface or wind maps. Textures alone do not prove a finished UE material.

Add an explicit authored-material path to the existing part/decoration helpers
rather than changing prototype tint semantics globally. Batch identity must
include the full mesh/material-set identity and render/collision policy, not
just mesh short name plus tint. Preserve incumbent `M_Field`, `M_Ground`,
`M_Rock`, structures, preview and crop behavior. A missing required imported
asset is logged and rejects the upgrade candidate; a displayed primitive
fallback is labeled and cannot pass visual acceptance.

For vegetation, derive uniform scale from verified units and desired height,
preserve aspect ratio, and place the root pivot on `GroundHeight`. Do not use
centered anisotropic rock normalization for a tree. Record source and imported
bounds, root offsets, leaf orientation and collision hulls. Use simple trunk
collision, no collision on leaf/grass/fern cards, and no large canopy collider.

Alternative rejected: applying the old Tint material to every slot. It removes
the very bark/leaf appearance the acquisition is intended to add.

### 3. Keep HISM placement and gameplay authority

Use `BuildDecorations` and its existing deterministic random seeds/exclusions,
with only measured small variants and restrained yaw/scale variation. Begin
with no more accepted trees than the baseline and no more than the existing
1150 grass-patch attempts, using one authored clump per accepted patch initially.
Counts are initial experiment limits, not an asserted performance budget.

Retain the home exclusion of `650 cm + footprint radius`, resource clearance
`130 cm + radius`, structure `225 cm + radius` and plot `175 cm + radius`;
increase visual clearance if a canopy masks interaction. Do not shrink these
to fit larger art. Preserve stream exclusions and edge barriers. Measure
collision/root footprints separately from decorative canopy reach. If a
non-removable tree would occupy buildable clearance or obstruct the normal route,
omit it rather than changing construction rules.

Keep `ResourceLayoutSignature` resource-site reservation even for cleared nodes;
never fill cleared ground with permanent decorative trees. Preserve the
`ResourceVisuals` / `ResourceProduceVisuals` ID and signature model. This first
bundle is principally decoration: berry/root/reed/flower/sapling identities need
not all be replaced merely because a tree mesh exists. If any visual is replaced,
preserve separately removable produce and persistent bases, never baked-in
unharvestable fruit. Keep edible targets, flowers and weeds legible in both
380/740 cm gameplay camera modes and the zoom range.

`HomesteadForageRenewal.cpp` currently hardcodes 3/15/8 produce-component counts.
Replace affected shape-count assertions with semantic ready/depleted/cleared
state, registered visible renderable content, focus/HUD, reward and collision
checks. Keep count diagnostics as observations, not artificial compatibility
requirements. Never weaken harvest-once or save/renewal assertions.

### 4. Material-only ground variation

Retain `BuildTerrain`, `GroundHeight`, `CellBase`, topology and stream/bank
ribbons. Blend the incumbent leaf floor toward the chosen grass surface with a
small deterministic world-space mask tied to woodland/meadow/route regions.
Use each surface's documented tile scale as a starting point; avoid a universal
XY/300 assumption for every material. Blend color/roughness/normals consistently,
with smooth boundaries and low-frequency variation, not contrast-hiding exposure.
No displacement, tessellation, vertex WPO, imported terrain or hidden geometry
change is allowed. Keep plot moisture and crop visuals distinct.

Grass Path 3 is an alternate route/bank surface only if needed, not a third
mandatory full-world texture layer. Existing moss rocks retain their authored
material and simple collision proxy.

### 5. Native dynamic sky is optional, separately measured

Keep `BuildLighting` / `UpdateLighting` as sole owner. First evaluate the foliage
under unchanged sky/exposure. Only after that comparison passes, trial one
native Volumetric Cloud component with a project-owned material instance, using
the documented engine material after its installed name is verified. Bind
coverage/density and any wind variation to the existing clear/rain/night state.
Do not add an HDRI backdrop, imported lights or an independent clock.

Start at restrained ground-view quality (one scattering octave; evaluate Beer
shadow maps), retain realtime skylight time slicing, and do not turn on cinematic
cloud tracing. Use a reversible candidate-only cloud enable switch rather than
adding an unrequested player settings screen. If the cloud-only comparison has
little visible value or consumes headroom, ship the existing sky. No external
sky asset or new plugin is required.

### 6. Bounded budgets are hypotheses, not asset metadata

Source-backed concerns: masked overdraw, WPO bounds, moving-sun shadow cache
invalidation and LOD/shadow work (S14-S17). Tree/scatter source polycounts are
not runtime triangle or VRAM budgets. There is no verified "Nanite wins" claim.

Initial proposed controls, recorded before the spike:

| Dimension | Initial control / gate | Measured evidence needed |
| --- | --- | --- |
| Asset breadth | One tree species initially, at most three tree variants and three understory mesh variants; no per-tuft actors | Actual imported meshes/slots, accepted instances and HISM batches |
| Textures | Start 2K tree/ground and 1K understory; increase only for a demonstrated camera-distance defect | Texture formats/mips/residency and delta VRAM; no mandatory 4K/8K set |
| Materials/instancing | Target at most 16 new mesh/material-set batches; share material instances when parameters match | Actual batch and draw cost, not component count alone |
| LOD/Nanite | Conventional authored/generated LODs first; one controlled Nanite A/B only if source/spike warrants | Same placements/settings; leaf density, silhouettes, streaming and shadow timing |
| Wind | Static first; add only supported bounded WPO after material correctness; initial distant cutoff 15-25 m | Root anchoring, bounds, culling, motion/temporal artifacts and WPO-on/off cost |
| Shadows | Trunk/canopy shadows retained where useful; grass shadow distance/density separately bounded | Low-sun moving-light and wind-on worst case; no stale-cache "optimization" |
| Memory | Aim for at most 1 GiB incremental resident GPU memory over matched baseline; zero pool-overflow warnings | Actual GPU-memory accounting if available; system RAM is not VRAM |
| Clouds | Disabled by default during foliage trial; aim at no more than 1 ms incremental GPU cost | Same scene cloud-only A/B, actual GPU timing; absent measurement means no cost claim |

These values are project starting limits, not publisher promises. If a source
needs more complexity just to be valid, test the documented fallback rather than
silently expanding budgets or adopting an experimental foliage pipeline.

### 7. Exactly two batched visual reviews

Use the accepted latest hotkey-safe renderer as control. Register camera
transforms, FOV/distance, deterministic seed/save fixture, time/weather, exposure
settling and settings once. Use synthetic fixture saves, never personal worlds.
Review matching clearing arrival, tree edge, resource approach, stream bank and
home/doorway views at dawn (06:00), clear noon, rainy noon and night (22:00).
Use the actual state-hour/day needed for the existing rain schedule. Include a
short normal-control walk and canopy-to-clearing turn; disclose any fixture
teleports/time setup separately from ordinary movement.

Batch A: representative tree/material/ground spike versus baseline. Admit or
reject the palette before wide placement. Batch B: coherent final candidate
after one bounded correction, including cloud-on/off comparison only if tried.
Compare silhouettes, leaf alpha/halos, root placement, surface scale, repetition,
resource contrast, open routes, wind and settled night readability. No endless
third cosmetic pass; retain useful isolated work and hand off unresolved defects.

Every accepted capture records actual Lit mode, Lighting on, Shader Complexity
off, requested scale 100%, runtime primary render fraction 1.0, actual
3840x2160 viewport/output, dynamic-resolution state and upscaler/scalability/
VSync/frame-cap settings. Recheck flags after save/load; PNG dimensions alone
do not qualify. No global exposure adjustment to conceal wrong assets.

### 8. Targeted functional and performance validation

After the offline gate, run existing native tests, then affected packaged
functional and renewal routes. Cover gather/deplete/early rejection/normal
renewal/re-gather across save/relaunch; persistent cleared saplings; plot and
structure decoration refresh; movement/camera/trunk/ground collision; and
constructible doorway/fire/storage access. Preserve accepted action/full-loop
coverage and audio behavior. Build a fresh candidate from the actual changed
content; an old Shipping cook cannot validate new imports.

For timing, use a separate screenshot-free packaged traversal after a 30-second
warmup: three 60-second runs for matched clear noon, low sun/wind and rain/night
views. Include actual moving sun and maximum intended wind, normal turns and
decoration refresh. Record CPU game/render times, GPU time if available, frame
time median/p95/p99, hitches, streaming/pool warnings and memory. Record competing
GPU activity rather than stopping unrelated processes.

Goal: CPU/GPU work fits 16.67 ms for a 60 FPS target at requested 4K/100%.
Proposed candidate regression gates: median frame time no worse than baseline
by 10%, p95 at most 20 ms, p99 at most 33.3 ms, under 1% of post-warmup frames
over 50 ms, and no reproducible new streaming/refresh hitch. These gates allow
occasional misses and therefore do NOT certify stable physical 60 FPS. State
which absolute goal and relative gates pass; do not silently relax one because
the baseline fails it. A baseline failure is reported separately.

If native 4K misses, reduce candidate density, expensive wind/shadows or clouds
first. A separately labeled 85%/70% comparison can inform later quality choices,
but does not satisfy the 4K/100 diagnostic. Missing GPU or Present evidence is
explicitly "unmeasured." Framebuffer/tick timing cannot prove scanout, VRR,
physical 60 Hz or fix the previously observed 30 Hz/tearing issue.

## Risks / Trade-offs

- High-resolution sources or unclear LOD hierarchy -> inspect one mesh first;
  fail admission before bulk conversion, use the explicit fallback.
- Mixed biogeography or source coloration -> fictional woodland, not a botanic
  reconstruction; restrained palette and matched review still decide coherence.
- Natural groundcover hides produce -> preserve resource exclusion/contrast;
  reduce cover rather than alter gathering range or rewards.
- Material preservation adds batch variation -> cap palette and share compatible
  instances; do not flatten all slots to win a draw-count proxy.
- Conventional LODs versus Nanite has no universal winner -> isolated optional
  A/B, no experimental pipeline adoption in this change.
- Dynamic clouds consume effort with little ground-view benefit -> optional
  independent gate; keeping the existing sky is an explicit valid outcome.
- Import gate remains blocked -> report only actual separately authorized
  no-engine preparation, not a runnable overnight promise; no engine invocation
  or environment implementation until the workflow is approved and proven.

## Migration Plan

1. Obtain a new apply request; coordinator establishes bounded run, accepted
   offline/fullscreen baseline and single engine writer (tasks1.1/1.2).
2. With a separate preparation assignment, refresh/admit exact sources, acquire
   only approved files and record receipts; inspect source data without engine
   execution. Tasks2.1/2.2 and source-only2.3 may precede the blocked1.3 gate;
   runtime-dependent2.3 evidence remains pending.
3. Obtain and prove the exact task1.3 authoring workflow approval, then register
   the task1.4 matched baseline before engine-dependent work. Complete remaining
   admission evidence, build the isolated namespace/representative spike and
   complete Batch A. Preparation alone never authorizes this step.
4. Integrate only the admitted palette, preserve state/terrain, run targeted
   checks and screenshot-free timing; complete Batch B.
5. Package in a fresh candidate directory with source/content/executable hashes,
   asset credits, proof index and honest technical status. No automatic preview
   selection, merge, PR or deletion of the earlier playable copy.
6. Coordinator reviews evidence and chooses any preview promotion. Jenny's
   aesthetic/controller approval remains separate. Rejection leaves the old
   selection/assets/saves intact; revert only candidate-owned changes through a
   normal reviewed commit, never reset shared work or overwrite personal saves.

## Open Questions

Deferrable empirical details: source mesh LOD hierarchy and units; actual leaf
alpha/subsurface response; wind data availability; best conventional/Nanite
choice; exact installed native cloud material; GPU/VRAM costs. The spike and
fallback policy already define how each answer affects implementation.
Current price/license gaps are not deferred admission: those assets are excluded.
