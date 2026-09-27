# SKM_TurnShoes

Soft, low-cut leather ankle shoes for everyday wear, the pre-industrial answer to tennis shoes.
They are built like the medieval turnshoe:

- **Upper.** One layer of about 2 mm vegetable-tanned calf.
- **Sole.** The upper is sewn inside-out to a thin sole with a flesh/grain seam, then turned
  right side out. The sole seam hides inside and shows only as a tight bead.
- **Cut.** The top edge stays below the ankle bones: 47-55 mm at the sides, 68 mm at the heel
  and 80 mm at the throat.
- **Lacing.** The shoe opens over the instep with a tongue. A leather thong criss-crosses
  through four pairs of awl-punched holes and ties in a bow at the throat.
- **Seams.** Butted, stitched seams run up the heel and the medial side.
- **Wear.** The toe and heel counter are burnished, the toe is scuffed, and flex creases run
  across the ball.

See `..\README.md` for the rebuild, the Unreal import settings and the mask convention.

## Numbers

| | |
| --- | --- |
| Mesh | 32,814 vertices, 65,520 triangles, one slot `M_TurnShoes` |
| Bones | 10 weighted, 15 exported (weighted + parents), max 3 influences per vertex |
| Sole | 5.0 mm (footbed −0.5 mm, sole bottom −5 mm) → lift the character **0.5 cm** |
| Fit | Snug: skin gap min 1.1 mm, median 1.8 mm |
| UV | 69.6 % atlas coverage, 58 px/cm on the upper, 106 px/cm on the lace |
| Coverage mask | 11,102 body triangles; 97.65 % of the foot below 4 cm is hidden |
| Suggested warmth | 1 / 10 |

## Test poses

The table counts vertices inside the posed body, for both shoes together.

| Pose | Inside | Max depth | Notes |
| --- | --- | --- | --- |
| bind | 0 | 0 mm | clean |
| walk_heel_strike | 0 | 0 mm | clean |
| walk_toe_off | 0 | 0 mm | clean |
| kneel_toes_flexed | 277 (0.84 %) | 2.9 mm | At the ball crease the vamp dips into the folded skin. The mask hides that skin. |
| deep_squat | 0 | 0 mm | clean |
| tiptoe | 0 | 0 mm | clean |

**Known weaknesses:**
- **Toe flex.** With the toes flexed 80°, the rigid toe box bends at the ball as one piece and
  doesn't wrinkle.
- **Hand-placed lacing.** The lace crossings are placed by hand and float on the flaps, so they
  never tighten or slide.
- **Toe shape.** The toe is gently pointed and a little long, as period turnshoes are.

## Files

| File | Contents |
| --- | --- |
| `SKM_TurnShoes.fbx` | Skeletal mesh, both shoes |
| `Textures\T_TurnShoes_{basecolor,normal,roughness,ao}.png` | 2048² PBR maps (basecolor sRGB; normal OpenGL; roughness, AO linear) |
| `BodyCoverageMask_TurnShoes.png` | Body UV0 coverage mask (sample at u − 1) |
| `TurnShoes.blend` | Shoes, the `root` armature, the body and face with review materials, and the primitive outfit |
| `report.json` | Metrics, per-pose poke-through, export settings, input and output hashes |
| `Renders\` | 4K Cycles reviews: `hero`, `side`, `back`, `top`, `full`; `detail_laces`, `detail_heel`, `detail_toe`; `pose_*` |
| `_work\` | Local only (gitignored): Workbench fit previews and the UV part map from the last build |

## Material notes

- **Upper.** Mid-brown calf dressed with tallow, albedo about 0.08-0.16, with smoky mottling.
  Fine pores, sparse grain breaks and irregular creases across the ball.
- **Wear.** The toe and heel are burnished lighter and glossier, and the toe has scuffs.
- **Seams and edges.** Dark waxed-linen stitches on the seams, darker cut edges on the lace
  flaps, and a slightly darker tongue.
- **Sole.** Darker sole leather, rubbed pale where it meets the ground.
- **Lace.** A burnished thong with twist creases.
- **Roughness** is about 0.61 on average.
