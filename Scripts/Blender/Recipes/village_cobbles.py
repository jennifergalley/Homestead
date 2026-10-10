"""Village cobbles: an irregular ~3 m patch of set granite cobbles (setts) bedded in earth, for the
paved apron in front of a well, a cottage door or a lane end.

Real-object research (written before modeling):
- English village setts are small hand-dressed granite blocks, 12-22 cm across, laid on a sand or
  earth bed so each sits slightly proud of its neighbours. Original laying leaves joints of 1-3 cm
  filled with sandy earth and lime grit, which silts up, sprouts moss and eventually grass.
- Each sett is not a cube: its top is pitched a degree or two off level, its arrises are rounded
  by wheels and boots, and the face is worn into a shallow dome (the "polished" look of old cobbles),
  with darker wet-grey granite in the worn middle and lichen/moss near the joints.
- A patch laid by a cottage or well is never a rectangle: the outer setts are sparser, sink into the
  packed earth around them and are half buried, so the paving fades into the track rather than
  ending on a line. Relief above the joint bed is only a few centimetres.

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Units are metres, Z up. SM_VillageCobbles PIVOT: bottom centre; z = 0 is the ground line. The patch
is lowest-relief (setts rise 1-4 cm above the joint bed) and its feathered rim sinks 2-3 cm below z = 0,
so it settles into uneven or tilted terrain; several overlapping instances read as one apron.
No collision. SM_VillageCobbles_LOD1 is a decimated copy sharing the UVs and textures.
"""
import math
import os
import random
import sys
from pathlib import Path

import numpy as np
from mathutils import Vector, noise

RECIPES = Path(__file__).resolve().parent
for path in (str(RECIPES), str(RECIPES.parent)):
    if path not in sys.path:
        sys.path.insert(0, path)
from stone_building import masonry  # noqa: E402
from stone_building.wall import stone_material  # noqa: E402

NAME = "VillageCobbles"
DESCRIPTION = ("Irregular ~3 m patch of set granite cobbles with earth/lime joints, low relief and a "
               "feathered rim that sinks below the ground line; no collision; pivot = bottom centre (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 60000
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 2048 if DRAFT else 4096, "samples": 16 if DRAFT else 48, "repack": False}
BEAUTY = {"meshes": {"SM_VillageCobbles": {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.02), "ground": "origin"}}}
NOTES = {"SM_VillageCobbles": ("Pivot = bottom centre on the ground line (z = 0). Irregular outline about 3 m "
                               "across; setts 12-22 cm, 1-4 cm above the joint bed; the rim sinks 2-3 cm below z = 0.")}
REPORT = {"pivot": "bottom centre, z = 0 ground line", "collision": "none", "max_relief_m": 0.04}

EXTENT = 1.72           # half-size of the UV square, m
OUTLINE_R = 1.38        # mean outline radius, m
FEATHER = 0.30          # width of the sparse, sinking rim band, m
SPACING = 0.168         # mean sett spacing, m
JITTER = 0.044          # lattice jitter, m
JOINT = 0.018           # mean joint width, m
CHAMFER = 0.012         # rounded corner size on plan, m
SUNK = -0.028           # rim height below the ground line
BED_TOP = 0.001         # joint bed height inside the patch
SETT_RISE = (0.007, 0.017)   # sett top above z = 0 inside the patch
DOME = 0.006            # worn-smooth crown on each sett top
TILT = math.radians(2.0)
BED_CELL = 0.05


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def radius_at(theta):
    c, s = math.cos(theta), math.sin(theta)
    return OUTLINE_R * (1.0 + 0.10 * noise.noise(Vector((c * 1.3, s * 1.3, 2.2)))
                        + 0.05 * noise.noise(Vector((c * 3.6, s * 3.6, 5.1)))
                        + 0.02 * noise.noise(Vector((c * 9.0, s * 9.0, 8.3))))


def interior(x, y):
    """1 well inside the outline, 0 at and beyond it, smooth across the feather band."""
    r = math.hypot(x, y)
    return smoothstep(0.0, FEATHER, radius_at(math.atan2(y, x)) - r)


def clip(poly, point, normal):
    """Sutherland-Hodgman against the half-plane (p - point) . normal <= 0."""
    out = []
    for i, a in enumerate(poly):
        b = poly[(i + 1) % len(poly)]
        da = (a[0] - point[0]) * normal[0] + (a[1] - point[1]) * normal[1]
        db = (b[0] - point[0]) * normal[0] + (b[1] - point[1]) * normal[1]
        if da <= 0:
            out.append(a)
        if (da < 0 < db) or (db < 0 < da):
            t = da / (da - db)
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
    return out


def tidy(poly, minimum=0.004):
    out = []
    for p in poly:
        if not out or math.dist(p, out[-1]) > minimum:
            out.append(p)
    if len(out) > 1 and math.dist(out[0], out[-1]) <= minimum:
        out.pop()
    return out


def chamfer(poly, size):
    out = []
    n = len(poly)
    for i, p in enumerate(poly):
        a, b = poly[i - 1], poly[(i + 1) % n]
        la, lb = math.dist(p, a), math.dist(p, b)
        c = min(size, 0.35 * la, 0.35 * lb)
        out.append((p[0] + (a[0] - p[0]) / la * c, p[1] + (a[1] - p[1]) / la * c))
        out.append((p[0] + (b[0] - p[0]) / lb * c, p[1] + (b[1] - p[1]) / lb * c))
    return out


def centroid(poly):
    area = cx = cy = 0.0
    for i, a in enumerate(poly):
        b = poly[(i + 1) % len(poly)]
        cross = a[0] * b[1] - b[0] * a[1]
        area += cross
        cx += (a[0] + b[0]) * cross
        cy += (a[1] + b[1]) * cross
    if abs(area) < 1e-9:
        return (sum(p[0] for p in poly) / len(poly), sum(p[1] for p in poly) / len(poly))
    return (cx / (3.0 * area), cy / (3.0 * area))


def make_sites(rng):
    angle = math.radians(17.0)
    ca, sa = math.cos(angle), math.sin(angle)
    sites = []
    count = int(OUTLINE_R * 1.3 / SPACING) + 2
    for j in range(-count, count + 1):
        for i in range(-count, count + 1):
            x = (i + 0.5 * (j % 2)) * SPACING * 1.12
            y = j * SPACING * 0.866 * 0.92
            x, y = x * ca - y * sa, x * sa + y * ca
            x += rng.uniform(-JITTER, JITTER)
            y += rng.uniform(-JITTER, JITTER)
            w = interior(x, y)
            reach = math.hypot(x, y) < radius_at(math.atan2(y, x)) + 0.05
            # Setts thin out and drop away across the feathered rim.
            if reach and rng.random() < smoothstep(0.0, 0.45, w) ** 0.6 + 0.05:
                sites.append((x, y, w, rng.uniform(-1.0, 1.0)))
    return sites


def sett_cells(sites, rng):
    """Voronoi-style cells: bisector half-plane clips between neighbours, offset for the joint
    and for each stone's size bias, inside a rough per-sett outline so rim stones are isolated."""
    cells = []
    for index, (x, y, w, bias) in enumerate(sites):
        outline_r = rng.uniform(0.10, 0.145)
        sides = 14
        poly = [(x + math.cos(2 * math.pi * k / sides + 0.3) * outline_r * (1 + rng.uniform(-0.10, 0.10)),
                 y + math.sin(2 * math.pi * k / sides + 0.3) * outline_r * (1 + rng.uniform(-0.10, 0.10)))
                for k in range(sides)]
        for other, (ox, oy, _, obias) in enumerate(sites):
            if other == index:
                continue
            d = math.hypot(ox - x, oy - y)
            if d > 0.45 or d < 1e-6:
                continue
            n = ((ox - x) / d, (oy - y) / d)
            half = d / 2.0 - JOINT / 2.0 + (bias - obias) * 0.014
            poly = clip(poly, (x + n[0] * half, y + n[1] * half), n)
            if len(poly) < 3:
                break
        poly = tidy(poly)
        if len(poly) >= 3:
            cells.append((poly, (x, y, w, bias)))
    return cells


def add_sett(m, poly, site, mat, rng, seed_vec):
    poly = chamfer(poly, CHAMFER)
    cx, cy = centroid(poly)
    w, bias = site[2], site[3]
    inside = smoothstep(0.05, 0.75, w)
    rise = rng.uniform(*SETT_RISE) + 0.004 * bias
    base = SUNK + (rise - SUNK) * inside
    tilt_dir = rng.uniform(0.0, 2 * math.pi)
    tilt_amp = math.tan(TILT) * rng.uniform(0.2, 1.0)
    tx, ty = math.cos(tilt_dir) * tilt_amp, math.sin(tilt_dir) * tilt_amp
    dist = [math.dist(p, (cx, cy)) for p in poly]
    mean_r = sum(dist) / len(dist)
    dome = DOME * rng.uniform(0.7, 1.25) * inside + 0.002

    def top(px, py):
        q = min(math.dist((px, py), (cx, cy)) / mean_r, 1.0)
        return base + tx * (px - cx) + ty * (py - cy) + dome * (1.0 - q * q) - dome * 0.18

    def inset(p, amount):
        d = math.dist(p, (cx, cy))
        k = min(amount, 0.6 * d)
        return (p[0] + (cx - p[0]) / d * k, p[1] + (cy - p[1]) / d * k)

    rings = []
    for amount, drop in ((0.0, 0.0), (0.0, 0.006), (0.007, 0.0025), (0.020, 0.0)):
        ring = []
        for p in poly:
            q = inset(p, amount) if amount else p
            wobble = 0.0011 * noise.noise(Vector((q[0] * 38 + seed_vec, q[1] * 38, 1.0)))
            ring.append((q[0], q[1], top(q[0], q[1]) - drop + wobble))
        rings.append(ring)
    bottom = [(p[0], p[1], min(p[2] for p in rings[0]) - 0.022) for p in poly]
    rings.insert(0, bottom)
    apex = (cx, cy, top(cx, cy) + 0.0006)

    n = len(poly)
    verts = [v for ring in rings for v in ring] + [apex]
    faces = []
    for r in range(len(rings) - 1):
        for i in range(n):
            j = (i + 1) % n
            faces.append((r * n + i, r * n + j, (r + 1) * n + j, (r + 1) * n + i))
    top_ring = (len(rings) - 1) * n
    for i in range(n):
        faces.append((top_ring + i, top_ring + (i + 1) % n, len(verts) - 1))
    offset, tint = m.identity(0.14)
    m.add(verts, faces, mat, offset, tint, extra={"rim": np.full(len(verts), inside)})


def joint_material(kit, name):
    """Sandy earth and lime grit between the setts, mossy in the damp, fading to packed earth at the rim."""
    g = kit.mats.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (3.1, 7.7, 1.9))
    rim = g.node("ShaderNodeAttribute", attribute_name="rim", attribute_type="GEOMETRY").outputs["Fac"]
    grit = g.noise(seeded, scale=110.0, detail=4.0).outputs["Fac"]
    color = g.ramp(grit, [(0.0, (0.060, 0.046, 0.032)), (0.5, (0.115, 0.092, 0.066)), (1.0, (0.200, 0.176, 0.140))])
    pebble_d = g.voronoi(seeded, scale=70.0).outputs["Distance"]
    pebbles = g.remap(pebble_d, 0.30, 0.16)
    color = g.mix(color, (0.125, 0.118, 0.100), g.math("MULTIPLY", pebbles, 0.55))
    lime = g.noise(g.vmath("ADD", seeded, (5.0, 1.0, 2.0)), scale=700.0, detail=1.0).outputs["Fac"]
    color = g.mix(color, (0.30, 0.28, 0.24), g.remap(lime, 0.73, 0.77, 0.0, 0.5))
    damp = g.remap(g.noise(g.vmath("ADD", seeded, (9.0, 4.0, 3.0)), scale=14.0, detail=5.0).outputs["Fac"], 0.40, 0.58)
    mossy = g.math("MULTIPLY", damp, g.remap(rim, 0.25, 0.9))
    moss = g.ramp(g.noise(seeded, scale=260.0, detail=3.0).outputs["Fac"],
                  [(0.0, (0.016, 0.034, 0.007)), (1.0, (0.060, 0.098, 0.016))])
    color = g.mix(color, moss, g.math("MULTIPLY", mossy, 0.92))
    cavity = g.math("SUBTRACT", 1.0, g.ao(distance=0.02, samples=16))
    color = g.mix(color, (0.012, 0.009, 0.006), g.remap(cavity, 0.15, 0.7, 0.0, 0.35))
    # Beyond the rim the bed is the packed earth of the lane around it.
    color = g.mix(color, (0.052, 0.038, 0.026), g.remap(rim, 0.6, 0.05))
    g.set("Base Color", color)
    g.set("Roughness", 0.93)
    height = g.math("ADD", g.math("MULTIPLY", g.noise(seeded, scale=900.0, detail=2.0).outputs["Fac"], 0.4),
                    g.math("MULTIPLY", pebbles, 0.7))
    g.set("Normal", g.bump(height, strength=0.7, distance=0.003))
    return g.mat


def add_bed(m, mat, rng):
    """Heightfield of joint earth under and between the setts; the rim sinks below the ground line."""
    n = int(2 * EXTENT / BED_CELL) + 1
    index = {}
    verts, rims = [], []
    for i in range(n):
        for j in range(n):
            x, y = -EXTENT + i * BED_CELL, -EXTENT + j * BED_CELL
            r = math.hypot(x, y)
            reach = radius_at(math.atan2(y, x)) + 0.14
            if r > reach:
                continue
            w = interior(x, y)
            edge = smoothstep(0.0, 1.0, w)
            z = SUNK * 1.3 + (BED_TOP - SUNK * 1.3) * edge
            # Past the outline the bed keeps diving so its stepped grid edge stays buried.
            z -= 0.018 * smoothstep(0.0, 0.14, r - radius_at(math.atan2(y, x)))
            z += 0.0015 * noise.noise(Vector((x * 9.0, y * 9.0, 3.3)))
            index[(i, j)] = len(verts)
            verts.append((x, y, z))
            rims.append(w)
    faces = []
    for i in range(n - 1):
        for j in range(n - 1):
            quad = [index.get(k) for k in ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))]
            if None not in quad:
                faces.append(tuple(quad))
    m.add(verts, faces, mat, extra={"rim": np.array(rims)})


def build(kit):
    rng = random.Random(4417)
    m = masonry.Masonry(seed=77)
    stone = stone_material(kit, "M_VillageCobblesStone", 21, lichen=0.14, moss=0.05, soil=0.25,
                           soil_height=0.12, patina=0.9)
    joint = joint_material(kit, "M_VillageCobblesJoint")
    sites = make_sites(rng)
    for number, (poly, site) in enumerate(sett_cells(sites, rng)):
        add_sett(m, poly, site, 0, rng, number * 3.7)
    add_bed(m, 1, rng)
    obj = m.build("SM_VillageCobbles", [stone, joint])

    masonry.unwrap(obj, shrink={1: 0.5})
    return masonry.finish(kit, obj, lod_ratios=(0.3,))
