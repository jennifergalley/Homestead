# Heroine wardrobe: modern set (MetaHuman)

Jenny (2026-10-09) asked for "a variety of modern-looking clothes - never mind the time period",
all "fitted, sexy, attractive, and/or revealing, similar to the original clothes". These garments
sit 2.0-2.6 mm off the skin (the starter tank top and shorts are 3.2 mm); the jacket, worn over a
top, about 7 mm. They go over the always-on base layer in `Assets\Characters\PrimitiveOutfit`, which
the game hides wherever a top or bottom covers it.

| Garment | Folder | What it is |
| --- | --- | --- |
| `SKM_ScoopTank` | `ScoopTank\` | Black 1x1 rib-knit tank: slim straps, deep scoop front and back, hem just over the navel |
| `SKM_CropTop` | `CropTop\` | Crimson jersey crop top: wide V-scoop neck, cap sleeves, hem well above the navel |
| `SKM_SkinnyJeans` | `SkinnyJeans\` | Mid-blue stretch-denim skinny jeans: low rise, back patch pockets, J-stitched fly, gold topstitching, ankle length |
| `SKM_SkinnyJeansDark` | `SkinnyJeansDark\` | The same jeans in a dark-rinse indigo (Jenny asked for both washes) |
| `SKM_Leggings` | `Leggings\` | Charcoal heather knit leggings: mid rise with a broad elastic band, skin-tight to just above the ankle bone |
| `SKM_BikerJacket` | `BikerJacket\` | Black pebbled-leather cropped jacket: low stand collar, gunmetal centre zip, ends at the waist |

The sneakers and ankle boots are in `Assets\Characters\Footwear` (`footwear.py`).

## Rebuild

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
  --python Scripts\Blender\Recipes\modern_wardrobe.py -- --stage all
#   --stage fit      fitted meshes + Workbench previews in _work\ (~4 min)
#   --stage build    fit, folds, UVs, textures, weights, thickness, test poses, FBX, masks, report,
#                    .blend (~25 min; --reuse-textures reuses cached maps)
#   --stage render   4K Cycles review renders from the saved .blend (~15 min)
```

The recipe reuses `clothing_wardrobe.py` (membrane fit, cloth build, skinning, shells) and the shared
`outfit\` modules. Its `fitted_params` hold each garment's cut. The knit (stockinette and rib),
warp-faced denim (with pockets, fly and fades) and pebbled-leather fabrics, plus the coverstitch,
waistband and zip seams, are in `outfit\wardrobe_tex.py`. Files, units, axes and the Unreal import
follow `Assets\Characters\Clothing\README.md`; `Scripts\Characters\import_heroine_garments.py`
imports them.

## Fit and test poses

`report.json` has every number. All garments: 0 vertices inside the body in the bind pose. In the
test poses the worst are the tank under the arm in the felling swing (2.8 %, 18 mm), the crop top
and jacket at the armpit in a walk (about 2 %, 13 mm), and the jeans and leggings in the knee pit
and seat in a deep squat (2.4-2.5 %, 5 mm). That is at or below the period wardrobe's levels, and
the body coverage masks hide skin under each garment.
