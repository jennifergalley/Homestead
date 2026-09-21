# Candidate resource source acquisition

Four exact assets: Shrub 04, Dry Branches Medium 01, small Fir Sapling and
Flower Empodium. See `docs\research\environment-assets\resource-palette-20260921.md`
for actual game-role mapping, visual selection and qualifications.

**This is a reviewed acquisition input, not an acquired/prepared asset receipt.**
All 29 declared URL/size/MD5 entries match the live publisher manifests checked
2026-09-21; declared total is **58,729,776 bytes**. The 1K source palette includes
the four FBXs and selected PBR/alpha/mask maps, not archives or authoring scenes.
No publisher preview image is shipped here.

Manifest SHA-256:
`BDB728405F33D82CDE68D27E0F316C3B2988311B74A328CEFCFE46175332BAE5`.

## Coordinated execution

Acquisition execution moved to MAIN so it can read its authoritative live
control directly. Do not launch a duplicate from the isolated source lane.
MAIN may run the following from its checkout after integrating this increment:

```powershell
python .\Scripts\Environment\ResourceAcquisition.py `
  --manifest .\Assets\Environment\WoodlandResources\candidate01\asset-manifest.json `
  --expected-sha256 BDB728405F33D82CDE68D27E0F316C3B2988311B74A328CEFCFE46175332BAE5 `
  --control .\Automation\run.json --run-id 20260921-033354-2d257ba0 `
  --source-root .\Assets\Source\woodland-resources-20260921 `
  --receipt .\Assets\Environment\WoodlandResources\candidate01\download-receipt.json
```

The helper reuses the established explicit-host/no-redirect streaming pattern,
exact bytes, publisher MD5 left-padding, SHA-256, retained failure partials and
immutable receipt. Run ID/state are checked before each file and stream chunk.
The current `until-complete` policy ignores a historical deadline; `bounded`
still enforces its timezone-aware deadline. Paused/stopped/changed runs fail.
No engine, compiler, Blender, source-file execution or automatic dependency
lookup occurs. Source output and receipt paths are explicit.

The helper does not edit the common downloader, fake its old run ID, or create a
stale control mirror. Re-execution verifies a matching sealed receipt; changed
files or missing receipted sources fail rather than overwrite or reacquire.
Incomplete `.download` files remain for diagnosis.

## Checks and next handoff

`python .\Scripts\Environment\ResourceAcquisition.Tests.py` passes offline
checks for the actual four-asset manifest, unsafe paths/hosts/redirects, size/
MD5 errors, no-overwrite/partial retention, stop handling and manifest pinning.

Actual acquisition must produce the receipt before geometry/material claims.
Source inspection and preparation are a separate coordinated increment.
MAIN alone executes admitted authoring and Unreal import at its reserved slot.
Do not infer mesh count, material slots, active UVs or working LODs from the
publisher's aggregate labels. Preserve separate berries/flowers/harvest state.

## Read-only source inventory

`ResourceInspection.py` reuses MAIN's existing `inspect_woodland_sources.py`
bounded FBX/image reader and requires its admitted hash. It additionally
attributes UV/near-zero normal data by material role and distinguishes unused
from referenced control-point bounds. It never evaluates a Blender scene,
follows embedded image paths, transforms geometry or exports assets.

MAIN can execute it with the actual reviewed reader hash supplied by its
existing invocation evidence:

```powershell
python .\Scripts\Environment\ResourceInspection.py `
  --manifest .\Assets\Environment\WoodlandResources\candidate01\asset-manifest.json `
  --receipt .\Assets\Environment\WoodlandResources\candidate01\download-receipt.json `
  --source-root .\Assets\Source\woodland-resources-20260921 `
  --output .\Assets\Environment\WoodlandResources\candidate01\source-inventory.json `
  --control .\Automation\run.json --run-id 20260921-033354-2d257ba0 `
  --inspector .\Scripts\inspect_woodland_sources.py `
  --inspector-sha256 '<actual-admitted-reader-hash>'
```

Substitute the actual admitted reader hash in the final argument; do not guess
it from a different checkout's line endings. A previously existing output is
not overwritten.
Run `python .\Scripts\Environment\ResourceInspection.Tests.py` for the
offline role/bounds cases. A read-only smoke against frozen tree LOD2 reproduced
its known 384,193 referenced control points, branch UV1/UV0 distinction and
338 branch zero-normal corner references without changing its hash.

The inventory reports actual PNG IHDR width/height/bit depth and decoded bands,
not an assumed 1024-square texture from a `1k` filename. Fan-triangle counts and
raw local transforms are source facts, not imported render geometry. Parser
stop checks occur at file/pass/geometry boundaries; the reused reader retains
its own node/depth/120-second bounds. No separate canopy experiment is included.

## Optional image-reference-free preparation at MAIN's authoring slot

`ResourcePreparation.py` reuses the already admitted selected-model algorithm
and `TreePreparation` official binary parser/writer modules, with a narrow
extension to explicit one/two-material bindings. MAIN invokes it only
through the existing root-only guarded Blender caller and inherited
`HOMESTEAD_SOURCE_STOP_PATH`, not a raw Blender launch from this source lane.
It requires:

- `--manifest`, `--receipt`, `--inventory`: the actual acquired/inspected inputs.
- `--selection Assets\Environment\WoodlandResources\candidate01\selection.json`:
  the nine main-confirmed models, IDs, geometry IDs and ordered material roles.
- `--source-root`: the unchanged raw directory above.
- `--output Assets\Environment\WoodlandResources\candidate01\Prepared`:
  MAIN's agreed fresh native-input directory.
- `--helper Scripts\Environment\TreePreparation.py` and
  `--helper-sha256 <actual-admitted-helper-hash>`.
- `--control Automation\run.json --run-id 20260921-033354-2d257ba0`.

It writes four `*_selected.fbx` candidates and provenance, retaining exactly
nine models: shrub a/c, branch a/b/c, fir a/c and flower a/b. Other models and
all image objects/connections are removed. Reparse comparison verifies selected
geometry/model/material arrays, transforms and non-image connections, including
material order. Both fir models preserve their two ordered material bindings.
The common helper remains unchanged.

Agreed native inputs are therefore
`Prepared\shrub_04_selected.fbx`,
`Prepared\dry_branches_medium_01_selected.fbx`,
`Prepared\fir_sapling_selected.fbx`, and
`Prepared\flower_empodium_selected.fbx`. MAIN's planned native names are
`SM_Shrub04_a/c`, `SM_DryBranchesMedium01_a/b/c`, `SM_FirSapling_a/c` and
`SM_FlowerEmpodium_a/b`; those packages are not produced by this script.

No scene import, texture loading, mesh merge/reduction, normal/UV repair,
world-axis conversion, PBR graph creation or render is performed. The previous
raw files stay byte-identical. Source cancellation is checked before/after each
asset operation; individual existing parse/encode/reparse calls are not
interrupted internally. Actual duration and caller containment remain MAIN's
execution evidence. Offline retained-data and explicit multi-role selection tests
pass, but this lane has
not executed the preparation against the newly acquired assets.

Image-free preparation is not a new mandatory infrastructure gate if MAIN's
existing native importer can safely use the same receipted originals with
explicit dependency loading disabled. Use the established route that reaches
the actual woodland sooner without skipping source/material correctness.

## Measured selection

`selection.json` is pinned to MAIN's actual inventory/receipt from
`c163a1da9895c492793e7fe809b561cdc80eeaf5`, not the earlier publisher estimates.
The nine selected models have 3,726 / 6,177 shrub triangles; 5,933 / 5,254 / 5,616
branch triangles; 157,402 / 124,743 fir triangles; and 758 / 758 flower triangles
(source fan counts until native triangulation is checked). All selected roles
use UV0 and have no reported near-zero normals, UV determinants or diagnostic
fan areas at the documented thresholds. Referenced bounds exclude unused
control points; actual native transformed bounds remain MAIN's measurement.

All 25 acquired image headers are 1024x1024. The unused shrub/fir `Mask` maps
remain preserved originals but are not proposed native shader inputs.
MAIN plans 23 texture maps and five materials for these resources, plus its
separate native canopy mesh. Whole flower clumps are removable produce above
an admitted low-grass base; there is no separate blossom slot/petal-only claim.
Both real fir heights are retained rather than enlarging the shorter variant.
