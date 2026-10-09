# Heroine wardrobe (MetaHuman)

Four skinned garments for the MetaHuman heroine, in a grounded pre-industrial idiom. All the
cloth is hand-spun and hand-woven, and every closure is a cord, a drawstring or a horn toggle.
They go over the always-on base layer in `Assets\Characters\PrimitiveOutfit` (tank top and shorts):

| Garment | Folder | What it is |
| --- | --- | --- |
| `SKM_LinenTee` | `LinenTee\` | Plain linen T-tunic shirt, woad blue-grey. Crew neck with a bias binding, set-in sleeves to mid upper arm, hem at the high hip. Fitted: it follows her waist in under the bust. |
| `SKM_LinenLongShirt` | `LinenLongShirt\` | Fitted unbleached linen shirt: 11 cm neck slit laced criss-cross through worked eyelets, close sleeves gathered into 3.4 cm cuffs, hip length. |
| `SKM_WoolTrousers` | `WoolTrousers\` | Walnut-brown fulled wool, 2/2 twill. Drawstring casing at the shorts' waist height; snug through the seat and thigh, tapered legs. Stays within 1 cm of the skin below the knee (for boots) and ends at the ankle bone. The left knee has a mended patch. |
| `SKM_FurCoat` | `FurCoat\` | Hip-length sheepskin coat, hair side in: suede outside, fleece turned out at the collar, cuffs and front edges. Closes with three horn toggles on leather loops. About 2 cm of loft, fitted and waisted over the long shirt. |

Jenny (2026-10-04) asked for every garment to fit her about as closely as the starter tank top and
shorts (3.2 mm off the skin): tight and fitted. The recipe's `FITTED` table sets the heroine's cloth
offsets, drape and fold depth; the clerk's looser outfit keeps the shared `outfit\` defaults.

Rebuild everything with the seeded recipe `Scripts\Blender\Recipes\clothing_wardrobe.py`, which
uses the shared `outfit\` modules. It runs under Blender 5.2, headless:

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
  --python Scripts\Blender\Recipes\clothing_wardrobe.py -- --stage all
#   --stage fit      fitted meshes + Workbench previews in _work\ (~3 min)
#   --stage build    fit, folds, UVs, textures, weights, thickness, fittings, poses, FBX, masks,
#                    report, .blend (~16 min; --reuse-textures skips texture synthesis ~4 min)
#   --stage render   4K Cycles renders from the saved .blend (~40 min)
```

Inputs:
- The coordinator's Unreal exports in `E:\Repos\HomesteadShared\MetaHumanBody` (body `..._Full.fbx`
  and the face).
- The base-layer shorts FBX. The shirts' hems are fitted outside its waistband.
- `Assets\Props\CordBelt\cord_belt_contour.json`. Every hem ends just above the cord belt.

`report.json` records their hashes along with every metric below.

## Files

Each garment folder holds:
- `SKM_<Garment>.fbx`
- `BodyCoverageMask_<Garment>.png`
- `Textures\T_<Garment>_{basecolor,normal,roughness,ao}.png`: 2048² for the cloth, 4096² for the coat.

Shared files:
- `report.json`
- `Clothing.blend`: body and face with review materials, the base layer, all four garments and the
  `root` armature.
- `Renders\`

Optional tileable detail maps (4 cm tile, OpenGL normals):
- `WoolTrousers\Textures\T_HomespunWool_Detail_{normal,roughness,tone}.png` for the wool.
- The existing `PrimitiveOutfit\Textures\T_HomespunWeave_Detail_*` suits the linen.

## Unreal import

- Units and axes are the same as the heroine's own FBX: centimetres (UnitScaleFactor 1), Z up,
  -Y forward.
- The armature object `root` exports as the root bone above `pelvis`.
- Only deforming bones are exported (weighted bones plus their parents). There are no leaf bones
  and no animation.
- Import onto `metahuman_base_skel`.
- Each mesh has one material slot, `M_<Garment>`.
- Normal maps are **OpenGL**: flip green in Unreal.
- Basecolor is sRGB; the other maps are linear.

| Garment | Vertices | Triangles | Weighted bones | Exported bones |
| --- | --- | --- | --- | --- |
| SKM_LinenTee | 13,200 | 26,394 | 47 | 66 |
| SKM_LinenLongShirt | 20,492 | 40,972 | 67 | 86 |
| SKM_WoolTrousers | 22,266 | 44,506 | 36 | 48 |
| SKM_FurCoat | 18,064 | 36,048 | 65 | 84 |

The coat's material is one atlas:
- The suede outer shell and the fleece at the collar, cuffs and front edges are at full texel
  density.
- The fleece lining (the inner shell) is at half density; it's only seen at the openings.
- The rims show the cut edge of the skin.
- Horn toggles and leather thongs sit in the bottom strip.

## How they were made

1. **Fit.** Each garment is cut as a structured quad pattern and cast onto the body in the bind
   pose. Tops have front and back panels, yoke ribbons over the shoulders, and sleeve tubes grown
   along the shoulder–elbow–wrist bone line; the coat also has a stand collar. The trousers are the
   shorts' hip section continued down each leg's hip–knee–ankle line.
2. **Membrane solve.** A membrane solve keeps every vertex outside the body (+ face) and outside
   the garments worn underneath:
   - the shirts clear the shorts' and trousers' waistbands;
   - the coat clears the tee and the long shirt.

   Drape floors are nearly off for her (`FITTED` hang 0.06-0.22, leg drape 0.08), so the tops
   follow the waist in and the trouser thighs hug the leg. Openings (the shirt slit, the coat front) are cut after the
   solve so both edges still meet.
3. **Folds.** Sculpted folds follow each garment: drape folds, elbow and knee bunching, cuff and
   neck gathers, ankle stacking.
4. **UVs.** The unwrap is grain-aligned.
5. **Textures.** Procedural numpy textures:
   - linen tabby and fulled wool twill, with slubs, hand-sewn felled, rolled and bound seams, and
     running stitches;
   - buttonhole-stitched eyelets and a knee patch;
   - dye streaks, sun fading, grime at the collar and cuffs, and mud and knees on the wool;
   - suede nap, smoke-tan mottling, sinew whip-stitching, fleece tufts and clumps, horn.
6. **Weights.** Data Transfer, nearest face interpolated, from the body + face:
   - Sleeves take their weights from the arm alone and trouser legs from their own leg, so a
     sleeve can't pick up the flank it hangs beside.
   - Weights are then smoothed (more where the cloth stands off the skin), capped at 8 influences
     and normalised.
7. **Thickness.** The recipe builds an explicit shell rather than using Solidify (no spikes where
   the slit ends):
   - linen 1.0–1.2 mm, 2.4–2.6 mm at the hems;
   - wool 1.9 mm, 3.4 mm at the hems;
   - coat 17–20 mm of fleece + skin, 24–26 mm at the trims, thinner under the arms and in the
     armpit so the arms can come down.

## Body coverage masks

- One mask per garment, in the same convention as `PrimitiveOutfit\BodyCoverageMask.png`:
  2048², UV0 `DiffuseUV` of the full body.
- The body UVs sit at u 1–2, so sample at `u − 1`.
- White means the skin is fully hidden: at least 12 mm inside every opening.
- The coat's mask counts skin up to 8 cm under it, since the coat sits over the shirt.
- Covered body triangles, out of 60,816:
  - LinenTee 7,975
  - LinenLongShirt 15,061
  - WoolTrousers 11,493
  - FurCoat 14,215

## Fit in the bind pose

- 0 vertices inside the body for all four garments.
- Median skin gap:
  - tee 3.7 mm;
  - long shirt 4.5 mm;
  - trousers 3.5 mm;
  - coat inner fleece 9.9 mm (the shirts sit in between).
- Every hem ends at least 6 mm above the cord belt's top (tee 6.9, long shirt 6.9, coat 8.6).
- Trousers below the knee: outer surface max 8.7 mm, p99 7.9 mm, mean 5.3 mm from the skin.
- Where the forage pouch rests on her right hip, the trousers come up to 0.95 mm into it (116
  pouch vertices). The pouch was fitted over the shorts; re-run `forage_pouch_fit.py` against
  the trousers if that shows.

## Test poses

Numbers are the percentage of garment vertices inside the posed body (+ face), with the maximum
depth in brackets. "Layers" is the percentage of outer-garment vertices pushed into the garment
beneath. For the coat it also gives how much shows through its whole loft ("through").

| Pose | Tee | Long shirt | Trousers | Coat | Layers |
| --- | --- | --- | --- | --- | --- |
| walk | 1.6 % (13 mm), armpit side | 1.1 % (11 mm), armpit side | clean | 2.3 % (21 mm), armpit side | coat over tee 1.7 % / through 0.4 %, over long shirt 2.0 % / 0.2 % |
| kneel, left leg forward | clean | clean | 0.8 % (7 mm), knee | clean | clean |
| deep squat | 1.1 % (8 mm), front hem into the thighs | 0.8 % (8 mm), same | 3.8 % (8 mm), knee pit, seat, hip crease | 2.5 % (21 mm), front hem into the thighs | shirts over waistbands 0.8–2 %, coat over shirts 1–1.2 % / through 0 % |
| arms overhead | 0.5 % (4 mm) | 0.3 % (4 mm) | clean | 0.3 % (8 mm) | < 0.2 % |
| felling swing | 3.0 % (19 mm), armpit side | 2.1 % (18 mm), armpit side | clean | 1.8 % (19 mm), armpit side | coat over shirts 0.5–0.6 % / through < 0.1 % |

Where they fail and why:
- **Walk and felling swing:** when the arms come down to her sides, the inner upper arm swings
  into the cloth under the arm, which is skinned to the ribs.
- **Coat under the arms:** it's worst on the coat, because 2 cm of fleece can't fold away.
- **Weight blend tested:** making that cloth follow the arm only moved the intersection into the
  ribs, so it was left as is.
- **Deep squat:**
  - The thighs rise into the shirt and coat hems.
  - The trousers pinch into the knee pit and the hip crease, like the shorts do.
  - Hide the skin under each garment with its mask and most of this is invisible. What remains is
    cloth-on-cloth at the armpits and knee pits.

Where cloth or corrective morphs would help: Chaos Cloth on the coat's lower back and front
panels, and armpit correctives.

## Warmth

Suggested relative warmth on the 0–10 scale, ranked by insulation:

| Rank | Garment | Suggested | Why |
| --- | --- | --- | --- |
| 1 | FurCoat | **+6.5** | ~18 mm fleece loft (24 mm at the trims), windproof skin, standing collar and closed cuffs. Wool fleece keeps insulating when damp. Roughly 1.0 clo. It stops at the high hip, so the thighs rely on the trousers. |
| 2 | WoolTrousers | **+2** | 1.9 mm fulled wool plus about 5 mm of trapped air, full leg cover to the ankle, warm when wet. Roughly 0.25–0.3 clo. |
| 3 | LinenLongShirt | **+1** | Thin linen (1 mm), fitted (4.5 mm median air gap), with long sleeves and closed cuffs. Roughly 0.2 clo. Linen wicks and cools when wet. |
| 4 | LinenTee | **+0.5** (or 0) | 1.2 mm linen, short sleeves, fitted (3.7 mm air gap). Roughly 0.1 clo. |

These match the targets (tee ≈ 0, long shirt ≈ 1, trousers ≈ 2, coat ≈ 6); the coat earns a
little more for the loft actually built.

## Renders

`Renders\` holds Cycles renders at 192 samples (AgX, Kloofendal HDRI, neutral review skin):

- **Tee with the base shorts:** `tee_front`, `tee_three_quarter`, `tee_back`.
- **Long shirt with the trousers:** `longshirt_front`, `longshirt_three_quarter`.
- **Trousers with the base tank top:** `trousers_back`, `trousers_side`.
- **Layered (long shirt + coat + trousers):** `layered_front`, `layered_three_quarter`, `layered_back`, `layered_side`.
- **Details:** `detail_coat_collar`, `detail_coat_cuff`, `detail_shirt_lacing`, `detail_trousers_knee`, `detail_tee`.
- **Poses:** `pose_layered_<pose>` and `pose_tee_<pose>` (tee + trousers) for walk, kneel, squat,
  arms overhead and felling swing.
