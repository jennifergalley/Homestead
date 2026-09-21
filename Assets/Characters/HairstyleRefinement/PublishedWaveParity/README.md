# Modular parity with published waves

These three `Modular` FBXs use the **exact hair from published a3a6dee**, not the
later feather-tip wave candidate in the parent namespace's working copy.
No shaping or visual improvement is claimed. The broad-lock/conspicuous-edge
qualification remains; normal-game camera review belongs to main.

`Joined` contains six byte-identical a3a6dee input copies, recovered offline
from already-cached local Git LFS objects and verified against the published
manifest. Its copied manifest/material records are the exact published bytes.
The copied neutral texture is likewise identical to the published texture.
There was no download, other-checkout modification or change to frozen inputs.

`Modular-manifest.json` maps the three new modular bases to their existing
Preferred/Willow/Hazel object paths. All nine slots retain their order; hair
is slot 3, exactly `M_Heroine_Hair_long01_Neutral`, using the frozen
`NeutralHairTint` contract. Permanent BaseBra/BaseBriefs and all other nonhair
surfaces, UVs, weights, normals, shader values, links and texture hashes match
the stable bases. The original 53-bone rig and head-only hair binding remain.

The recipe is `Scripts\Characters\build_published_hairstyle_parity.py`.
Use installed Blender 4.5.14 with the existing offline/factory-startup/
disable-autoexec/two-thread flags and process-local Build resources. It reuses
the existing modular transfer helper; no renderer or sculpt step is involved.
It refuses to download a missing published LFS object.

This remains the immutable-a3a6dee reference alternative. The parent's final
instruction instead selects the latest feather-tip waves plus stock Bob in all
12 canonical Joined + 6 canonical Modular paths (`..\final-bundle.json`).
Those final waves are a new revision, not replacements under the old a3a6dee
label. The publication audit records every difference explicitly.

```powershell
python .\Scripts\Characters\check_hairstyle_final_bundle.py
```

CC0 long01/MPFB admission and licenses are inherited from
`Assets\Characters\provenance.json` and
`Assets\Characters\Source\Licenses\LICENSE.ASSETS.md`.
No private reference, new acquisition, game-asset extraction, commit/push,
Unreal import, compiler or gameplay validation occurred in this source lane.
