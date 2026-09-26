# Primitive outfit (MetaHuman heroine)

Makeshift homespun clothing for the MetaHuman heroine:

- an off-white cropped tank top: narrow straps, a modest scoop neck, and a rolled hem that stops
  above the navel;
- low-rise oatmeal linen shorts: a drawstring casing well below the navel and short
  upper-thigh legs.

Both are fitted to her full body (`SKM_MHC_Heroine_BodyMesh_Full`) in the bind pose and skinned
to `metahuman_base_skel`.

The seeded recipe `Scripts\Blender\Recipes\primitive_outfit.py` rebuilds everything here. It
runs under Blender 5.2, not the pinned 4.5 character pipeline:

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
  --python Scripts\Blender\Recipes\primitive_outfit.py -- --stage all      # build + renders
#   --stage build    fit, textures, rig, test poses, exports, mask, .blend (~7 min)
#   --stage render   4K Cycles review renders from the saved .blend
```

The inputs are the coordinator's Unreal exports in `E:\Repos\HomesteadShared\MetaHumanBody`
(`SKM_MHC_Heroine_BodyMesh_Full.fbx`, `SKM_MHC_Heroine_FaceMesh.fbx`). `report.json` records their
hashes, the landmarks, the pattern parameters and the metrics.

## Files

| File | Contents |
| --- | --- |
| `SKM_PrimitiveTankTop.fbx` | Tank top skeletal mesh, material `M_PrimitiveTankTop`, 32 bones |
| `SKM_PrimitiveShorts.fbx` | Shorts skeletal mesh (drawstring and loose threads joined in), material `M_PrimitiveShorts`, 28 bones |
| `SKM_PrimitiveOutfit.fbx` | Both garments in one mesh, two material slots, 56 bones |
| `Textures\T_PrimitiveTankTop_*`, `Textures\T_PrimitiveShorts_*` | 4096² `basecolor` (sRGB), `normal` (OpenGL, +Y), `roughness`, `ao` (linear) |
| `Textures\T_HomespunWeave_Detail_*` | 1024² tileable weave detail (`normal` OpenGL, `roughness`, `tone`); one tile = 4 cm of cloth |
| `BodyCoverageMask.png` | Body UV0 mask, white = skin fully under cloth |
| `PrimitiveOutfit.blend` | Body + face (review materials), garments and the `root` armature |
| `Renders\` | 4K Cycles beauty and test-pose renders |
| `report.json` | Machine-readable metrics, export settings and file hashes |

## Unreal import

- Units and axes match Unreal's own FBX export: centimetres (UnitScaleFactor 1), Z up, -Y
  forward. The armature object is named `root`, so it becomes the root bone above `pelvis`.
- There are no leaf bones and no animation.
- Only deforming bones used by the garment, plus their parents, are exported.
- Import onto the existing `metahuman_base_skel`.
- The normal maps are **OpenGL**. In Unreal, tick *Flip Green Channel* on the texture, or invert G
  in the material.
- Albedo: tank top ≈0.59 linear mean, max 0.74. Shorts ≈0.47/0.40/0.29.
- Roughness is ≈0.89 mean. There is no sheen.
- Optional: multiply the weave detail maps in at UV × (tile per 4 cm) for close-ups.

## Weights

The recipe transfers weights with Data Transfer (`POLYINTERP_NEAREST`, all vertex groups by
name) from the body, joined with the face for the strap tops. It then smooths them lightly, most
where the cloth stands off the skin. Finally it caps them at 8 influences, prunes below 0.01 and
normalizes.

Experiments showed the transferred weights are already the best choice. Raising the cap to 12
(the body's own maximum) changed nothing, and shifting hip-crease weight toward the pelvis or
the thigh made poke-through 3–10× worse.

## Body coverage mask

- The mask is 2048², in UV0 `DiffuseUV` of `SKM_MHC_Heroine_BodyMesh_Full`.
- The body's UVs occupy u 1–2 (a UDIM-style tile), so the mask is shifted by −1 in u. Sample it at
  `frac(u)`, or `u − 1`.
- White means that body triangle lies fully under cloth, at least 12 mm from any garment opening.
  It is safe to hide or mask out.
- 4,701 of 60,816 body triangles are covered: tank top 1,982, shorts 2,719.

## Fit and test poses

- Bind pose: no vertices inside the body.
  - Tank top: skin gap min 2.9 mm, median 3.0 mm.
  - Shorts: skin gap min 3.4 mm, median 3.6 mm, more at the crotch and inner leg.
- The table below counts vertices inside the posed body (body plus face).

| Pose | Tank top | Shorts |
| --- | --- | --- |
| bend forward 60° | 1 vertex (0.6 mm) | clean |
| twist 45° | clean | clean |
| arms overhead | 0.7 % (max 3.3 mm, armhole edge) | clean |
| kneel, left leg forward | clean | 0.4 % (max 5.8 mm, front groin) |
| deep squat (118° hip flexion) | clean | 8.2 % (max 7.9 mm, front hip crease and inner leg hem) |

The squat and kneel failures come from the body itself. Linear-blend skinning collapses the front
hip crease and her thigh meets her belly, so the fold closes around the cloth. Hide the body with
the coverage mask so the cloth stays visible there. Use cloth or corrective morphs if deep
squats become gameplay poses.

## Renders

`Renders\` holds Cycles 192-sample renders (AgX, Kloofendal HDRI, neutral review skin):

- **Beauty**, 2160×3840 portrait: `beauty_front`, `beauty_back`, `beauty_three_quarter`,
  `beauty_side`.
- **Details**, 3840×2160: `beauty_detail_midriff`, `beauty_detail_back`, `beauty_detail_strap`,
  `beauty_detail_hem`.
- **Test poses**: `pose_bend_forward_60`, `pose_twist_45`, `pose_arms_overhead`,
  `pose_kneel_left_forward`, `pose_squat`.

Review notes:
- **Fit:** the navel and the whole midriff stay exposed in every view. Up close the weave, the
  hand running stitches, the rolled and felled seams and the loose threads all read.
- **Tank top:** its side panels bridge the hollow between bust and arm, so the armhole binding
  stands a few mm off the side of the chest. The gap is larger with the arms raised.
- **Shorts:** they have a slightly boxy linen silhouette at the side and seat. In the kneel pose
  the raised leg's opening gapes; in the squat the front hip crease swallows the cloth (see
  above).
- **No drape:** the cloth follows the skin rigidly and does not hang under gravity. For example,
  the neckline stays on the skin in the forward bend.
- **Kneel render:** the pose is grounded at her lowest point, so the rear knee hovers a little
  above the floor. It's a test render only.
