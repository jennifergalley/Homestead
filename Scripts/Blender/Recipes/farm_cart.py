"""Abandoned early-Victorian English/Cornish two-wheeled farm tip cart (butt/tumbril).

Research notes (written before modeling): small 1830s-1840s British farm carts used an oak/elm plank
body about 7-8 ft long by 4-5 ft wide, removable/hung side boards and raves, long twin shafts for a
horse, and two large iron-tyred spoked wheels. Elm hubs (naves), oak spokes and curved oak felloes
carried a wrought iron tyre. This cart has been abandoned for about twenty wet Cornish years: the
nearside wheel has collapsed and lies flat under the listed body, side boards are missing or hanging,
shaft tips rest on the ground, iron tyres/straps are rusted, and the oak/elm is rain-silvered with
lichen and mossy damp stains. No weeds are modeled; Unreal supplies the overgrowth. Original
procedural geometry/materials only. Shafts point +X; overall length is about 4.3 m.
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

NAME = "FarmCart"
DESCRIPTION = "Abandoned 1830s-40s Cornish/English two-wheeled farm tip cart with collapsed nearside wheel, listed boarded body, long shafts and rusted iron tyres."
COLLISION = "convex"
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture. Researched from period English/Cornish butt/tumbril farm cart construction."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, -12), "focus": (0.75, -0.45, 1.45),
          "views": ["hero", "detail", "eye"], "detail_distance": 2.05, "eye_distance": 4.4}
REPORT = {
    "unreal_frame": "Extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "Bottom centre of the settled cart at z=0.",
    "orientation": "Long axis and horse shafts run along +X; rear/tail is -X. Nearside (-Y in Blender, +Y after Unreal mirror) is collapsed.",
    "authored_scale_m": {"overall_length_including_shafts": 4.3, "body_length": 2.4, "body_width": 1.4, "wheel_diameter": 1.34},
}

SEED = 184012
ROLL = math.radians(20.0)      # listed down toward canonical -Y (nearside)
PITCH = math.radians(8.5)      # shaft tips down, tail up
_TMAT = Matrix.Rotation(ROLL, 4, "X") @ Matrix.Rotation(PITCH, 4, "Y")


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def T(p):
    return _TMAT @ Vector(p)


def rusted_iron(kit, name, seed=0.0):
    g = HM.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.29, seed * 0.53, seed * 0.13))
    big = g.noise(seeded, scale=7.0, detail=5.0, roughness=0.62).outputs["Fac"]
    fine = g.noise(seeded, scale=85.0, detail=6.0, roughness=0.72).outputs["Fac"]
    cells = g.voronoi(seeded, scale=62.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    pits = g.remap(cells, 0.0, 0.018, 1.0, 0.0)
    scale = g.remap(fine, 0.39, 0.62)
    orange = g.remap(big, 0.43, 0.67, 0.0, 1.0)
    color = g.mix((0.024, 0.021, 0.018), (0.075, 0.052, 0.032), scale)
    color = g.mix(color, g.mix((0.065, 0.030, 0.014), (0.170, 0.066, 0.020), orange), g.remap(fine, 0.47, 0.58, 0.20, 0.86))
    damp = g.noise(g.vmath("ADD", seeded, (5.0, 2.0, 8.0)), scale=18.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.030, 0.042, 0.028), g.remap(damp, 0.64, 0.75, 0.0, 0.20))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(fine, 0.2, 0.8, 0.84, 0.98))
    g.set("Metallic", g.remap(scale, 0.55, 1.0, 0.02, 0.10))
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fine, 0.38), g.math("MULTIPLY", big, 0.42)),
                    g.math("MULTIPLY", pits, 0.75))
    g.set("Normal", g.bump(height, strength=0.75, distance=0.0026))
    return g.mat


def lichen_material(kit):
    green = kit.material("M_FarmCartCrustLichenGreyGreen", (0.112, 0.135, 0.086), roughness=0.98)
    ochre = kit.material("M_FarmCartCrustLichenYellowOchre", (0.170, 0.138, 0.058), roughness=0.98)
    return green, ochre


def weathered_wood(name, seed=0.0, darker=False):
    g = HM.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    seeded = g.vmath("ADD", p, (seed * 0.31, seed * 0.17, seed * 0.53))
    long_vec = g.combine(g.math("ADD", g.math("MULTIPLY", x, 26.0), g.math("MULTIPLY", g.noise(seeded, scale=4.0, detail=3.0).outputs["Fac"], 1.8)),
                         g.math("MULTIPLY", y, 26.0),
                         g.math("ADD", g.math("MULTIPLY", z, 1.05), seed))
    fibre = g.noise(long_vec, scale=7.0, detail=10.0, roughness=0.62).outputs["Fac"]
    streak = g.noise(g.combine(g.math("MULTIPLY", x, 10.0), g.math("MULTIPLY", y, 10.0),
                               g.math("ADD", g.math("MULTIPLY", z, 0.50), seed)), scale=10.0, detail=5.0).outputs["Fac"]
    checks = g.noise(g.combine(g.math("MULTIPLY", x, 9.0), g.math("MULTIPLY", y, 9.0),
                                 g.math("MULTIPLY", z, 0.34)), scale=14.0, detail=6.0, roughness=0.68).outputs["Fac"]
    crack = g.remap(checks, 0.70, 0.83, 0.0, 0.55)
    if darker:
        dark, mid, light = (0.026, 0.023, 0.019), (0.078, 0.069, 0.055), (0.125, 0.110, 0.088)
    else:
        dark, mid, light = (0.045, 0.042, 0.035), (0.148, 0.138, 0.116), (0.225, 0.210, 0.178)
    tone = g.math("ADD", g.math("MULTIPLY", fibre, 0.68), g.math("MULTIPLY", streak, 0.28))
    color = g.ramp(tone, [(0.24, dark), (0.56, mid), (0.88, light)])
    damp = g.noise(g.vmath("ADD", seeded, (7.0, 3.0, 1.0)), scale=18.0, detail=4.0).outputs["Fac"]
    color = g.mix(color, (0.030, 0.043, 0.033), g.remap(damp, 0.65, 0.77, 0.0, 0.30 if not darker else 0.42))
    color = g.mix(color, (0.014, 0.012, 0.010), crack)
    g.set("Base Color", color)
    g.set("Roughness", g.remap(streak, 0.3, 0.7, 0.88, 0.97))
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fibre, 0.34), g.math("MULTIPLY", streak, 0.10)),
                    g.math("MULTIPLY", crack, 0.85))
    g.set("Normal", g.bump(height, strength=0.40, distance=0.00115))
    return g.mat


def rusted_iron(kit, name, seed=0.0):
    g = HM.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (seed * 0.29, seed * 0.53, seed * 0.13))
    broad = g.noise(seeded, scale=6.0, detail=5.0, roughness=0.62).outputs["Fac"]
    mid = g.noise(g.vmath("ADD", seeded, (2.1, 4.0, 1.7)), scale=24.0, detail=6.0, roughness=0.70).outputs["Fac"]
    fine = g.noise(g.vmath("ADD", seeded, (5.0, 1.0, 3.2)), scale=145.0, detail=4.0, roughness=0.65).outputs["Fac"]
    pit = g.remap(fine, 0.66, 0.79, 0.0, 1.0)
    scale = g.remap(mid, 0.55, 0.72, 0.0, 1.0)
    rust = g.mix((0.068, 0.030, 0.014), (0.175, 0.068, 0.020), g.remap(broad, 0.40, 0.68))
    dark = g.mix((0.020, 0.019, 0.018), (0.070, 0.058, 0.045), g.remap(mid, 0.34, 0.62))
    color = g.mix(rust, dark, g.remap(scale, 0.30, 0.90, 0.12, 0.65))
    color = g.mix(color, (0.010, 0.009, 0.008), g.math("MULTIPLY", pit, 0.55))
    g.set("Base Color", color)
    g.set("Roughness", g.remap(fine, 0.25, 0.8, 0.84, 0.98))
    g.set("Metallic", g.remap(scale, 0.55, 1.0, 0.015, 0.10))
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", broad, 0.25), g.math("MULTIPLY", mid, 0.45)),
                    g.math("MULTIPLY", pit, 0.62))
    g.set("Normal", g.bump(height, strength=0.62, distance=0.0022))
    return g.mat


def _make_box_mesh(kit, name, center, size, axes, mat, transform=T, seed=0, rough=True):
    cx, cy, cz = [Vector(a).normalized() for a in axes]
    sx, sy, sz = size
    c = Vector(center)
    corners = []
    coords = []
    for ix in (-1, 1):
        for iy in (-1, 1):
            for iz in (-1, 1):
                local = cx * (ix * sx / 2) + cy * (iy * sy / 2) + cz * (iz * sz / 2)
                p = c + local
                corners.append(tuple(transform(p)))
                lx, ly, lz = ix * sx / 2, iy * sy / 2, iz * sz / 2
                longest = max(range(3), key=lambda k: (sx, sy, sz)[k])
                if longest == 0:
                    coords.append((ly, lz, lx))
                elif longest == 1:
                    coords.append((lx, lz, ly))
                else:
                    coords.append((lx, ly, lz))
    def idc(ix, iy, iz):
        return ((0 if ix < 0 else 1) * 4 + (0 if iy < 0 else 1) * 2 + (0 if iz < 0 else 1))
    faces = [
        (idc(-1,-1,-1), idc(-1,1,-1), idc(-1,1,1), idc(-1,-1,1)),
        (idc(1,-1,-1), idc(1,-1,1), idc(1,1,1), idc(1,1,-1)),
        (idc(-1,-1,-1), idc(1,-1,-1), idc(1,-1,1), idc(-1,-1,1)),
        (idc(-1,1,-1), idc(-1,1,1), idc(1,1,1), idc(1,1,-1)),
        (idc(-1,-1,-1), idc(-1,1,-1), idc(1,1,-1), idc(1,-1,-1)),
        (idc(-1,-1,1), idc(1,-1,1), idc(1,1,1), idc(-1,1,1)),
    ]
    obj = kit.mesh(name, corners, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    if rough:
        kit.roughen(obj, strength=0.0016, scale=38.0, seed=seed, subdivide=1)
    return kit.recalc_normals(obj)


def board(kit, name, center, size, mat, transform=T, axes=((1,0,0),(0,1,0),(0,0,1)), seed=0):
    return _make_box_mesh(kit, name, center, size, axes, mat, transform=transform, seed=seed, rough=True)


def cylinder_between(kit, name, a, b, radius, mat, sides=14, bevel=0.001):
    a, b = Vector(a), Vector(b)
    obj = kit.cylinder(name, radius, (b - a).length, location=tuple((a + b) * 0.5), material=mat, sides=sides, bevel=bevel, bevel_segments=2)
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return obj


def tubeT(kit, name, pts, radii, sides, mat):
    return kit.tube(name, [T(p) for p in pts], radii=radii, sides=sides, material=mat)


def ring_rect(kit, name, center, ax1, ax2, width_axis, radius, radial, width, mat, transform=lambda p: p, sides=96, wobble=0.0, seed=0):
    rng = random.Random(seed)
    c = Vector(center); a1 = Vector(ax1).normalized(); a2 = Vector(ax2).normalized(); wv = Vector(width_axis).normalized()
    segs = sides if not DRAFT else max(32, sides // 2)
    verts, faces, coords = [], [], []
    for ir, r in enumerate((radius - radial / 2, radius + radial / 2)):
        for iw, ww in enumerate((-width / 2, width / 2)):
            for i in range(segs):
                a = math.tau * i / segs
                rr = r + wobble * noise.noise(Vector((math.cos(a) * 2.7 + seed, math.sin(a) * 2.7, seed * 0.1)))
                p = c + a1 * (math.cos(a) * rr) + a2 * (math.sin(a) * rr) + wv * ww
                verts.append(tuple(transform(p)))
                coords.append((math.cos(a) * rr, math.sin(a) * rr, ww))
    def idx(ir, iw, i): return ir * 2 * segs + iw * segs + (i % segs)
    for i in range(segs):
        faces.append((idx(1,0,i), idx(1,0,i+1), idx(1,1,i+1), idx(1,1,i)))
        faces.append((idx(0,0,i+1), idx(0,0,i), idx(0,1,i), idx(0,1,i+1)))
        faces.append((idx(0,1,i), idx(1,1,i), idx(1,1,i+1), idx(0,1,i+1)))
        faces.append((idx(0,0,i+1), idx(1,0,i+1), idx(1,0,i), idx(0,0,i)))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def ring_arc_rect(kit, name, center, ax1, ax2, width_axis, radius, radial, width, mat,
                  a0, a1, transform=lambda p: p, sides=20, wobble=0.0, seed=0):
    c = Vector(center); a1v = Vector(ax1).normalized(); a2v = Vector(ax2).normalized(); wv = Vector(width_axis).normalized()
    segs = max(4, sides)
    verts, faces, coords = [], [], []
    for ir, r in enumerate((radius - radial / 2, radius + radial / 2)):
        for iw, ww in enumerate((-width / 2, width / 2)):
            for i in range(segs + 1):
                a = a0 + (a1 - a0) * i / segs
                rr = r + wobble * noise.noise(Vector((math.cos(a) * 2.7 + seed, math.sin(a) * 2.7, seed * 0.1)))
                p = c + a1v * (math.cos(a) * rr) + a2v * (math.sin(a) * rr) + wv * ww
                verts.append(tuple(transform(p)))
                coords.append((math.cos(a) * rr, math.sin(a) * rr, ww))
    stride = segs + 1
    def idx(ir, iw, i): return ir * 2 * stride + iw * stride + i
    for i in range(segs):
        faces.append((idx(1,0,i), idx(1,0,i+1), idx(1,1,i+1), idx(1,1,i)))
        faces.append((idx(0,0,i+1), idx(0,0,i), idx(0,1,i), idx(0,1,i+1)))
        faces.append((idx(0,1,i), idx(1,1,i), idx(1,1,i+1), idx(0,1,i+1)))
        faces.append((idx(0,0,i+1), idx(1,0,i+1), idx(1,0,i), idx(0,0,i)))
    for end in (0, segs):
        faces.append((idx(0,0,end), idx(1,0,end), idx(1,1,end), idx(0,1,end)))
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def crust_patch(kit, name, center, normal, tangent, radius, mat, seed, transform=T):
    rng = random.Random(seed)
    c = transform(center)
    n = (transform(Vector(center) + Vector(normal)) - transform(center)).normalized()
    t = (transform(Vector(center) + Vector(tangent)) - transform(center)).normalized()
    b = n.cross(t).normalized()
    sides = rng.randint(7, 12)
    verts = [tuple(c + n * 0.003)]
    coords = [(0, 0, 0)]
    for i in range(sides):
        a = math.tau * i / sides + rng.uniform(-0.14, 0.14)
        r = radius * rng.uniform(0.45, 1.18)
        verts.append(tuple(c + t * (math.cos(a) * r) + b * (math.sin(a) * r * rng.uniform(0.42, 0.95)) + n * rng.uniform(0.002, 0.004)))
        coords.append((math.cos(a) * r, math.sin(a) * r, 0.0))
    faces = [(0, i, 1 + (i % sides)) for i in range(1, sides + 1)]
    obj = kit.mesh(name, verts, faces, material=mat)
    kit.tag_coords(obj.data, coords)
    return kit.recalc_normals(obj)


def spoke(kit, name, a, b, radius, mat, broken=False, seed=0):
    if broken:
        rng = random.Random(seed)
        a = Vector(a); b = Vector(b)
        t = rng.uniform(0.42, 0.76)
        mid = a.lerp(b, t)
        tip = mid + (b - a).normalized() * rng.uniform(0.03, 0.09) + Vector((rng.uniform(-0.02, 0.02), rng.uniform(-0.02, 0.02), rng.uniform(-0.01, 0.03)))
        return kit.tube(name, [a, mid, tip], radii=[radius, radius * 0.7, 0.002], sides=8, material=mat)
    return cylinder_between(kit, name, a, b, radius, mat, sides=10, bevel=0.001)


def upright_wheel(kit, prefix, center, side_y, wood, hubmat, iron, broken=False):
    parts = []
    c = Vector(center)
    # Wheel plane X/Z, axle width along Y. Canonical points transformed with the cart body.
    transform = T
    yaxis = Vector((0, 1 if side_y > 0 else -1, 0))
    parts.append(ring_rect(kit, f"{prefix}_OakFelloes", c, (1,0,0), (0,0,1), yaxis, 0.610, 0.095, 0.105, wood, transform=transform, sides=96, wobble=0.006, seed=11))
    parts.append(ring_rect(kit, f"{prefix}_RustedIronTyre", c, (1,0,0), (0,0,1), yaxis, 0.670, 0.040, 0.082, iron, transform=transform, sides=112, wobble=0.003, seed=12))
    hub_a = transform(c - yaxis * 0.155); hub_b = transform(c + yaxis * 0.155)
    parts.append(cylinder_between(kit, f"{prefix}_ElmNaveHub", hub_a, hub_b, 0.105, hubmat, sides=32, bevel=0.006))
    parts.append(cylinder_between(kit, f"{prefix}_IronHubBandA", transform(c - yaxis * 0.120), transform(c - yaxis * 0.084), 0.111, iron, sides=32, bevel=0.002))
    parts.append(cylinder_between(kit, f"{prefix}_IronHubBandB", transform(c + yaxis * 0.084), transform(c + yaxis * 0.120), 0.111, iron, sides=32, bevel=0.002))
    for i in range(14):
        a = math.tau * i / 14
        root = c + Vector((math.cos(a) * 0.115, 0, math.sin(a) * 0.115))
        rim = c + Vector((math.cos(a) * 0.560, 0, math.sin(a) * 0.560))
        if broken and i in (1, 2, 8):
            parts.append(spoke(kit, f"{prefix}_BrokenSpoke_{i}", transform(root), transform(rim), 0.018, wood, broken=True, seed=70+i))
        else:
            parts.append(spoke(kit, f"{prefix}_OakSpoke_{i}", transform(root), transform(rim), 0.017, wood))
    return parts


def collapsed_wheel(kit, wood, hubmat, iron):
    # Final-space collapsed near wheel: plane nearly horizontal, tucked under the low side of the body.
    parts = []
    c = Vector((-0.02, -0.63, -0.055))
    ax1 = Vector((1.0, 0.10, 0.02)).normalized()
    ax2 = Vector((-0.08, 0.98, 0.08)).normalized()
    width = ax1.cross(ax2).normalized()
    xf = lambda p: Vector(p)
    arcs = [(-0.10, 0.72), (0.92, 1.62), (1.93, 2.70), (3.05, 3.78), (4.20, 5.15), (5.45, 6.05)]
    for j, (a0, a1) in enumerate(arcs):
        parts.append(ring_arc_rect(kit, f"NearsideCollapsed_SeparateFelloe_{j}", c, ax1, ax2, width, 0.610, 0.095, 0.105,
                                   wood, a0, a1, transform=xf, sides=18 if not DRAFT else 9, wobble=0.010, seed=40 + j))
    for j, (a0, a1) in enumerate([(-0.04, 0.60), (1.05, 1.48), (2.02, 2.58), (3.22, 3.70), (4.38, 5.00), (5.60, 6.00)]):
        parts.append(ring_arc_rect(kit, f"NearsideCollapsed_BrokenIronTyre_{j}", c + width * 0.010, ax1, ax2, width, 0.670, 0.040, 0.078,
                                   iron, a0, a1, transform=xf, sides=18 if not DRAFT else 8, wobble=0.004, seed=61 + j))
    parts.append(cylinder_between(kit, "NearsideCollapsed_ElmHub", c - width * 0.150, c + width * 0.150, 0.103, hubmat, sides=32, bevel=0.006))
    rng = random.Random(SEED + 99)
    for i in range(14):
        a = math.tau * i / 14
        root = c + (math.cos(a) * ax1 + math.sin(a) * ax2) * 0.115
        rim = c + (math.cos(a) * ax1 + math.sin(a) * ax2) * 0.560
        if i in (0, 1, 3, 5, 7, 9, 11, 13):
            parts.append(spoke(kit, f"NearsideCollapsed_SnappedSpoke_{i}", root, rim, 0.017, wood, broken=True, seed=90 + i))
        elif rng.random() < 0.25:
            # Missing spoke: leave the empty mortice visible with two short stubs.
            mid = root.lerp(rim, 0.23)
            parts.append(spoke(kit, f"NearsideCollapsed_SpokeStub_{i}", root, mid, 0.016, wood))
        else:
            parts.append(spoke(kit, f"NearsideCollapsed_OakSpoke_{i}", root, rim, 0.016, wood))
    # Loose broken felloe segment lying beside it.
    for j, a0 in enumerate((0.15, 0.68)):
        pts = []
        for k in range(6):
            a = a0 + 0.30 * k / 5
            pts.append(c + (math.cos(a) * ax1 + math.sin(a) * ax2) * 0.62 + width * 0.07)
        parts.append(kit.tube(f"NearsideLooseFelloeSegment_{j}", pts, radii=[0.030,0.033,0.035,0.033,0.028,0.014], sides=10, material=wood))
    return parts


def rust_streak(kit, name, x, y, z_top, length, mat, side=1):
    # A thin irregular vertical stain/decal on a side board in canonical space.
    w = 0.018
    pts = [(x - w, y, z_top), (x + w, y, z_top - 0.012), (x + w * 0.55, y, z_top - length), (x - w * 0.65, y, z_top - length * 0.92)]
    verts = [tuple(T(p)) for p in pts]
    obj = kit.mesh(name, verts, [(0,1,2,3)], material=mat)
    return obj


def settle(objects):
    lo = min((obj.matrix_world @ v.co).z for obj in objects for v in obj.data.vertices)
    for obj in objects:
        obj.location.z -= lo


def build(kit):
    rng = random.Random(SEED)
    wood = weathered_wood("M_FarmCartRainSilveredOakElm", seed=61.0, darker=False)
    darkwood = weathered_wood("M_FarmCartRottedDarkEndgrain", seed=62.0, darker=True)
    iron = rusted_iron(kit, "M_FarmCartRustedIron", seed=13.0)
    lichen_green, lichen_ochre = lichen_material(kit)
    darkgap = kit.material("M_FarmCartBlackBoardGaps", (0.014, 0.012, 0.010), roughness=0.98)
    C.zero_subsurface(wood, darkwood, iron, lichen_green, lichen_ochre, darkgap)
    parts = []

    # Floor: individual planks with uneven gaps and lifted corners.
    y0 = -0.54
    for i in range(7):
        y = y0 + i * 0.18 + rng.uniform(-0.006, 0.006)
        z = 0.545 + rng.uniform(-0.006, 0.006)
        mat = darkwood if i in (0, 1) else wood
        if i == 2:
            parts.append(board(kit, f"FloorTailPlank_{i}_FrontRemnant", (0.32, y, z),
                               (1.04, 0.150, 0.050), mat, seed=10+i))
            parts.append(board(kit, f"FloorTailPlank_{i}_RearRemnant", (-0.91, y, z - 0.010),
                               (0.56, 0.145, 0.043), darkwood, seed=30+i))
            parts.append(board(kit, "JaggedDarkRotHoleUnderTailFloor", (-0.43, y, z - 0.018),
                               (0.34, 0.132, 0.020), darkgap, seed=43))
            continue
        parts.append(board(kit, f"FloorLongPlank_{i}", (0.02 + rng.uniform(-0.015, 0.015), y, z),
                           (2.34 + rng.uniform(-0.035, 0.025), 0.150, 0.050), mat, seed=10+i))
        if i < 6:
            parts.append(board(kit, f"FloorDarkGap_{i}", (0.0, y + 0.088, z + 0.004), (2.28, 0.012, 0.010), darkgap, seed=50+i))
    # Cross bearers and axle bed.
    for x in (-0.90, -0.15, 0.58, 1.03):
        parts.append(board(kit, f"UnderCrossBearer_{x:.1f}", (x, 0.0, 0.485), (0.120, 1.35, 0.090), wood, seed=70+int(x*10)))
    parts.append(tubeT(kit, "OakAxleBeam", [(0.16, -0.91, 0.455), (0.16, 0.91, 0.455)], [0.058, 0.058], 24, darkwood))
    parts.append(cylinder_between(kit, "IronAxlePin", T((0.16, -0.98, 0.455)), T((0.16, 0.98, 0.455)), 0.025, iron, sides=18))

    # Sides, stakes and raves. Nearside (-Y) is missing/hanging because the wheel collapsed there.
    side_levels = [(0.675, 0.145), (0.855, 0.145), (1.045, 0.135)]
    for j, (z, h) in enumerate(side_levels):
        parts.append(board(kit, f"OffsideSideBoard_{j}", (0.00, 0.705, z), (2.36, 0.052, h), wood, seed=100+j))
    # Nearside: top rail remains, middle gone, lower board droops outward.
    parts.append(board(kit, "NearsideLowerHangingBoard", (-0.15, -0.750, 0.655), (1.80, 0.052, 0.145), darkwood,
                       axes=((0.998,0.025,0.020), (0.0,0.88,-0.48), (-0.025,0.48,0.88)), seed=111))
    parts.append(board(kit, "NearsideRearShortBoard", (-0.83, -0.705, 0.840), (0.52, 0.052, 0.135), wood, seed=112))
    parts.append(board(kit, "NearsideTopRave", (0.08, -0.720, 1.060), (2.10, 0.045, 0.060), wood, seed=113))
    for x in (-1.10, -0.52, 0.10, 0.72, 1.12):
        parts.append(board(kit, f"OffsideUprightStake_{x:.2f}", (x, 0.755, 0.875), (0.065, 0.070, 0.620), darkwood if x < -1 else wood, seed=130+int((x+2)*20)))
    for x in (-1.04, -0.18, 0.54, 1.06):
        parts.append(board(kit, f"NearsideBrokenStake_{x:.2f}", (x, -0.760, 0.820), (0.060, 0.062, 0.430 if x < 0.8 else 0.560), darkwood, seed=150+int((x+2)*20)))
    # Front and tail boards; rear tailgate is cracked and partly missing.
    parts.append(board(kit, "FrontBoardAgainstShafts", (1.205, 0.0, 0.775), (0.055, 1.34, 0.365), wood, seed=170))
    parts.append(board(kit, "RearTailGateLeftRemnant", (-1.195, -0.30, 0.805), (0.060, 0.52, 0.390), darkwood, seed=171))
    parts.append(board(kit, "RearTailGateRightRemnant", (-1.205, 0.39, 0.865), (0.055, 0.58, 0.300), wood, seed=172))
    # Two boards have actually come away: one leans on a nail, one lies against the collapsed wheel on the ground.
    parts.append(board(kit, "NearsideBoardHangingByOneNail", (-0.62, -0.875, 0.520), (0.94, 0.047, 0.135), darkwood,
                       axes=((0.95, 0.05, -0.30), (0.0, 0.70, -0.72), (0.31, 0.68, 0.66)), seed=188))
    parts.append(_make_box_mesh(kit, "DroppedSideBoardOnGroundAgainstWheel", (-0.46, -1.18, -0.030), (1.18, 0.155, 0.050),
                                ((0.96, 0.20, 0.0), (-0.20, 0.96, 0.04), (0.01, -0.04, 1.0)), darkwood,
                                transform=lambda p: Vector(p), seed=189))

    # Long shafts and braces; after the pitch transform their tips rest on the ground.
    for y, nm in ((-0.39, "NearShaft"), (0.39, "FarShaft")):
        pts = [(0.88, y, 0.58), (1.35, y * 0.94, 0.52), (1.86, y * 0.88, 0.44), (2.45, y * 0.78, 0.27), (3.06, y * 0.70, 0.165)]
        parts.append(tubeT(kit, nm, pts, [0.045, 0.040, 0.035, 0.031, 0.025], 22, wood))
        # scuffed ground tip
        parts.append(tubeT(kit, nm + "IronTipStrap", [(2.86, y*0.73, 0.205), (3.06, y*0.70, 0.165)], [0.030,0.027], 16, iron))
    parts.append(tubeT(kit, "ShaftCrossBar", [(1.28, -0.50, 0.555), (1.28, 0.50, 0.555)], [0.026, 0.026], 18, wood))
    for y in (-0.43, 0.43):
        parts.append(tubeT(kit, f"DiagonalShaftBrace_{y}", [(0.55, y, 0.50), (1.18, y*0.92, 0.565)], [0.018,0.015], 12, iron))

    # Wheels: offside still upright, nearside collapsed flat under the low side.
    parts.extend(upright_wheel(kit, "OffsideWheel", (0.16, 0.900, 0.475), 1, wood, darkwood, iron, broken=False))
    parts.extend(collapsed_wheel(kit, darkwood, darkwood, iron))

    # Iron straps, rivets and rust streaks from fittings.
    for x in (-0.78, 0.12, 0.82):
        parts.append(board(kit, f"OffsideVerticalIronStrap_{x}", (x, 0.737, 0.870), (0.036, 0.016, 0.530), iron, seed=210+int(x*10)))
        parts.append(board(kit, f"NearsideIronStrap_{x}", (x, -0.748, 0.820), (0.034, 0.016, 0.360), iron, seed=220+int(x*10)))
    for i in range(54 if not DRAFT else 26):
        side = 1 if i % 2 else -1
        x = rng.uniform(-1.05, 1.12)
        y = 0.735 * side
        z = rng.uniform(0.63, 1.10)
        p = T((x, y, z))
        parts.append(kit.sphere(f"PeenedRivet_{i}", rng.uniform(0.006, 0.010), location=tuple(p), material=iron, segments=8, rings=4, scale=(1.0, 1.0, 0.55)))
        if i < 20:
            parts.append(rust_streak(kit, f"RustRun_{i}", x + rng.uniform(-0.015,0.015), y + 0.006 * side, z - 0.015, rng.uniform(0.10,0.30), iron, side=side))

    # Lichen and damp moss flakes, not grass/weeds.
    for i in range(30 if not DRAFT else 10):
        x = rng.uniform(-1.10, 1.10)
        y = rng.choice([-1, 1]) * rng.uniform(0.36, 0.69)
        z = rng.choice([0.565, 1.085, 1.12]) + rng.uniform(-0.010, 0.012)
        mat = lichen_ochre if rng.random() < 0.30 else lichen_green
        parts.append(crust_patch(kit, f"IrregularCrustoseLichen_{i}", (x, y, z), (0, 0, 1), (1, 0, 0),
                                 rng.uniform(0.016, 0.046), mat, 270 + i))
    for i in range(16 if not DRAFT else 6):
        # dark cracks and missing board-splinters around the nearside lower board.
        x = rng.uniform(-1.02, 0.90)
        parts.append(tubeT(kit, f"NearsideSplinter_{i}", [(x, -0.79, 0.63 + rng.uniform(-0.04, 0.03)),
                                                           (x + rng.uniform(0.08,0.22), -0.84, 0.60 + rng.uniform(-0.05, 0.04))],
                           [rng.uniform(0.006,0.012), 0.0015], 5, darkwood))

    settle(parts)
    return kit.join(parts, "SM_FarmCart", unwrap=False, reshade=True, smooth_angle=48)



