# SKM_WovenSandals

Woven plant-fibre sandals for the heroine, after the ~9,000-10,000-year-old Fort Rock
sagebrush-bark sandals and Japanese waraji:

- **Sole.** A flat twined sole about 1 cm thick. Weft rows of paired twisted strands cross the
  foot over a few warps, and the outermost warp forms a corded rim wrapped by the wefts.
- **Toe cord.** A two-ply twisted cord rises between the first and second toes and splits over
  the forefoot to side loops.
- **Ties.** The ties cross the instep, pass the rear side loops and a heel loop, wrap the ankle
  and are knotted in front with hanging tails.
- **What shows.** The toes and most of the foot stay visible.

See `..\README.md` for the rebuild, the Unreal import settings and the mask convention.

## Numbers

| | |
| --- | --- |
| Mesh | 24,802 vertices, 49,456 triangles, one slot `M_WovenSandals` |
| Bones | 10 weighted, 15 exported (weighted + parents), max 3 influences per vertex |
| Sole | 10.5 mm (top at −0.6 mm, bottom at −10.5 mm); 276 × 121 mm → lift the character **1.05 cm** |
| UV | 34 % atlas coverage, but 37 px/cm on the sole and 53 px/cm on the cords, which is ample |
| Coverage mask | 2,373 body triangles: the sole of the foot only. 22 % of the foot below 4 cm is hidden; the toes and top stay visible. |
| Suggested warmth | 0 / 10 |

## Test poses

The table counts vertices inside the posed body, for both sandals together.

| Pose | Inside | Max depth | Notes |
| --- | --- | --- | --- |
| bind | 4 (0.02 %) | 0.7 mm | The toe cord's root at the web between the first and second toes. It is not visible. |
| walk_heel_strike | 4 | 0.8 mm | Same toe-web vertices |
| walk_toe_off | 4 | 0.8 mm | Same toe-web vertices |
| kneel_toes_flexed | 72 (0.29 %) | 1.9 mm | The toe cord presses into the top of the forefoot at the ball crease. This is barely visible and reads like a tight cord. |
| deep_squat | 4 | 0.7 mm | Same toe-web vertices |
| tiptoe | 4 | 0.8 mm | Same toe-web vertices |

**Known weaknesses:**
- **Rigid sole.** The sole stays rigid from the ball forward, because the toe bones are folded
  into `ball_*`. With the toes flexed, the sole bends at the ball rather than following each
  toe.
- **Tight cords.** The cords sit taut on the skin and never swing, so the knot tails are rigid.

## Files

| File | Contents |
| --- | --- |
| `SKM_WovenSandals.fbx` | Skeletal mesh, both sandals |
| `Textures\T_WovenSandals_{basecolor,normal,roughness,ao}.png` | 2048² PBR maps (basecolor sRGB; normal OpenGL; roughness, AO linear) |
| `BodyCoverageMask_WovenSandals.png` | Body UV0 coverage mask (sample at u − 1) |
| `WovenSandals.blend` | Sandals, the `root` armature, the body and face with review materials, and the primitive outfit |
| `report.json` | Metrics, per-pose poke-through, export settings, input and output hashes |
| `Renders\` | 4K Cycles reviews: `hero`, `side`, `back`, `top`, `full`; `detail_toe`, `detail_ankle`, `detail_rim`; `pose_*` |
| `_work\` | Local only (gitignored): Workbench fit previews and the UV part map from the last build |

## Material notes

- **Colour.** The bark fibre is grey-tan, albedo about 0.17-0.30, and roughness is about 0.87.
- **Sole weave.** Twisted weft strands show gaps between the rows. The rim wrap is its own
  pattern.
- **Wear.** The top of the sole is compressed and darkened under the heel and ball, and the
  bottom is soiled.
- **Cords.** S-twisted two-ply cord with fibre noise.
