# Tree source preparation lane

## Scope and authorization

Reactivated 2026-09-21 by Homestead Orchestrator for independent source-only
preparation of the already admitted Poly Haven Tree Small 02. The current
game-plan policy snapshot supplied by the coordinator supersedes this branch's
historical planning-only and arbitrary iteration/deadline restrictions.
This lane implements only the tree-source portion of task 2.3 and supports
later LOD/material work; it does not claim whole central tasks complete.

Owned files: `Scripts\Environment\TreePreparation.*`,
`Assets\Environment\TreeSmall02Prepared\`, and this addendum.
Raw copied originals remain ignored and unchanged at
`Assets\Source\woodland-preparation-20260920-182217-d1f84e39\tree_small_02`.
The coordinator supplied original download receipts and source inventory in
this session's `files\tree-source-context` directory.

Reuse official Blender 4.5.14 FBX IO and standard mesh reduction, with factory
startup, disabled automatic asset script execution, CPU-only operations, and
lane-owned profile/temp/output. Inspect actual geometry first. Prevent imported
legacy absolute texture references from resolving; materials must use only
the admitted texture whitelist. No custom decimation engine or handcrafted tree.

Do not modify central proposal/design/tasks, game code, native import commandlet,
common guard/build scripts, existing imported content, launcher or main checkout.
No Unreal/UBT/UAT/cook/native helpers, GUI/GPU captures, external research,
downloads, account/security changes, schedules or Notion writes.

## Deliverable and acceptance

Produce versioned source-derived conventional LOD FBXs (separate files are
acceptable), retaining a valid source-derived LOD0 and explicitly identifying
practical reduced candidates. Record actual triangle counts per material,
bounds, root/pivot, units, both UV layers and all three material slots; bind
outputs to source/license/tool hashes. Reimport exports through Blender to
check the handoff contract, and inspect a concise CPU/source-only silhouette
comparison when useful. This is not an in-game image or runtime performance pass.

Main implementer owns Unreal import, material reconstruction, instance placement,
collision, runtime LOD selection and playable validation. Freeze a version before
handoff; later experiments must use a new version. Commit and push this private
branch only after verification; no force/merge/PR.

## Progress

- [x] Confirmed isolated worktree, scoped authorization and existing OpenSpec context.
- [x] Verified copied originals against acquisition receipts and inspected geometry.
- [x] Prepared conventional reduced variants and exact material/texture contract.
- [x] Checked exports, provenance and source-only visual comparison.
- [x] Frozen v3 and reverified it without changing any prepared-file hash.

The source commit and remote ref are reported to the coordinator/main at
handoff after the private push succeeds; they are not self-referential fields
inside the frozen asset directory.

## Source results

Selected handoff is `Assets\Environment\TreeSmall02Prepared\v3`, with exact
integration contract in that namespace's README. Actual source triangles:
2,062,487 (branches 94,814, leaves 1,939,380, trunk 28,293).
Selected reduced levels: 378,926 / 231,785 / 170,289 triangles. All retain the
three ordered material regions and both UV layers; four export/reimport checks
passed. Root remains (0,0,0); source height is about 4.5567 m with approximately
2.4 cm below-root geometry. FBX unit metadata is explicit, not guessed scaling.

Aggressive leaf reduction produced visible canopy loss and was rejected.
Planar reduction retained excessive source geometry. The final conservative
standard-collapse chain preserves approximately 99% front/side pixel coverage
at its nearest reduced level; farther levels lose more detail (LOD3 side about
12% thinner), documented rather than called an invisible optimization.
Two internal CPU/source views were reviewed; they are not game screenshots.
Raw trials remain in this session's files, not in the published source namespace.

No main game code, central planning artifacts, existing content, launcher,
Unreal processes or shared authoring/build tools were modified/launched.
Main still owns in-game appearance, material response, collisions, instancing,
runtime LOD selection and performance. This lane does not mark those tasks done.

## September 21 visual resource continuation

Jenny's 13:39 Arizona feedback rejects the sparse woodland and primitive
resource silhouettes. Main confirmed live baseline `5ccddea` and owns the
canonical proposal/design/tasks, composition, native adaptation and all engine
execution. This addendum records only this lane's coordinated source scope;
the branch's older central task counts are not current integrated progress.

Reuse Tree Small 02, Fern 02, the four already prepared Grass Medium 01 meshes,
Grass Ground and incumbent moss rocks/leaf floor. The current 16-tree cap,
full-canopy exclusions/separation and omission of all nonselected trees are
composition concerns, not reasons to wait for another asset catalog.

Main reserved and authorized acquisition of exactly these four CC0 assets:
Shrub 04, Dry Branches Medium 01, small Fir Sapling and Flower Empodium.
Publisher previews were inspected before selection. Shrub 04's living broadleaf
shoots suit a compact berry-base assembly better than sparse Shrub 02. Preserve
separate ready/harvested produce; the source shrub does not contain berries.
Fallen log acquisition is deferred. Existing natural leaves/grass can provide
honestly labeled generic low-herb and bank-grass silhouettes for roots/reeds,
not a claim of botanical crop or cattail authenticity.

Owned continuation paths: `Assets\Environment\WoodlandResources\candidate01`,
fresh ignored `Assets\Source\woodland-resources-20260921`, narrowly named
`Scripts\Environment\Resource*` tools/tests and the resource-palette research
note. Keep frozen TreeSmall02 v3 unchanged. No main-checkout, common-script,
central-plan, existing-content, launcher or source-original edits.

Acquisition reuses the established explicit-host/no-redirect download pattern,
expected bytes and publisher MD5 (left-padded to 32 hex digits), SHA-256 receipts,
no overwrite, and current coordinator stop checks. Do not spoof the old
hardcoded run. Main will execute source preparation at its reserved admitted
slot after receiving input hashes/inventory; this lane launches no Blender,
compiler, editor, import or build.

Canopy-tier continuation is a proposal/script only until main executes it.
Do not repeat known near-view failures or promise a 15k tree: the prior
73k/55k collapse trials lost canopy detail, and preserving boundaries by planar
reduction left over 1.4M triangles. Preserve the measured 231k near source.
A lower mid tier needs evaluation at its actual switch distance; a far tier
needs a concrete leaf/cluster-preserving approach and measured role UV/PBR,
not arbitrary whole-tree collapse. Main explicitly accepts evidence-driven
counts rather than exact 60k/15k targets.

### Continuation status

- [x] Read current baseline resource/decorative roles and existing source contracts.
- [x] Visually compared author previews and selected the four named acquisitions.
- [x] Agreed isolated acquisition ownership and main-only authoring execution.
- [x] MAIN acquired the exact pinned files after targeted checks and live control input.
- [x] Read actual source inventory and author the explicit selected preparation inputs.

Ordinary in-game beauty is the acceptance target. Publisher thumbnails and
source checks support selection; they are not that deliverable.

Acquisition execution subsequently moved to MAIN, with the same four-asset
reservation, because MAIN can read its authoritative live run control directly.
This lane supplies the pinned manifest and tested narrow helper; no stale
control mirror or duplicate download is created. The actual source receipt and
inventory remain pending until MAIN executes acquisition.

MAIN subsequently reported successful acquisition of all 29 originals
(58,729,776 bytes) with publisher MD5 and actual SHA-256 checks. This lane
published a read-only inspector that reuses the existing bounded reader and
adds per-role UV/normal/reference diagnostics; actual image dimensions are
read from headers, not guessed from "1k". An optional MAIN-only preparation
adapter reuses the existing image-reference stripper without changing model
geometry, roles, UVs or transforms. Actual inventory and prepared-output
execution evidence belong to MAIN.

MAIN also took the canopy LOD experiment into its existing native static-mesh
reduction path. No separate canopy preparation script is being developed here.

The final source selection is pinned to MAIN inventory commit
`c163a1da9895c492793e7fe809b561cdc80eeaf5`: shrub a/c, branch a/b/c, fir a/c and
flower a/b, nine meshes and five source material roles. Both fir models retain
two ordered material links. All selected roles use UV0 and have zero flagged
near-zero normal corners/UV determinants/diagnostic fan areas at the stated
thresholds. All 25 image headers are actually 1024x1024.

The MAIN-only preparation adapter reuses the existing selected-object algorithm
and official parser/encoder, with explicit one/two-material counts instead of
the grass-only one-material assumption. It removes unused models and image
objects, preserves exact selected bindings/arrays/transforms, and reparses to
check identity. It writes only fresh `candidate01\Prepared\*_selected.fbx` inputs
and provenance. Actual execution/containment and native beauty acceptance remain
MAIN's responsibilities, not source-lane claims.

## September 21 tree-variety continuation

Jenny explicitly rejected a forest built from one copied tree and requested
distinct models, sizes, varieties/species. Main retained acquisition/import/
engine ownership and asked this lane for a bounded source palette only.

The local inventory contains only Tree Small 02 and Fir Sapling (`a`/`c`).
No other tree FBX/glTF/GLB/Blend exists in the known project asset roots;
resource shrubs are not reclassified as trees and LODs are not variants.
The truthful ready palette is therefore three forms, not four-to-six species.

The frozen complete palette and exact limitations live in
`docs\research\environment-assets\tree-palette-20260921.md`. It retains those
ready forms and recommends four acquisition-required additions:
Jacaranda Tree, Island Tree 02, Fir Tree 01 `c`, and Fir Sapling Medium `c`.
They provide spreading mature broadleaf, gnarled low broadleaf, tall mature
conifer and intermediate conifer-pole silhouettes. Public descriptors supplied
native metre bounds, indexed triangle/material facts and layout transforms;
publisher previews were personally compared.

Roles are stable silhouette/age strata, not botanical species. The result is
a fictional warm mixed woodland, explicitly not a European ecology: existing
Tree Small 02 is African *Burkea africana*, Jacaranda is subtropical South
American, and Island Tree 02 is coastal/unspecified. No species claim is hidden.

No recommended addition exists locally, so none is import-ready. Main must
approve/acquire and establish actual hashes before any preparation. Catalog
`lods:true` is not treated as a measured FBX LOD chain. Fir Tree 01's 249 MB
FBX exceeds the established per-file cap and requires an explicit acquisition
decision, not a silent policy relaxation. No files were downloaded, prepared
or imported by this continuation.
