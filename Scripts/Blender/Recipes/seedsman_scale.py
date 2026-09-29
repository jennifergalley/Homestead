"""Victorian counter beam scale for Tregear's seedsman shop.

Real-object research (written before modeling):
- Mid-19th-century British shop scales used a cast-iron pedestal/base with a central
  pillar, a brass or steel beam with knife-edge fulcrum, and two suspended brass pans
  for dry goods. Counter examples were about 45 cm high and 50-60 cm wide; the bases
  were often japanned black or dark green with gold pinstriping, and the brass pans
  tarnished except on handled rims.
- Bell and cylindrical brass weights sat beside the scale: nested ounce/pound weights
  with small knobs, rubbed bright on top and dark in recesses. Working scales were
  dusty with flour/bran in the base grooves and chipped paint at the edges.
- Dimensions here: about 0.55 m wide by 0.30 m deep by 0.45 m high, with the display
  beam running along X and the best/front view toward -Y. It stands on the counter, so
  the pivot is bottom-centre. Original procedural geometry/materials only.
"""
import math
import sys
from pathlib import Path

from mathutils import Vector, noise

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "SeedsmanScale"
DESCRIPTION = "Victorian counter beam scale: japanned cast-iron base, brass pans, beam and weights."
COLLISION = "box"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -24), "focus": (0.165, -0.065, 0.115), "detail_distance": 0.24,
          "detail_fstop": 32.0}
REPORT = {"dimensions_m": {"width": 0.55, "depth": 0.30, "height": 0.45},
          "pivot": "bottom-centre; beam spans X; customer/front side -Y"}


def pan_dish(kit, name, loc, mat, radius=0.073, depth=0.018):
    segs, rings = 48, 7
    verts, faces, coords = [], [], []
    for r in range(rings + 1):
        rr = r / rings
        for j in range(segs):
            a = math.tau * j / segs
            rad = radius * rr
            z = loc.z - depth * (1 - rr) ** 1.55 + 0.002 * rr
            verts.append((loc.x + rad * math.cos(a), loc.y + rad * math.sin(a), z))
            coords.append((rad * math.cos(a), rad * math.sin(a), z))
    # Centre ring degenerates to tiny circle; upward winding.
    for r in range(rings):
        for j in range(segs):
            faces.append((r * segs + j, r * segs + (j + 1) % segs,
                          (r + 1) * segs + (j + 1) % segs, (r + 1) * segs + j))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def hanging_pan(kit, prefix, x, z, brass):
    parts = []
    loc = Vector((x, -0.005, z))
    parts.append(pan_dish(kit, prefix + "_Pan", loc, brass))
    pts = [Vector((x + math.cos(a) * 0.073, -0.005 + math.sin(a) * 0.073, z + 0.001))
           for a in [math.tau * i / 64 for i in range(65)]]
    parts.append(kit.tube(prefix + "_RolledRim", pts, radius=0.0022, sides=10, material=brass))
    ring_top = Vector((x, -0.005, z + 0.125))
    for k, a in enumerate((math.radians(90), math.radians(210), math.radians(330))):
        rim = Vector((x + math.cos(a) * 0.066, -0.005 + math.sin(a) * 0.066, z + 0.002))
        # A pair of fine rods approximates chain at this scale without spending thousands of links.
        mid = (rim + ring_top) * 0.5 + Vector((0.004 * math.sin(k), 0.002 * math.cos(k), 0.0))
        parts.append(kit.tube(f"{prefix}_Chain_{k}", [rim, mid, ring_top], radius=0.0011, sides=6, material=brass))
    parts.append(kit.tube(prefix + "_HangerLoop", [ring_top + Vector((-0.012, 0, 0)),
                                                   ring_top + Vector((0.012, 0, 0))],
                          radius=0.0018, sides=8, material=brass))
    return parts


def weight(kit, name, x, y, z, radius, height, brass):
    parts = [kit.cylinder(name, radius, height, (x, y, z + height / 2), material=brass,
                          sides=32, bevel=0.0012, bevel_segments=2),
             kit.cylinder(name + "_Knob", radius * 0.34, height * 0.34, (x, y, z + height * 1.08),
                          material=brass, sides=22, bevel=0.0008, bevel_segments=1)]
    parts.append(C.ring_band(kit, name + "_TurnedGroove", radius * 0.82, z + height * 0.70,
                             0.003, 0.0013, brass, sides=32))
    return parts


def build(kit):
    m = kit.mats
    iron = m.painted_metal("M_SeedsmanScaleJapannedIron", paint=(0.020, 0.042, 0.030),
                           exposed=(0.28, 0.27, 0.25), rust=0.22, wear=0.55, seed=410.0)
    brass = m.brass("M_SeedsmanScaleAgedBrass", polished=(0.60, 0.42, 0.18),
                    tarnish=(0.095, 0.065, 0.030), wear=0.28)
    gold = m.brass("M_SeedsmanScaleGoldPinstripe", polished=(0.72, 0.50, 0.18),
                   tarnish=(0.12, 0.08, 0.03), wear=0.48)
    C.zero_subsurface(iron, brass, gold)
    parts = []

    base = kit.cylinder("ScaleOvalBase", 0.145, 0.036, (0, -0.002, 0.018), material=iron,
                        sides=64, bevel=0.003, bevel_segments=2)
    base.scale.x = 1.78
    base.scale.y = 0.92
    parts.append(base)
    top = kit.cylinder("ScaleRaisedTopPlate", 0.103, 0.020, (0, -0.002, 0.054), material=iron,
                       sides=56, bevel=0.002, bevel_segments=2)
    top.scale.x = 1.42
    top.scale.y = 0.72
    parts.append(top)
    parts.append(C.ring_band(kit, "BaseGoldPinstripe", 0.137, 0.039, 0.004, 0.0012, gold, sides=96))
    parts[-1].scale.x = 1.78
    parts[-1].scale.y = 0.92

    # Pillar and ornamental collars.
    parts.append(kit.cylinder("CentralPillar", 0.018, 0.295, (0, 0, 0.205), material=iron,
                              sides=30, bevel=0.0014, bevel_segments=1))
    for i, z in enumerate((0.082, 0.124, 0.318, 0.350)):
        parts.append(kit.cylinder(f"PillarCollar_{i}", 0.030 if i in (1, 2) else 0.024,
                                  0.012, (0, 0, z), material=iron, sides=32,
                                  bevel=0.0012, bevel_segments=1))

    # Beam, fulcrum, pointer and graduations.
    parts.append(kit.tube("BrassBalanceBeam", [Vector((-0.245, 0, 0.382)), Vector((0.0, 0, 0.390)),
                                               Vector((0.245, 0, 0.382))],
                          radius=0.0065, sides=16, material=brass))
    parts.append(kit.cylinder("BeamFulcrumCap", 0.026, 0.018, (0, -0.001, 0.374),
                              material=brass, sides=32, bevel=0.001))
    parts.append(kit.tube("BalancePointer", [Vector((0.0, -0.006, 0.376)), Vector((0.0, -0.024, 0.315))],
                          radius=0.0014, sides=6, material=brass))
    parts.append(C.board(kit, "TinyScaleArc", (0.060, 0.003, 0.012), (0, -0.026, 0.314),
                         brass, bevel=0.0005, rough=0.0))
    for k in range(9):
        x = (k - 4) * 0.006
        parts.append(C.board(kit, f"ScaleMark_{k}", (0.001, 0.004, 0.007 if k == 4 else 0.004),
                             (x, -0.029, 0.318), brass, bevel=0.0, rough=0.0))

    parts.extend(hanging_pan(kit, "Left", -0.215, 0.235, brass))
    parts.extend(hanging_pan(kit, "Right", 0.215, 0.230, brass))

    # Bell/cylinder weights grouped on the front-right of the iron base.
    parts.extend(weight(kit, "WeightLarge", 0.120, -0.075, 0.070, 0.025, 0.055, brass))
    parts.extend(weight(kit, "WeightMedium", 0.175, -0.063, 0.070, 0.019, 0.040, brass))
    parts.extend(weight(kit, "WeightSmall", 0.205, -0.010, 0.070, 0.013, 0.030, brass))
    parts.extend(weight(kit, "WeightTiny", 0.146, -0.015, 0.070, 0.010, 0.023, brass))

    return kit.join(parts, "SM_SeedsmanScale", unwrap=False, reshade=True, smooth_angle=60)
