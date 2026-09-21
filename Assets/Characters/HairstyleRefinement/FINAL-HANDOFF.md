# Final source freeze — ready for parent commit

## Selected set

`final-bundle.json` is authoritative: **12 Joined + 6 Modular FBXs**, with
matching source snapshots. Canonical paths now contain the latest feather-tip
waves and the stock-based bob01 adaptation. Bob consolidation was a technical
transfer of validated hair, not another shaping pass.

- Joined mappings: `LongWave-manifest.json`, `Bob-manifest.json`.
- Modular mappings: `Modular-manifest.json`; same chosen joined hair per body.
- Materials/textures: the two style-specific material records and referenced
  neutral textures. Joined hair is slot 7; modular hair is slot 3.
- All nine modular slots, permanent BaseBra/BaseBriefs, original 53-bone bind,
  nonhair surfaces/materials/UVs/weights/normals and head-only hair remain valid.
- `BobReuse` is the stock-authoring donor/reference.
- `PublishedWaveParity` is an immutable-a3a6dee alternative/reference, not the
  final selected wave revision.
- `artifact-hashes.json` is the full namespace/script/source hash receipt;
  `final-bundle-hashes.json` covers the selected set and its handoff records.

## Explicit difference from immutable a3a6dee

These root working-copy files changed **after a3a6dee but before the wave freeze**:

| Joined stem (each `.fbx` and `.blend`) |
|---|
| `SK_Heroine_LongWave` |
| `SK_Heroine_LongWave_Apron` |
| `SK_Heroine_Willow_LongWave` |
| `SK_Heroine_Willow_LongWave_Apron` |
| `SK_Heroine_Hazel_LongWave` |
| `SK_Heroine_Hazel_LongWave_Apron` |

`LongWave-manifest.json` also changed: **13 differing files in total**.
`publication-audit.json` contains exact published/working SHA-256 values.
These latest feather-tip/local-reference files are a **new final revision**,
not a3a6dee. The parent's final instruction explicitly selected them.

`LongWave-materials.json` remains equivalent to the published record; the
neutral texture is byte-identical. Appearance source and palette numbers match
013c70b after newline normalization. No wave file/palette value was rewritten
during final Bob consolidation. Parent-owned `EARLY-WAVES.md` and
`incumbent-neutral-tint.patch` remain untouched.

## Verification performed

```powershell
python Scripts\Characters\check_hairstyle_final_bundle.py
```

Key output:

```text
FINAL_TECHNICAL_BUNDLE_VERIFIED: 12 joined + 6 modular; latest waves + stock-based Bob
IMMUTABLE_REFERENCE_VERIFIED: a3a6dee ; palette/neutral texture unchanged; original inputs and frozen/parent files unchanged
```

```powershell
python Scripts\Characters\check_hairstyle_refinement.py
openspec validate refine-playable-heroine-hairstyles --strict
git diff --quiet 1425aff -- Assets\Characters\ModularClothing
```

Results:

```text
HAIRSTYLE_RECEIPTS_VERIFIED: 12 joined + 6 modular FBXs; protected inputs, alpha, coverage, rig and palette verified
Change 'refine-playable-heroine-hairstyles' is valid
Stable modular-base diff: exit 0 (matches 1425aff)
```

The actual Blender export/re-import consolidation additionally reported:

```text
CANONICAL_HAIRSTYLE_BUNDLE_VERIFIED: 12 joined + 6 modular; Bob consolidated; waves/palette untouched
HAIRSTYLE_INDEPENDENT_ROUNDTRIPS_VERIFIED 12
```

Evidence: `canonical-consolidation.json`, `roundtrip-validation.json`, per-mesh
manifests and logs under `Build\CharacterPreview\HairstyleRefinement\FinalBundle`.
**145 original input files** and **26 frozen wave/palette/parent-handoff files**
are hash-protected. The stable modular originals also match commit 1425aff.
All recipe Python files parse; the PowerShell wrapper parses; scoped whitespace
checks pass. Blender used offline, factory-startup, disable-autoexec and two CPU
threads. No new aesthetic render loop was performed for consolidation.

## Limits and ownership

**No visual acceptance:** waves still show chunky broad locks and a conspicuous
cut edge; natural ends remain unproven. Bob is an unapproved stock adaptation.
Source checks do not establish ordinary-camera quality or gameplay correctness.
Review in the normal game camera and retain the immutable original alternative
if the result is worse.

No source-lane compiler, Unreal import, cook, game launch, commit or push occurred.
Parent owns the one final source commit and main owns runtime/gameplay acceptance.
Source-quality task 1.1 and gameplay tasks 1.3/2.3 remain open. Do not silently
promote late files under the old a3a6dee label.

All source writers have completed. Do not run rebuild commands while parent
stages this final freeze. Default checks above are read-only. Receipt-writing
flags update metadata only; preview-writing flags are not part of this freeze.
