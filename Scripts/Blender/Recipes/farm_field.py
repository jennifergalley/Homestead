"""Overgrown abandoned farm-field modules: old furrows, dead stalks and bean stakes.

Real-object research:
- Ridge-and-furrow cultivation leaves raised beds about 0.7-1.0 m apart; after two wet Cornish
  decades the ridges slump into rounded 12-18 cm rolls with furrows partly filled by moss, dead
  stubble and washed loam. Dark loam is not black: average linear albedo around 0.05-0.09 with a
  slightly paler dry crust on high spots.
- Abandoned turnip/brassica and dock stems weather to straw-grey, stay standing as hollow,
  angular stems with seed pods, and break at nodes. Hazel bean poles keep bark and lichens for
  years, lean inward or fall flat, and their rotten flax/hemp twine collapses between poles.

Everything below is original procedural geometry/materials; no downloaded sources. Blender
(x,y,z) imports to Unreal as (x,-y,z). Furrow rows run along local X.
"""
import importlib.util
import math
import os
import random
from pathlib import Path

from mathutils import Vector, noise

_spec = importlib.util.spec_from_file_location("farm_common", Path(__file__).with_name("farm") / "common.py")
common = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(common)

NAME = "FarmField"
DESCRIPTION = ("Modular abandoned field details: an old slumped ridge-and-furrow patch, dead crop/weed "
               "stalks, and old hazel bean/pea stakes with rotten twine.")
COLLISION = "none"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 16 if DRAFT else 96}
BEAUTY = {
    "pose": (0, 0, 0),
    "focus": (0.0, 0.0, 0.08),
    "views": ["hero", "detail", "eye"],
    "meshes": {
        "SM_FarmFurrows": {"focus": (0.0, 0.0, 0.04), "eye_distance": 5.2, "ground": "origin"},
        "SM_FarmDeadStalks": {"focus": (0.0, 0.0, 0.55), "eye_distance": 2.1},
        "SM_FarmStakes": {"focus": (0.0, 0.0, 1.0), "eye_distance": 3.0},
    },
}
REPORT = {
    "unreal_frame": "Blender (x,y,z) imports as Unreal (x,-y,z); extents in report are Unreal cm.",
    "furrow_axis": "Old crop rows/ridges run along local X; the 6 m patch length is X and width is Y.",
    "pivot_notes": ("SM_FarmFurrows origin is at the patch centre on the average ground line z=0, with feathered "
                    "edges below zero; stalks and stakes use bottom-centre pivots."),
}

SEED = 185120
FURROW_X = 6.0
FURROW_Y = 4.0
GRID_X = 112 if not DRAFT else 72
GRID_Y = 82 if not DRAFT else 48


def _materials(kit):
    loam = common.loam(kit, "M_FarmFieldDarkCornishLoam", seed=1.0)
    grass = common.moss_lichen(kit, "M_FarmFieldGrassMossFilm", seed=2.0)
    straw = common.straw(kit, "M_FarmFieldBleachedDeadStalk", seed=3.0)
    hazel = kit.mats.bark("M_FarmFieldHazelBark", light=(0.105, 0.095, 0.075), dark=(0.028, 0.024, 0.018),
                          scale=0.32, roughness=0.94, lichen=0.55, stretch=0.20)
    twine = common.twine(kit, "M_FarmFieldRottenTwine", seed=4.0)
    common.zero_subsurface(loam, grass, straw, hazel, twine)
    return dict(loam=loam, grass=grass, straw=straw, hazel=hazel, twine=twine)


def _hash2(x, y, salt=0.0):
    return (math.sin(x * 127.1 + y * 311.7 + salt * 74.7) * 43758.5453) % 1.0


def _furrow_height(x, y):
    edge_x = common.smoothstep(FURROW_X * 0.5, FURROW_X * 0.5 - 0.42, abs(x))
    edge_y = common.smoothstep(FURROW_Y * 0.5, FURROW_Y * 0.5 - 0.35, abs(y))
    edge = edge_x * edge_y
    phase = (y + 0.045 * noise.noise(Vector((x * 0.55, 0.0, 1.2))) +
             0.015 * noise.noise(Vector((x * 2.1, 0.0, 3.1)))) / 0.88
    ridge = (0.5 + 0.5 * math.cos(phase * math.tau)) ** 1.55
    ridge *= 0.88 + 0.18 * noise.noise(Vector((x * 0.75, y * 0.75, 2.0)))
    # Furrow bottoms hover around 0 to -4 cm, ridge tops 12-18 cm.
    h = -0.025 + 0.172 * ridge
    h += 0.012 * noise.noise(Vector((x * 1.3, y * 1.3, 4.0)))
    h += 0.004 * noise.noise(Vector((x * 7.0, y * 7.0, 5.0)))
    # Two decades of slumping: softened crests, no hard rows near the edge.
    h *= edge
    # Feather all sides down below z=0 so it melts into sloping terrain.
    h += -0.080 * (1.0 - edge) ** 1.8
    # Washed hollows and clods.
    cells, pts = noise.voronoi(Vector((x * 12.5, y * 12.5, 9.0)), distance_metric="DISTANCE", exponent=2.5)
    if _hash2(pts[0].x, pts[0].y, 2.0) > 0.74:
        h += edge * 0.011 * max(0.0, 1.0 - cells[0] / 0.42) ** 1.8
    return h


def _furrow_material_indices(obj):
    for poly in obj.data.polygons:
        c = poly.center
        # Moss/grass film favours wetter furrow bottoms and patch margins.
        phase = c.y / 0.88
        furrow = (0.5 - 0.5 * math.cos(phase * math.tau)) ** 2
        margin = 1.0 - (common.smoothstep(FURROW_X * 0.5, FURROW_X * 0.5 - 0.50, abs(c.x)) *
                        common.smoothstep(FURROW_Y * 0.5, FURROW_Y * 0.5 - 0.44, abs(c.y)))
        if furrow > 0.52 or margin > 0.35:
            poly.material_index = 1


def build_furrows(kit, mats):
    verts, faces = [], []
    for i in range(GRID_X):
        x = -FURROW_X * 0.5 + FURROW_X * i / (GRID_X - 1)
        for j in range(GRID_Y):
            y = -FURROW_Y * 0.5 + FURROW_Y * j / (GRID_Y - 1)
            verts.append((x, y, _furrow_height(x, y)))
    for i in range(GRID_X - 1):
        for j in range(GRID_Y - 1):
            a = i * GRID_Y + j
            faces.append((a, a + GRID_Y, a + GRID_Y + 1, a + 1))
    sheet = kit.mesh("FarmFurrows_Heightfield", verts, faces, material=mats["loam"])
    sheet.data.materials.append(mats["grass"])
    for poly in sheet.data.polygons:
        if poly.normal.z < 0:
            poly.flip()
    _furrow_material_indices(sheet)
    # Hand planar UV before finalization; no smart-project seams across ridges.
    obj = kit.join([sheet], "SM_FarmFurrows", pivot=None, unwrap=False, reshade=True, smooth_angle=80)
    uv = obj.data.uv_layers.active.data
    for loop in obj.data.loops:
        co = obj.data.vertices[loop.vertex_index].co
        uv[loop.index].uv = ((co.x + FURROW_X * 0.5) / FURROW_X, (co.y + FURROW_Y * 0.5) / FURROW_Y)
    return obj


def _curved_stem_points(base, height, lean, seed, bends=6):
    rng = random.Random(seed)
    pts = []
    base = Vector(base)
    lean = Vector(lean)
    phase = rng.uniform(0, math.tau)
    for i in range(bends):
        u = i / (bends - 1)
        sway = Vector((0.035 * math.sin(u * math.pi * 1.3 + phase), 0.025 * math.sin(u * math.pi * 2.1 + phase * 0.7), 0))
        pts.append(tuple(base + lean * u + sway * u + Vector((0, 0, height * u))))
    return pts


def _seed_pods(kit, prefix, stem_pts, mats, rng, count, seed):
    parts = []
    tip = Vector(stem_pts[-1])
    up = (Vector(stem_pts[-1]) - Vector(stem_pts[-2])).normalized()
    for i in range(count):
        a = rng.uniform(0, math.tau)
        h = rng.uniform(0.0, 0.22)
        side = Vector((math.cos(a), math.sin(a), rng.uniform(-0.1, 0.2))).normalized()
        stalk_base = tip - up * h
        stalk_tip = stalk_base + side * rng.uniform(0.035, 0.085) + up * rng.uniform(0.00, 0.025)
        parts.append(kit.tube(f"{prefix}_PodStem_{i}", [stalk_base, stalk_tip], radius=0.0022, sides=6, material=mats["straw"]))
        pod = kit.sphere(f"{prefix}_DryPod_{i}", rng.uniform(0.006, 0.012), location=tuple(stalk_tip),
                         material=mats["straw"], segments=8, rings=4, scale=(0.55, 0.55, rng.uniform(1.4, 2.6)))
        pod.rotation_euler = side.to_track_quat("Z", "Y").to_euler()
        parts.append(pod)
    return parts


def build_dead_stalks(kit, mats):
    rng = random.Random(SEED + 200)
    parts = []
    stems = 20 if not DRAFT else 18
    for i in range(stems):
        r = 0.40 * math.sqrt(rng.random())
        a = rng.uniform(0, math.tau)
        base = (math.cos(a) * r, math.sin(a) * r, 0.0)
        height = rng.uniform(0.50, 1.12)
        lean = (rng.uniform(-0.16, 0.16), rng.uniform(-0.16, 0.16), 0)
        pts = _curved_stem_points(base, height, lean, SEED + 210 + i, bends=7)
        rad0 = rng.uniform(0.006, 0.012) if i % 5 else rng.uniform(0.012, 0.018)
        radii = [rad0 * (1.0 - 0.55 * j / (len(pts) - 1)) for j in range(len(pts))]
        parts.append(kit.tube(f"FarmDeadStalk_{i}", pts, radii=radii, sides=10 if rad0 > 0.010 else 8,
                              material=mats["straw"]))
        # Dock/brassica side branches.
        for k in range(rng.randint(1, 4)):
            uidx = rng.randint(2, len(pts) - 2)
            basep = Vector(pts[uidx])
            side = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(0.05, 0.55))).normalized()
            length = rng.uniform(0.08, 0.22)
            branch = [basep, basep + side * length * 0.65 + Vector((0, 0, 0.025)), basep + side * length]
            parts.append(kit.tube(f"FarmDeadStalk_{i}_Branch_{k}", branch,
                                  radii=[radii[uidx] * 0.42, radii[uidx] * 0.28, 0.0025],
                                  sides=6, material=mats["straw"]))
        if rng.random() < 0.78:
            parts.extend(_seed_pods(kit, f"FarmDeadStalk_{i}", pts, mats, rng, rng.randint(4, 10), SEED + 280 + i))
        # Bent and snapped stems with exposed dark break.
        if rng.random() < 0.22:
            snap = Vector(pts[rng.randint(3, len(pts) - 2)])
            parts.append(common.make_splinter(kit, f"FarmDeadStalk_{i}_SnapShard", snap, (rng.uniform(-0.5, 0.5), rng.uniform(-0.5, 0.5), 0.25),
                                              (1, 0, 0), (0, 0, 1), rng.uniform(0.004, 0.009),
                                              rng.uniform(0.003, 0.006), rng.uniform(0.035, 0.10),
                                              mats["straw"], SEED + 350 + i, rows=4))
    # Dead leaves and fallen stalk fragments at the clump foot, settled on ground.
    for i in range(28 if not DRAFT else 10):
        x, y = rng.uniform(-0.43, 0.43), rng.uniform(-0.43, 0.43)
        if x * x + y * y > 0.20:
            continue
        length = rng.uniform(0.08, 0.25)
        angle = rng.uniform(0, math.tau)
        parts.append(kit.tube(f"FarmDeadStalks_GroundStem_{i}",
                              [(x, y, 0.012), (x + math.cos(angle) * length, y + math.sin(angle) * length, 0.014)],
                              radius=rng.uniform(0.0025, 0.006), sides=6, material=mats["straw"]))
    return kit.join(parts, "SM_FarmDeadStalks", pivot="base", unwrap=True, reshade=True, smooth_angle=44)


def _pole(kit, name, base, top, radius, mats, seed):
    rng = random.Random(seed)
    pts = []
    base, top = Vector(base), Vector(top)
    for i in range(9):
        u = i / 8
        p = base.lerp(top, u)
        p.x += 0.025 * math.sin(u * math.pi * 1.5 + seed) * (1 - abs(u - 0.5))
        p.y += 0.020 * math.sin(u * math.pi * 1.9 + seed * 0.3) * (1 - abs(u - 0.5))
        pts.append(tuple(p))
    radii = [radius * (1.0 - 0.18 * i / 8) for i in range(9)]
    pole = kit.tube(name, pts, radii=radii, sides=12, material=mats["hazel"])
    def disp(co, pco):
        return -0.0014 * abs(math.sin(math.atan2(pco.y, pco.x) * 5.0 + pco.z * 18.0 + seed)) + 0.0008 * math.sin(pco.z * 80.0)
    kit.displace(pole, disp)
    return pole


def build_stakes(kit, mats):
    rng = random.Random(SEED + 500)
    parts = []
    bases = [(-1.24, -0.44, 0), (-0.82, 0.43, 0), (-0.43, -0.44, 0), (0.13, 0.40, 0),
             (0.58, -0.40, 0), (1.04, 0.37, 0)]
    apexes = [(-1.02, 0.02, 2.20), (-0.60, -0.03, 2.08), (-0.18, 0.02, 2.28),
              (0.24, -0.04, 2.05), (0.65, 0.02, 2.30), (0.82, -0.02, 1.94)]
    pole_tops = []
    for i, (base, top) in enumerate(zip(bases, apexes)):
        parts.append(_pole(kit, f"FarmStakes_LeaningHazelPole_{i}", base, top, rng.uniform(0.014, 0.022), mats, SEED + 510 + i))
        pole_tops.append(Vector(top))
    # Two fallen poles lying flat and settled on the ground.
    for i, y in enumerate((-0.58, 0.54)):
        x0 = rng.uniform(-1.22, -0.88)
        x1 = rng.uniform(0.86, 1.26)
        parts.append(_pole(kit, f"FarmStakes_FallenHazelPole_{i}", (x0, y, 0.020), (x1, y + rng.uniform(-0.08, 0.08), 0.030),
                           rng.uniform(0.013, 0.019), mats, SEED + 540 + i))
    # Rotten twine runs between pole pairs and sags; broken fragments hang from nodes.
    for i in range(len(bases) - 1):
        for z in (0.78, 1.22, 1.62):
            if rng.random() < 0.30:
                continue
            a = Vector(bases[i]).lerp(pole_tops[i], z / max(pole_tops[i].z, 0.1))
            b = Vector(bases[i + 1]).lerp(pole_tops[i + 1], z / max(pole_tops[i + 1].z, 0.1))
            mid = (a + b) * 0.5 + Vector((0, 0, -rng.uniform(0.03, 0.11)))
            parts.append(kit.tube(f"FarmStakes_RottenTwine_{i}_{z:.1f}", [a, mid, b],
                                  radius=0.0028, sides=6, material=mats["twine"]))
            if rng.random() < 0.45:
                end = a if rng.random() < 0.5 else b
                parts.append(kit.tube(f"FarmStakes_HangingTwine_{i}_{z:.1f}", [end, end + Vector((rng.uniform(-0.02, 0.02), rng.uniform(-0.02, 0.02), -rng.uniform(0.06, 0.18)))],
                                      radius=0.0025, sides=5, material=mats["twine"]))
    for i in range(20 if not DRAFT else 7):
        pole = rng.choice(bases)
        parts.append(kit.sphere(f"FarmStakes_BaseMoss_{i}", rng.uniform(0.008, 0.024),
                                location=(pole[0] + rng.uniform(-0.04, 0.04), pole[1] + rng.uniform(-0.04, 0.04), rng.uniform(0.005, 0.045)),
                                material=mats["hazel"] if rng.random() < 0.45 else mats["loam"], segments=8, rings=4, scale=(1.0, 0.8, 0.2)))
    return kit.join(parts, "SM_FarmStakes", pivot="base", unwrap=True, reshade=True, smooth_angle=46)


def build(kit):
    mats = _materials(kit)
    return [
        build_furrows(kit, mats),
        build_dead_stalks(kit, mats),
        build_stakes(kit, mats),
    ]
