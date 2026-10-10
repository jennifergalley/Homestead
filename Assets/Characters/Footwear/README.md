# Heroine footwear

This folder holds five pairs of skinned footwear for the MetaHuman heroine. Each pair is one
skeletal mesh with both feet and one material slot, and each is fitted to her full body
(`SKM_MHC_Heroine_BodyMesh_Full`) and skinned to `metahuman_base_skel`.

| Pair | What it is | Sole | Character offset | Warmth (0-10) |
| --- | --- | --- | --- | --- |
| [`FurBoots`](FurBoots/README.md) | Sheepskin winter boots to mid-calf, worn hair side in, with the fur cuff turned out, a smoked suede outside, a gathered hide sole and leather thong wraps | 12.0 mm | +1.20 cm | 4 |
| [`WovenSandals`](WovenSandals/README.md) | Twined plant-fibre (sagebrush-bark) sandals with a corded rim, a toe cord, side and heel loops and ankle ties | 10.5 mm | +1.05 cm | 0 |
| [`TurnShoes`](TurnShoes/README.md) | Soft low-cut vegetable-tanned turnshoes laced over the instep with a leather thong | 5.0 mm | +0.50 cm | 1 |
| [`Sneakers`](Sneakers/README.md) | Low canvas sneakers, light on the lanes: a slim off-white duck low-top on a vulcanised gum sole with foxing, toe cap and flat laces | 12.0 mm | +1.20 cm | 1 |
| [`AnkleBoots`](AnkleBoots/README.md) | Flat leather ankle boots, sleek and laced to the bone: close-fitting cognac calf with a round almond toe, a slim welted sole and a flat stacked heel | 9.5 mm | +0.95 cm | 3 |

**Offset:** every footbed sits 0.5-0.6 mm below the bare sole, which rests at z = 0. The sole
bottom is at −(sole thickness). To stand on the ground in footwear, lift the character by the
sole thickness. `report.json` stores that value as `character_offset_cm`.

## Rebuild

The seeded recipe `Scripts\Blender\Recipes\footwear.py` and its `shoemaking\` package
rebuild everything here under Blender 5.2, headless. The package reuses `outfit\` for body
import, weight helpers, FBX export and the render setup.

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
  --python Scripts\Blender\Recipes\footwear.py -- --stage all            # build + renders, all pairs
#   --stage fit      geometry only, with Workbench fit previews in <Name>\_work (seconds)
#   --stage build    geometry, UVs, textures, weights, test poses, FBX, mask, .blend (~3 min per pair)
#   --stage render   4K Cycles review renders from the saved .blend (~40 s per view)
#   --only FurBoots TurnShoes   --views hero detail_cuff   --samples 160
```

The inputs are the coordinator's Unreal exports in `E:\Repos\HomesteadShared\MetaHumanBody`:
`SKM_MHC_Heroine_BodyMesh_Full.fbx`, plus the face for full-figure renders. Each `report.json`
records their SHA-256.

### How it is built

1. **Last.** `shoemaking\last.py` builds a parametric shoemaker's last from her foot mesh:
   - It cuts the body with a sweep of planes: horizontal down the shaft, a fan about a pivot in
     front of the ankle, then vertical along the foot.
   - It takes each cut's 2D convex hull and samples it as a radius ρ(s, α).
   - A concave envelope over the toes stops the toes printing through the vamp.
2. **Layers.** Every shoe layer is an offset of that envelope with a flat floor. The recipe
   smooths it, then pushes it out to a minimum normal distance from the skin.
3. **Parts.** `shoemaking\pairs.py` builds each pair's parts, following the construction notes
   at the top of the file: soles, uppers, cuff, thongs, laces, cords and knots. It builds the
   left foot and mirrors it, so both feet share one UV layout.
4. **Textures.** `shoemaking\leather.py` synthesizes every texture per texel in numpy, from
   rest position, part id and construction attributes. It uses no scanned or downloaded
   images. AO combines a Cycles-baked term with a cavity term.
5. **Weights.** The recipe transfers weights from the body with Data Transfer (nearest face,
   interpolated) and folds the toe bones into `ball_*`, so the toe box moves as one piece. It
   then smooths them, caps them at 8 influences and normalizes them.

### Modern pairs (Sneakers, AnkleBoots)

`shoemaking\modern.py` builds the two modern pairs on the same last:

- **Laced upper.** `laced_upper` makes the lining, the outer upper with a lacing slit over a
  stepped-down tongue, and the rolled top edge. Upper faces hidden inside the sole unit are
  dropped (`MeshBuilder.add_grid(keep=...)`).
- **Sole unit.** `sole_unit` sweeps a separate sole round the upper on a stadium of plan
  columns: a vulcanised unit whose foxing tape hugs the canvas (sneakers), or a welt, sole edge
  and stacked heel (boots).
- **Hardware.** Eyelets, criss-cross lacing and a bow.
- **Toe and last.** `toe_box` replaces the forefoot with a designed round toe and toe spring
  instead of following her toes. `square_bottom` lasts the boot's bottom square, so the welt
  follows the upper within about 2 mm.
- **Textures.** `leather.py` adds the `canvas`, `vulc_rubber`, `flat_lace`, `calf`, `welt_sole`
  and `round_lace` styles. The white canvas, rubber and lace may reach 0.7 linear albedo
  (`ALBEDO_MAX`); every other part keeps the 0.6 cap.
- **Trousers.** The ankle boots' `report.json` records `trouser_standin`. Snug trousers sit over
  the shaft, which stands at most 6.4 mm off the skin.

Rebuild just these with `--only Sneakers AnkleBoots`. That leaves the other three pairs untouched.

## Unreal import (all pairs)

- **Units and axes.** Centimetres (UnitScaleFactor 1), Z up, −Y forward. The armature object
  is named `root`, so it becomes the root bone above `pelvis`.
- **Bones.** The FBX has no leaf bones and no animation. It carries only the deforming bones
  the pair uses, plus their parents. Import it onto the existing `metahuman_base_skel`.
- **Material.** There is one slot, `M_<Name>`.
- **Textures.** Each map is 2048²:
  - `T_<Name>_basecolor`: sRGB.
  - `T_<Name>_normal`: **OpenGL** (+Y). Tick *Flip Green Channel* on import.
  - `T_<Name>_roughness` and `T_<Name>_ao`: linear.
- **Body coverage mask.** `BodyCoverageMask_<Name>.png` is 2048² and uses the same convention
  as `PrimitiveOutfit\BodyCoverageMask.png`:
  - It lives in body UV0 (`DiffuseUV`). The body UVs occupy u 1-2, so sample at `u − 1`.
  - White means that body triangle lies fully inside the shoe shell and at least 12 mm from any
    opening, so it is safe to hide.
  - Thongs, laces and cords never count as cover.
  - The boots, the turnshoes and the sneakers hide the whole foot.
  - The sandals hide only the sole of the foot, so the toes and the top stay visible.

## Test poses

The recipe applies each pose to the body armature and counts the footwear vertices that end up
inside the posed body. The pairs' README files hold the per-pose tables.

| Pose | Definition |
| --- | --- |
| `walk_heel_strike` | Left heel strike: left thigh −25°, calf 5°; right leg in toe-off (ball −50°) |
| `walk_toe_off` | Mirror of the above: left toe-off with the ball flexed 50° |
| `kneel_toes_flexed` | Upright kneel on both knees, calves 100°, toes tucked under (ball −80°) |
| `deep_squat` | Flat-footed: hip 118°, knee 135°, ankle 35° dorsiflexion |
| `tiptoe` | Up on the balls of the feet: ankles 40° plantar flexion, toes flat (ball −40°) |

Two failure modes are shared by all pairs:

- **Ball crease in `kneel_toes_flexed`.** With the toes bent 80°, the body's skin folds in at
  the ball crease. The rigid toe box then dips into the top of the forefoot: 3 mm for the
  turnshoes and 10 mm for the thick boot lining. The coverage mask hides that skin, so the
  poke-through is invisible.
- **Shaft top in `deep_squat`, fur boots only.** The calf and hamstring fold closes onto the
  back of the boot top.

## Provenance

All footwear geometry and textures are original and generated by the recipe. The review renders
use the CC0 Poly Haven HDRI *Kloofendal 48d Partly Cloudy (Pure Sky)* for lighting only. The
review scenes append the primitive outfit and the heroine's MetaHuman body and face, which are
for context only and are not part of the footwear assets.
