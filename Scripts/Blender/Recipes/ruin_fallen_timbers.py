"""Ruined manor detail: fallen, fire-charred oak roof timbers crossed on the ground.

Real-object research: an 1851 Cornish granite gentry house would carry Delabole slate on
substantial oak/deal purlins and rafters. A burnt, roofless ruin leaves heavy members on the
floor with soft adzed/sawn arrises, longitudinal seasoning checks, iron-spike scars, jagged
snapped ends with torn fibre bundles, alligator-cracked charcoal at the burnt ends, and rain-
silvered grey-brown oak darkened by soil and lichen where it lies against the ground.

Geometry (Unreal cm): three beams lie crossed within a roughly 430 x 230 cm footprint, under
50 cm high. Pivot is the settled ground-centre of the pile. Everything is original procedural
geometry/materials; no downloaded geometry or textures.
"""
import math
import os
import random
from pathlib import Path
import sys

from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_rocks as rocks

NAME = "RuinFallenTimbers"
DESCRIPTION = "Ruined manor detail: crossed fallen fire-charred oak roof timbers with split ends, checks, char relief and a hand-forged spike."
COLLISION = "box"
TRIANGLE_BUDGET = 150000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96,
        "maps": ("basecolor", "roughness", "normal", "ao", "metallic")}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.18, -0.05, 0.22),
          "views": ["hero", "detail", "eye"], "eye_distance": 3.6}
REPORT = {
    "unreal_frame": "Extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "Ground centre of the settled timber pile at (0, 0, 0).",
    "placement": "Drop on the manor floor/yard as collapsed roof debris. Longest beam runs roughly X; crossed purlin is raised and visibly rests on it.",
    "collision_note": "Box collision from the prop importer; authored height stays under 60 cm.",
}

SEED = 184017


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


def _rounded_section(width, depth, rng, samples=36):
    hw, hd = width / 2, depth / 2
    bevel = min(width, depth) * 0.17
    pts = []
    corners = [(hw - bevel, hd - bevel, 0.0, math.pi / 2),
               (-(hw - bevel), hd - bevel, math.pi / 2, math.pi),
               (-(hw - bevel), -(hd - bevel), math.pi, math.pi * 1.5),
               (hw - bevel, -(hd - bevel), math.pi * 1.5, math.tau)]
    per = max(4, samples // 4)
    for cx, cz, a0, a1 in corners:
        for i in range(per):
            a = a0 + (a1 - a0) * i / per
            x = cx + math.cos(a) * bevel
            z = cz + math.sin(a) * bevel
            # Rough adzing keeps the beam rectangular but not machine-perfect.
            pts.append((x + rng.uniform(-0.004, 0.004), z + rng.uniform(-0.004, 0.004)))
    return pts


def _polyline(points, spacing):
    source = [Vector(p) for p in points]
    out = []
    for a, b in zip(source, source[1:]):
        steps = max(2, int((b - a).length / spacing))
        for j in range(steps):
            out.append(a.lerp(b, j / steps))
    out.append(source[-1])
    return out


def _beam_mesh(kit, name, points, width, depth, material, seed, roll=0.0, spacing=0.050, jagged_last=False):
    rng = random.Random(seed)
    pts = _polyline(points, spacing if not DRAFT else spacing * 2.0)
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + (b - a).length)
    rows, coords = [], []
    for i, p in enumerate(pts):
        prevp, nextp = pts[max(0, i - 1)], pts[min(len(pts) - 1, i + 1)]
        wave = 0.010 * math.sin(i * 0.37 + seed) + 0.006 * math.sin(i * 1.17 + seed * 0.1)
        t, side, up = _frame(nextp - prevp, roll + wave)
        taper = 1.0 + 0.025 * math.sin(i * 0.21 + seed)
        sec = _rounded_section(width * taper, depth * (1.0 + 0.018 * math.sin(i * 0.31)), rng,
                               28 if DRAFT else 44)
        row, crow = [], []
        for x, z in sec:
            # Long checks are real geometry: localised grooves running along the grain.
            angle = math.atan2(z / max(depth * 0.5, 1e-4), x / max(width * 0.5, 1e-4))
            groove = 0.0
            for phase, amp, freq in ((0.20, 0.016, 7.0), (2.65, 0.012, 5.0), (4.55, 0.009, 9.0)):
                if abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase))) < 0.075:
                    groove -= amp * (0.45 + 0.55 * math.sin(lengths[i] * freq + seed))
            end_jag = 0.0
            if jagged_last and i == len(pts) - 1:
                end_jag = rng.uniform(-0.075, 0.025)
            row.append(tuple(p + t * end_jag + side * (x + groove * math.copysign(1.0, x or 1.0)) + up * z))
            crow.append((x, z, lengths[i]))
        rows.append(row)
        coords.append(crow)
    obj = kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)

    total = max(lengths[-1], 1e-6)

    def disp(co, pco):
        s = pco.z / total
        angle = math.atan2(pco.y / max(depth * 0.5, 1e-4), pco.x / max(width * 0.5, 1e-4))
        fibres = math.sin(pco.z * 58.0 + 7.0 * math.sin(pco.x * 19.0 + seed)) * 0.0016
        adze = -abs(math.sin(angle * 9.0 + pco.z * 1.7 + seed)) * 0.0015
        end_tear = max(0.0, 0.13 - min(s, 1.0 - s)) / 0.13
        split = 0.0
        for phase, depth_m in ((0.20, 0.010), (2.65, 0.008), (4.55, 0.006)):
            d = abs(math.atan2(math.sin(angle - phase), math.cos(angle - phase)))
            split -= max(0.0, 1.0 - d / 0.070) ** 2 * depth_m * (0.45 + 0.55 * math.sin(pco.z * 8.0 + seed))
        return fibres + adze + split + end_tear * 0.004 * math.sin((pco.x + pco.y) * 95.0 + seed)

    kit.displace(obj, disp)
    return obj


def _dark_check(kit, name, center, direction, side, up, length, width, material):
    d, s, u = Vector(direction).normalized(), Vector(side).normalized(), Vector(up).normalized()
    c = Vector(center)
    # Narrow tapered shake: a V floor, not a black rectangular decal.
    depth = min(width * 0.45, 0.004)
    verts = [
        c - d * length / 2 + u * 0.0015,
        c - d * length * 0.32 - s * width / 2 + u * 0.0010,
        c - d * length * 0.32 + s * width / 2 + u * 0.0010,
        c - s * width * 0.42 - u * depth,
        c + s * width * 0.42 - u * depth,
        c + d * length * 0.32 - s * width / 2 + u * 0.0010,
        c + d * length * 0.32 + s * width / 2 + u * 0.0010,
        c + d * length / 2 + u * 0.0015,
    ]
    faces = [(0, 1, 3), (0, 3, 4), (0, 4, 2), (3, 5, 7), (4, 7, 6), (3, 7, 4)]
    obj = kit.mesh(name, [tuple(v) for v in verts], faces, material)
    return obj


def _splinter(kit, name, base, direction, side, up, width, thickness, length, material, seed):
    rng = random.Random(seed)
    base = Vector(base)
    d = Vector(direction).normalized()
    s = Vector(side).normalized()
    u = Vector(up).normalized()
    rows, coords = [], []
    for i in range(5):
        t = i / 4
        center = base + d * (length * t) + s * rng.uniform(-0.008, 0.008) * t + u * rng.uniform(-0.010, 0.012) * t
        w = width * (1.0 - 0.82 * t) * rng.uniform(0.86, 1.12)
        th = thickness * (1.0 - 0.88 * t) * rng.uniform(0.80, 1.18)
        rows.append([tuple(center - s * w / 2 - u * th / 2),
                     tuple(center + s * w / 2 - u * th / 2),
                     tuple(center + s * w / 2 + u * th / 2),
                     tuple(center - s * w / 2 + u * th / 2)])
        coords.append([(-w / 2, -th / 2, length * t), (w / 2, -th / 2, length * t),
                       (w / 2, th / 2, length * t), (-w / 2, th / 2, length * t)])
    obj = kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)
    return obj


def _char_tiles(kit, parts, prefix, center, direction, side, up, width, depth, material, seed):
    rng = random.Random(seed)
    d = Vector(direction).normalized()
    s = Vector(side).normalized()
    u = Vector(up).normalized()
    origin = Vector(center)
    nx, nz = (6, 5) if DRAFT else (10, 8)
    for ix in range(nx):
        for iz in range(nz):
            if rng.random() < 0.10:
                continue
            x0 = -width / 2 + width * ix / nx + rng.uniform(-0.006, 0.004)
            x1 = -width / 2 + width * (ix + 1) / nx + rng.uniform(-0.004, 0.006)
            z0 = -depth / 2 + depth * iz / nz + rng.uniform(-0.006, 0.004)
            z1 = -depth / 2 + depth * (iz + 1) / nz + rng.uniform(-0.004, 0.006)
            lift = rng.uniform(0.002, 0.010)
            c0 = origin + d * lift
            verts = [c0 + s * x0 + u * z0, c0 + s * x1 + u * z0,
                     c0 + s * x1 + u * z1, c0 + s * x0 + u * z1]
            parts.append(kit.mesh(f"{prefix}_CharBlock_{ix}_{iz}", [tuple(v) for v in verts], [(0, 1, 2, 3)], material))
    # Raised soot blisters along the first few centimetres of the sleeve.
    for i in range(20 if not DRAFT else 8):
        a = rng.uniform(0, math.tau)
        rr = rng.uniform(0.15, 0.48)
        p = origin + d * rng.uniform(0.025, 0.18) + s * (math.cos(a) * width * rr) + u * (math.sin(a) * depth * rr)
        parts.append(kit.sphere(f"{prefix}_CharBlister_{i}", rng.uniform(0.006, 0.018), location=tuple(p),
                                material=material, segments=8, rings=4, scale=(1.0, 0.7, 0.35)))


def _cylinder_between(kit, name, a, b, radius, material, sides=10):
    a, b = Vector(a), Vector(b)
    length = (b - a).length
    obj = kit.cylinder(name, radius, length, location=tuple((a + b) / 2), material=material,
                       sides=sides, bevel=0.0015, bevel_segments=2)
    obj.rotation_euler = (b - a).to_track_quat("Z", "Y").to_euler()
    return obj


def _leaf_litter(kit, name, loc, yaw, scale, material):
    x, y, z = loc
    pts = [(-0.040, 0.000), (-0.020, 0.035), (0.000, 0.055), (0.022, 0.034), (0.045, 0.000),
           (0.020, -0.030), (0.000, -0.044), (-0.024, -0.028)]
    c, s = math.cos(yaw), math.sin(yaw)
    verts = []
    for px, py in pts:
        verts.append((x + scale * (px * c - py * s), y + scale * (px * s + py * c), z + 0.001 * math.sin(px * 80)))
    return kit.mesh(name, verts, [tuple(range(len(verts)))], material)


def _settle(objects):
    lo = min((obj.matrix_world @ v.co).z for obj in objects for v in obj.data.vertices)
    for obj in objects:
        obj.location.z -= lo


def _hewn_oak_material(kit):
    g = kit.mats.Graph("M_RuinFallenTimbersStraightWeatheredOak")
    p = g.coord()
    x, y, z = g.separate(p)
    long_vec = g.combine(g.math("MULTIPLY", x, 18.0), g.math("MULTIPLY", y, 18.0),
                         g.math("MULTIPLY", z, 2.0))
    fibre = g.noise(long_vec, scale=8.0, detail=9.0, roughness=0.58).outputs["Fac"]
    facet = g.noise(g.combine(g.math("MULTIPLY", x, 8.0), g.math("MULTIPLY", y, 8.0),
                              g.math("MULTIPLY", z, 0.45)), scale=7.0, detail=3.0,
                    roughness=0.55).outputs["Fac"]
    checks = g.voronoi(g.combine(g.math("MULTIPLY", x, 0.9), g.math("MULTIPLY", y, 0.9),
                                 g.math("MULTIPLY", z, 0.18)), scale=34.0,
                       feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(checks, 0.0, 0.010, 0.65, 0.0)
    tone = g.math("ADD", g.math("MULTIPLY", fibre, 0.78), g.math("MULTIPLY", facet, 0.08))
    color = g.ramp(tone, [(0.20, (0.060, 0.054, 0.044)), (0.62, (0.155, 0.145, 0.120)),
                          (0.92, (0.215, 0.198, 0.162))])
    color = g.mix(color, (0.018, 0.014, 0.010), crack)
    grime = g.noise(p, scale=28.0, detail=5.0, roughness=0.7).outputs["Fac"]
    color = g.mix(color, (0.045, 0.038, 0.030), g.remap(grime, 0.58, 0.78, 0.0, 0.28))
    g.set("Base Color", color)
    g.set("Roughness", 0.94)
    height = g.math("SUBTRACT", g.math("ADD", g.math("MULTIPLY", fibre, 0.45),
                                       g.math("MULTIPLY", facet, 0.08)),
                    g.math("MULTIPLY", crack, 0.60))
    g.set("Normal", g.bump(height, strength=0.45, distance=0.0015))
    return g.mat


def _alligator_char_material(kit):
    g = kit.mats.Graph("M_RuinFallenTimbersBlockyAlligatorChar")
    p = g.coord()
    x, y, z = g.separate(p)
    cells = g.voronoi(g.combine(g.math("MULTIPLY", x, 1.0), g.math("MULTIPLY", y, 1.0),
                                g.math("MULTIPLY", z, 0.35)), scale=95.0,
                      feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.remap(cells, 0.0, 0.045, 1.0, 0.0)
    block = g.noise(p, scale=34.0, detail=4.0, roughness=0.62).outputs["Fac"]
    coal = g.mix((0.006, 0.005, 0.004), (0.040, 0.036, 0.032), g.remap(block, 0.35, 0.72))
    coal = g.mix(coal, (0.002, 0.002, 0.002), crack)
    g.set("Base Color", coal)
    g.set("Roughness", g.remap(block, 0.25, 0.75, 0.68, 0.92))
    height = g.math("SUBTRACT", g.math("MULTIPLY", block, 0.35), g.math("MULTIPLY", crack, 0.95))
    g.set("Normal", g.bump(height, strength=0.95, distance=0.004))
    return g.mat


def build(kit):
    rng = random.Random(SEED)
    m = kit.mats
    grey_oak = _hewn_oak_material(kit)
    scorched = m.wood("M_RuinFallenTimbersScorchedGradientOak", light=(0.20, 0.135, 0.078),
                      dark=(0.050, 0.033, 0.022), grain=0.75, roughness=0.88, weathering=0.32,
                      grime=0.65, seed=22.0, char_above=0.18, char_band=0.13, soot_band=0.30, relief=2.2)
    char = _alligator_char_material(kit)
    fresh = m.wood("M_RuinFallenTimbersTornFibre", light=(0.31, 0.235, 0.145),
                   dark=(0.095, 0.058, 0.030), grain=0.55, roughness=0.86, weathering=0.30,
                   grime=0.18, seed=23.0, relief=1.7)
    iron = m.wrought_iron("M_RuinFallenTimbersOldIronSpike", rust=0.82, wear=0.16, seed=24.0)
    soil = kit.material("M_RuinFallenTimbersContactSoil", (0.055, 0.042, 0.030), roughness=0.98)
    check_mat = kit.material("M_RuinFallenTimbersDeepChecks", (0.035, 0.030, 0.024), roughness=0.96)
    for mat in (grey_oak, scorched, char, fresh, soil):
        if hasattr(mat, "node_tree") and mat.node_tree:
            bsdf = next((n for n in mat.node_tree.nodes if n.bl_idname == "ShaderNodeBsdfPrincipled"), None)
            if bsdf and "Subsurface Weight" in bsdf.inputs:
                bsdf.inputs["Subsurface Weight"].default_value = 0.0

    parts = []
    beams = [
        dict(label="PrincipalPurlin", start=(-1.88, -0.22, 0.090), mid=(0.00, -0.14, 0.098),
             end=(1.88, -0.06, 0.090), width=0.215, depth=0.165, roll=math.radians(6)),
        dict(label="CrossRafter", start=(-1.20, 0.80, 0.265), mid=(0.02, -0.10, 0.292),
             end=(1.25, -0.84, 0.265), width=0.190, depth=0.145, roll=math.radians(-18)),
        dict(label="ShortOffcut", start=(-1.58, 0.92, 0.072), mid=(-0.62, 0.72, 0.082),
             end=(0.72, 0.62, 0.076), width=0.150, depth=0.115, roll=math.radians(25)),
    ]
    for i, spec in enumerate(beams):
        start, mid, end = Vector(spec["start"]), Vector(spec["mid"]), Vector(spec["end"])
        d = (end - start).normalized()
        char_len = 0.24 if i < 2 else 0.20
        a, b = start + d * char_len, end - d * char_len
        parts.append(_beam_mesh(kit, f"{spec['label']}_SilveredCore", [tuple(a), tuple(mid), tuple(b)],
                                spec["width"], spec["depth"], grey_oak, SEED + i, spec["roll"]))
        for suffix, inner, outer, sign in (("A", a, start, -1), ("B", b, end, 1)):
            unburnt_end = (spec["label"] == "ShortOffcut" and suffix == "B") or (spec["label"] == "CrossRafter" and suffix == "A")
            parts.append(_beam_mesh(kit, f"{spec['label']}_ScorchedEnd_{suffix}", [tuple(inner), tuple(outer)],
                                    spec["width"] * 1.015, spec["depth"] * 1.02, scorched,
                                    SEED + 40 + i * 3 + sign, spec["roll"], spacing=0.038, jagged_last=True) if not unburnt_end
                         else _beam_mesh(kit, f"{spec['label']}_RottenEnd_{suffix}", [tuple(inner), tuple(outer)],
                                         spec["width"] * 1.010, spec["depth"] * 1.015, fresh,
                                         SEED + 40 + i * 3 + sign, spec["roll"], spacing=0.038, jagged_last=True))
            t, side, up = _frame((outer - inner), spec["roll"])
            if not unburnt_end:
                _char_tiles(kit, parts, f"{spec['label']}_{suffix}", outer, t, side, up,
                            spec["width"] * 0.96, spec["depth"] * 0.92, char, SEED + 80 + i * 9 + sign)
        # Varied torn fibre bundles on one snapped end.
        snap = end if i != 1 else start
        out_dir = (end - start).normalized() * (1 if i != 1 else -1)
        _, side, up = _frame(out_dir, spec["roll"])
        for k in range(18 if (not DRAFT and i == 0) else 10):
            off = side * rng.uniform(-spec["width"] * 0.40, spec["width"] * 0.40) + up * rng.uniform(-spec["depth"] * 0.35, spec["depth"] * 0.36)
            mat = fresh if rng.random() < 0.62 else char
            parts.append(_splinter(kit, f"{spec['label']}_TornBundle_{k}", snap + off, out_dir, side, up,
                                   rng.uniform(0.016, 0.060), rng.uniform(0.008, 0.030),
                                   rng.uniform(0.12, 0.32) * (1.0 if i == 0 else 0.72), mat,
                                   SEED + 140 + i * 20 + k))
        # Deep exposed checks as dark recessed strips on visible faces.
        dvec = (end - start).normalized()
        _, side, up = _frame(dvec, spec["roll"])
        for k in range(10 if not DRAFT else 5):
            s0 = rng.uniform(0.20, 0.78)
            c = start.lerp(end, s0) + up * (spec["depth"] * 0.515) + side * rng.uniform(-spec["width"] * 0.32, spec["width"] * 0.32)
            parts.append(_dark_check(kit, f"{spec['label']}_OpenCheck_{k}", c, dvec, side, up,
                                     rng.uniform(0.20, 0.80), rng.uniform(0.003, 0.008), check_mat))

    # Hand-forged spike, bent and proud of the crossed timber.
    parts.append(_cylinder_between(kit, "OldIronSpikeShank", (0.34, -0.20, 0.330), (0.30, -0.18, 0.145), 0.0085, iron, 10))
    head = kit.cylinder("OldIronSpikeHammeredHead", 0.030, 0.012, location=(0.345, -0.205, 0.338),
                        material=iron, sides=12, bevel=0.004)
    head.rotation_euler = (math.radians(4), math.radians(-13), math.radians(18))
    parts.append(head)
    parts.append(_cylinder_between(kit, "BentIronSpikeTwo", (-0.56, 0.08, 0.275), (-0.48, 0.13, 0.120), 0.0065, iron, 8))
    parts.append(kit.cylinder("BentIronSpikeTwoHead", 0.020, 0.008, location=(-0.565, 0.075, 0.286),
                              material=iron, sides=10, bevel=0.0025))

    # Dirt, lichen and leaf litter exactly where the timbers touch the wet ground.
    for i in range(34 if not DRAFT else 12):
        x = rng.uniform(-2.04, 2.05)
        y = rng.uniform(-1.05, 1.03)
        if rng.random() < 0.70 and abs(y) > 0.78:
            continue
        parts.append(kit.sphere(f"ContactCharSoil_{i}", rng.uniform(0.008, 0.030), location=(x, y, rng.uniform(0.004, 0.020)),
                                material=soil if rng.random() < 0.65 else char, segments=8, rings=4, scale=(1.0, 0.75, 0.16)))
    for i in range(26 if not DRAFT else 8):
        mat = char if rng.random() < 0.55 else fresh
        parts.append(_splinter(kit, f"GroundWoodCharFlake_{i}", (rng.uniform(-1.8, 1.8), rng.uniform(-0.95, 0.95),
                                                                 rng.uniform(0.006, 0.024)),
                               Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0.05)),
                               Vector((0, 1, 0)), Vector((0, 0, 1)),
                               rng.uniform(0.010, 0.035), rng.uniform(0.003, 0.010),
                               rng.uniform(0.045, 0.15), mat, SEED + 500 + i))

    _settle(parts)
    obj = kit.join(parts, "SM_RuinFallenTimbers", unwrap=True, reshade=True, smooth_angle=48)
    meshes = [obj]
    for idx, ratio in enumerate((0.44, 0.18), 1):
        lod = rocks.lod(kit, obj, f"SM_RuinFallenTimbers_LOD{idx}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=48))
    print("HOMESTEAD_LODS", [rocks.triangles(o) for o in meshes])
    return meshes
