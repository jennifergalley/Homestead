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

### 1. Admission before import, and a separate offline execution gate

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

Dependency refinement authorized by the coordinator on 2026-09-20: completion
of tasks1.1/1.2 permits a separately assigned no-engine preparation lane for
tasks2.1/2.2 and the source-only portion of2.3 while1.3 remains blocked. This
changes ordering, not the offline/privacy requirements or import authorization.
The current assignment revises planning only; acquisition awaits coordinator
review and a separate apply continuation.

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
execution and environment game/material implementation remain blocked until
task1.3 is approved and proven. No other task receives an ordering exception.

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
