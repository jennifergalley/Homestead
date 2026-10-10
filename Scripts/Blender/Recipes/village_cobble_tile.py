"""Village cobble tile: a 4 m x 4 m SQUARE of set granite cobbles (setts) that tiles with itself
edge to edge, so the village square can be paved almost wall to wall with a grid of instances.

Real-object research (written before modeling):
- English village setts are small hand-dressed granite blocks, 12-22 cm across, laid on a sand or
  earth bed so each sits slightly proud of its neighbours. Joints are 1-3 cm of sandy earth and lime
  grit that silts up, sprouts moss and settles a centimetre or so below the stone tops.
- Setts are not cubes: each top is pitched a degree or two off level, the arrises are rounded by
  wheels and boots, and the face is worn into a shallow dome. A square is laid in staggered courses
  (like brick) or in loose fans, so the joints form an irregular Voronoi-like network and never a grid.
- Relief over the joint bed is only 1-3 cm; the whole surface sits a little below the surrounding
  ground line so it reads as paving bedded in earth.

Tiling method (the reason this is not village_cobbles.py): the sett lattice is generated as a PERIODIC
Voronoi/power diagram. Sites sit on a staggered lattice with an even number of courses and period
exactly 4 m, jittered, and every cell is clipped against its neighbours including the +-4 m images.
Each cell is built at its own position and at all eight +-4 m translations, every triangle is cut
cleanly on the square |x|,|y| <= 2, and cut points are computed from identical edges on both sides,
so the cut vertices of opposite tile edges are equal after a 4 m shift: setts that straddle a tile edge
are split exactly on the line and rejoin when tiles touch, at any yaw 0/90/180/270. Cut setts keep the
cell's original texture coordinates, so the granite grain continues across the seam. No feathered rim.

Each sett is a low pillow: an apex, a top ring (cell inset by the joint half-width) and a joint-bed
ring shared with its neighbours at the groove bottom (z -1.2 cm), 3n triangles per sett; fine grain,
lichen and joint grit live in the baked textures and normal map, not in geometry. A skirt on the four
edges drops to z = -3 cm and is buried between neighbouring tiles; at the apron's outer edge it hides
the cut so the paving sits in the ground. UVs are one planar top-down projection (periodic, no overlaps).

Everything below is original project-authored procedural geometry/materials; no downloaded sources.
Units are metres, Z up. SM_VillageCobbleTile PIVOT: bottom centre of the footprint (x = y = 0); z = 0 is
the ground line: sett tops at z 0.004-0.02, joint bottoms at z -0.012, skirt base at z = -0.03.
No collision. SM_VillageCobbleTile_LOD1 is a periodic 30 x 30 heightfield sampled from LOD0 sharing
its UVs and textures (its edge vertices are identical, so LOD1 tiles also join without cracks).
"""
import math
import os
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

RECIPES = Path(__file__).resolve().parent
for path in (str(RECIPES), str(RECIPES.parent)):
    if path not in sys.path:
        sys.path.insert(0, path)
from stone_building import masonry  # noqa: E402
from stone_building.wall import stone_material  # noqa: E402

NAME = "VillageCobbleTile"
DESCRIPTION = ("Seamless 4 m x 4 m tile of set granite cobbles with earth/lime joints and very low relief; "
               "periodic sett lattice so tiles join edge to edge at any yaw; no collision; pivot = bottom "
               "centre (original).")
COLLISION = "none"
TRIANGLE_BUDGET = 9500
PROVENANCE = "Original project-authored procedural geometry and procedural materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 2048 if DRAFT else 4096, "samples": 16 if DRAFT else 48, "repack": False,
        "maps": ("basecolor", "roughness", "normal")}
BEAUTY = {"meshes": {"SM_VillageCobbleTile": {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.0), "ground": "origin"}}}
NOTES = {"SM_VillageCobbleTile": (
    "Pivot = bottom centre (x = y = 0), z = 0 ground line. 4.0 x 4.0 m footprint tiles edge to edge at yaw "
    "0/90/180/270: the sett lattice is periodic and cut on the square. Sett tops z 0.004-0.02, joint bottoms "
    "z -0.012, skirt base z -0.03. One planar UV island (u = (x+2)/4, v = (y+2)/4).")}
REPORT = {"pivot": "bottom centre of the footprint, z = 0 ground line (skirt base at z = -0.03)",
          "collision": "none", "tile_m": 4.0, "max_relief_m": 0.03}

HALF = 2.0              # half edge of the tile, m
PERIOD = 2.0 * HALF
RELAX_STEPS = 60        # site relaxation passes
RELAX_RADIUS = 0.27     # site repulsion radius, m (about one sett spacing plus a margin)
SIZE_BIAS = 0.020       # per-sett bisector shift (bigger/smaller setts), m
INSET = 0.017           # top ring inset from the joint centre line (half joint plus rounded arris), m
GROOVE = -0.012         # joint bed height, m
TOP_RISE = (0.004, 0.015)   # sett top centre above z = 0
DOME = 0.004            # worn-smooth crown, m
TILT = math.radians(2.0)
SKIRT_Z = -0.03         # buried base of the tile, m
MIN_FACE_AREA = 1e-8    # m^2; smaller faces are degenerate for the FBX importer
SLIVER_RATIO = 1e-4     # area / longest-edge^2 below this is a hairline sliver
SKIRT_MIN_LENGTH = 1e-4  # m; shorter boundary edges get no skirt quad
EDGE_EPS = 1e-9
LOD_GRID = 30           # LOD1 heightfield quads per side


def clip(poly, point, normal):
    """Sutherland-Hodgman against the half-plane (p - point) . normal <= 0 (xy tuples)."""
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


def tidy(poly, minimum=0.003):
    out = []
    for p in poly:
        if not out or math.dist(p, out[-1]) > minimum:
            out.append(p)
    if len(out) > 1 and math.dist(out[0], out[-1]) <= minimum:
        out.pop()
    return out


def centroid(poly):
    area = cx = cy = 0.0
    for i, a in enumerate(poly):
        b = poly[(i + 1) % len(poly)]
        cross = a[0] * b[1] - b[0] * a[1]
        area += cross
        cx += (a[0] + b[0]) * cross
        cy += (a[1] + b[1]) * cross
    return (cx / (3.0 * area), cy / (3.0 * area)) if abs(area) > 1e-12 else poly[0]


def inset_ring(poly, amount):
    """Mitered inward offset keeping vertex correspondence (poly is counter-clockwise)."""
    n = len(poly)
    normals = []
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        d = math.dist(a, b)
        normals.append((-(b[1] - a[1]) / d, (b[0] - a[0]) / d))
    ring = []
    for i, p in enumerate(poly):
        n1, n2 = normals[i - 1], normals[i]
        denom = max(1.0 + n1[0] * n2[0] + n1[1] * n2[1], 0.4)
        ring.append((p[0] + (n1[0] + n2[0]) / denom * amount, p[1] + (n1[1] + n2[1]) / denom * amount))
    return ring


def make_sites(rng):
    """C4-symmetric site set: sites relaxed in one 2 x 2 m quadrant (mutual repulsion against all rotated and
    +-4 m images, so cells stay sett-sized across the quadrant borders), then rotated 0/90/180/270 about the
    tile centre. Returns (x, y, bias, orbit, spin) with spin the number of quarter turns."""
    per_x, per_y = 9, 10
    quad = []
    for j in range(per_y):
        for i in range(per_x):
            quad.append([(i + 0.5 + rng.uniform(-0.5, 0.5)) * HALF / per_x,
                         (j + 0.5 + rng.uniform(-0.5, 0.5)) * HALF / per_y])
    quad = np.array(quad)
    rot = np.array([[0.0, -1.0], [1.0, 0.0]])
    for _ in range(RELAX_STEPS):
        images = []
        for spin in range(4):
            turned = quad @ np.linalg.matrix_power(rot, spin).T
            for sx in (-1, 0, 1):
                for sy in (-1, 0, 1):
                    images.append(turned + np.array([sx * PERIOD, sy * PERIOD]))
        images = np.concatenate(images)
        push = np.zeros_like(quad)
        for k, p in enumerate(quad):
            delta = p - images
            dist = np.hypot(delta[:, 0], delta[:, 1])
            near = (dist < RELAX_RADIUS) & (dist > 1e-9)
            push[k] = (delta[near] / dist[near, None] * (RELAX_RADIUS - dist[near, None])).sum(axis=0)
        quad = np.clip(quad + push * 0.25, 0.02, HALF - 0.02)
    sites = []
    for orbit, (x, y) in enumerate(quad):
        bias = rng.uniform(-1.0, 1.0)
        for spin in range(4):
            px, py = float(x), float(y)
            for _ in range(spin):
                px, py = -py, px
            sites.append((px, py, bias, orbit, spin))
    return sites


def periodic_cells(sites):
    """Exact Voronoi/power cells; neighbours include the +-4 m images so the diagram is periodic."""
    images = [(sx * PERIOD, sy * PERIOD) for sx in (-1, 0, 1) for sy in (-1, 0, 1)]
    cells = []
    for index, (x, y, bias, orbit, spin) in enumerate(sites):
        poly = [(x - 0.34, y - 0.34), (x + 0.34, y - 0.34), (x + 0.34, y + 0.34), (x - 0.34, y + 0.34)]
        for other, (ox, oy, obias, _, _) in enumerate(sites):
            for sx, sy in images:
                if other == index and sx == 0 and sy == 0:
                    continue
                px, py = ox + sx, oy + sy
                d = math.hypot(px - x, py - y)
                if d > 0.75 or d < 1e-9:
                    continue
                n = ((px - x) / d, (py - y) / d)
                half = d / 2.0 + (bias - obias) * SIZE_BIAS * 0.5
                poly = clip(poly, (x + n[0] * half, y + n[1] * half), n)
                if len(poly) < 3:
                    break
            if len(poly) < 3:
                break
        poly = tidy(poly)
        if len(poly) >= 3:
            cells.append((poly, (x, y, bias, orbit, spin)))
    return cells


def groove_z(x, y):
    """Joint bed height: gently uneven, periodic in x and y and unchanged by a quarter turn."""
    return GROOVE + 0.0012 * math.cos(2 * math.pi * 8 * x / PERIOD) * math.cos(2 * math.pi * 8 * y / PERIOD)


def sett_triangles(poly, site, rng, spin=0):
    """Triangles (three vertices of (x, y, z, joint) each) and a slot (0 stone, 1 joint)."""
    cx, cy = centroid(poly)
    bias = site[2]
    inradius = min(abs((poly[(i + 1) % len(poly)][0] - poly[i][0]) * (cy - poly[i][1])
                       - (poly[(i + 1) % len(poly)][1] - poly[i][1]) * (cx - poly[i][0]))
                   / max(math.dist(poly[i], poly[(i + 1) % len(poly)]), 1e-9) for i in range(len(poly)))
    amount = min(INSET, 0.42 * inradius)
    ring_a = inset_ring(poly, amount)
    base = rng.uniform(*TOP_RISE) + 0.002 * bias
    tilt_dir = rng.uniform(0.0, 2 * math.pi) + spin * math.pi / 2.0
    tilt_amp = math.tan(TILT) * rng.uniform(0.2, 1.0)
    tx, ty = math.cos(tilt_dir) * tilt_amp, math.sin(tilt_dir) * tilt_amp
    mean_r = sum(math.dist(p, (cx, cy)) for p in ring_a) / len(ring_a)
    dome = DOME * rng.uniform(0.7, 1.25)

    def top(px, py):
        q = min(math.dist((px, py), (cx, cy)) / mean_r, 1.0)
        return base + tx * (px - cx) + ty * (py - cy) + dome * (1.0 - q * q) - dome * 0.2

    n = len(poly)
    a = [(p[0], p[1], top(p[0], p[1]), 0.0) for p in ring_a]
    c = [(p[0], p[1], groove_z(p[0], p[1]), 1.0) for p in poly]
    apex = (cx, cy, top(cx, cy) + 0.0005, 0.0)
    tris = []
    for i in range(n):
        j = (i + 1) % n
        tris.append(((a[i], a[j], apex), 0))
        tris.append(((a[i], c[i], c[j]), 1))
        tris.append(((a[i], c[j], a[j]), 1))
    return tris


def lerp(a, b, t):
    return tuple(a[k] + (b[k] - a[k]) * t for k in range(len(a)))


def clip_axis(poly, axis, bound, sign):
    """Keep the part of the polygon with sign * (p[axis] - bound) <= 0, cut exactly on the line."""
    out = []
    for i, a in enumerate(poly):
        b = poly[(i + 1) % len(poly)]
        da, db = sign * (a[axis] - bound), sign * (b[axis] - bound)
        if da <= 0:
            out.append(a)
        if (da < 0 < db) or (db < 0 < da):
            cut = list(lerp(a, b, da / (da - db)))
            cut[axis] = bound
            out.append(tuple(cut))
    return out


def clip_to_tile(verts):
    poly = list(verts)
    for axis in (0, 1):
        for bound, sign in ((HALF, 1.0), (-HALF, -1.0)):
            poly = clip_axis(poly, axis, bound, sign)
            if len(poly) < 3:
                return []
    area = 0.0
    for i, p in enumerate(poly):
        q = poly[(i + 1) % len(poly)]
        area += p[0] * q[1] - q[0] * p[1]
    return poly if abs(area) > 2e-10 else []


def on_side(p, q):
    """Outward direction when both points lie on the same tile edge, else None."""
    for axis in (0, 1):
        for bound, sign in ((HALF, 1.0), (-HALF, -1.0)):
            if abs(p[axis] - bound) < EDGE_EPS and abs(q[axis] - bound) < EDGE_EPS:
                return axis, sign
    return None


def add_piece(m, tris, shift, offset, tint):
    """Clip one translated copy of a sett to the tile and add it (shared vertices, skirt on tile edges).
    pcoord is the position before the +-4 m translation, so cut setts continue across the seam."""
    sx, sy = shift
    pool, keys, faces, slots = [], {}, [], []

    def vid(p):
        key = (round(p[0], 6), round(p[1], 6), round(p[2], 6))
        if key not in keys:
            keys[key] = len(pool)
            pool.append((p, (p[0] - sx, p[1] - sy, p[2], p[3])))
        return keys[key]

    skirt = []
    for verts, slot in tris:
        moved = [(v[0] + sx, v[1] + sy, v[2], v[3]) for v in verts]
        if (max(v[0] for v in moved) <= -HALF or min(v[0] for v in moved) >= HALF
                or max(v[1] for v in moved) <= -HALF or min(v[1] for v in moved) >= HALF):
            continue
        poly = clip_to_tile(moved)
        if not poly:
            continue
        ids = [vid(p) for p in poly]
        for k in range(1, len(poly) - 1):
            if len({ids[0], ids[k], ids[k + 1]}) == 3 and usable_triangle(poly[0], poly[k], poly[k + 1]):
                faces.append((ids[0], ids[k], ids[k + 1]))
                slots.append(slot)
        for k, p in enumerate(poly):
            q = poly[(k + 1) % len(poly)]
            if on_side(p, q) and math.dist(p[:2], q[:2]) >= SKIRT_MIN_LENGTH:
                skirt.append((p, q))
    for p, q in skirt:
        low_p, low_q = (p[0], p[1], SKIRT_Z, 1.0), (q[0], q[1], SKIRT_Z, 1.0)
        a, b, c, d = vid(p), vid(low_p), vid(low_q), vid(q)
        # Two triangles, not a quad, so the FBX importer never has to split a skirt face.
        faces.extend([(a, b, c), (a, c, d)])
        slots.extend([1, 1])
    if not faces:
        return
    verts = np.array([[p[0][0], p[0][1], p[0][2]] for p in pool])
    pcoord = np.array([p[1][:3] for p in pool])
    joint = np.array([p[1][3] for p in pool])
    base = len(m.faces)
    m.add(verts, faces, 0, offset, tint, pcoord=pcoord, extra={"joint": joint})
    for k, slot in enumerate(slots):
        m.face_mats[base + k] = slot


def joint_material(kit, name):
    """Sandy earth and lime grit between the setts, mossy in the damp groove bottoms."""
    g = kit.mats.Graph(name)
    p = g.coord()
    seeded = g.vmath("ADD", p, (3.1, 7.7, 1.9))
    joint = g.node("ShaderNodeAttribute", attribute_name="joint", attribute_type="GEOMETRY").outputs["Fac"]
    grit = g.noise(seeded, scale=110.0, detail=4.0).outputs["Fac"]
    color = g.ramp(grit, [(0.0, (0.060, 0.046, 0.032)), (0.5, (0.115, 0.092, 0.066)), (1.0, (0.200, 0.176, 0.140))])
    pebble_d = g.voronoi(seeded, scale=70.0).outputs["Distance"]
    pebbles = g.remap(pebble_d, 0.30, 0.16)
    color = g.mix(color, (0.125, 0.118, 0.100), g.math("MULTIPLY", pebbles, 0.55))
    lime = g.noise(g.vmath("ADD", seeded, (5.0, 1.0, 2.0)), scale=700.0, detail=1.0).outputs["Fac"]
    color = g.mix(color, (0.30, 0.28, 0.24), g.remap(lime, 0.73, 0.77, 0.0, 0.5))
    damp = g.remap(g.noise(g.vmath("ADD", seeded, (9.0, 4.0, 3.0)), scale=14.0, detail=5.0).outputs["Fac"], 0.40, 0.58)
    mossy = g.math("MULTIPLY", damp, g.remap(joint, 0.25, 0.95))
    moss = g.ramp(g.noise(seeded, scale=260.0, detail=3.0).outputs["Fac"],
                  [(0.0, (0.016, 0.034, 0.007)), (1.0, (0.060, 0.098, 0.016))])
    color = g.mix(color, moss, g.math("MULTIPLY", mossy, 0.92))
    # Dark earth where the groove bottoms out, lighter dry grit near the stone arris.
    color = g.mix(color, (0.030, 0.022, 0.014), g.remap(joint, 0.55, 1.0, 0.0, 0.55))
    g.set("Base Color", color)
    g.set("Roughness", 0.93)
    height = g.math("ADD", g.math("MULTIPLY", g.noise(seeded, scale=900.0, detail=2.0).outputs["Fac"], 0.4),
                    g.math("MULTIPLY", pebbles, 0.7))
    g.set("Normal", g.bump(height, strength=0.7, distance=0.003))
    return g.mat


def set_planar_uvs(obj):
    mesh = obj.data
    layer = mesh.uv_layers.active or mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        co = mesh.vertices[loop.vertex_index].co
        layer.data[loop.index].uv = ((co.x + HALF) / PERIOD, (co.y + HALF) / PERIOD)


def usable_triangle(a, b, c):
    """False for zero-area and hairline triangles: Unreal's FBX importer refuses to triangulate a mesh holding them."""
    ab = (b[0] - a[0], b[1] - a[1], b[2] - a[2])
    ac = (c[0] - a[0], c[1] - a[1], c[2] - a[2])
    cross = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
    area = 0.5 * math.sqrt(cross[0] ** 2 + cross[1] ** 2 + cross[2] ** 2)
    longest = max(math.dist(a, b), math.dist(b, c), math.dist(c, a))
    return area >= MIN_FACE_AREA and area >= SLIVER_RATIO * longest * longest


def build_lod1(kit, lod0):
    """Periodic heightfield sampled (3 x 3 footprint average) from LOD0 with an identical edge ring."""
    bvh = BVHTree.FromObject(lod0, bpy.context.evaluated_depsgraph_get())
    step = PERIOD / LOD_GRID

    def wrap(v):
        return (v + HALF) % PERIOD - HALF

    def sample(x, y):
        heights = []
        for ox in (-0.33, 0.0, 0.33):
            for oy in (-0.33, 0.0, 0.33):
                px, py = wrap(x + ox * step), wrap(y + oy * step)
                hit = bvh.ray_cast(Vector((px, py, 1.0)), Vector((0, 0, -1)), 2.0)
                heights.append(hit[0].z if hit[0] is not None else GROOVE)
        return sum(heights) / len(heights)

    z = [[sample(-HALF + i * step, -HALF + j * step) for j in range(LOD_GRID)] for i in range(LOD_GRID)]
    count = LOD_GRID + 1
    verts, faces = [], []
    for i in range(count):
        for j in range(count):
            verts.append((-HALF + i * step, -HALF + j * step, z[i % LOD_GRID][j % LOD_GRID]))
    for i in range(LOD_GRID):
        for j in range(LOD_GRID):
            a, b, c, d = i * count + j, (i + 1) * count + j, (i + 1) * count + j + 1, i * count + j + 1
            faces.extend([(a, b, c), (a, c, d)])
    base = len(verts)

    def skirt(path):
        nonlocal verts
        for p, q in zip(path, path[1:]):
            lo_p, lo_q = len(verts), len(verts) + 1
            verts += [(verts[p][0], verts[p][1], SKIRT_Z), (verts[q][0], verts[q][1], SKIRT_Z)]
            faces.extend([(p, lo_p, lo_q), (p, lo_q, q)])

    bottom = [i * count for i in range(count)]
    right = [LOD_GRID * count + j for j in range(count)]
    top = [i * count + LOD_GRID for i in range(count)][::-1]
    left = [j for j in range(count)][::-1]
    skirt(bottom)
    skirt(right)
    skirt(top)
    skirt(left)
    mesh = bpy.data.meshes.new("SM_VillageCobbleTile_LOD1")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("SM_VillageCobbleTile_LOD1", mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.uv_layers.new(name="UVMap")
    set_planar_uvs(obj)
    for material in lod0.data.materials:
        mesh.materials.append(material)
    return kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=80.0)


def build(kit):
    rng = random.Random(9021)
    m = masonry.Masonry(seed=91)
    stone = stone_material(kit, "M_VillageCobbleTileStone", 23, lichen=0.14, moss=0.05, soil=0.25,
                           soil_height=0.12, patina=0.9)
    joint = joint_material(kit, "M_VillageCobbleTileJoint")
    cells = periodic_cells(make_sites(rng))
    shifts = [(sx * PERIOD, sy * PERIOD) for sx in (-1, 0, 1) for sy in (-1, 0, 1)]
    identities = {}

    for poly, site in cells:
        orbit, spin = site[3], site[4]
        # The four quarter-turn copies of a sett share every random draw (tilt turns with it), which makes the
        # whole tile 4-fold symmetric: a tile at yaw 0/90/180/270 is the same mesh, so any yaw joins any other.
        tris = sett_triangles(poly, site, random.Random(9021 * 1000 + orbit), spin)
        if orbit not in identities:
            identities[orbit] = m.identity(0.14)
        offset, tint = identities[orbit]
        # Cut setts keep the cell's own (unshifted) coordinates so grain and joint grit continue across the seam.
        for shift in shifts:
            add_piece(m, tris, shift, offset, tint)
    obj = m.build("SM_VillageCobbleTile", [stone, joint])
    set_planar_uvs(obj)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=80.0)
    return [obj, build_lod1(kit, obj)]
