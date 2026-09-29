"""Abandoned mid-19th-century Cornish/English horse swing plough, tipped onto its landside.

Research notes (written before modeling): Ransomes and other English swing ploughs of the
1830s-1850s commonly used a long wooden beam with a draught hake/clevis at the nose, two long
wooden stilt handles braced by rungs, and an iron working body: share, curved cast-iron mouldboard,
landside and a vertical knife coulter. Overall length was about 8-9 ft (2.4-2.75 m). Left outside
for two decades on a wet Cornish farm, the ash/oak beam and stilts silver and split while the cast
and wrought iron loses shine under orange-brown rust, dark scale and damp lichen. This asset is
modeled at 2.76 m long, lying tipped about 70 degrees onto its landside so the landside and handle
ends rest at z=0; nose/hitch points +X. Original procedural geometry and procedural materials only.
"""
import math
import os
import random
import sys
from pathlib import Path

from mathutils import Matrix, Vector, noise

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_materials as HM
import store.common as C

NAME = "FarmPlough"
DESCRIPTION = "Abandoned 1850s English/Cornish horse swing plough tipped onto its landside: rotten wood, cast-iron mouldboard, share, landside, knife coulter and draught clevis."
COLLISION = "convex"
TRIANGLE_BUDGET = 40000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture. Researched from period English/Ransomes-type swing plough construction."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -8), "focus": (0.12, -0.06, 0.34),
          "views": ["hero", "detail", "eye"], "detail_distance": 1.90, "eye_distance": 4.0}
REPORT = {
    "unreal_frame": "Extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "Bottom centre of the settled tipped plough at z=0.",
    "orientation": "Long beam axis runs along X with draught clevis/hitch toward +X; the plough is already tipped onto its landside for placement in grass.",
    "authored_scale_m": {"overall_length": 2.82, "tipped_width": 1.04, "tipped_height": 1.12},
}

SEED = 185104
TILT = math.radians(70.0)
_TILT_MAT = Matrix.Rotation(TILT, 4, "X")


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def T(p):
    """Canonical upright plough coordinates -> final tipped pose."""
    v = Vector(p); return _TILT_MAT @ Vector((v.x * 0.91, v.y, v.z))


def _frame(direction, roll=0.0):
    t = Vector(direction).normalized()
    seed = Vector((0, 0, 1)) if abs(t.z) < 0.92 else Vector((0, 1, 0))
    side = t.cross(seed).normalized()
    up = side.cross(t).normalized()
    if roll:
        q = Matrix.Rotation(roll, 4, t)
        side = q @ side
        up = q @ up
    return t, side, up


def _rect_section(width, depth, samples=28, seed=0):
    rng = random.Random(seed)
    hw, hd = width / 2, depth / 2
    bevel = min(width, depth) * 0.18
    pts = []
    corners = [(hw - bevel, hd - bevel, 0, math.pi / 2), (-(hw - bevel), hd - bevel, math.pi / 2, math.pi),
               (-(hw - bevel), -(hd - bevel), math.pi, math.pi * 1.5), (hw - bevel, -(hd - bevel), math.pi * 1.5, math.tau)]
    per = max(4, samples // 4)
    for cx, cz, a0, a1 in corners:
        for i in range(per):
            a = a0 + (a1 - a0) * i / per
            pts.append((cx + math.cos(a) * bevel + rng.uniform(-0.0018, 0.0018),
                        cz + math.sin(a) * bevel + rng.uniform(-0.0018, 0.0018)))
    return pts


def _polyline(points, spacing=0.045):
    source = [Vector(p) for p in points]
    out = []
    for a, b in zip(source, source[1:]):
        steps = max(2, int((b - a).length / spacing))
        for j in range(steps):
            out.append(a.lerp(b, j / steps))
    out.append(source[-1])
    return out


def beam_mesh(kit, name, points, width, depth, material, seed=0, spacing=0.05, samples=32, roll=0.0, jagged_end=None):
    rng = random.Random(seed)
    pts = _polyline(points, spacing * (1.75 if DRAFT else 1.0))
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    rows, coords = [], []
    for i, p in enumerate(pts):
        prevp, nextp = pts[max(0, i - 1)], pts[min(len(pts) - 1, i + 1)]
        t, side, up = _frame(nextp - prevp, roll + 0.035 * math.sin(i * 0.53 + seed))
        taper = 1.0 + 0.025 * math.sin(i * 0.31 + seed)
        sec = _rect_section(width * taper, depth * (1 + 0.018 * math.sin(i * 0.47)), samples, seed + i)
        row, crow = [], []
        for x, z in sec:
            groove = 0.0
            angle = math.atan2(z / max(depth, 1e-4), x / max(width, 1e-4))
            for phase, amount in ((0.15, 0.006), (2.7, 0.004), (4.8, 0.004)):
                d = abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase)))
                groove -= max(0, 1 - d / 0.09) ** 2 * amount * (0.5 + 0.5 * math.sin(lengths[i] * 11 + seed))
            end = 0.0
            if jagged_end == "last" and i == len(pts) - 1:
                end = rng.uniform(-0.035, 0.020)
            if jagged_end == "first" and i == 0:
                end = rng.uniform(-0.020, 0.035)
            co = p + t * end + side * (x + groove * math.copysign(1, x or 1)) + up * z
            row.append(tuple(T(co)))
            crow.append((x, z, lengths[i]))
        rows.append(row); coords.append(crow)
    obj = kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)
    total = max(lengths[-1], 1e-6)

    def disp(_co, pco):
        fibres = math.sin(pco.z * 74.0 + seed) * 0.0011
        checks = 0.0
        angle = math.atan2(pco.y / max(depth, 1e-4), pco.x / max(width, 1e-4))
        for phase in (0.15, 2.7, 4.8):
            d = abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase)))
            checks -= max(0, 1 - d / 0.07) ** 2 * 0.0035 * (0.3 + 0.7 * math.sin(pco.z * 8.5 + seed))
        endrot = max(0.0, 0.10 - min(pco.z / total, 1 - pco.z / total)) * 0.010
        return fibres + checks + endrot * math.sin((pco.x + pco.y) * 120 + seed)
    kit.displace(obj, disp)
    return obj


def tubeT(kit, name, points, radii, sides, mat):
    return kit.tube(name, [T(p) for p in points], radii=radii, sides=sides, material=mat)


def cylinder_between(kit, name, a, b, radius, mat, sides=12, bevel=0.001):
    a, b = Vector(a), Vector(b)
    length = (b - a).length
    obj = kit.cylinder(name, radius, length, location=tuple((a + b) * 0.5), material=mat, sides=sides, bevel=bevel, bevel_segments=2)
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return obj


def ring_mesh(kit, name, center, axis1, axis2, radius, tube_radius, mat, sides=64, tube=10, broken=False, seed=0):
    rng = random.Random(seed)
    c = Vector(center); a1 = Vector(axis1).normalized(); a2 = Vector(axis2).normalized()
    verts, faces, coords = [], [], []
    segs = sides if not DRAFT else max(24, sides // 2)
    for i in range(segs):
        theta = 2 * math.pi * i / segs
        if broken and 0.20 * math.tau < theta < 0.33 * math.tau:
            # still build a thinner rusted ghost of the missing segment rather than a perfect new ring
            pass
        radial = math.cos(theta) * a1 + math.sin(theta) * a2
        tangent = (-math.sin(theta) * a1 + math.cos(theta) * a2).normalized()
        rlocal = tube_radius * (0.75 if (broken and 0.20 * math.tau < theta < 0.33 * math.tau) else 1.0)
        for j in range(tube):
            phi = 2 * math.pi * j / tube
            p = c + radial * radius + radial * (math.cos(phi) * rlocal) + tangent * (math.sin(phi) * rlocal)
            verts.append(tuple(p)); coords.append((math.cos(phi) * rlocal, math.sin(phi) * rlocal, theta * radius))
    def idx(i, j): return (i % segs) * tube + (j % tube)
    for i in range(segs):
        for j in range(tube):
            faces.append((idx(i, j), idx(i + 1, j), idx(i + 1, j + 1), idx(i, j + 1)))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def rusted_iron(kit, name, seed=0.0, bright=False):
    g = HM.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.17, seed * 0.61))
    big = g.noise(seeded, scale=8.0, detail=5.0, roughness=0.62).outputs["Fac"]
    fine = g.noise(seeded, scale=75.0, detail=6.0, roughness=0.72).outputs["Fac"]
    cells = g.voronoi(seeded, scale=55.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    pits = g.remap(cells, 0.0, 0.020, 1.0, 0.0)
    flake = g.remap(fine, 0.42, 0.67)
    orange = g.remap(big, 0.44, 0.68, 0.0, 1.0)
    dark_scale = g.mix((0.022, 0.020, 0.018), (0.065, 0.052, 0.040), flake)
    rust = g.mix((0.070, 0.031, 0.014), (0.180, 0.070, 0.022), orange)
    color = g.mix(dark_scale, rust, g.remap(fine, 0.48, 0.58, 0.15, 0.82))
    damp = g.noise(g.vmath("ADD", seeded, (3.0, 1.0, 7.0)), scale=17.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.030, 0.041, 0.027), g.remap(damp, 0.64, 0.75, 0.0, 0.22))
    if bright:
        rub = g.remap(fine, 0.77, 0.83, 0.0, 0.22)
        color = g.mix(color, (0.26, 0.24, 0.21), rub)
    g.set("Base Color", color)
    g.set("Roughness", g.remap(fine, 0.2, 0.8, 0.82, 0.98))
    g.set("Metallic", g.remap(flake, 0.55, 1.0, 0.02, 0.18 if bright else 0.08))
    height = g.math("ADD", g.math("MULTIPLY", fine, 0.35), g.math("MULTIPLY", big, 0.45))
    height = g.math("SUBTRACT", height, g.math("MULTIPLY", pits, 0.70))
    g.set("Normal", g.bump(height, strength=0.75, distance=0.0028))
    return g.mat


def lichen_material(kit):
    green = kit.material("M_FarmPloughCrustLichenGreyGreen", (0.115, 0.135, 0.088), roughness=0.97)
    ochre = kit.material("M_FarmPloughCrustLichenOchre", (0.165, 0.135, 0.060), roughness=0.98)
    return green, ochre


def weathered_wood(name, seed=0.0, darker=False):
    g = HM.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.31, seed * 0.17, seed * 0.53))
    long_vec = g.combine(g.math("ADD", g.math("MULTIPLY", x, 26.0), g.math("MULTIPLY", g.noise(seeded, scale=4.0, detail=3.0).outputs["Fac"], 1.8)),
                         g.math("MULTIPLY", y, 26.0),
                         g.math("ADD", g.math("MULTIPLY", z, 1.15), seed))
    fibre = g.noise(long_vec, scale=7.0, detail=10.0, roughness=0.62).outputs["Fac"]
    streak = g.noise(g.combine(g.math("MULTIPLY", x, 10.0), g.math("MULTIPLY", y, 10.0),
                               g.math("ADD", g.math("MULTIPLY", z, 0.55), seed)), scale=10.0, detail=5.0).outputs["Fac"]
    checks = g.noise(g.combine(g.math("MULTIPLY", x, 9.0), g.math("MULTIPLY", y, 9.0),
                                 g.math("MULTIPLY", z, 0.34)), scale=14.0, detail=6.0, roughness=0.68).outputs["Fac"]
    crack = g.remap(checks, 0.70, 0.83, 0.0, 0.55)
    if darker:
        dark, mid, light = (0.026, 0.023, 0.019), (0.078, 0.069, 0.055), (0.125, 0.110, 0.088)
    else:
        dark, mid, light = (0.045, 0.042, 0.035), (0.148, 0.138, 0.116), (0.225, 0.210, 0.178)
    tone = g.math("ADD", g.math("MULTIPLY", fibre, 0.68), g.math("MULTIPLY", streak, 0.28))
    color = g.ramp(tone, [(0.24, dark), (0.56, mid), (0.88, light)])
    wet = g.noise(g.vmath("ADD", seeded, (7.0, 3.0, 1.0)), scale=18.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.032, 0.044, 0.034), g.remap(wet, 0.66, 0.76, 0.0, 0.24 if not darker else 0.34))
    color = g.mix(color, (0.014, 0.012, 0.010), crack)
    g.set("Base Color", color)
    g.set("Roughness", g.remap(streak, 0.3, 0.7, 0.88, 0.97))
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fibre, 0.34), g.math("MULTIPLY", streak, 0.10)),
                    g.math("MULTIPLY", crack, 0.85))
    g.set("Normal", g.bump(height, strength=0.42, distance=0.0012))
    return g.mat


def rusted_iron(kit, name, seed=0.0, bright=False):
    g = HM.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.37, seed * 0.17, seed * 0.61))
    broad = g.noise(seeded, scale=6.0, detail=5.0, roughness=0.62).outputs["Fac"]
    mid = g.noise(g.vmath("ADD", seeded, (2.1, 4.0, 1.7)), scale=24.0, detail=6.0, roughness=0.70).outputs["Fac"]
    fine = g.noise(g.vmath("ADD", seeded, (5.0, 1.0, 3.2)), scale=145.0, detail=4.0, roughness=0.65).outputs["Fac"]
    pit = g.remap(fine, 0.66, 0.79, 0.0, 1.0)
    scale = g.remap(mid, 0.55, 0.72, 0.0, 1.0)
    rust = g.mix((0.070, 0.032, 0.014), (0.185, 0.075, 0.022), g.remap(broad, 0.40, 0.68))
    dark = g.mix((0.020, 0.019, 0.018), (0.070, 0.058, 0.045), g.remap(mid, 0.34, 0.62))
    color = g.mix(rust, dark, g.remap(scale, 0.30, 0.90, 0.10, 0.62))
    color = g.mix(color, (0.010, 0.009, 0.008), g.math("MULTIPLY", pit, 0.55))
    if bright:
        rub = g.remap(fine, 0.82, 0.90, 0.0, 0.14)
        color = g.mix(color, (0.30, 0.28, 0.23), rub)
    g.set("Base Color", color)
    g.set("Roughness", g.remap(fine, 0.25, 0.8, 0.84, 0.98))
    g.set("Metallic", g.remap(scale, 0.55, 1.0, 0.015, 0.10 if not bright else 0.16))
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", broad, 0.25), g.math("MULTIPLY", mid, 0.45)),
                    g.math("MULTIPLY", pit, 0.62))
    g.set("Normal", g.bump(height, strength=0.62, distance=0.0022))
    return g.mat


def mouldboard(kit, mat):
    rows_front, rows_back, coords_front, coords_back = [], [], [], []
    sx, sy = (18 if not DRAFT else 11), (10 if not DRAFT else 7)
    for ix in range(sx + 1):
        u = ix / sx
        x = -0.28 + 0.86 * u
        rowf, rowb, cf, cb = [], [], [], []
        for iy in range(sy + 1):
            v = iy / sy
            sweep = v ** 1.25
            y = -0.035 + 0.43 * sweep + 0.035 * math.sin(u * math.pi) * v
            z = 0.105 + 0.23 * u + 0.060 * math.sin(v * math.pi) + 0.035 * v * u
            # Rolled mouldboard: upper outer lip curls over.
            y += 0.045 * smoothstep(0.55, 1.0, v) * math.sin(u * math.pi * 0.85)
            z += 0.055 * smoothstep(0.55, 1.0, v) * (1 - 0.25 * u)
            p = Vector((x, y, z))
            rowf.append(tuple(T(p + Vector((0, -0.006, 0)))))
            rowb.append(tuple(T(p + Vector((0, 0.006, 0)))))
            cf.append((-0.006, v * 0.42, x)); cb.append((0.006, v * 0.42, x))
        rows_front.append(rowf); rows_back.append(rowb); coords_front.append(cf); coords_back.append(cb)
    rows = rows_front + list(reversed(rows_back))
    coords = coords_front + list(reversed(coords_back))
    obj = kit.loft("CurvedCastIronMouldboard", rows, material=mat, coords=coords, cap_start=True, cap_end=True)
    kit.roughen(obj, strength=0.0014, scale=45.0, seed=44, subdivide=0)
    return obj


def thick_plate(kit, name, points, thickness, mat, seed=0):
    # points in canonical order, thickness approximately in canonical Y.
    front = [tuple(T(Vector(p) + Vector((0, -thickness * 0.5, 0)))) for p in points]
    back = [tuple(T(Vector(p) + Vector((0, thickness * 0.5, 0)))) for p in points]
    verts = front + back
    n = len(points)
    faces = [tuple(range(n)), tuple(range(2 * n - 1, n - 1, -1))]
    for i in range(n):
        faces.append((i, (i + 1) % n, n + (i + 1) % n, n + i))
    coords = [(-thickness * 0.5, i / n, p[0]) for i, p in enumerate(points)] + [(thickness * 0.5, i / n, p[0]) for i, p in enumerate(points)]
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    kit.roughen(obj, strength=0.0012, scale=65.0, seed=seed, subdivide=1)
    return kit.recalc_normals(obj)


def mouldboard(kit, mat):
    rows_front, rows_back, coords_front, coords_back = [], [], [], []
    sx, sy = (26 if not DRAFT else 14), (16 if not DRAFT else 9)
    for ix in range(sx + 1):
        u = ix / sx
        x = -0.42 + 0.92 * u
        rowf, rowb, cf, cb = [], [], [], []
        for iy in range(sy + 1):
            v = iy / sy
            # Helicoid mouldboard: lower front starts by the share, the rear/top curls up and out.
            y = -0.020 + (0.070 + 0.315 * (1.0 - u) ** 0.75) * v
            y += 0.060 * math.sin(math.pi * u) * v + 0.075 * smoothstep(0.62, 1.0, v) * (1.0 - 0.25 * u)
            z = 0.070 + 0.080 * u + (0.080 + 0.310 * (1.0 - 0.35 * u)) * v
            z += 0.055 * math.sin(math.pi * v) * (1.0 - 0.20 * u)
            # The lip rolls over, so at eye level the concave rusted working face is obvious.
            z += 0.035 * smoothstep(0.72, 1.0, v)
            p = Vector((x, y, z))
            rowf.append(tuple(T(p + Vector((0, -0.007, 0)))))
            rowb.append(tuple(T(p + Vector((0, 0.007, 0)))))
            cf.append((-0.007, v * 0.50, x)); cb.append((0.007, v * 0.50, x))
        rows_front.append(rowf); rows_back.append(rowb); coords_front.append(cf); coords_back.append(cb)
    obj = kit.loft("HelicoidCastIronMouldboard", rows_front + list(reversed(rows_back)),
                   material=mat, coords=coords_front + list(reversed(coords_back)), cap_start=True, cap_end=True)
    kit.roughen(obj, strength=0.0010, scale=38.0, seed=144, subdivide=0)
    return obj


def crust_patch(kit, name, center, normal, tangent, radius, mat, seed):
    rng = random.Random(seed)
    c = T(center)
    n = (_TILT_MAT @ Vector(normal)).normalized()
    t = (_TILT_MAT @ Vector(tangent)).normalized()
    b = n.cross(t).normalized()
    sides = rng.randint(7, 11)
    verts = [tuple(c + n * 0.003)]
    coords = [(0, 0, 0)]
    for i in range(sides):
        a = math.tau * i / sides + rng.uniform(-0.12, 0.12)
        r = radius * rng.uniform(0.45, 1.15)
        verts.append(tuple(c + t * (math.cos(a) * r) + b * (math.sin(a) * r * rng.uniform(0.45, 0.95)) + n * rng.uniform(0.002, 0.004)))
        coords.append((math.cos(a) * r, math.sin(a) * r, 0.0))
    faces = []
    for i in range(1, sides + 1):
        faces.append((0, i, 1 + (i % sides)))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def rivet(kit, name, p, mat, r=0.012):
    return kit.sphere(name, r, location=tuple(T(p)), material=mat, segments=12, rings=6, scale=(1.0, 1.0, 0.55))


def splinter(kit, name, base, direction, mat, seed):
    rng = random.Random(seed)
    b = Vector(base); d = Vector(direction).normalized()
    side = d.cross(Vector((0, 0, 1))).normalized() if abs(d.z) < 0.92 else Vector((0, 1, 0))
    pts = [b, b + d * rng.uniform(0.08, 0.16) + side * rng.uniform(-0.015, 0.015),
           b + d * rng.uniform(0.18, 0.30) + side * rng.uniform(-0.025, 0.025) + Vector((0, 0, rng.uniform(-0.015, 0.015)))]
    return tubeT(kit, name, pts, [0.010, 0.006, 0.0015], 5, mat)


def settle(objects):
    lo = min((obj.matrix_world @ v.co).z for obj in objects for v in obj.data.vertices)
    for obj in objects:
        obj.location.z -= lo


def build(kit):
    rng = random.Random(SEED)
    wood = weathered_wood("M_FarmPloughRainSilveredOak", seed=51.0, darker=False)
    broken = weathered_wood("M_FarmPloughRottedBrokenWood", seed=52.0, darker=True)
    iron = rusted_iron(kit, "M_FarmPloughLayeredRustIron", seed=9.0, bright=True)
    dark_gap = kit.material("M_FarmPloughDarkRotCracks", (0.018, 0.015, 0.011), roughness=0.98)
    lichen_green, lichen_ochre = lichen_material(kit)
    C.zero_subsurface(wood, broken, iron, dark_gap, lichen_green, lichen_ochre)
    parts = []

    # Main oak beam and rear stilt block.
    beam_pts = [(1.34, 0.00, 0.72), (0.88, -0.015, 0.64), (0.32, 0.00, 0.51), (-0.35, -0.02, 0.42), (-0.62, -0.02, 0.38)]
    parts.append(beam_mesh(kit, "WeatheredMainBeam", beam_pts, 0.125, 0.105, wood, seed=10, spacing=0.045, samples=36))
    parts.append(beam_mesh(kit, "RearStiltHeadBlock", [(-0.48, -0.26, 0.33), (-0.50, 0.26, 0.33)], 0.090, 0.105, wood, seed=12, spacing=0.035, samples=28, roll=math.pi/2))

    # Two handles; the low/landside handle is snapped and frayed.
    right_handle = [(-0.44, 0.22, 0.34), (-0.82, 0.30, 0.54), (-1.18, 0.36, 0.82), (-1.53, 0.40, 0.98)]
    left_handle = [(-0.44, -0.22, 0.31), (-0.78, -0.31, 0.47), (-1.10, -0.37, 0.66), (-1.29, -0.40, 0.70)]
    parts.append(beam_mesh(kit, "HighIntactHandle", right_handle, 0.052, 0.043, wood, seed=20, spacing=0.038, samples=24))
    parts.append(beam_mesh(kit, "LowSnappedHandle", left_handle, 0.054, 0.045, broken, seed=21, spacing=0.035, samples=24, jagged_end="last"))
    for k in range(5):
        parts.append(splinter(kit, f"LowHandleSplinter_{k}", left_handle[-1], (-1.0, -0.2 + 0.1 * k, 0.14), broken, 91 + k))
    for x, z, nm in [(-0.82, 0.49, "FrontStiltRung"), (-1.13, 0.70, "RearStiltRung")]:
        parts.append(tubeT(kit, nm, [(x, -0.29, z), (x - 0.025, 0.32, z + 0.04)], [0.022, 0.020], 18, wood))

    # Iron plough body.
    parts.append(mouldboard(kit, iron))
    share_pts = [(-0.28, -0.170, 0.075), (0.76, -0.055, 0.060), (0.25, 0.205, 0.108), (-0.28, 0.170, 0.130)]
    parts.append(thick_plate(kit, "BluntedTriangularShare", share_pts, 0.020, iron, seed=31))
    parts.append(beam_mesh(kit, "GroundContactLandsideIron", [(-0.55, -0.255, 0.075), (0.86, -0.235, 0.070)],
                           0.070, 0.115, iron, seed=32, spacing=0.045, samples=22, roll=0.08))
    coulter_pts = [(0.58, -0.025, 0.16), (0.67, -0.020, 0.73), (0.73, -0.018, 0.73), (0.72, -0.010, 0.22), (0.64, -0.005, 0.12)]
    parts.append(thick_plate(kit, "KnifeCoulter", coulter_pts, 0.018, iron, seed=33))
    # Coulter clamp and stay rods.
    parts.append(tubeT(kit, "CoulterClampBand", [(0.55, -0.060, 0.57), (0.74, -0.058, 0.57)], [0.012, 0.012], 12, iron))

    # Draught hake/clevis: plates, pin and an oval ring at the nose.
    parts.append(beam_mesh(kit, "BoltedNoseIronStrapTop", [(1.02, 0.000, 0.785), (1.36, 0.000, 0.800)],
                           0.030, 0.032, iron, seed=61, spacing=0.04, samples=10))
    parts.append(tubeT(kit, "HitchForkLeft", [(1.19, -0.065, 0.715), (1.40, -0.070, 0.725)], [0.018, 0.015], 12, iron))
    parts.append(tubeT(kit, "HitchForkRight", [(1.19, 0.065, 0.715), (1.40, 0.070, 0.725)], [0.018, 0.015], 12, iron))
    parts.append(cylinder_between(kit, "HitchPinThroughClevis", T((1.37, -0.105, 0.725)), T((1.37, 0.105, 0.725)), 0.014, iron, sides=14))
    ring_center = T((1.415, 0.0, 0.725))
    parts.append(ring_mesh(kit, "DraughtClevisOvalRing", ring_center, T((1.51, 0.0, 0.80)) - ring_center,
                           T((1.51, 0.09, 0.72)) - ring_center, 0.070, 0.011, iron, sides=56, tube=8, broken=False, seed=70))

    # Bolts, washers and rust lumps where iron is fastened to wood.
    for i, p in enumerate([(0.38, -0.07, 0.50), (0.38, 0.07, 0.50), (-0.36, -0.21, 0.33), (-0.36, 0.21, 0.34),
                           (0.05, 0.19, 0.24), (0.52, 0.20, 0.31), (0.61, -0.03, 0.50), (1.14, -0.055, 0.705),
                           (1.14, 0.055, 0.705)]):
        parts.append(rivet(kit, f"PeenedBolt_{i}", p, iron, r=0.011 + 0.002 * (i % 2)))

    # Sparse crustose lichen on upward, damp wood faces; irregular patches, not discs.
    for i in range(14 if not DRAFT else 6):
        x = rng.uniform(-1.15, 1.05)
        y = rng.choice([-1, 1]) * rng.uniform(0.025, 0.054)
        z = rng.uniform(0.43, 0.74)
        mat = lichen_ochre if rng.random() < 0.35 else lichen_green
        parts.append(crust_patch(kit, f"IrregularCrustoseLichen_{i}", (x, y, z), (0, 0, 1), (1, 0, 0),
                                 rng.uniform(0.018, 0.044), mat, 210 + i))

    settle(parts)
    obj = kit.join(parts, "SM_FarmPlough", unwrap=False, reshade=True, smooth_angle=52)
    return obj



