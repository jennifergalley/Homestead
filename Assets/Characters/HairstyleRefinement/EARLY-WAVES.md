# Early playable-selector wave source drop

This milestone contains six shorter-wave FBXs plus their six packed source
scenes, neutral texture and import manifest. Bob and modular hair parity are
separate follow-ups. **Source-ready only; main owns import/build/play acceptance.**

`LongWave-manifest.json` maps every file in `Joined` to its existing
`/Game/SurvivalGame/Characters/Heroine/...` object path, covering Preferred,
Willow and Hazel with Tunic/Apron outfits. Do not change actor transforms,
nonhair material slots or the original skeleton/animations.

Replace only hair slot 7 with `M_Heroine_Hair_long01_Neutral`. Its masked,
two-sided DefaultLit material uses `Textures\T_LongWave_Neutral.png` (sRGB),
texture alpha for opacity, roughness 0.70 and RGB multiplied by `ColorTint`.
Use `HomesteadLook::NeutralHairTint(HairColor)` only for this new material
(and the forthcoming neutral bob). Material default ColorTint must be the
helper's Chestnut value `(0.055, 0.011, 0.003, 1)`, not white.
Legacy ponytail and eyebrow materials keep `HairTint`.

The minimal runtime commit changes only `HomesteadAppearance.h/.cpp` and two
lines in incumbent Character tint dispatch. It adds `HairColorCount=5`,
`NeutralHairTint`, and Blonde index 4 without changing default Chestnut or
existing 0..3 legacy tint meanings. It does not depend on wardrobe authority,
modular assets or the new UI. If cherry-pick context conflicts, the included
`incumbent-neutral-tint.patch` contains just the Character change; apply the
Appearance helper diff too. The UI owner supplies the independent Controller
palette-cycling patch. Selecting Bob never changes hair color automatically.

Tips now end at 111.05-112.14 cm versus the old 89.15-89.29 cm. Existing
coordinates at/above 120 cm are retained; lower geometry is cut with staggered
tapered-lock edges, not vertically compressed. Original nonhair positions,
UVs, weights, material identities and corner normals are preserved, with
round-trip position error below 0.000000064 m and 53 original authoring bones.
Hashes and per-body checks are in the manifest. Broad card grouping and end
silhouette still need ordinary in-game back/side/movement review.

Reuse: existing admitted CC0 MakeHuman long01 geometry, original game_engine
rig/weights and chestnut texture alpha. Only the required shorter end topology
and a neutral color-support texture were custom changes. License/provenance:
`Assets\Characters\provenance.json`,
`Assets\Characters\Variants\provenance.json`,
`Assets\Characters\BodyPresets\provenance.json`, and
`Assets\Characters\Source\Licenses\LICENSE.ASSETS.md`.
No download, private reference, copied game art or asset-script execution.
