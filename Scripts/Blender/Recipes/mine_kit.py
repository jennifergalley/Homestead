"""Mine kit: the timber sets, a barrow and ore baskets, the shaft ladder and the sump water of the shallow level.

    .\\Scripts\\Blender\\New-Prop.ps1 mine_kit -BeautySamples 64

Real-object research:
- Timber sets ("sets" or "stulls and caps") in 19th-century metal mines were round larch or Baltic
  pine, 20-25 cm across, bark stripped: two legs battered (leaning) a few degrees inward and a cap
  (collar) across their heads, the joints cut as a shallow saddle so the cap sits in the leg tops,
  wedged tight to the rock with split "lagging" poles over the caps where the back was loose. Old
  timber is grey-brown, black with damp toward the floor, furred white with fungus here and there.
- No tramway (Jenny: "In Poldark they hauled it out by hand"): broken rock went out in round willow
  baskets carried or dragged, and in plank wheelbarrows with an iron-tyred wooden wheel.
- Shaft ladders were wooden: two 7 x 10 cm stiles and round rungs about 30 cm apart, 45 cm wide,
  spiked to the shaft's timbering.

Pivots (Unreal frame; meshes are symmetric in Y or authored with Y negated): every piece stands on
the floor at z = 0; sets and rails run along +X (place with the gallery's yaw); the ladder stands at
its foot with rungs along Y, stiles leaning on -X. Everything is generated here: no scanned or
downloaded geometry or textures.
"""
import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_rocks as rocks  # noqa: E402
from mine import layout as L  # noqa: E402

NAME = "MineKit"
DESCRIPTION = ("Mine kit: gallery and drift timber sets, a wooden barrow, a willow ore basket, a 10 m shaft "
               "ladder and the sump's water.")
COLLISION = "box"
TRIANGLE_BUDGET = 400000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 48}
BAKE_MESHES = {"SM_MineSumpWater": False, "*": True}
BEAUTY = {"pose": (0, 0, -90), "ground": "origin", "views": ["hero", "detail"],
          "meshes": {"SM_MineTimberSet": {"focus": (0.0, 1.5, 2.2), "eye_distance": 5.0},
                     "SM_MineTimberSetDrift": {"focus": (0.0, 1.3, 2.0), "eye_distance": 4.5},
                     "SM_MineBarrow": {"focus": (0.0, 0.0, 0.4), "eye_distance": 3.0},
                     "SM_MineBasket": {"focus": (0.0, 0.0, 0.2), "eye_distance": 1.6},
                     "SM_MineLadder": {"focus": (0.0, 0.0, 1.5), "eye_distance": 4.0},
                     "SM_MineSumpWater": {"focus": (0.0, 0.0, 0.0), "eye_distance": 3.0}}}
REPORT = {"pivot": "Floor level; sets and rails run along +X (place with the gallery's yaw)."}

LADDER_LENGTH, LADDER_WIDTH, RUNG_STEP = 10.0, 0.46, 0.30


def log(kit, name, a, b, r0, r1, mat, seed, knots=4):
    """A bark-stripped round timber from a to b, tapering r0 -> r1, a little out of round with knots."""
    pts = [tuple(np.asarray(a) + (np.asarray(b) - np.asarray(a)) * t) for t in np.linspace(0, 1, 14)]
    obj = kit.tube(name, pts, radii=[r0 + (r1 - r0) * t for t in np.linspace(0, 1, 14)], sides=18, material=mat)
    rng = np.random.default_rng(seed)
    noise = rocks.Noise(seed)
    knot_at = rng.uniform(0.1, 0.9, knots)
    knot_ang = rng.uniform(0, 2 * math.pi, knots)
    axis = np.asarray(b, float) - np.asarray(a, float)
    length = np.linalg.norm(axis)
    axis /= length

    def fn(co, pcoord):
        t = float(np.dot(np.asarray(co) - np.asarray(a), axis)) / length
        out = 0.006 * noise.fbm(np.asarray(co)[None, :] * 3.0, 3)[0]
        for kt, ka in zip(knot_at, knot_ang):
            ang = math.atan2(pcoord[1], pcoord[0])
            d = math.hypot((t - kt) * length / 0.05, (math.remainder(ang - ka, 2 * math.pi)) / 0.35)
            out += 0.008 * math.exp(-d * d)
        return out
    kit.displace(obj, fn)
    return obj


def timber_set(kit, name, half, height, mat, seed):
    """Two battered legs and a cap across their heads; the cap sits in saddles cut in the leg tops."""
    rng = np.random.default_rng(seed)
    parts = []
    batter = math.radians(4.0)
    r = 0.11
    for side in (-1, 1):
        foot = (rng.uniform(-0.03, 0.03), side * half, -0.08)
        head = (foot[0] + rng.uniform(-0.03, 0.03), side * (half - math.tan(batter) * height), height)
        parts.append(log(kit, f"Leg{side}", foot, head, r * 1.08, r, mat, seed + side + 2))
    span = half - math.tan(batter) * height + 0.32
    parts.append(log(kit, "Cap", (0.0, -span, height + r * 0.6), (0.0, span, height + r * 0.6 + 0.02),
                     r * 1.05, r * 0.98, mat, seed + 5))
    # Wedges driven between the cap and the rock, and two lagging poles laid over the cap.
    for k, y in enumerate((-span * 0.5, span * 0.45)):
        parts.append(kit.box(f"Wedge{k}", (0.12, 0.08, 0.10), (0.0, -y, height + r * 1.9), rotation=(0, 8, 0),
                             material=mat))
    for k, x in enumerate((-0.18, 0.2)):
        parts.append(log(kit, f"Lag{k}", (x, -span * 0.85, height + r * 1.7), (x + 0.02, span * 0.8, height + r * 1.75),
                         0.045, 0.04, mat, seed + 9 + k, knots=1))
    return kit.join(parts, name, pivot=None, smooth_angle=50.0)


def barrow(kit, wood, iron):
    """A miner's wooden wheelbarrow: an open-ended plank tray on two shafts, a solid wheel with an iron
    tyre at the front, two legs at the back. Wheel forward along +X, handles to -X, resting on its legs."""
    parts = []
    tray_z, tray_l, tray_w = 0.42, 0.85, 0.62
    tilt = 7.0
    for side in (-1, 1):
        shaft = [(-0.85, side * 0.27, 0.48), (0.0, side * 0.24, 0.38), (0.52, side * 0.12, 0.25)]
        parts.append(kit.tube(f"Shaft{side}", [(p[0], -p[1], p[2]) for p in shaft], radius=0.025, sides=10,
                              material=wood))
        parts.append(kit.box(f"Leg{side}", (0.05, 0.05, 0.40), (-0.42, -side * 0.26, 0.2), rotation=(0, 6, 0),
                             material=wood, bevel=0.006))
        parts.append(kit.box(f"Side{side}", (tray_l, 0.025, 0.24), (-0.05, -side * tray_w / 2, tray_z + 0.12),
                             rotation=(side * 12.0, tilt, 0), material=wood, bevel=0.005))
    parts.append(kit.box("Bottom", (tray_l, tray_w - 0.06, 0.025), (-0.05, 0.0, tray_z), rotation=(0, tilt, 0),
                         material=wood, bevel=0.005))
    parts.append(kit.box("Back", (0.025, tray_w + 0.04, 0.26), (-0.48, 0.0, tray_z + 0.13), rotation=(0, -18, 0),
                         material=wood, bevel=0.005))
    parts.append(kit.cylinder("Wheel", 0.2, 0.06, (0.52, 0.0, 0.2), rotation=(90, 0, 0), material=wood, sides=28,
                              bevel=0.008))
    parts.append(kit.cylinder("Tyre", 0.205, 0.064, (0.52, 0.0, 0.2), rotation=(90, 0, 0), material=iron, sides=28,
                              cap=False))
    parts.append(kit.cylinder("Axle", 0.018, 0.30, (0.52, 0.0, 0.2), rotation=(90, 0, 0), material=iron, sides=10))
    return kit.join(parts, "SM_MineBarrow", pivot=None, smooth_angle=40.0)


def basket(kit, willow, stone):
    """A round willow ore basket (a 'flasket'), about 50 cm across, with two hand-holds, half full of
    broken rock: woven stakes and weavers swept as thin rods."""
    parts = []
    rng = np.random.default_rng(31)
    r0, r1, h = 0.20, 0.26, 0.30
    stakes = 24
    for k in range(stakes):
        a = 2 * math.pi * k / stakes
        pts = [((r0 + (r1 - r0) * t) * math.cos(a), (r0 + (r1 - r0) * t) * math.sin(a), 0.01 + h * t)
               for t in np.linspace(0, 1, 6)]
        parts.append(kit.tube(f"Stake{k}", pts, radius=0.006, sides=6, material=willow))
    rows = 22
    for j in range(rows):
        t = (j + 0.5) / rows
        r = r0 + (r1 - r0) * t + 0.006
        pts = []
        for i in range(97):
            a = 2 * math.pi * i / 96
            weave = 0.007 * math.cos(a * stakes / 2 + j * math.pi)
            pts.append(((r + weave) * math.cos(a), (r + weave) * math.sin(a), 0.01 + h * t))
        parts.append(kit.tube(f"Weaver{j}", pts, radius=0.0065, sides=6, material=willow, cap=False))
    rim = [((r1 + 0.012) * math.cos(a), (r1 + 0.012) * math.sin(a), h + 0.01) for a in np.linspace(0, 2 * math.pi, 97)]
    parts.append(kit.tube("Rim", rim, radius=0.016, sides=8, material=willow, cap=False))
    for side in (-1, 1):
        loop = [(side * (r1 + 0.012 + 0.05 * math.sin(t)), 0.06 * math.cos(t), h + 0.01 + 0.03 * math.sin(t))
                for t in np.linspace(0, math.pi, 13)]
        parts.append(kit.tube(f"Handle{side}", loop, radius=0.011, sides=8, material=willow))
    parts.append(kit.cylinder("Base", r0, 0.02, (0, 0, 0.01), material=willow, sides=24))
    for k in range(12):
        a, d = rng.uniform(0, 2 * math.pi), rng.uniform(0, r0 * 0.9)
        s = rng.uniform(0.05, 0.09)
        parts.append(kit.box(f"Rock{k}", (s, s * 0.8, s * 0.6), (d * math.cos(a), d * math.sin(a), h * 0.62 + rng.uniform(0, 0.05)),
                             rotation=tuple(rng.uniform(0, 60, 3)), material=stone, bevel=0.01))
    return kit.join(parts, "SM_MineBasket", pivot=None, smooth_angle=50.0)

def ladder(kit, mat_wood, mat_iron):
    parts = []
    for side in (-1, 1):
        y = side * LADDER_WIDTH / 2
        parts.append(kit.box(f"Stile{side}", (0.07, 0.10, LADDER_LENGTH), (0.0, y, LADDER_LENGTH / 2), material=mat_wood,
                             bevel=0.01))
    for k in range(int(LADDER_LENGTH / RUNG_STEP)):
        z = 0.28 + k * RUNG_STEP
        parts.append(kit.cylinder(f"Rung{k}", 0.019, LADDER_WIDTH + 0.06, (0.0, 0.0, z), rotation=(90, 0, 0),
                                  material=mat_wood, sides=12))
    # Cleats every 2 m that spike the stiles to the shaft lining, only along the ladder (one past its top
    # floated over the trapdoor once the ladder was dropped under the lid).
    for k in range(int((LADDER_LENGTH - 0.6) / 2.0) + 1):
        for side in (-1, 1):
            parts.append(kit.box(f"Cleat{k}{side}", (0.10, 0.03, 0.03), (-0.06, side * LADDER_WIDTH / 2, 0.5 + k * 2.0),
                                 material=mat_iron))
    return kit.join(parts, "SM_MineLadder", pivot=None, smooth_angle=35.0)


def sump_water(kit, mat):
    hx, hy = L.SUMP["half"]
    obj = kit.box("Water", (2 * hx + 0.3, 2 * hy + 0.3, 0.01), (0.0, 0.0, -0.005), material=mat)
    return kit.join([obj], "SM_MineSumpWater", pivot=None)


def build(kit):
    timber = kit.mats.aged_wood("M_MineTimber", light=(0.21, 0.165, 0.11), dark=(0.055, 0.045, 0.035), roughness=0.86,
                                saw=0.15, grime=0.7, seed=3.0)
    plank = kit.mats.aged_wood("M_MineCartWood", light=(0.24, 0.18, 0.11), dark=(0.07, 0.05, 0.035), roughness=0.84,
                               saw=0.5, grime=0.6, seed=11.0)
    iron = kit.mats.wrought_iron("M_MineIron", rust=0.75, wear=0.35, seed=5.0)
    water = kit.material("M_MineSumpWater", (0.008, 0.010, 0.010), roughness=0.04)
    willow = kit.mats.aged_wood("M_MineWillow", light=(0.30, 0.22, 0.13), dark=(0.10, 0.07, 0.04), roughness=0.75,
                                saw=0.0, grime=0.5, seed=13.0)
    rock = kit.mats.granite("M_MineBasketRock", grain=0.004, scale=0.3, patina=0.2, lichen=0.0, moss=0.0, soil=0.0,
                            seed=17.0)
    return [
        timber_set(kit, "SM_MineTimberSet", L.GALLERY_HALF - 0.22, L.GALLERY_WALL_H - 0.05, timber, 11),
        timber_set(kit, "SM_MineTimberSetDrift", L.DRIFT_HALF - 0.2, L.DRIFT_WALL_H - 0.05, timber, 21),
        barrow(kit, plank, iron),
        basket(kit, willow, rock),
        ladder(kit, timber, iron),
        sump_water(kit, water),
    ]
