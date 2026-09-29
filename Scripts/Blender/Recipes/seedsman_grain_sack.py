"""Open hessian grain sack with wheat and wooden scoop for Tregear's shop.

Real-object research (written before modeling):
- Corn merchants displayed grain in coarse jute/hessian sacks with the top untied and
  rolled outward into a thick cuff so customers could see the contents. A standing
  part-filled sack slumps into a square-ish foot, bulges at the belly and wrinkles
  where the cuff weight folds the cloth. Hessian weave is coarse, with slubs, loose
  fibres and dusty flour/bran bloom in creases.
- Wheat kernels are 5-7 mm long, oval with a groove and matte golden-brown bran;
  a convincing display reads as thousands of kernels from a metre away, but near the
  rim the top needs individual grains, not a flat yellow disc. Wooden shop scoops were
  carved/turned from beech or pine, with a thin oval bowl and a simple handle.
- Dimensions here: roughly 0.60 m tall, 0.50 m across at the rolled mouth and broad
  enough at the base to stand on the floor. Pivot is bottom-centre; front/best view
  faces -Y. Original procedural geometry/materials only.
"""
import math
import random
import sys
from pathlib import Path

from mathutils import Euler, Matrix, Vector, noise

sys.path.insert(0, str(Path(__file__).resolve().parent))
import store.common as C

NAME = "SeedsmanGrainSack"
DESCRIPTION = "Open rolled-down hessian sack of wheat grain with wooden scoop; bottom-centre pivot."
COLLISION = "convex"
TRIANGLE_BUDGET = 20000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 96}
BEAUTY = {"pose": (0, 0, -16), "focus": (0.015, -0.055, 0.505), "detail_distance": 0.32}
REPORT = {"dimensions_m": {"height": 0.60, "mouth_width": 0.50, "depth": 0.43},
          "pivot": "bottom-centre; best display face -Y"}


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def grain_material(kit, name):
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seed = g.vmath("ADD", p, (1.7, 4.2, 2.1))
    tone = g.noise(seed, scale=38.0, detail=4.0, roughness=0.62).outputs["Fac"]
    stripe = g.wave(g.combine(g.math("MULTIPLY", x, 2.0), g.math("MULTIPLY", y, 1.0),
                              g.math("MULTIPLY", z, 9.0)), scale=18.0, distortion=2.0,
                    detail=2.0, kind="BANDS", direction="Z").outputs["Fac"]
    color = g.ramp(g.math("ADD", g.math("MULTIPLY", tone, 0.65), g.math("MULTIPLY", stripe, 0.35)),
                   [(0.20, (0.28, 0.17, 0.055)), (0.58, (0.55, 0.36, 0.115)),
                    (0.92, (0.73, 0.54, 0.20))])
    dust = g.noise(seed, scale=12.0, detail=3.0).outputs["Fac"]
    color = g.mix(color, (0.64, 0.55, 0.36), g.remap(dust, 0.68, 0.82, 0.0, 0.22))
    g.set("Base Color", color)
    g.set("Roughness", 0.82)
    groove = g.voronoi(g.vmath("MULTIPLY", seed, (1.0, 1.0, 0.22)), scale=85.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    height = g.math("ADD", g.math("MULTIPLY", tone, 0.25), g.math("MULTIPLY", g.remap(groove, 0.0, 0.06), -0.35))
    g.set("Normal", g.bump(height, strength=0.33, distance=0.00045))
    return g.mat


def sack_body(kit, mat):
    sides = 56
    ts = [0.0, 0.025, 0.06, 0.12, 0.20, 0.32, 0.46, 0.60, 0.73, 0.84, 0.92, 0.985]
    rows, coords = [], []
    for t in ts:
        z = 0.50 * t
        body = math.sin(math.pi * min(t, 0.98) / 0.98) ** 0.22
        base = smoothstep(0.0, 0.16, t)
        mouth = smoothstep(0.70, 0.98, t)
        rx = 0.105 + 0.145 * body * (1 - 0.22 * mouth) + 0.020 * (1 - base)
        ry = 0.080 + 0.105 * body * (1 - 0.18 * mouth) + 0.018 * (1 - base)
        cx = -0.016 * t + 0.008 * math.sin(t * 5.0)
        cy = -0.018 * math.sin(math.pi * t) - 0.018 * mouth
        row, pco = [], []
        for i in range(sides):
            a = 2 * math.pi * i / sides
            square = 1 + 0.10 * math.cos(4 * a) * smoothstep(0.0, 0.20, t) * (1 - mouth * 0.5)
            pleat = 1.0 - 0.10 * mouth * max(0.0, math.cos(12 * a + 0.6))
            wr = 0.006 * noise.noise(Vector((math.cos(a) * 5.0 + 1.0, math.sin(a) * 5.0, t * 8.0)))
            x = cx + (rx * square * pleat + wr) * math.cos(a)
            y = cy + (ry * square * pleat + 0.7 * wr) * math.sin(a)
            zz = z + 0.004 * noise.noise(Vector((i * 0.3, 2.0, t * 7.0))) * smoothstep(0.0, 0.10, t)
            if t < 0.025:
                zz = 0.004 * math.sin(a) ** 2
            row.append((x, y, zz))
            pco.append((x, y, z))
        rows.append(row)
        coords.append(pco)
    obj = kit.loft("OpenSackBody", rows, material=mat, coords=coords, cap_start=True, cap_end=False)

    def folds(co, pco):
        a = math.atan2(pco.y, pco.x)
        vertical = math.sin(13 * a + 1.2) * smoothstep(0.12, 0.55, pco.z) * smoothstep(0.52, 0.30, pco.z)
        mouth = math.sin(16 * a + 0.4) * smoothstep(0.34, 0.50, pco.z)
        weave = noise.noise(Vector((pco.x * 80, pco.y * 80, pco.z * 12)))
        return 0.0045 * vertical + 0.0035 * mouth + 0.0013 * weave

    kit.displace(obj, folds)
    return obj


def cuff(kit, mat):
    pts = []
    sides = 73
    for i in range(sides):
        a = 2 * math.pi * i / (sides - 1)
        wob = 1 + 0.035 * noise.noise(Vector((math.cos(a) * 3.2, math.sin(a) * 3.2, 3.0)))
        x = 0.235 * wob * math.cos(a) - 0.010
        y = 0.180 * wob * math.sin(a) - 0.030
        z = 0.485 + 0.010 * math.sin(3 * a + 0.3) + 0.006 * noise.noise(Vector((math.cos(a) * 6, math.sin(a) * 6, 0)))
        pts.append(Vector((x, y, z)))
    return kit.tube("RolledHessianCuff", pts, radius=lambda t: 0.025 + 0.006 * math.sin(math.tau * t * 5.0) ** 2,
                    sides=16, material=mat)


def grain_heap(kit, mat):
    rings, segs = 16, 64
    verts, faces, coords = [(0, -0.020, 0.485)], [], [(0, -0.020, 0.485)]
    for i in range(1, rings + 1):
        rr = i / rings
        for j in range(segs):
            a = math.tau * (j + 0.35 * (i % 2)) / segs
            x = 0.205 * rr * math.cos(a) - 0.005
            y = 0.148 * rr * math.sin(a) - 0.032
            heap = 0.044 * (1 - rr * rr) + 0.006 * noise.noise(Vector((x * 36, y * 36, 1.3)))
            z = 0.455 + heap
            verts.append((x, y, z))
            coords.append((x, y, z))
    for j in range(segs):
        faces.append((0, 1 + j, 1 + (j + 1) % segs))
    for i in range(rings - 1):
        a0, b0 = 1 + i * segs, 1 + (i + 1) * segs
        for j in range(segs):
            faces.append((a0 + j, b0 + j, b0 + (j + 1) % segs, a0 + (j + 1) % segs))
    obj = kit.mesh("GrainHeapedSurface", verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def add_kernels(kit, parts, mat):
    rng = random.Random(2931)
    for k in range(214):
        # Biased toward the visible front and rim.
        a = rng.uniform(-math.pi * 0.95, math.pi * 0.45)
        rr = math.sqrt(rng.random()) * rng.uniform(0.12, 0.98)
        x = 0.200 * rr * math.cos(a) - 0.005
        y = 0.145 * rr * math.sin(a) - 0.033
        z = 0.459 + 0.043 * (1 - rr * rr) + rng.uniform(0.004, 0.018)
        if k >= 190:
            # A few proud kernels near the front rim catch the eye at 1 m instead of
            # disappearing into the baked grain surface.
            x = rng.uniform(-0.11, 0.13)
            y = rng.uniform(-0.155, -0.045)
            rr2 = min(0.96, math.hypot((x + 0.005) / 0.200, (y + 0.033) / 0.145))
            z = 0.468 + 0.038 * (1 - rr2 * rr2) + rng.uniform(0.010, 0.022)
        kernel = kit.sphere(f"WheatKernel_{k:03d}", rng.uniform(0.0045, 0.0062),
                            location=(0, 0, 0), material=mat, segments=8, rings=5,
                            scale=(1.0, 0.48, 0.34))
        rot = Euler((rng.uniform(-0.45, 0.45), rng.uniform(-0.30, 0.30), rng.uniform(0, math.tau)), "XYZ")
        kernel.data.transform(Matrix.Translation((x, y, z)) @ Matrix.Rotation(rot.z, 4, "Z") @
                              Matrix.Rotation(rot.y, 4, "Y") @ Matrix.Rotation(rot.x, 4, "X"))
        # The transform order above keeps scale baked; tag pcoord after it so kernels vary.
        kit.tag_coords(kernel.data)
        parts.append(kernel)


def scoop_bowl(kit, mat):
    segs, rings = 28, 8
    verts, faces = [], []
    for r in range(rings + 1):
        rr = r / rings
        for j in range(segs):
            a = math.tau * j / segs
            x = 0.105 * rr * math.cos(a)
            y = 0.070 * rr * math.sin(a)
            z = 0.012 * rr ** 1.6 - 0.020 * (1 - rr) ** 1.2
            if y > 0.030:
                z += 0.015 * (y - 0.030) / 0.040
            verts.append((x, y, z))
    for r in range(rings):
        for j in range(segs):
            faces.append((r * segs + j, r * segs + (j + 1) % segs,
                          (r + 1) * segs + (j + 1) % segs, (r + 1) * segs + j))
    obj = kit.mesh("WoodenScoopBowl", verts, faces, material=mat)
    kit.tag_coords(obj.data, [(v[1], v[2], v[0]) for v in verts])
    matx = (Matrix.Translation(Vector((-0.040, -0.080, 0.515))) @
            Euler((math.radians(18), math.radians(-9), math.radians(-12)), "XYZ").to_matrix().to_4x4())
    obj.data.transform(matx)
    return kit.recalc_normals(obj)


def build(kit):
    m = kit.mats
    hessian = m.hessian("M_SeedsmanSackHessian", base=(0.33, 0.27, 0.17),
                        dark=(0.11, 0.087, 0.052), dust=0.20, seed=310.0)
    twine = m.rawhide("M_SeedsmanSackFrayedTwine", color=(0.22, 0.17, 0.10), strands=2, twist=120.0)
    grain = grain_material(kit, "M_SeedsmanWheatGrain")
    scoop = m.aged_wood("M_SeedsmanWoodenScoop", light=(0.30, 0.19, 0.095),
                        dark=(0.10, 0.055, 0.026), roughness=0.78, saw=0.20, grime=0.23, seed=311.0)
    C.zero_subsurface(hessian, twine, grain, scoop)

    parts = [sack_body(kit, hessian), cuff(kit, hessian), grain_heap(kit, grain)]
    add_kernels(kit, parts, grain)
    parts.append(scoop_bowl(kit, scoop))
    parts.append(kit.tube("ScoopHandle", [Vector((-0.010, -0.038, 0.518)), Vector((0.060, 0.038, 0.570)),
                                          Vector((0.115, 0.105, 0.623))],
                          radius=lambda t: 0.015 * (1 - 0.28 * t), sides=18, material=scoop))
    parts.append(kit.tube("LooseFrayFront", [Vector((-0.11, -0.205, 0.478)), Vector((-0.135, -0.225, 0.420)),
                                             Vector((-0.128, -0.205, 0.370))],
                          radius=0.0016, sides=5, material=twine))
    parts.append(kit.tube("LooseFraySide", [Vector((0.155, -0.135, 0.480)), Vector((0.185, -0.150, 0.435))],
                          radius=0.0014, sides=5, material=twine))
    return kit.join(parts, "SM_SeedsmanGrainSack", unwrap=False, reshade=True, smooth_angle=62)
