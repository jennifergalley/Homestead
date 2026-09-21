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
