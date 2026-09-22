# Design

## Reuse and ownership

Baseline `c933270622946404e792ce0e2e4bc117912d4f19`. Reuse
UProceduralMeshComponent and global-coordinate normal sampling, not a new terrain
framework. Reuse ResourceNode, Clear/hatchet/rewards, placement/tilling checks,
existing admitted art, engine guards, cooked assets where unchanged and native
test routes. Current height function is analytic, not a seeded generator.

Generation45e owns only `Simulation/HomesteadWorldGeneration.h/.cpp` and
`Tests/HomesteadWorldGenerationTests.cpp`. Authority6e4 owns only
`Simulation/HomesteadSimulation.h/.cpp` and `Tests/HomesteadSimulationTests.cpp`.
Main owns canonical docs, World/Controller/save wrapper, shared native adapters
and all shared compiler/engine integration. Progression0a owns docs/catalog only.

## Current joint contract, generation version4 palette correction

- Namespace `Homestead::Generation`: `WorldDescriptor{uint64_t seed,
  uint32_t generationVersion=4}`, `ChunkCoord{int32_t x,y}`,
  `GeneratedEntityKey{ChunkCoord chunk,uint32_t localId}` with exact comparison.
  Durable identity includes descriptor plus composite key, never a hash alone.
- Chunk size2400cm,24x24 terrain cells (100cm NEW-world quads),625 row-major
  vertices. This intentionally replaces the old25cm fixed-world tessellation.
  Global integer-cm/int64 intermediate coordinates; negative ownership uses floor
  and half-open intervals. Height and +/-50cm normal samples use the same global
  function so shared edges do not depend on load order.
- Integer-hash/fixed-point rolling terrain noise. Version3 must produce plainly
  legible rolling slopes and valleys at gameplay scale, not merely a nonzero
  numeric range that looks flat. Keep bounded walkable slopes, exact global seam
  sampling/normals/collision, stream agreement and a safe spawn/camera segment.
  Flatter pockets are discovered from the same authoritative height function for
  building; they are not visual-only flattening or a prepared home clearing.
  Unsupported version/range returns explicit status without mutating output.
- `GenerateChunk` and `FindEntity` expose the same immutable baseline.
  `TerrainSample` includes height, normal and woodland weight. Max36 mature-tree
  candidate slots/chunk with bounded clustered jitter and high occupancy, plus
  at most28 forage candidates. Stable local IDs encode kind/candidate BEFORE
  filtering, independent of acceptance order, rewards or economy values.
  The first real version1 image showed orchard-like rows. Version2 retains slot
  identity but varies offsets toward deterministic macro-clusters and permits
  occasional small natural openings; it does not create a prepared house site.
  Old version1 saves are rejected explicitly, not silently regenerated as version2.
- Version3 also carries a stable visual species/variant identifier derived from
  world descriptor plus generated key, independent of load order and transient
  handles. It maps to a small license-verified palette with genuinely different
  silhouettes/ages/sizes; random scale of one mesh is not diversity. Durable
  resource keys, felling edits and current provisional rewards remain unchanged,
  leaving room for later species-specific yields without rekeying cleared trees.
  Each mapped mesh uses its own measured pivot/trunk anchor/collision and LOD/
  material readiness checks. TreeSmall02 is one role, not the whole canopy.
- Version4 retains version3 terrain and key positions, but changes the stable
  mature-role distribution after firsthand v3 review: ordinary broadleaf60%,
  ordinary conifer35%, landmark/accent5%. Accent keys in the protected start-view
  neighborhood are reassigned to ordinary broadleaf rather than removed. Runtime
  maps ordinary broadleaf to TreeSmall02, ordinary conifer to FirPole, and the
  rare accent to Jacaranda. This prevents giant-canopy clusters near spawn while
  preserving deterministic identity and density.
- All seven existing forage kinds and appended `ForestTree=7` derive from
  generated keys. No fixed98-node population in the new world. Keep loose branch
  scatter restrained (initially at most one branch patch/chunk); other kinds
  may use up to four slots. Ensure reachable bootstrap roles for the default
  starting region without granting tools or creating a house clearing.
- Retain the existing stream-center formula
  `1500 + 180*sin(y/800)` as a shared, explicitly limited first adaptation.
  Generation owns consistent channel/water-bank sampling; Simulation and World
  must agree. It is not seeded regional drainage, mountains/rivers/lakes complete,
  nor proof water follows a watershed. Preserve that shared libm formula; integer
  hash/IDs/noise are exact, while cross-platform stream/normal comparisons use
  documented tolerances. Do not promise cross-platform bitwise floating results.
  Broader hydrology is roadmap work.

## Authority, edits and save handling

`State` carries the descriptor and sparse `ResourceEdit{key,cleared,readyAtHour}`.
The moving3x3 active resource/physics cache materializes generated nodes with
positive transient handles starting1000000. Handles are not durable identity:
retain them for overlapping active keys, never reuse during the session, reject
exhaustion explicitly. Persistent structure IDs remain in their separate lower
namespace. Rendering variation is derived from key, not transient handle.

`SetActiveWorldRegion(Point)->Result` prepares the bounded active cache and
increments revision only on change. Expose side-effect-free resolution of a
generated key plus saved edits for the outer visual band. Activation/load errors
must preserve prior state. Main invalidates/recomputes focus after activation.

Both ForestTree Harvest and Clear use the same permanent fell transaction:
hatchet,300cm reach, provisional8Branch+2Fiber once, atomic full-pack rejection,
cleared=true and no regrowth. Native primary and clear inputs both play existing
chopping feedback. Ordinary sapling harvest remains renewable. Build/Till reject
uncleared sapling/tree occupancy; mature trunks use conservative50cm circle vs
cell overlap, not merely matching cell index. Actual cleared sites also remove
conflicting decorative cover.

Only minimal deterministic starting safety: pawn at(-1000,0),90cm trunk-center
exclusion and a60cm-radius rear-camera segment from(-1300,-80) to(-1000,0).
No home-site disk, row of cleared house cells or equivalent disguised clearing.
Apply exclusions consistently in authority, occupancy and render resolution.

Fresh-start camera correction uses actual loaded tree bounds and the normal camera
sweep to select a less-obstructed initial yaw. It does not hide trees, alter their
collision/materials, clear vegetation or constrain later manual orbit. Bounding-box
scores are only an initial-view heuristic; actual images remain the acceptance.
Fresh starts snap to their generated ground; saved views/structure heights remain
authoritative on load. Continuous camera-foliage handling remains a separate limit.

Portable savev5, generation version4, wrapper6/new disclosed test profile.
Do not append invented trees/seeds to old saves. Persist edits, structures and
plots independently of live chunks; leave/return/restart reapplies them.
First runtime supported coordinates are+/-1000000cm (10km each direction), not
a3x3 world. Explicit record/byte limits must fail atomically before a mutation
or save, never silently evict edits or create an unexplained exploration wall.
Initial sparse-edit ceiling16384,8MiB save payload and existing4096 structure/plot limits are
disclosed safety bounds, not an unlimited-world claim; unedited exploration must
not consume durable edit records. Continue existing global plot time semantics
within that bound; spatial rendering is limited to loaded chunks.

## Runtime integration and budget

Prepare colliding3x3 terrain before releasing obsolete chunks or moving a loaded
save/fixture pawn onto it. Keep current/near ground and overlapping resources
until replacements are ready. Ordinary adjacent movement is protected by the
neighbor halo; teleport/load must explicitly prepare destination first.
Visible scenery may extend to5x5 chunks, but uses the SAME generated identities
and persistent edits. It becomes actionable as the active window moves; it is
not a separate unremovable forest. Never unload live occupied player ground.

Use existing native-scale shared meshes/LODs. Appropriate instancing requires
real cooked material usage/readiness, not fallback shaders. Measure actual live
chunk/component/instance/triangle bounds and actor timing separately from
captures; no fixed tree count is an aesthetic acceptance criterion.
Remove fixed edge rails/home-clearance assumptions, update labels/startup/focus/
placement and save surfaces, preserve controller/camera/audio/day-night work.

Outer visual-only mature trees use one hierarchical instanced batch per exact
loaded mesh path. Active3x3 actionable trees remain individual static-mesh
components so focus, collision, felling and observer behavior do not change.
The outer5x5 ring is rebuilt from generated keys, persistent edits and exact
ground-anchored transforms whenever the world descriptor, active chunk or layout
signature changes; mutable instance indices never become authority. Rebuilds
destroy prior batches, omit cleared keys and retain no stale components across
travel, return or reload. Inventory evidence compares expected outer keys to
total batch instances, exact representative transforms and distinct mesh paths.
Active-tree batching is a separate reviewable layer only if runtime measurement
shows the outer-only optimization is insufficient.

## Stages and acceptance

First deliverable crosses the old80m boundary in actual generated woodland,
fells a reachable mature tree with real controls, frees a build/plot site and
survives travel away/return plus process save/reload. Capture untouched-start and
actual player-cleared site views. Disclosed resource provisioning is allowed
for a controlled build fixture, never labelled ordinary or untouched-start.
Inspect lushness, seamless terrain and readable resources, not only counters.
Neutral-daylight images must show a close mixed grove with multiple silhouettes
and a wide ordinary gameplay-camera slope/valley view with visible elevation.

Later stages: mountains, drainage-connected rivers and lakes, expanded biomes
and progression/economy integration. Version3 rolling woodland relief is required
now, but is not evidence those broader regional systems are complete.
