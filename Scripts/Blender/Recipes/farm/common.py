"""Shared builders for the abandoned farm-field procedural prop recipes.

Original geometry/materials only.  Units are metres, Z up.  All materials are
opaque, high-roughness procedural PBR intended to be baked by New-Prop.ps1.
"""
import math
import random

import bpy
from mathutils import Matrix, Vector, noise


def smoothstep(a, b, x):
    if a == b:
        return 1.0 if x >= b else 0.0
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def zero_subsurface(*mats):
    for mat in mats:
        if mat and mat.node_tree:
            bsdf = next((n for n in mat.node_tree.nodes if n.bl_idname == "ShaderNodeBsdfPrincipled"), None)
            if bsdf and "Subsurface Weight" in bsdf.inputs:
                bsdf.inputs["Subsurface Weight"].default_value = 0.0


def frame(direction, roll=0.0):
    t = Vector(direction).normalized()
    seed = Vector((0, 0, 1)) if abs(t.z) < 0.92 else Vector((0, 1, 0))
    side = t.cross(seed).normalized()
    up = side.cross(t).normalized()
    if roll:
        q = Matrix.Rotation(roll, 4, t)
        side = q @ side
        up = q @ up
    return t, side, up


def _polyline(points, spacing):
    pts = [Vector(p) for p in points]
    out = []
    for a, b in zip(pts, pts[1:]):
        steps = max(2, int((b - a).length / spacing))
        for j in range(steps):
            out.append(a.lerp(b, j / steps))
    out.append(pts[-1])
    return out


def rounded_rect_section(width, depth, seed=0, samples=24, bevel=0.18):
    rng = random.Random(seed)
    hw, hd = width * 0.5, depth * 0.5
    bv = min(hw, hd) * bevel
    pts = []
    corners = [(hw - bv, hd - bv, 0, math.pi / 2),
               (-(hw - bv), hd - bv, math.pi / 2, math.pi),
               (-(hw - bv), -(hd - bv), math.pi, math.pi * 1.5),
               (hw - bv, -(hd - bv), math.pi * 1.5, math.tau)]
    per = max(3, samples // 4)
    for cx, cz, a0, a1 in corners:
        for i in range(per):
            a = a0 + (a1 - a0) * i / per
            pts.append((cx + math.cos(a) * bv + rng.uniform(-0.0025, 0.0025),
                        cz + math.sin(a) * bv + rng.uniform(-0.0025, 0.0025)))
    return pts


def cleft_section(width, depth, seed=0, samples=16, triangular=False):
    rng = random.Random(seed)
    if triangular:
        base = [(-0.52, -0.42), (0.35, -0.50), (0.58, -0.08), (0.18, 0.48), (-0.35, 0.43), (-0.58, -0.05)]
    else:
        base = [(-0.50, -0.43), (-0.15, -0.52), (0.28, -0.48), (0.54, -0.20),
                (0.48, 0.26), (0.18, 0.52), (-0.27, 0.48), (-0.55, 0.13)]
    pts = []
    for i, (x0, z0) in enumerate(base):
        x1, z1 = base[(i + 1) % len(base)]
        steps = max(1, samples // len(base))
        for j in range(steps):
            f = j / steps
            wob = rng.uniform(-0.025, 0.025)
            pts.append(((x0 + (x1 - x0) * f + wob) * width,
                        (z0 + (z1 - z0) * f + rng.uniform(-0.025, 0.025)) * depth))
    return pts


def beam(kit, name, points, width, depth, material, seed, spacing=0.075, section="rect",
         roll=0.0, taper=None, cap_start=True, cap_end=True, material_slots=None, smooth=True):
    """Irregular lofted oak member along an arbitrary path."""
    rng = random.Random(seed)
    pts = _polyline(points, spacing)
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    total = max(lengths[-1], 1e-6)
    rows, coords = [], []
    sides = 28 if section == "rect" else 18
    for i, p in enumerate(pts):
        prevp, nextp = pts[max(0, i - 1)], pts[min(len(pts) - 1, i + 1)]
        twist = roll + 0.10 * math.sin(lengths[i] * 2.7 + seed) + 0.035 * math.sin(lengths[i] * 11.0)
        t, side_v, up_v = frame(nextp - prevp, twist)
        u = lengths[i] / total
        mul = taper(u) if callable(taper) else 1.0
        w = width * mul * (1.0 + 0.035 * math.sin(u * math.tau * 1.7 + seed))
        d = depth * mul * (1.0 + 0.025 * math.sin(u * math.tau * 2.1 + seed * 0.7))
        if section == "tri":
            sec = cleft_section(w, d, seed + i, samples=sides, triangular=True)
        elif section == "cleft":
            sec = cleft_section(w, d, seed + i, samples=sides, triangular=False)
        else:
            sec = rounded_rect_section(w, d, seed + i, samples=sides)
        row, crow = [], []
        for sx, sz in sec:
            angle = math.atan2(sz / max(d * 0.5, 1e-4), sx / max(w * 0.5, 1e-4))
            split = 0.0
            for phase, amp in ((0.25, 0.0045), (2.7, 0.0035), (4.8, 0.0025)):
                diff = abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase)))
                split -= max(0.0, 1.0 - diff / 0.09) ** 2 * amp * (0.4 + 0.6 * math.sin(lengths[i] * 12 + seed))
            jag = rng.uniform(-0.0018, 0.0018)
            row.append(tuple(p + side_v * (sx + split + jag) + up_v * (sz + rng.uniform(-0.0015, 0.0015))))
            crow.append((sx, sz, lengths[i]))
        rows.append(row)
        coords.append(crow)
    obj = kit.loft(name, rows, material=material_slots or material, coords=coords,
                   cap_start=cap_start, cap_end=cap_end)

    def disp(co, pco):
        u = pco.z / total
        fibres = 0.0015 * math.sin(pco.z * 58.0 + pco.x * 34.0 + seed)
        facets = -0.0012 * abs(math.sin(math.atan2(pco.y, pco.x) * 6.0 + pco.z * 1.8))
        checks = 0.0
        angle = math.atan2(pco.y / max(depth * 0.5, 1e-4), pco.x / max(width * 0.5, 1e-4))
        for phase, amp in ((0.25, 0.004), (2.7, 0.003), (4.8, 0.002)):
            diff = abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase)))
            checks -= max(0.0, 1.0 - diff / 0.075) ** 2 * amp * (0.35 + 0.65 * math.sin(pco.z * 10.0 + seed))
        end_soft = 0.0015 * max(0.0, 0.08 - min(u, 1.0 - u)) / 0.08 * math.sin((pco.x + pco.y) * 120 + seed)
        return fibres + facets + checks + end_soft

    if smooth:
        kit.displace(obj, disp)
    return obj


def make_splinter(kit, name, base, direction, side, up, width, thickness, length, material, seed, rows=5):
    rng = random.Random(seed)
    base = Vector(base)
    d = Vector(direction).normalized()
    s = Vector(side).normalized()
    u = Vector(up).normalized()
    rings, coords = [], []
    for i in range(rows):
        t = i / (rows - 1)
        centre = base + d * (length * t) + s * rng.uniform(-0.012, 0.012) * t + u * rng.uniform(-0.010, 0.018) * t
        w = width * (1.0 - 0.86 * t) * rng.uniform(0.80, 1.18)
        th = thickness * (1.0 - 0.90 * t) * rng.uniform(0.75, 1.20)
        rings.append([tuple(centre - s * w / 2 - u * th / 2),
                      tuple(centre + s * w / 2 - u * th / 2),
                      tuple(centre + s * w / 2 + u * th / 2),
                      tuple(centre - s * w / 2 + u * th / 2)])
        coords.append([(-w / 2, -th / 2, length * t), (w / 2, -th / 2, length * t),
                       (w / 2, th / 2, length * t), (-w / 2, th / 2, length * t)])
    return kit.loft(name, rings, material=material, coords=coords, cap_start=True, cap_end=True)


def dark_crack(kit, name, center, direction, side, up, length, width, material):
    c = Vector(center)
    d = Vector(direction).normalized()
    s = Vector(side).normalized()
    u = Vector(up).normalized()
    depth = min(width * 0.55, 0.006)
    verts = [
        c - d * length / 2 + u * 0.0015,
        c - d * length * 0.38 - s * width / 2 + u * 0.001,
        c - d * length * 0.38 + s * width / 2 + u * 0.001,
        c - s * width * 0.40 - u * depth,
        c + s * width * 0.40 - u * depth,
        c + d * length * 0.38 - s * width / 2 + u * 0.001,
        c + d * length * 0.38 + s * width / 2 + u * 0.001,
        c + d * length / 2 + u * 0.0015,
    ]
    return kit.mesh(name, [tuple(v) for v in verts], [(0, 1, 3), (0, 3, 4), (0, 4, 2), (3, 5, 7), (4, 7, 6), (3, 7, 4)], material)


def settle(parts):
    lo = min((obj.matrix_world @ v.co).z for obj in parts for v in obj.data.vertices)
    for obj in parts:
        obj.location.z -= lo
    return lo


def box_panel(kit, name, center, axes, half_sizes, material):
    cx = Vector(center)
    ax = [Vector(a).normalized() for a in axes]
    hx, hy, hz = half_sizes
    verts = []
    for sx in (-1, 1):
        for sy in (-1, 1):
            for sz in (-1, 1):
                verts.append(tuple(cx + ax[0] * hx * sx + ax[1] * hy * sy + ax[2] * hz * sz))
    faces = [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]
    return kit.mesh(name, verts, faces, material)


def weathered_oak(kit, name="M_FarmWeatheredOak", seed=0.0, lichen=0.35, grime=0.35):
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    long_vec = g.combine(g.math("MULTIPLY", x, 25.0), g.math("MULTIPLY", y, 25.0),
                         g.math("ADD", g.math("MULTIPLY", z, 1.8), seed))
    fibre = g.noise(long_vec, scale=7.5, detail=10.0, roughness=0.58).outputs["Fac"]
    rings = g.wave(g.combine(x, y, g.math("MULTIPLY", z, 0.10)), scale=18.0, distortion=7.0,
                   detail=4.0, kind="RINGS", direction="Z").outputs["Fac"]
    checks = g.voronoi(g.combine(g.math("MULTIPLY", x, 0.8), g.math("MULTIPLY", y, 0.8),
                                 g.math("MULTIPLY", z, 0.20)), scale=42.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(checks, 0.0, 0.012, 0.85, 0.0)
    tone = g.math("ADD", g.math("MULTIPLY", fibre, 0.74), g.math("MULTIPLY", rings, 0.20))
    color = g.ramp(tone, [(0.18, (0.105, 0.102, 0.090)), (0.58, (0.205, 0.198, 0.175)),
                          (0.94, (0.300, 0.288, 0.250))])
    color = g.mix(color, (0.035, 0.030, 0.024), crack)
    wet = g.noise(p, scale=18.0, detail=6.0, roughness=0.65).outputs["Fac"]
    color = g.mix(color, (0.070, 0.062, 0.050), g.remap(wet, 0.58, 0.80, 0.0, grime))
    lich = g.noise(g.combine(g.math("MULTIPLY", x, 1.5), g.math("MULTIPLY", y, 1.5), g.math("MULTIPLY", z, 0.32)),
                   scale=10.0, detail=5.0, roughness=0.60).outputs["Fac"]
    lmask = g.remap(lich, 0.62, 0.79, 0.0, lichen)
    color = g.mix(color, (0.22, 0.25, 0.16), lmask)
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fibre, 0.42), g.math("MULTIPLY", rings, 0.10)),
                    g.math("MULTIPLY", crack, 0.95))
    rough = g.remap(fibre, 0.25, 0.75, 0.82, 0.97)
    g.set("Base Color", color)
    g.set("Roughness", rough)
    g.set("Normal", g.bump(height, strength=0.55, distance=0.0020))
    zero_subsurface(g.mat)
    return g.mat


def rotten_wood(kit, name="M_FarmDarkRot", seed=0.0):
    g = kit.mats.Graph(name)
    p = g.coord()
    n = g.noise(p, scale=22.0, detail=8.0, roughness=0.70).outputs["Fac"]
    cells = g.voronoi(p, scale=55.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(cells, 0.0, 0.035, 1.0, 0.0)
    color = g.ramp(n, [(0.20, (0.018, 0.014, 0.010)), (0.65, (0.055, 0.045, 0.033)),
                       (0.95, (0.100, 0.088, 0.060))])
    color = g.mix(color, (0.010, 0.008, 0.006), crack)
    g.set("Base Color", color)
    g.set("Roughness", 0.98)
    g.set("Normal", g.bump(g.math("SUBTRACT", n, crack), strength=0.85, distance=0.004))
    zero_subsurface(g.mat)
    return g.mat


def moss_lichen(kit, name="M_FarmMossLichen", seed=0.0):
    g = kit.mats.Graph(name)
    p = g.coord()
    n = g.noise(p, scale=35.0, detail=7.0, roughness=0.66).outputs["Fac"]
    c = g.ramp(n, [(0.18, (0.030, 0.040, 0.020)), (0.60, (0.070, 0.095, 0.045)),
                   (0.90, (0.155, 0.175, 0.105))])
    g.set("Base Color", c)
    g.set("Roughness", 0.96)
    g.set("Normal", g.bump(n, strength=0.45, distance=0.0025))
    zero_subsurface(g.mat)
    return g.mat


def rusted_iron(kit, name="M_FarmRustedIron", seed=0.0):
    if hasattr(kit.mats, "wrought_iron"):
        mat = kit.mats.wrought_iron(name, rust=0.88, wear=0.08, seed=seed)
        zero_subsurface(mat)
        return mat
    return kit.material(name, (0.09, 0.045, 0.020), roughness=0.92)


def loam(kit, name="M_FarmLoam", seed=0.0):
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    fine = g.noise(g.combine(g.math("MULTIPLY", x, 1.0), g.math("MULTIPLY", y, 1.0), g.math("MULTIPLY", z, 0.35)),
                   scale=48.0, detail=9.0, roughness=0.68).outputs["Fac"]
    crust = g.noise(p, scale=11.0, detail=4.0, roughness=0.55).outputs["Fac"]
    color = g.ramp(fine, [(0.18, (0.038, 0.030, 0.022)), (0.58, (0.065, 0.052, 0.038)),
                          (0.93, (0.105, 0.086, 0.060))])
    color = g.mix(color, (0.135, 0.112, 0.078), g.remap(crust, 0.62, 0.84, 0.0, 0.35))
    moss = g.noise(g.combine(x, y, g.math("MULTIPLY", z, 0.1)), scale=7.0, detail=5.0).outputs["Fac"]
    color = g.mix(color, (0.052, 0.072, 0.033), g.remap(moss, 0.64, 0.82, 0.0, 0.22))
    g.set("Base Color", color)
    g.set("Roughness", 0.96)
    g.set("Normal", g.bump(fine, strength=0.35, distance=0.0022))
    zero_subsurface(g.mat)
    return g.mat


def straw(kit, name="M_FarmDeadStraw", seed=0.0):
    g = kit.mats.Graph(name)
    p = g.coord()
    x, y, z = g.separate(p)
    fibres = g.noise(g.combine(g.math("MULTIPLY", x, 36.0), g.math("MULTIPLY", y, 36.0),
                               g.math("ADD", g.math("MULTIPLY", z, 2.0), seed)),
                     scale=7.0, detail=7.0, roughness=0.58).outputs["Fac"]
    spots = g.noise(p, scale=31.0, detail=5.0, roughness=0.67).outputs["Fac"]
    color = g.ramp(fibres, [(0.15, (0.105, 0.080, 0.045)), (0.58, (0.215, 0.170, 0.090)),
                            (0.95, (0.320, 0.270, 0.155))])
    color = g.mix(color, (0.075, 0.064, 0.050), g.remap(spots, 0.58, 0.83, 0.0, 0.40))
    g.set("Base Color", color)
    g.set("Roughness", 0.88)
    g.set("Normal", g.bump(fibres, strength=0.32, distance=0.0010))
    zero_subsurface(g.mat)
    return g.mat


def twine(kit, name="M_FarmRottenTwine", seed=0.0):
    if hasattr(kit.mats, "rawhide"):
        mat = kit.mats.rawhide(name, color=(0.14, 0.105, 0.060), strands=3, twist=62.0)
        zero_subsurface(mat)
        return mat
    return kit.material(name, (0.11, 0.08, 0.045), roughness=0.92)


def add_boolean_box_cut(target, center, size, name="MortiseCut"):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    cutter = bpy.context.object
    cutter.name = name
    cutter.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = target.modifiers.new(name, "BOOLEAN")
    mod.operation = "DIFFERENCE"
    mod.object = cutter
    bpy.context.view_layer.objects.active = target
    try:
        bpy.ops.object.modifier_apply(modifier=mod.name)
    finally:
        bpy.data.objects.remove(cutter, do_unlink=True)
    return target
