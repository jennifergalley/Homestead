# Stock-based straight bob source drop

This is the reuse-first authoring donor/reference. The final technical
consolidation transfers its validated hair to the canonical Joined/Modular Bob
paths and supersedes the earlier parametric Bob there. Frozen wave assets and
palette are not rewritten. **Source validation is not gameplay or Jenny's art
approval.**

## Reuse choice

The input is the already-admitted fitted CC0 MakeHuman **bob01** in the original
joined Heroine/Variants/BodyPresets FBXs. This keeps its fitted cap, layered cards,
UV atlas, strand detail and head binding. The concrete quality gap was the
unapproved diagonal hair covering the central face. Only that fringe and the
lowest chin/nape ends are clipped; no cap is rebuilt, and no vertical compression
is applied. Every original hair coordinate at or above 155.5 cm remains present.

The neutral strand texture is derived from the admitted `T_Bob_Chestnut.png`,
retaining its alpha byte-for-byte. No new private reference, game asset, download,
purchase, account or garment work is involved. CC0 admission and license evidence
remain in `Assets\Characters\provenance.json` and `Source\Licenses`.

## Import handoff

`manifest.json` maps six `Joined` FBXs to the existing Bob selector paths. Three
optional `Modular` FBXs target the corresponding stable modular Bob paths.
Matching `.blend` snapshots are included. Nonhair surfaces/UVs/weights/normals,
nonhair material values/links/texture hashes, 53-bone binds and permanent bra/
briefs coverage are checked through actual FBX round trips.

The exact material remains **`M_Heroine_Hair_bob01_Neutral`**, at joined slot 7
or modular slot 3. `materials.json` points to `Textures\T_BobReuse_Neutral.png`:
sRGB, masked, two-sided, texture RGB multiplied by **ColorTint**, with
`HomesteadLook::NeutralHairTint` (unchanged five-color contract). This is an
alternative texture for the Bob neutral material, **not an additional hairstyle
ID or a material-role wildcard**. Do not import both candidate material records
over one another indiscriminately. Brows and ponytail keep legacy `HairTint`.
Selecting Bob does not automatically select Blonde.

CPU front/three-quarter/back views in
`Build\CharacterPreview\HairstyleRefinement\BobReuse` use explicit blonde tint.
They show the face uncovered and retain the stock side-part/layered appearance.
The shortened fringe is deliberately above the eyes; this is still a style
candidate requiring ordinary-game review, not a claim to reproduce Hades II.

## Reproduce and validate

Use the same process-local environment and Blender 4.5.14 flags as the established
hair pipeline: background, factory-startup, disable-autoexec, offline, two threads.
Run `Scripts\Characters\build_hairstyle_bob_reuse.py -- --preview`.
Add `--joined-only` to omit modular output/input requirements for an independent
legacy-selector drop. The helper scripts are reused read-only; writes go only to
this BobReuse namespace and its CPU Build directory.

```powershell
python .\Scripts\Characters\check_hairstyle_bob_reuse.py
python .\Scripts\Characters\check_hairstyle_waves.py
```

The manifest contains both protected original-source hashes and frozen wave/
Appearance hashes. Verification is read-only. The source lane does not import,
compile, cook, commit, push or promote any runtime assets.
