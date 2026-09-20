# Woodland source preparation 01

**Source acquisition/inspection only. Not imported, cooked, rendered or promoted.**
Task1.3 remains blocked and task2.3 remains partial. These records do not feed
the existing bootstrap or `Assets\asset-manifest.json`.

Authorized run: `20260920-182217-d1f84e39`, deadline
`2026-09-21T02:22:17.1884351Z`. Planning checkpoint: `ba091b6`.
Source root (ignored):
`Assets\Source\woodland-preparation-20260920-182217-d1f84e39`.

## Provenance and selection

Refreshed2026-09-20 from the exact Poly Haven pages and public info/file API.
Each page's structured metadata declares CC0 and describes free availability.
The publisher license permits commercial use and source redistribution:
https://polyhaven.com/license and
https://creativecommons.org/publicdomain/zero/1.0/ .
Website text/previews are not assumed CC0; raw HTML evidence remains ignored.

| Asset | Credited authors/roles | Files | Actual bytes |
| --- | --- | ---: | ---: |
| [Tree Small02](https://polyhaven.com/a/tree_small_02) | Rico Cilliers: All | 14 | 175367829 |
| [Fern02](https://polyhaven.com/a/fern_02) | Rob Tuytel: scanning; Rico Cilliers: modeling | 6 | 6823781 |
| [Grass Medium01](https://polyhaven.com/a/grass_medium_01) | Rob Tuytel: Photography; Rico Cilliers: Modeling | 7 | 12084518 |
| [Grass Ground](https://polyhaven.com/a/grass_ground) | Charlotte Baglioni: All | 3 | 54349106 |
| Total | Poly Haven CC0 sources | 30 | 248625234 |

Three FBX files,26 PNG maps and one JPG. Tree/ground maps are2048x2048;
understory maps1024x1024. No blend/scatter scene, archive, extra resolution,
fallback asset, Fab content or retained Brown Mud Leaves/Rock Moss download.
No account, API key, checkout or license-claim action was required. API terms
https://github.com/Poly-Haven/Public-API/blob/master/ToS.md permit free access;
requests identify `HomesteadSourcePreparation/1.0` and credit Poly Haven.
This is an authoring helper, not a new network dependency in the game.

Maps: diffuse, DX normal, roughness and AO for vegetation, with explicit foliage
alpha; grass dry diffuse is retained as a source option, not a new game preset.
Ground uses diffuse/DX normal/roughness only. Original FBX-linked GL/EXR filenames
are recorded but not followed or rewritten. Future reviewed import must wire the
selected maps explicitly. No source conversion occurred.

## Persistent evidence

- `asset-manifest.json`: exact URLs, original publisher size/MD5, author/license,
  catalog revision and hashes of13 raw metadata/page evidence files.
- `download-receipt.json`: all30 actual sizes/SHA256/MD5; every publisher MD5
  matched. Repeat verification leaves the receipt unchanged. Missing/changed
  receipted sources fail rather than being silently replaced.
- `source-inventory.json`: binary-FBX object/geometry/model/material/connection/
  transform facts and all27 image headers/channels/extrema; source SHA256 on
  every entry. Reinspection exactly reproduced this file without rewriting it.
- `protected-before.json`:234 preexisting tracked source/config/content/asset/
  launcher file hashes, all reconfirmed unchanged after acquisition/inspection.
  It is not a claim to have inspected personal saves.
- `helper-tests.txt`:19 passing focused helper tests; no game/Unreal suite run.
- `preparation-receipt.json`: final proof/helper identity and explicit limits.

Detailed source findings and per-model counts:
`docs\research\environment-assets\diffs\2026-09-20-delta.md`.

## Helper behavior and limits

Python3.13.3, Pillow12.1.0 were already installed; no dependency installation.
No Blender, Editor, commandlet, cooker, UnrealPak, game or asset-supplied Python
was executed.

```powershell
python -m unittest discover -s .\Tests -p 'Woodland*Tests.py' -v
python .\Scripts\prepare_woodland_sources.py download
python .\Scripts\inspect_woodland_sources.py
```

These commands are run-bound, not permission to extend/restart the run.
With the sealed sources present, `download` is verification only. Metadata and
selection creation refuse existing outputs; inspection verifies an existing
inventory rather than replacing changed evidence.

Acquisition rejects unexpected hosts, redirects, paths, assets, extensions,
resolutions, sizes and hashes; maximum128MiB/file and512MiB total,45-second
socket timeout and a checked five-minute transfer budget. Failed partials stay
`.download`; no retry-to-accept/receipt overwrite. The binary-FBX reader accepts
versions7400/7500 only, validates offsets/property lengths, caps nesting64,
nodes100000, one expanded array128MiB and retained decoded arrays256MiB, and
checks a120-second parse/hierarchy budget. It is not a general FBX importer.
Unused arrays/raw blobs are skipped as bounded data, not executed or fully
validated as render inputs. Images are bounded to16Mi pixels and verified/
decoded only as PNG/JPEG.

Known fixtures cover raw/compressed arrays,7400/7500 offsets, a quad bound to
two separate model instances, hierarchy, material slots, units/transforms, PNG
alpha, and rejection of invalid headers/offsets/counts/depth/expansion/indices/
nonfinite vertices/mislabeled images. Acquisition tests cover unsafe names/
URLs, redirects, size/hash/HTTP-length failures and preservation of existing
files/partials/receipts. An external UNC/script-looking texture reference in
the FBX fixture is recorded without opening it.

No runtime triangle count, imported pivot/LOD/material, collision, wind,
GPU/VRAM/frame timing, render quality, aesthetic approval or new runnable
candidate is claimed. The selected Shipping preview/profile is unchanged.
