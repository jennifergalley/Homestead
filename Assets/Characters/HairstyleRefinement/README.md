# Hairstyle source drop

## Published checkpoint versus frozen working copy

Parent-owned `EARLY-WAVES.md` and `incumbent-neutral-tint.patch` remain untouched.
The published wave checkpoint is **a3a6dee**; the independent palette/legacy tint
dispatch checkpoint is **013c70b**. The six local joined wave FBXs and their
manifest are the later feather-tip candidate and **do not match a3a6dee**.
Neither the later candidate nor its modular-wave parity outputs may silently
replace that published checkpoint. The neutral texture is byte-identical to
the published texture; Appearance source matches after newline normalization.

**Final selection requested by parent:** `final-bundle.json` selects exactly
**12 canonical Joined FBXs + 6 canonical Modular FBXs**: latest feather-tip
waves plus the stock-based bob01 adaptation. The Bob is technically consolidated
from the validated BobReuse donor without further shaping; the latest wave
FBXs, texture, material/manifest and palette were not rewritten during this
consolidation. `Modular-manifest.json` matches the same canonical joined donors.

This is a **new final revision**, explicitly not immutable a3a6dee.
`publication-audit.json` gives exact published/working hashes for the six wave
FBXs, six wave source blends and changed wave manifest. Parent requested these
latest differences in the one final commit. Parent/main must publish and name
that new revision rather than substitute it under the old checkpoint label.

```powershell
python .\Scripts\Characters\check_hairstyle_final_bundle.py
```

`BobReuse` remains the stock-authoring donor/reference. `PublishedWaveParity`
retains exact a3a6dee input copies and matching modular outputs as a separate
immutable-reference alternative, not the selected final wave revision. The
earlier unapproved parametric Bob at canonical paths has been superseded.
All variants remain source-only; original rollback sources stay unchanged.

`check_hairstyle_waves.py` checks the working copy by default. Use
`--published-commit a3a6dee` to require an exact publication match; rejection in
this later-candidate working copy is intentional. No frozen mesh, texture,
material/manifest, palette source or parent-owned handoff file is changed to
make that check pass.

**Source/interchange only. No Unreal import, cook, gameplay, visual acceptance,
commit or push is claimed by this lane.** Existing original files are rollback
inputs and are not overwritten. This drop has no wardrobe dependency.

**Wave visual limitation:** CPU three-quarter review still shows broad, chunky
locks and a conspicuous cut edge. This avoids the rejected vertical compression
and its accordion ridges, but is not a confidently approved natural end
silhouette. Task 1.1 remains open; these are importable source candidates, not a
claim that the requested visual finish is complete.

## Import contract (main owns execution)

`LongWave-manifest.json` and `Bob-manifest.json` map six joined FBXs each to the
**existing** `/Game/SurvivalGame/Characters/Heroine/SK_Heroine_…` object paths.
Reimport those paths using the existing shared skeleton; do not create a new
skeleton, change mesh transforms, or import replacement animations. The twelve
`.blend` snapshots and the scoped wave/stock-bob/consolidation recipes are the
reconstructible source. Original actions remain untouched and hash-protected.

The hair material remains **slot 7** on every joined mesh. Replace only that
slot with:

| Style | New material | Texture |
|---|---|---|
| LongWave | `M_Heroine_Hair_long01_Neutral` | `Textures\T_LongWave_Neutral.png` |
| Bob | `M_Heroine_Hair_bob01_Neutral` | `Textures\T_BobReuse_Neutral.png` |

Use masked, two-sided materials, sRGB textures, texture alpha as opacity mask,
roughness 0.70, and texture RGB multiplied by vector parameter **ColorTint**.
Material records supply the remaining values. New `_Neutral` materials require
`HomesteadLook::NeutralHairTint(HairColor)`; material defaults should use its
Chestnut value, not white. **Do not apply this helper to legacy hair or brows.**
Legacy ponytail and eyebrow materials retain `HairTint`, including unchanged
indices 0–3. Its index 4 is a modest warm eyebrow multiplier, not blonde/platinum
face paint. The new blonde presentation is implemented on neutral waves/bob;
the unchanged legacy ponytail is not claimed to become genuinely blonde.

`HairColorCount = 5`: 0 Chestnut, 1 Dark brown, 2 Black, 3 Copper, 4 Blonde.
Chestnut remains the default. Changing Bob style must not mutate HairColor.
No material change is permitted on skin, eyes, clothing, shoes or other slots.

## Stable modular-base parity

The parent authorized read-only access to the finished separate BaseBra/BaseBriefs
sources at 23:53 Arizona on 2026-09-20. `Modular-manifest.json` maps six additional
FBXs in `Modular` to:

`/Game/SurvivalGame/Characters/ModularClothing/{Body}/SK_Modular_{Body}_Base_{Style}.SK_Modular_{Body}_Base_{Style}`

Bodies are Preferred/Willow/Hazel; styles are LongWave/Bob. Matching `.blend`
snapshots are beside these exports. The **identical joined candidate hair** is
transferred, not re-authored on a different head or body. The known wave end
visual limitation therefore remains unchanged.

Modular hair is **slot 3**, not joined slot 7. All nine slots retain their order:
skin, BaseBra, BaseBriefs, hair, eyes, brows, lashes, teeth, tongue. Only hair's
material is replaced by the shared `_Neutral` material and `NeutralHairTint`.
Permanent bra/briefs face counts, all nonhair geometry/UVs/weights/corner normals,
nonhair material values/links/texture hashes and the original bind are verified
against the stable modular FBXs. Re-encoding transferred hair custom normals has
a separately bounded component error below 0.001; base normals are unchanged.

Every file under the source `Assets\Characters\ModularClothing` namespace is
hash-protected, including all ponytails and garments. None is overwritten.
Their original material records, coverage and import contract remain authoritative.
CPU front/back coverage views are diagnostic only, not game or art acceptance.

```powershell
.\Scripts\Characters\Build-HairstyleRefinement.ps1 -Modular -Preview
python .\Scripts\Characters\check_hairstyle_refinement.py
```

## Source method and checks

Waves are cut from actual evaluated incumbent FBX hair surfaces, **not vertically
compressed**. All original coordinates at or above 120 cm remain exactly present.
Below that, old ends are removed at a staggered, narrowing-lock contour. The
roughly 111–112 cm endpoints sit near the 111.05 cm shoulder/waist midpoint.
Retained upstream wave spacing is unchanged; no original lower waves are packed
into the shorter span. Hair-only seams are welded for smooth shading.
Only the newly cut band below 120 cm remaps its hair UVs toward the admitted
atlas's feathered ends; this reuses real strand alpha rather than leaving an
opaque guillotine edge. The original alpha texture itself is unchanged.

The final bob adapts admitted fitted bob01. Its cap, layered cards, UV strands
and alpha are reused; only the unapproved central diagonal fringe and lowest
chin/nape ends were clipped. Coordinates at/above 155.5 cm remain present.
It is not a recreation of Hades II art or a claim of reference approval.

Every export checks ordered nonhair polygon corners, positions, all UV layers,
bone influences, material identities and restored corner normals. Pre-export
position/UV/weight error is zero. FBX round-trip error is below 0.000000064 m;
nonhair normal error is zero. All 53 original bone names/parents remain; bind matrices
are identical before export, with at most 0.000015 Euler round-trip noise.
Scale follows the existing metre / FBX_SCALE_UNITS contract. Hair is head-weighted,
without physics. Full protected-input hashes include existing ponytails/actions.

Back/three-quarter/front **CPU source** views for each body/style are under
`Build\CharacterPreview\HairstyleRefinement`. They are diagnostic T-pose views,
not gameplay evidence. The stock-bob contact sheet is
`BobReuse\CPU-source-contact.png`; older parametric/modular preview sheets are
legacy diagnostics, not evidence for the final stock Bob. The wave contact
sheet retains its explicit unresolved-quality warning.
Main must still inspect actual back/side walking/actions,
selection and saved colors in a cooked candidate. The retained broad wave cards
and simple bob need ordinary game-camera aesthetic review; source checks are
not Jenny's visual approval.

## Rebuild and check

```powershell
.\Scripts\Characters\Build-HairstyleRefinement.ps1 -Style LongWave -Preview
.\Scripts\Characters\Build-HairstyleRefinement.ps1 -Style Bob -Preview
python .\Scripts\Characters\check_hairstyle_refinement.py
python .\Scripts\Characters\check_hairstyle_final_bundle.py
```

The wrapper launches only the already-installed Blender 4.5.14, background,
factory-startup, disable-autoexec, offline, two CPU threads. Resources, temporary
files, logs and CPU 8-sample previews stay in this lane's Build directory.
No exporter body-deletion helper is called. No private images are read.
The Bob wrapper now builds the stock reuse source and runs
`finalize_hairstyle_bundle.py`; the old parametric Bob command is rejected.
Do not rebuild while parent stages this final freeze. Verification commands
are read-only unless a receipt/preview-writing flag is explicitly requested.
`provenance.json` and the retained CC0 license record admitted sources.
Legacy FBX absolute texture paths are rebased in-process to admitted images in
this worktree, with external path/image searching disabled.

For a fresh independent interchange pass, launch
`verify_hairstyle_roundtrips.py` with the same Blender flags and process-local
environment as the wrapper. It verifies nonhair shader values, node links,
texture filenames, head-only hair weights, scale and bind as well as surfaces.
`preview_hairstyle_palette.py` uses the same CPU settings to render all five
colors directly from the C++ neutral palette. The cheap receipt check refreshes
`artifact-hashes.json`, including `.blend` sources, textures, scripts and license,
only when `--refresh-receipts` is explicitly supplied. The default checker is
read-only. `--refresh-previews` rewrites contact sheets and must not be used
during the wave freeze.
