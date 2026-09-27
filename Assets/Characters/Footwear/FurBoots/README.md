# SKM_FurBoots

Warm winter boots for the heroine that reach mid-calf. They are built like a northern mukluk or
Sami-style boot:

- **Upper.** One piece of sheepskin worn hair side in, about 1.5 cm of wool on a 2 mm skin. The
  outside is the smoke-tanned suede flesh side.
- **Cuff.** The top of the upper turns out into a thick fur cuff, 64 mm deep and 15 mm thick.
- **Sole.** A thicker smoked-hide sole is gathered up around the foot. It is whip-stitched to
  the upper 21 mm above the ground with sinew, which puckers it at the toe and heel.
- **Thongs.** A leather thong wraps the ankle in a figure-eight over the instep and ends in an
  overhand knot with two tails. A second thong cinches the shaft below the cuff. The fur
  visibly compresses under both.
- **Seams.** A whip-stitched back seam runs up the shaft.

They are designed to go over snug wool trousers whose lower leg stays within 1.0 cm of the skin.

See `..\README.md` for the rebuild, the Unreal import settings and the mask convention.

## Numbers

| | |
| --- | --- |
| Mesh | 38,208 vertices, 76,000 triangles, one slot `M_FurBoots` |
| Bones | 18 weighted, 25 exported (weighted + parents), max 4 influences per vertex |
| Sole | 12.0 mm (footbed −0.5 mm, sole bottom −12 mm) → lift the character **1.2 cm** |
| Shaft top | z = 335 mm (mid-calf); cuff 64 mm deep |
| Inner shaft gap (ankle → top) | min 18.97 mm, median 19.2 mm (target ≥ 18 mm for the trousers) |
| Foot inner gap | min 3.9 mm (lining sits closer over the foot, so the boot isn't clown-sized) |
| Loft | mean 16.6 mm wall (wool + skin) over the shaft and foot; cuff 15 mm |
| UV | 82.6 % atlas coverage, ~30 px/cm on the suede and cuff, 9 px/cm on the mostly hidden lining |
| Coverage mask | 14,764 body triangles; 97.2 % of the foot below 4 cm is hidden |
| Suggested warmth | 4 / 10 |

## Test poses

The table counts vertices inside the posed body, for both boots together.

| Pose | Inside | Max depth | Notes |
| --- | --- | --- | --- |
| bind | 0 | 0 mm | clean |
| walk_heel_strike | 0 | 0 mm | clean |
| walk_toe_off | 0 | 0 mm | clean |
| kneel_toes_flexed | 98 (0.26 %) | 10.4 mm | At the ball crease the thick lining dips into the top of the forefoot. It is hidden by the mask. |
| deep_squat | 566 (1.5 %) | 39 mm | The back of the shaft top, which stands 3.5 cm off the skin, is caught in the closing calf and hamstring fold. 74 of these vertices lie within 2 cm of the opening, so some skin can overlap the back of the cuff. |
| tiptoe | 0 | 0 mm | clean |

**Known weaknesses:**
- **Deep squat.** The squat is the one visible failure, and it is inherent to a fat mid-calf
  boot on linear-blend skinning. A corrective morph that flattens the back of the shaft top in
  the squat would fix it. So would a lower boot top, if squatting in boots becomes common.
- **Rigid shaft.** The shaft does not slump or wrinkle under gravity.
- **Rigid toe box.** The toe bones are folded into `ball_*`, so the toe box moves as one piece.

## Files

| File | Contents |
| --- | --- |
| `SKM_FurBoots.fbx` | Skeletal mesh, both boots |
| `Textures\T_FurBoots_{basecolor,normal,roughness,ao}.png` | 2048² PBR maps (basecolor sRGB; normal OpenGL; roughness, AO linear) |
| `BodyCoverageMask_FurBoots.png` | Body UV0 coverage mask (sample at u − 1) |
| `FurBoots.blend` | Boots, the `root` armature, the body and face with review materials, and the primitive outfit for context |
| `report.json` | Metrics, per-pose poke-through, export settings, input and output hashes |
| `Renders\` | 4K Cycles reviews: `hero`, `side`, `back`, `top`, `full`; `detail_cuff`, `detail_ankle`, `detail_sole`; `trousers`; `pose_*` for the five test poses |
| `_work\` | Local only (gitignored): Workbench fit previews and the UV part map from the last build |

## Material notes

- **Suede.** Smoke-mottled tan (albedo 0.15-0.24), nap and streaks, faint healed scratches and
  veining, flex creases across the front of the ankle, and soil darkening toward the ground.
- **Wool.** Shearling cuff with domain-warped, crimped curly locks. The tips are cream (about
  0.5 albedo), the partings darker, with dirtier patches. The lining is cleaner.
- **Sole.** Darker smoked hide, worn paler underfoot, with the sinew whip-stitch bead around the
  sole seam.
- **Thongs.** Burnished dark leather thongs with cut edges and twist creases.
- **Roughness** is 0.87 on average.

## Renders

- `trousers` and every other FurBoots render put the boots over a review stand-in of snug dark
  wool trousers. The stand-in is her legs pushed out 7 mm and 2.5 mm thick, hemmed at the ankle
  bone, and it is not an asset.
- Renders are lifted by the sole thickness so the soles rest on the floor.
