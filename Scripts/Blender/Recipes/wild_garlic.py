"""Wild garlic / ramsons (Allium ursinum) woodland-floor clump.

Original procedural geometry and UV-only procedural materials. The atlas is
intentionally overlapped by part class for consistent non-repacked bakes:
leaves, tepals, stems, flower centers, and anthers each own a fixed region.
"""
import math
import random

import bpy
from mathutils import Matrix, Vector

NAME = "WildGarlic"
DESCRIPTION = "Forageable woodland ramsons clump: glossy broad leaves with loose white flower umbels."
COLLISION = "none"
TRIANGLE_BUDGET = 90000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 4096, "samples": 64, "repack": False, "maps": ["basecolor", "roughness", "normal"]}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.04, -0.03, 0.29)}

SEED = 314159
LEAF_RECT = (0.00, 0.62, 0.00, 1.00)
LEAF_TILES = {
    "mid": (0.00, 0.155, 0.00, 1.00),
    "yellow": (0.155, 0.310, 0.00, 1.00),
    "dark": (0.310, 0.465, 0.00, 1.00),
    "underside": (0.465, 0.620, 0.00, 1.00),
}
PETAL_RECT = (0.64, 0.78, 0.00, 0.68)
SPATHE_RECT = (0.785, 0.84, 0.00, 0.68)
STEM_RECT = (0.86, 1.00, 0.00, 1.00)
CENTER_RECT = (0.64, 0.73, 0.74, 0.84)
ANTHER_RECT = (0.75, 0.84, 0.74, 0.84)
DEAD_RECT = (0.64, 0.73, 0.86, 1.00)


def _clamp(value, low=0.0, high=1.0):
    return max(low, min(high, value))


def _rect_uv(rect, u, v):
    u0, u1, v0, v1 = rect
    return (u0 + (u1 - u0) * u, v0 + (v1 - v0) * v)


def _assign_vertex_uvs(obj, uvs):
    layer = obj.data.uv_layers["UVMap"].data
    for poly in obj.data.polygons:
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            layer[loop_index].uv = uvs[vertex_index]
    return obj


def _assign_tube_uvs(obj, rect, sides, rings):
    layer = obj.data.uv_layers["UVMap"].data
    body_faces = (rings - 1) * sides
    for poly_index, poly in enumerate(obj.data.polygons):
        is_body = poly_index < body_faces
        seam_face = is_body and poly_index % sides == sides - 1
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            if vertex_index < rings * sides:
                ring = vertex_index // sides
                side = vertex_index % sides
                u = side / sides
                if seam_face and side == 0:
                    u = 1.0
                v = ring / max(1, rings - 1)
            else:
                u, v = 0.5, 0.0 if vertex_index == rings * sides else 1.0
            layer[loop_index].uv = _rect_uv(rect, u, v)
    return obj


def _assign_rect_from_bbox(obj, rect):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    span_x = max(max(xs) - min(xs), 0.0001)
    span_y = max(max(ys) - min(ys), 0.0001)
    uvs = [_rect_uv(rect, (v.co.x - min(xs)) / span_x, (v.co.y - min(ys)) / span_y)
           for v in obj.data.vertices]
    return _assign_vertex_uvs(obj, uvs)


def _local_uv(g, rect):
    u0, u1, v0, v1 = rect
    u_raw, v_raw, _ = g.separate(g.uv())
    return g.remap(u_raw, u0, u1), g.remap(v_raw, v0, v1)


def _make_materials(kit):
    mats = {}

    def leaf_material(key, base, tip, margin, vein, rough=(0.50, 0.61), damage=False):
        g = kit.mats.Graph("M_WildGarlicLeaf_" + key)
        u, v = _local_uv(g, LEAF_TILES[key])
        across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
        margins = g.remap(across, 0.36, 0.50, 0.0, 1.0)
        base_lift = g.remap(v, 0.0, 0.24, 1.0, 0.0)
        mottle = g.noise(g.combine(g.math("MULTIPLY", u, 1.0), g.math("MULTIPLY", v, 2.6), 0.0),
                         scale=22.0, detail=7.0, roughness=0.62, dims="2D").outputs["Fac"]
        vein_vec = g.combine(g.math("ADD", u, g.math("MULTIPLY", g.math("SUBTRACT", u, 0.5),
                                                     g.math("MULTIPLY", v, 0.26))),
                             g.math("MULTIPLY", v, 0.28), 0.0)
        fine_wave = g.wave(vein_vec, scale=15.0, distortion=1.0, detail=3.0,
                           kind="BANDS", direction="X").outputs["Fac"]
        fine_veins = g.math("MULTIPLY", g.remap(fine_wave, 0.80, 1.0, 0.0, 0.34),
                            g.remap(across, 0.035, 0.48, 0.35, 1.0))
        midrib = g.remap(across, 0.0, 0.020, 1.0, 0.0)
        veins = g.math("MAXIMUM", midrib, fine_veins)
        leaf = g.mix(base, tip, g.remap(v, 0.10, 0.86))
        leaf = g.mix(leaf, tuple(min(1.0, c * 1.24) for c in base), base_lift)
        leaf = g.mix(leaf, margin, margins)
        leaf = g.mix(leaf, tuple(c * 0.82 for c in base), g.remap(mottle, 0.30, 0.58, 0.0, 0.22))
        leaf = g.mix(leaf, tuple(min(1.0, c * 1.15) for c in vein), veins)
        if damage:
            speck = g.noise(g.combine(g.math("MULTIPLY", u, 2.0), g.math("MULTIPLY", v, 6.0), 0.0),
                            scale=35.0, detail=5.0, dims="2D").outputs["Fac"]
            yellow_tip = g.remap(v, 0.82, 0.98, 0.0, 0.75)
            brown_tip = g.math("MULTIPLY", g.remap(v, 0.91, 1.0, 0.0, 0.95),
                               g.remap(speck, 0.42, 0.78, 0.25, 1.0))
            leaf = g.mix(leaf, (0.42, 0.36, 0.12), yellow_tip)
            leaf = g.mix(leaf, (0.145, 0.070, 0.026), brown_tip)
        roughness = g.remap(g.math("MAXIMUM", g.math("MULTIPLY", veins, 0.45), mottle),
                            0.0, 1.0, rough[0], rough[1])
        g.set("Base Color", leaf)
        g.set("Roughness", roughness)
        g.set("Subsurface Weight", 0.34)
        g.set("Subsurface Radius", (0.012, 0.032, 0.008))
        height = g.math("ADD", g.math("MULTIPLY", veins, 0.50), g.math("MULTIPLY", mottle, 0.10))
        g.set("Normal", g.bump(height, strength=0.20, distance=0.00055))
        return g.mat

    mats["leaf_mid"] = leaf_material("mid", (0.085, 0.245, 0.065), (0.110, 0.305, 0.080),
                                     (0.045, 0.165, 0.042), (0.165, 0.350, 0.120))
    mats["leaf_yellow"] = leaf_material("yellow", (0.115, 0.255, 0.060), (0.170, 0.320, 0.075),
                                        (0.075, 0.160, 0.038), (0.190, 0.350, 0.115), damage=True)
    mats["leaf_dark"] = leaf_material("dark", (0.048, 0.175, 0.052), (0.070, 0.230, 0.060),
                                      (0.025, 0.110, 0.035), (0.115, 0.270, 0.095))
    mats["leaf_underside"] = leaf_material("underside", (0.125, 0.265, 0.095), (0.155, 0.315, 0.115),
                                           (0.085, 0.190, 0.070), (0.180, 0.355, 0.135),
                                           rough=(0.56, 0.66))

    g = kit.mats.Graph("M_WildGarlicPetal")
    u, v = _local_uv(g, SPATHE_RECT)
    across = g.math("ABSOLUTE", g.math("SUBTRACT", u, 0.5))
    midline = g.remap(across, 0.0, 0.045, 1.0, 0.0)
    base_green = g.remap(v, 0.0, 0.26, 0.55, 0.0)
    translucency = g.noise(g.combine(u, v, 0.0), scale=35.0, detail=3.0, dims="2D").outputs["Fac"]
    white = g.mix((0.86, 0.84, 0.78), (1.0, 0.98, 0.91), g.remap(v, 0.12, 0.92))
    white = g.mix(white, (0.66, 0.78, 0.50), g.math("MAXIMUM", g.math("MULTIPLY", midline, 0.45), base_green))
    white = g.mix(white, (0.72, 0.70, 0.64), g.remap(translucency, 0.72, 0.95, 0.0, 0.16))
    g.set("Base Color", white)
    g.set("Roughness", 0.46)
    g.set("Subsurface Weight", 0.34)
    g.set("Subsurface Radius", (0.018, 0.020, 0.012))
    g.set("Normal", g.bump(g.math("MAXIMUM", midline, translucency), strength=0.18, distance=0.00035))
    mats["petal"] = g.mat

    g = kit.mats.Graph("M_WildGarlicStem")
    u, v = _local_uv(g, STEM_RECT)
    stripe = g.wave(g.combine(g.math("MULTIPLY", u, 0.35), g.math("MULTIPLY", v, 0.08), 0.0),
                    scale=2.2, distortion=0.15, detail=1.0, kind="BANDS", direction="X").outputs["Fac"]
    node = g.noise(g.combine(u, v, 0.0), scale=18.0, detail=3.0, dims="2D").outputs["Fac"]
    stem = g.ramp(g.math("ADD", g.math("MULTIPLY", stripe, 0.12), g.math("MULTIPLY", node, 0.16)),
                  [(0.08, (0.078, 0.170, 0.052)),
                   (0.66, (0.128, 0.260, 0.074)),
                   (1.00, (0.172, 0.320, 0.100))])
    g.set("Base Color", stem)
    g.set("Roughness", g.remap(stripe, 0.0, 1.0, 0.42, 0.58))
    g.set("Subsurface Weight", 0.18)
    g.set("Subsurface Radius", (0.010, 0.020, 0.006))
    g.set("Normal", g.bump(node, strength=0.06, distance=0.00016))
    mats["stem"] = g.mat

    g = kit.mats.Graph("M_WildGarlicSpathe")
    u, v = _local_uv(g, PETAL_RECT)
    fibre = g.wave(g.combine(g.math("MULTIPLY", u, 0.7), g.math("MULTIPLY", v, 0.18), 0.0),
                   scale=8.0, distortion=1.8, detail=3.0, kind="BANDS", direction="X").outputs["Fac"]
    age = g.noise(g.combine(g.math("MULTIPLY", u, 1.2), g.math("MULTIPLY", v, 2.2), 0.0),
                  scale=26.0, detail=5.0, dims="2D").outputs["Fac"]
    membrane = g.mix((0.58, 0.68, 0.43), (0.76, 0.78, 0.62), g.remap(v, 0.05, 0.85))
    membrane = g.mix(membrane, (0.45, 0.36, 0.17), g.math("MULTIPLY", g.remap(age, 0.62, 0.86),
                                                          g.remap(v, 0.62, 1.0)))
    membrane = g.mix(membrane, (0.48, 0.62, 0.34), g.remap(fibre, 0.78, 1.0, 0.0, 0.30))
    g.set("Base Color", membrane)
    g.set("Roughness", 0.72)
    g.set("Subsurface Weight", 0.48)
    g.set("Subsurface Radius", (0.020, 0.026, 0.012))
    g.set("Normal", g.bump(fibre, strength=0.16, distance=0.00028))
    mats["spathe"] = g.mat

    mats["center"] = kit.material("M_WildGarlicFlowerCenter", (0.45, 0.49, 0.20), roughness=0.66)
    mats["anther"] = kit.material("M_WildGarlicAnther", (0.86, 0.72, 0.28), roughness=0.70)
    mats["dead"] = kit.material("M_WildGarlicDeadBases", (0.145, 0.086, 0.032), roughness=0.82)
    return mats


def _blade_width(t, max_width):
    return max_width * (math.sin(math.pi * t) ** 0.54) * (0.78 + 0.22 * math.sin(math.pi * t))


def _make_leaf(kit, name, mat, uv_rect, petiole_mat, base, angle, rng, length, max_width,
               arch, droop, side_bend, damaged=False, underside=False):
    rows, cols = 42, 18
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    right = Vector((-math.sin(angle), math.cos(angle), 0.0))
    up = Vector((0.0, 0.0, 1.0))
    yaw_wobble = rng.uniform(-0.08, 0.08)
    petiole_len = min(length * 0.38, rng.uniform(0.055, 0.095))
    blade_len = max(0.145, length - petiole_len)
    petiole_points = []
    petiole_rings = 9
    petiole_roll = rng.choice((-1, 1)) * rng.uniform(1.25, 2.05) if underside or rng.random() < 0.38 else rng.uniform(-0.48, 0.48)
    phase = rng.uniform(0.0, math.tau)
    for i in range(petiole_rings):
        t = i / (petiole_rings - 1)
        petiole_points.append(Vector(base) + direction * (petiole_len * (0.97 * t)) +
                              right * (side_bend * 0.28 * math.sin(math.pi * t)) +
                              up * (-0.012 + rng.uniform(0.012, 0.024) * t + 0.010 * math.sin(math.pi * t)))
    petiole = kit.tube(name + "_Petiole", petiole_points, radius=lambda t: 0.00155 - 0.00028 * t,
                       sides=8, material=petiole_mat, cap=True, roll=rng.uniform(0, math.tau))
    _assign_tube_uvs(petiole, STEM_RECT, 8, petiole_rings)
    blade_base = petiole_points[-1]

    twist = rng.uniform(-0.38, 0.55)
    roll0 = petiole_roll + rng.uniform(-0.22, 0.22)
    notch_side = rng.choice((-1, 1))
    notch_t = rng.uniform(0.58, 0.88)
    notch_width = rng.uniform(0.035, 0.075)
    notch_depth = rng.uniform(0.15, 0.30) if damaged or rng.random() < 0.12 else 0.0
    verts, uvs, faces = [], [], []
    for i in range(rows + 1):
        t = i / rows
        travel = blade_len * (0.985 * t - 0.030 * math.sin(math.pi * t))
        lateral_curve = side_bend * math.sin(math.pi * t) + 0.018 * math.sin(2.2 * math.pi * t + phase) * t
        z = 0.020 * t + arch * (math.sin(math.pi * t) ** 0.92) - (droop * 0.58) * (t ** 2.05)
        spine = blade_base + direction * travel + right * lateral_curve + right * (yaw_wobble * t * blade_len * 0.12) + up * z
        width = _blade_width(t, max_width)
        width *= 1.0 + 0.028 * math.sin(10.0 * math.pi * t + phase) + 0.015 * math.sin(17.0 * math.pi * t + phase * 0.7)
        roll = roll0 + twist * (t ** 1.25) + 0.14 * math.sin(math.pi * 2.0 * t + phase)
        lateral = right * math.cos(roll) + up * math.sin(roll)
        crease_up = up * math.cos(roll) - right * math.sin(roll)
        for j in range(cols + 1):
            s = (j / cols) * 2.0 - 1.0
            notch = notch_depth * math.exp(-((t - notch_t) / notch_width) ** 2) * max(0.0, (abs(s) - 0.56) / 0.44) ** 1.7
            if notch_side > 0 and s > 0:
                s_eff = s - notch
            elif notch_side < 0 and s < 0:
                s_eff = s + notch
            else:
                s_eff = s
            midrib_valley = -0.0026 * (1.0 - abs(s_eff)) ** 1.3
            v_crease = 0.0078 * (abs(s_eff) ** 1.12) + midrib_valley
            ripple = 0.0018 * math.sin(9.0 * math.pi * t + phase) * abs(s_eff)
            verts.append(spine + lateral * (s_eff * width * 0.5) + crease_up * (v_crease + ripple))
            uvs.append(_rect_uv(uv_rect, j / cols, t))
    for i in range(rows):
        for j in range(cols):
            a = i * (cols + 1) + j
            faces.append((a, a + cols + 1, a + cols + 2, a + 1))
    obj = kit.mesh(name, verts, faces, mat)
    _assign_vertex_uvs(obj, uvs)
    kit.recalc_normals(obj)
    return [petiole, obj]


def _add_octa(verts, faces, uvs, center, radius, rect):
    base = len(verts)
    offsets = [Vector((radius, 0, 0)), Vector((-radius, 0, 0)), Vector((0, radius, 0)),
               Vector((0, -radius, 0)), Vector((0, 0, radius)), Vector((0, 0, -radius))]
    verts.extend([center + off for off in offsets])
    face_indices = [(4, 0, 2), (4, 2, 1), (4, 1, 3), (4, 3, 0),
                    (5, 2, 0), (5, 1, 2), (5, 3, 1), (5, 0, 3)]
    faces.extend([tuple(base + i for i in face) for face in face_indices])
    for off in offsets:
        uvs.append(_rect_uv(rect, 0.5 + off.x / max(radius, 0.0001) * 0.34,
                            0.5 + off.y / max(radius, 0.0001) * 0.34))


def _add_tepal(verts, faces, uvs, center, normal, radial, length, width, bend, phase):
    rows, cols = 6, 4
    normal = normal.normalized()
    radial = radial.normalized()
    cross = normal.cross(radial).normalized()
    base_index = len(verts)
    for i in range(rows + 1):
        t = i / rows
        w = width * (math.sin(math.pi * t) ** 0.72) * (0.35 + 0.65 * t)
        for j in range(cols + 1):
            s = (j / cols) * 2.0 - 1.0
            cup = normal * (bend * math.sin(math.pi * t) * (1.0 - 0.45 * abs(s)))
            flutter = normal * (0.00035 * math.sin(phase + t * math.pi * 2.0) * abs(s))
            verts.append(center + radial * (length * t) + cross * (s * w * 0.5) + cup + flutter)
            uvs.append(_rect_uv(PETAL_RECT, j / cols, t))
    for i in range(rows):
        for j in range(cols):
            a = base_index + i * (cols + 1) + j
            faces.append((a, a + cols + 1, a + cols + 2, a + 1))


def _make_flower_meshes(kit, mats, flowers):
    petal_verts, petal_faces, petal_uvs = [], [], []
    center_verts, center_faces, center_uvs = [], [], []
    anther_verts, anther_faces, anther_uvs = [], [], []
    world_up = Vector((0, 0, 1))
    for index, (center, normal, flower_scale, phase) in enumerate(flowers):
        normal = normal.normalized()
        axis_a = normal.cross(world_up)
        if axis_a.length < 0.001:
            axis_a = Vector((1, 0, 0))
        axis_a.normalize()
        axis_b = normal.cross(axis_a).normalized()
        stagger = (index % 3) * math.radians(7.5)
        for petal in range(6):
            angle = stagger + petal * math.tau / 6.0
            radial = axis_a * math.cos(angle) + axis_b * math.sin(angle)
            _add_tepal(petal_verts, petal_faces, petal_uvs, center + normal * 0.0007, normal, radial,
                       flower_scale * 0.0058, flower_scale * 0.0024, flower_scale * 0.0009,
                       phase + petal * 1.7)
            _add_octa(anther_verts, anther_faces, anther_uvs,
                      center + radial * (flower_scale * 0.0023) + normal * (flower_scale * 0.0014),
                      flower_scale * 0.00042, ANTHER_RECT)
        _add_octa(center_verts, center_faces, center_uvs, center + normal * 0.00065,
                  flower_scale * 0.00105, CENTER_RECT)
    parts = []
    if petal_verts:
        obj = kit.mesh("FlowerTepals", petal_verts, petal_faces, mats["petal"])
        _assign_vertex_uvs(obj, petal_uvs)
        kit.recalc_normals(obj)
        parts.append(obj)
    if center_verts:
        obj = kit.mesh("FlowerCenters", center_verts, center_faces, mats["center"])
        _assign_vertex_uvs(obj, center_uvs)
        kit.recalc_normals(obj)
        parts.append(obj)
    if anther_verts:
        obj = kit.mesh("FlowerAnthers", anther_verts, anther_faces, mats["anther"])
        _assign_vertex_uvs(obj, anther_uvs)
        kit.recalc_normals(obj)
        parts.append(obj)
    return parts


def _make_spathe(kit, mat, base, normal, angle, scale):
    rows, cols = 8, 4
    verts, uvs, faces = [], [], []
    rng = random.Random(SEED + int(scale * 1000) + int(angle * 100))
    for strip in range(2):
        direction = Vector((math.cos(angle + rng.uniform(-0.45, 0.45)),
                            math.sin(angle + rng.uniform(-0.45, 0.45)), -0.34)).normalized()
        right = normal.cross(direction)
        if right.length < 0.001:
            right = Vector((-math.sin(angle), math.cos(angle), 0))
        right.normalize()
        curl_axis = direction.cross(right).normalized()
        base_index = len(verts)
        length = scale * rng.uniform(0.013, 0.023)
        width = scale * rng.uniform(0.0024, 0.0052)
        side_offset = right * rng.uniform(-0.0025, 0.0025)
        for i in range(rows + 1):
            t = i / rows
            strip_w = width * (1.0 - 0.42 * t) * (0.75 + 0.25 * math.sin(math.pi * t))
            center = base + side_offset + direction * (length * t) + Vector((0, 0, -scale * 0.015 * t * t))
            curl = curl_axis * (scale * 0.007 * math.sin(t * math.pi * 0.9 + strip * 0.7))
            for j in range(cols + 1):
                s = (j / cols) * 2 - 1
                torn = 1.0 - 0.18 * max(0.0, t - 0.55) * (1.0 + math.sin(17.0 * t + strip))
                verts.append(center + right * (s * strip_w * 0.5 * torn) + curl * (s * s))
                uvs.append(_rect_uv(SPATHE_RECT, 0.50 + 0.38 * s, 0.05 + 0.54 * t))
        for i in range(rows):
            for j in range(cols):
                a = base_index + i * (cols + 1) + j
                faces.append((a, a + cols + 1, a + cols + 2, a + 1))
    obj = kit.mesh("PaperySpathe", verts, faces, mat)
    _assign_vertex_uvs(obj, uvs)
    kit.recalc_normals(obj)
    return obj


def _make_bud(kit, name, mat, center, normal, scale):
    normal = normal.normalized()
    points = [center - normal * (0.0012 * scale),
              center + normal * (0.0022 * scale),
              center + normal * (0.0058 * scale)]
    bud = kit.tube(name, points, radii=[0.00040 * scale, 0.00088 * scale, 0.00018 * scale],
                   sides=8, material=mat, cap=True, roll=0.3)
    _assign_tube_uvs(bud, SPATHE_RECT, 8, 3)
    return bud


def _make_dead_bases(kit, mat, rng, rosettes):
    parts = []
    for index, root in enumerate(rosettes):
        for strip in range(3):
            angle = rng.uniform(0, math.tau)
            direction = Vector((math.cos(angle), math.sin(angle), 0))
            right = Vector((-math.sin(angle), math.cos(angle), 0))
            length = rng.uniform(0.035, 0.070)
            width = rng.uniform(0.004, 0.009)
            rows, cols = 6, 2
            verts, uvs, faces = [], [], []
            phase = rng.uniform(0, math.tau)
            for i in range(rows + 1):
                t = i / rows
                center = Vector(root) + direction * (length * t) + right * (0.006 * math.sin(math.pi * t + phase))
                center.z = rng.uniform(-0.004, 0.002) + 0.006 * math.sin(math.pi * t) * rng.uniform(0.0, 0.6)
                w = width * (1 - 0.72 * t)
                for j in range(cols + 1):
                    s = (j / cols) * 2 - 1
                    verts.append(center + right * (s * w * 0.5) + Vector((0, 0, 0.0015 * s * math.sin(t * math.pi))))
                    uvs.append(_rect_uv(DEAD_RECT, 0.12 + 0.20 * j / cols, 0.12 + 0.72 * t))
            for i in range(rows):
                for j in range(cols):
                    a = i * (cols + 1) + j
                    faces.append((a, a + cols + 1, a + cols + 2, a + 1))
            obj = kit.mesh(f"DeadBase{index}_{strip}", verts, faces, mat)
            _assign_vertex_uvs(obj, uvs)
            kit.recalc_normals(obj)
            parts.append(obj)
    return parts


def _make_scape_and_umbel(kit, mats, rng, base, scape_index):
    height = rng.uniform(0.248, 0.312)
    lean_angle = rng.uniform(0, math.tau)
    lean = Vector((math.cos(lean_angle), math.sin(lean_angle), 0)) * rng.uniform(0.012, 0.034)
    points = []
    rings = 18
    for i in range(rings):
        t = i / (rings - 1)
        sway = Vector((math.sin(lean_angle + math.pi / 2), math.cos(lean_angle + math.pi / 2), 0))
        points.append(Vector(base) + lean * (t ** 1.25) + sway * (0.006 * math.sin(math.pi * t + scape_index)) +
                      Vector((0, 0, -0.012 + height * t)))
    scape = kit.tube(f"Scape{scape_index}", points, radius=lambda t: 0.0028 - 0.00055 * t,
                     sides=3, material=mats["stem"], cap=True, roll=rng.uniform(0, math.tau))
    _assign_tube_uvs(scape, STEM_RECT, 3, rings)
    top = points[-1]
    flower_count = rng.randint(13, 19)
    pedicels, flowers = [], []
    for i in range(flower_count):
        az = i * math.radians(137.508) + rng.uniform(-0.24, 0.24)
        z = rng.uniform(-0.08, 0.58)
        radial = math.sqrt(max(0.0, 1.0 - z * z))
        direction = Vector((math.cos(az) * radial, math.sin(az) * radial, z)).normalized()
        length = rng.uniform(0.013, 0.024)
        start = top + direction * 0.002
        end = top + direction * length + Vector((0, 0, rng.uniform(-0.002, 0.004)))
        side = Vector((-math.sin(az), math.cos(az), 0)) * rng.uniform(-0.0025, 0.0025)
        ped_points = []
        ped_rings = 8
        for p in range(ped_rings):
            t = p / (ped_rings - 1)
            sag = Vector((0, 0, -0.0028 * math.sin(math.pi * t) * (1.0 - max(direction.z, 0.0))))
            ped_points.append(start.lerp(end, t) + side * math.sin(math.pi * t) + sag)
        ped = kit.tube(f"Pedicel{scape_index}_{i}", ped_points, radius=0.00050, sides=8,
                       material=mats["stem"], cap=True, roll=rng.uniform(0, math.tau))
        _assign_tube_uvs(ped, STEM_RECT, 8, ped_rings)
        pedicels.append(ped)
        flower_normal = (end - top).normalized()
        if rng.random() < 0.18:
            pedicels.append(_make_bud(kit, f"Bud{scape_index}_{i}", mats["spathe"], end, flower_normal,
                                      rng.uniform(0.85, 1.18)))
        else:
            flowers.append((end, flower_normal, rng.uniform(0.72, 0.98), rng.uniform(0, math.tau)))
    spathe = _make_spathe(kit, mats["spathe"], top - Vector((0, 0, 0.002)), Vector((0, 0, 1)),
                          lean_angle + math.pi + rng.uniform(-0.7, 0.7), rng.uniform(0.8, 1.15))
    return [scape, spathe] + pedicels, flowers


def build(kit):
    rng = random.Random(SEED)
    mats = _make_materials(kit)
    parts = []
    rosettes = [Vector((0.000, 0.000, -0.003)),
                Vector((0.045, -0.030, -0.006)),
                Vector((-0.050, 0.030, -0.005)),
                Vector((0.025, 0.058, -0.004))]

    leaf_count = 22
    damaged_leaves = {3, 11, 18}
    underside_leaves = {7, 14}
    for i in range(leaf_count):
        ring_angle = i * math.tau / leaf_count + rng.uniform(-0.18, 0.18)
        rosette = rosettes[i % len(rosettes)] + Vector((rng.uniform(-0.010, 0.010),
                                                        rng.uniform(-0.010, 0.010), 0))
        length = rng.uniform(0.190, 0.262)
        broad = rng.uniform(0.040, 0.066) * (0.92 if i < 4 else 1.0)
        arch = rng.uniform(0.090, 0.155)
        droop = rng.uniform(0.055, 0.125)
        if i % 7 == 0:
            arch *= 1.20
            droop *= 0.70
        side_bend = rng.uniform(-0.035, 0.035)
        if i in damaged_leaves:
            leaf_key = "yellow"
        elif i in underside_leaves:
            leaf_key = "underside"
        elif i % 5 == 0:
            leaf_key = "dark"
        else:
            leaf_key = "mid"
        parts.extend(_make_leaf(kit, f"Leaf{i:02d}", mats["leaf_" + leaf_key], LEAF_TILES[leaf_key],
                                mats["stem"], rosette, ring_angle, rng, length, broad, arch, droop,
                                side_bend, damaged=i in damaged_leaves,
                                underside=i in underside_leaves))

    parts.extend(_make_dead_bases(kit, mats["dead"], rng, rosettes))

    all_flowers = []
    scape_bases = [rosettes[0], rosettes[1], rosettes[2], rosettes[3]]
    for index, base in enumerate(scape_bases):
        scape_parts, flowers = _make_scape_and_umbel(kit, mats, rng, base, index)
        parts.extend(scape_parts)
        all_flowers.extend(flowers)
    parts.extend(_make_flower_meshes(kit, mats, all_flowers))

    return kit.join(parts, "SM_WildGarlic", pivot="base", unwrap=False, reshade=True, smooth_angle=52)
