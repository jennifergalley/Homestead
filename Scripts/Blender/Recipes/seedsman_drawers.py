"""Seedsman's wall cabinet / seed chest for Tregear's, Seedsman & Corn Merchant.

Real-object research (written before modeling):
- Victorian seed merchants and corn chandlers used "nests" of many shallow wooden
  drawers for packeted seeds, peas and beans. Surviving seed chests are joiner-built
  pine or deal cases with plinths, cornices, rows of small drawers, turned wooden or
  brass knobs, and card/paper label holders on every drawer. Drawer fronts do not form
  a modern perfect grid: timber shrinks, gaps are uneven, labels cockle, and old labels
  go tea-brown and smudged. Worn paint exposes warm pine at knobs, arrises and the
  plinth where boots and sacks knock it.
- This cabinet is 1.8 m tall, 1.4 m wide and 0.45 m deep, sized for the back wall of
  a single-room Cornish seedsman. Front/drawer faces point -Y. The bottom-centre pivot
  matches static furniture placement. Original procedural geometry/materials only.
"""
import math
import random
import sys
from pathlib import Path

from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "SeedsmanDrawers"
DESCRIPTION = "Victorian seedsman's wall seed chest: 7 x 7 small drawers, labels, knobs, plinth and cornice; front -Y."
COLLISION = "box"
TRIANGLE_BUDGET = 30000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96, "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -9), "focus": (0.0, -0.24, 0.96)}
REPORT = {"dimensions_m": {"width": 1.40, "depth": 0.45, "height": 1.80},
          "front": "drawer grid and labels face -Y; bottom-centre pivot"}


def _stroke(kit, name, x, y, z, length, angle, mat):
    dx = math.cos(angle) * length * 0.5
    dz = math.sin(angle) * length * 0.5
    return kit.tube(name, [Vector((x - dx, y, z - dz)), Vector((x + dx, y, z + dz))],
                    radius=0.00085, sides=4, material=mat)


def _label(kit, name, x, y, z, w, h, paper, brass, ink, rng):
    parts = []
    parts.append(C.board(kit, name + "_Paper", (w, 0.003, h), (x, y - 0.0045, z),
                         paper, bevel=0.0, rough=0.00025, seed=rng.randrange(10000)))
    # Folded brass holder: two vertical rails, bottom lip and two tiny tacks.
    parts.append(C.board(kit, name + "_HolderBottom", (w + 0.014, 0.004, 0.004),
                         (x, y - 0.007, z - h * 0.52), brass, bevel=0.0, rough=0.0))
    for sx in (-1, 1):
        parts.append(C.board(kit, f"{name}_HolderSide_{sx}", (0.004, 0.004, h + 0.007),
                             (x + sx * (w * 0.5 + 0.005), y - 0.007, z - 0.0005),
                             brass, bevel=0.0, rough=0.0))
        parts.append(kit.cylinder(f"{name}_Tack_{sx}", 0.0032, 0.0025,
                                  location=(x + sx * w * 0.38, y - 0.010, z + h * 0.33),
                                  rotation=(90, 0, 0), material=brass, sides=6, bevel=0.0))
    for k in range(rng.randrange(2, 5)):
        sx = x + rng.uniform(-w * 0.28, w * 0.25)
        sz = z + rng.uniform(-h * 0.14, h * 0.17)
        parts.append(_stroke(kit, f"{name}_Smudge_{k}", sx, y - 0.0105, sz,
                             rng.uniform(0.010, 0.026), rng.uniform(-0.45, 0.45), ink))
    return parts


def build(kit):
    rng = random.Random(1851)
    m = kit.mats
    case = m.painted_wood("M_SeedsmanDrawersPaintedPine", paint=(0.060, 0.082, 0.058),
                          under=(0.20, 0.12, 0.055), wear=0.62, grime=0.55, seed=210.0)
    drawer = m.painted_wood("M_SeedsmanDrawersDrawerFaces", paint=(0.075, 0.090, 0.062),
                            under=(0.23, 0.14, 0.070), wear=0.72, grime=0.42, seed=211.0)
    inside = m.aged_wood("M_SeedsmanDrawersInteriorPine", light=(0.27, 0.18, 0.10),
                         dark=(0.095, 0.055, 0.028), roughness=0.84, saw=0.45, grime=0.36, seed=212.0)
    brass = m.brass("M_SeedsmanDrawersTarnishedBrass", polished=(0.52, 0.36, 0.16),
                    tarnish=(0.08, 0.060, 0.030), wear=0.35)
    paper = m.paper("M_SeedsmanDrawersOldSeedLabels", color=(0.46, 0.38, 0.25),
                    string_shadow=0.15, seed=214.0)
    ink = kit.material("M_SeedsmanDrawersInkSmudges", (0.034, 0.027, 0.019), roughness=0.95)
    C.zero_subsurface(case, drawer, inside, brass, paper, ink)

    parts = []
    # Casework and mouldings: the back is just inside the wall; front is -Y.
    parts.append(C.board(kit, "Cabinet_Back", (1.34, 0.030, 1.58), (0, 0.205, 0.92),
                         inside, bevel=0.004, rough=0.0012, seed=1))
    parts.append(C.board(kit, "Cabinet_LeftSide", (0.055, 0.45, 1.72), (-0.672, 0.0, 0.90),
                         case, bevel=0.008, rough=0.0015, seed=2))
    parts.append(C.board(kit, "Cabinet_RightSide", (0.055, 0.45, 1.72), (0.672, 0.0, 0.90),
                         case, bevel=0.008, rough=0.0015, seed=3))
    parts.append(C.board(kit, "Cabinet_TopRail", (1.36, 0.45, 0.050), (0, 0.0, 1.705),
                         case, bevel=0.009, rough=0.0012, seed=4))
    parts.append(C.board(kit, "Cabinet_BottomRail", (1.36, 0.45, 0.052), (0, 0.0, 0.120),
                         case, bevel=0.009, rough=0.0012, seed=5))
    parts.append(C.board(kit, "Cabinet_Plinth", (1.44, 0.50, 0.105), (0, -0.005, 0.052),
                         case, bevel=0.014, rough=0.0018, seed=6))
    parts.append(C.board(kit, "Cabinet_Cornice", (1.46, 0.50, 0.105), (0, -0.005, 1.772),
                         case, bevel=0.016, rough=0.0014, seed=7))
    parts.append(C.board(kit, "Cabinet_CorniceLip", (1.48, 0.062, 0.045), (0, -0.256, 1.700),
                         case, bevel=0.010, rough=0.001, seed=8))
    parts.append(C.board(kit, "Cabinet_PlinthWear", (1.30, 0.018, 0.028), (0, -0.246, 0.145),
                         inside, bevel=0.003, rough=0.001, seed=9))

    cols, rows = 7, 7
    grid_w, grid_h = 1.235, 1.390
    drawer_w = grid_w / cols
    drawer_h = grid_h / rows
    x0 = -grid_w / 2 + drawer_w / 2
    z0 = 0.255
    face_y = -0.232
    ajar = {(1, 4): 0.055, (5, 2): 0.030}
    # Rail shadows behind the drawer grid keep tiny gap lines from flattening in the bake.
    for c in range(cols + 1):
        x = -grid_w / 2 + c * drawer_w
        parts.append(C.dark_gap(kit, f"Drawer_VGap_{c}", (0.006, 0.006, grid_h + 0.020),
                                (x, face_y - 0.010, z0 + grid_h / 2 - drawer_h / 2)))
    for r in range(rows + 1):
        z = z0 - drawer_h / 2 + r * drawer_h
        parts.append(C.dark_gap(kit, f"Drawer_HGap_{r}", (grid_w + 0.020, 0.006, 0.006),
                                (0, face_y - 0.010, z)))

    for r in range(rows):
        for c in range(cols):
            x = x0 + c * drawer_w + rng.uniform(-0.0015, 0.0015)
            z = z0 + r * drawer_h + rng.uniform(-0.0012, 0.0012)
            protrude = ajar.get((c, r), 0.0)
            front_y = face_y - protrude
            # Slightly ajar drawers show a darker side box and shadowed cavity.
            if protrude:
                parts.append(C.board(kit, f"DrawerBox_{c}_{r}", (drawer_w - 0.020, protrude, drawer_h - 0.025),
                                     (x, face_y - protrude * 0.50, z), inside,
                                     bevel=0.004, rough=0.001, seed=1000 + r * 7 + c))
                parts.append(C.dark_gap(kit, f"DrawerCavity_{c}_{r}", (drawer_w - 0.020, 0.006, drawer_h - 0.020),
                                        (x, face_y + 0.010, z)))
            parts.append(C.board(kit, f"DrawerFront_{c}_{r}", (drawer_w - 0.018, 0.035, drawer_h - 0.018),
                                 (x, front_y, z), drawer, bevel=0.0, rough=0.0014,
                                 seed=2000 + r * 7 + c))
            parts.append(C.board(kit, f"DrawerBottomWear_{c}_{r}", (drawer_w - 0.045, 0.006, 0.009),
                                 (x, front_y - 0.021, z - drawer_h * 0.38), inside,
                                 bevel=0.001, rough=0.0006, seed=2500 + r * 7 + c))
            label_z = z + drawer_h * 0.17
            parts.extend(_label(kit, f"Label_{c}_{r}", x, front_y - 0.019, label_z,
                                drawer_w * 0.54, drawer_h * 0.21, paper, brass, ink, rng))
            knob_z = z - drawer_h * 0.15
            knob = kit.cylinder(f"DrawerKnob_{c}_{r}", 0.014, 0.020,
                                location=(x, front_y - 0.035, knob_z), rotation=(90, 0, 0),
                                material=brass, sides=14, bevel=0.0)
            parts.append(knob)
            parts.append(kit.sphere(f"DrawerKnobCap_{c}_{r}", 0.010,
                                    location=(x, front_y - 0.047, knob_z),
                                    material=brass, segments=12, rings=6, scale=(1.0, 0.55, 1.0)))

    return kit.join(parts, "SM_SeedsmanDrawers", unwrap=False, reshade=True, smooth_angle=58)
