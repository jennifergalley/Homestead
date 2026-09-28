"""Ruined manor detail: dense common ivy mat clinging to a broken wall face.

Real-object research: common ivy (Hedera helix) climbs old masonry as a dense, self-clinging
mat, not as separate bead-curtain strands. Woody runners flatten against stone, branch repeatedly
and root into cracks with brown adventitious rootlets. Juvenile climbing shoots carry overlapping
3-5 lobed, dark glossy leaves with pale veins, usually 4-10 cm long, arranged like shingles over
the wall face. Coverage is ragged, heavier low and along damp edges, thinner near the broken top,
with a mound spilling over the wall head and only a few free shoots hanging from the edges.

Geometry (Unreal cm): about 250 cm wide, 200 cm down one wall face, depth off the wall roughly
10-25 cm. Pivot/orientation: origin is the wall-head outer-face centre line; +X runs along the
wall, +Y goes back across the 56 cm wall top, -Y is out/down the visible hanging face, and Z=0 is
the broken wall head. Blender (x, y, z) imports as Unreal (x, -y, z), so the mat protrudes toward
Unreal +Y. Leaves are alpha-free real geometry on a shared non-repacked UV atlas.
"""
import math
import os
import random
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_rocks as rocks

NAME = "RuinIvy"
DESCRIPTION = "Ruined manor detail: dense common-ivy wall mat with woody clinging runners, rootlet fuzz and thousands of alpha-free lobed leaves."
COLLISION = "none"
TRIANGLE_BUDGET = 100000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 80, "repack": False,
        "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.10, -0.92),
          "views": ["hero", "detail", "eye"], "detail_distance": 0.75, "eye_distance": 3.0}
REPORT = {
    "unreal_frame": "Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "Wall-head outside face centre line: X along wall, +Y over the 56 cm wall top, -Y out from/down the visible wall face, Z=0 at broken top.",
    "placement": "Place the origin on the top/front edge of a RuinWall run. The mat hangs into Blender -Y / Unreal +Y and negative Z.",
    "foliage": "Leaves are individual alpha-free lobed meshes using a shared non-repacked UV atlas; no masked cards.",
}

SEED = 184071
LEAF_DARK = (0.00, 0.38, 0.00, 1.00)
LEAF_YOUNG = (0.38, 0.64, 0.00, 1.00)
STEM_RECT = (0.66, 0.83, 0.00, 1.00)
ROOT_RECT = (0.84, 1.00, 0.00, 1.00)


def _rect(rect, u, v):
    u0, u1, v0, v1 = rect
    return (u0 + (u1 - u0) * u, v0 + (v1 - v0) * v)


def _assign_uvs(obj, uvs):
    layer = obj.data.uv_layers["UVMap"].data
    for poly in obj.data.polygons:
        for loop_index, vertex_index in zip(poly.loop_indices, poly.vertices):
            layer[loop_index].uv = uvs[vertex_index]
    return obj


def _basis(normal, tip_dir):
    z_axis = Vector(normal).normalized()
    y_axis = Vector(tip_dir) - z_axis * Vector(tip_dir).dot(z_axis)
    if y_axis.length < 1e-5:
        y_axis = Vector((0, 0, -1))
    y_axis.normalize()
    x_axis = y_axis.cross(z_axis).normalized()
    return x_axis, y_axis, z_axis


def _matrix(origin, x_axis, y_axis, z_axis):
    return Matrix(((x_axis.x, y_axis.x, z_axis.x, origin[0]),
                   (x_axis.y, y_axis.y, z_axis.y, origin[1]),
                   (x_axis.z, y_axis.z, z_axis.z, origin[2]),
                   (0, 0, 0, 1)))


def _ivy_outline(lobes=5):
    # Hedera helix juvenile leaves: broad, shallow, rounded lobes with entire margins.
    if lobes == 5:
        left = [(0.50, 0.000), (0.430, 0.080), (0.340, 0.165), (0.235, 0.285),
                (0.185, 0.365), (0.265, 0.430), (0.365, 0.485), (0.295, 0.610),
                (0.340, 0.675), (0.440, 0.745), (0.480, 0.900), (0.50, 1.000)]
    else:
        left = [(0.50, 0.000), (0.425, 0.095), (0.325, 0.215), (0.205, 0.385),
                (0.295, 0.475), (0.390, 0.560), (0.455, 0.815), (0.50, 1.000)]
    return left + [(1.0 - u, v) for u, v in reversed(left[:-1])]


def _leaf(kit, name, origin, normal, tip_dir, length, material, seed, uv_rect, young=False):
    rng = random.Random(seed)
    lobes = 3 if rng.random() < (0.36 if young else 0.22) else 5
    outline = _ivy_outline(lobes)
    center = (0.50 + rng.uniform(-0.012, 0.012), 0.47 + rng.uniform(-0.020, 0.020))
    width = length * (0.74 if lobes == 5 else 0.60)
    verts, coords, uvs = [], [], []

    def local(u, v):
        # Convex, leathery leaf: shallow V around the palmate midrib plus curled edges.
        x = (u - 0.5) * width
        y = v * length
        midrib = -0.0025 * (1.0 - abs(u - 0.5) * 2.0) * math.sin(math.pi * v)
        cupping = 0.010 * (abs(u - 0.5) * 2.0) ** 1.7 * math.sin(math.pi * v)
        ripple = 0.0018 * math.sin((u * 19.0 + v * 13.0 + seed) * 2.1)
        return Vector((x, y, midrib + cupping + ripple))

    verts.append(local(*center))
    coords.append(Vector((center[0], center[1], 0)))
    uvs.append(_rect(uv_rect, center[0], center[1]))
    for u, v in outline:
        uu = max(0.0, min(1.0, u + rng.uniform(-0.010, 0.010)))
        vv = max(0.0, min(1.0, v + rng.uniform(-0.010, 0.010)))
        verts.append(local(uu, vv))
        coords.append(Vector((uu, vv, 0)))
        uvs.append(_rect(uv_rect, uu, vv))
    faces = [(0, 1 + i, 1 + ((i + 1) % len(outline))) for i in range(len(outline))]
    obj = kit.mesh(name, [tuple(v) for v in verts], faces, material)
    kit.tag_coords(obj.data, coords)
    _assign_uvs(obj, uvs)
    x_axis, y_axis, z_axis = _basis(normal, tip_dir)
    obj.matrix_world = _matrix(origin, x_axis, y_axis, z_axis)
    return obj


def _tube(kit, name, pts, radii, material, sides, rect):
    obj = kit.tube(name, [tuple(p) for p in pts], radii=radii, sides=sides, material=material)
    kit.assign_tube_uvs(obj, rect, sides, len(pts))
    return obj


def _rootlets(kit, parts, rng, base, along, count, material, phase):
    base, along = Vector(base), Vector(along)
    for i in range(count):
        t = rng.random()
        p = base + along * t + Vector((rng.uniform(-0.025, 0.025), rng.uniform(-0.004, 0.006), rng.uniform(-0.018, 0.018)))
        end = p + Vector((rng.uniform(-0.020, 0.020), rng.uniform(0.010, 0.035), rng.uniform(-0.020, 0.020)))
        mid = p.lerp(end, 0.55) + Vector((rng.uniform(-0.010, 0.010), 0.006, rng.uniform(-0.010, 0.010)))
        parts.append(_tube(kit, f"Rootlet_{phase}_{i}", [p, mid, end],
                           [0.0019, 0.0012, 0.00045], material, 4, ROOT_RECT))


def _ivy_leaf_material(kit, name, base, tip, vein, roughness, bronze=0.0, underside=False):
    g = kit.mats.Graph(name)
    u, v, _ = g.separate(g.coord())
    mottle = g.noise(g.combine(u, v, 0.0), scale=18.0, detail=5.0).outputs["Fac"]
    fine = g.noise(g.combine(g.math("MULTIPLY", u, 2.0), g.math("MULTIPLY", v, 2.0), 0.0),
                   scale=65.0, detail=3.0).outputs["Fac"]
    tissue = g.mix(base, tip, g.remap(v, 0.25, 0.95))
    tissue = g.mix(tissue, tuple(max(0.0, c * 0.72) for c in base), g.remap(mottle, 0.45, 0.70, 0.0, 0.32))
    if bronze:
        age = g.noise(g.combine(g.math("MULTIPLY", u, 0.7), g.math("MULTIPLY", v, 1.3), 0.0),
                      scale=8.0, detail=3.0).outputs["Fac"]
        tissue = g.mix(tissue, (0.085, 0.052, 0.026), g.remap(age, 0.62, 0.82, 0.0, bronze))
    du = g.math("SUBTRACT", u, 0.5)
    dv = g.math("SUBTRACT", v, 0.075)
    veins = None
    for angle, length, width in ((0.0, 0.93, 0.014), (-0.48, 0.70, 0.012), (0.48, 0.70, 0.012),
                                 (-0.88, 0.52, 0.011), (0.88, 0.52, 0.011)):
        dx, dy = math.sin(angle), math.cos(angle)
        along = g.math("ADD", g.math("MULTIPLY", du, dx), g.math("MULTIPLY", dv, dy))
        perp = g.math("ABSOLUTE", g.math("SUBTRACT", g.math("MULTIPLY", du, dy), g.math("MULTIPLY", dv, dx)))
        line = g.math("MULTIPLY", g.remap(perp, width, 0.0, 0.0, 1.0),
                      g.math("MULTIPLY", g.remap(along, 0.015, 0.095, 0.0, 1.0),
                             g.remap(along, length, length * 0.72, 0.0, 1.0)))
        veins = line if veins is None else g.math("MAXIMUM", veins, line)
    tissue = g.mix(tissue, vein, veins)
    if underside:
        tissue = g.mix(tissue, (0.070, 0.095, 0.060), 0.55)
    g.set("Base Color", tissue)
    g.set("Roughness", g.remap(fine, 0.25, 0.75, roughness - 0.05, roughness + 0.08))
    g.set("Subsurface Weight", 0.10 if not underside else 0.05)
    height = g.math("ADD", g.math("MULTIPLY", fine, 0.12), g.math("MULTIPLY", veins, 0.70))
    g.set("Normal", g.bump(height, strength=0.30, distance=0.00045))
    return g.mat


def _make_mats(kit):
    m = kit.mats
    leaf_dark = _ivy_leaf_material(kit, "M_RuinIvyDarkGlossLeaves", (0.014, 0.045, 0.014),
                                   (0.022, 0.060, 0.018), (0.150, 0.165, 0.120), 0.38)
    leaf_young = _ivy_leaf_material(kit, "M_RuinIvyYoungNewLeaves", (0.038, 0.090, 0.026),
                                    (0.060, 0.120, 0.035), (0.165, 0.190, 0.120), 0.40)
    leaf_bronze = _ivy_leaf_material(kit, "M_RuinIvyBronzedOldLeaves", (0.014, 0.038, 0.013),
                                     (0.026, 0.050, 0.017), (0.130, 0.140, 0.105), 0.42, bronze=0.45)
    leaf_under = _ivy_leaf_material(kit, "M_RuinIvyPaleUndersides", (0.045, 0.075, 0.042),
                                    (0.065, 0.095, 0.055), (0.135, 0.150, 0.110), 0.58, underside=True)
    stem = m.bark("M_RuinIvyWoodyFlattenedStems", light=(0.125, 0.096, 0.064),
                  dark=(0.040, 0.030, 0.020), scale=0.45, roughness=0.86, lichen=0.22)
    root = m.bark("M_RuinIvyAdventitiousRootFuzz", light=(0.112, 0.086, 0.055),
                  dark=(0.036, 0.027, 0.018), scale=0.28, roughness=0.92, lichen=0.12)
    return leaf_dark, leaf_young, leaf_bronze, leaf_under, stem, root


def build(kit):
    rng = random.Random(SEED)
    leaf_dark, leaf_young, leaf_bronze, leaf_under, stem, root = _make_mats(kit)
    parts = []

    # Flattened woody scaffold: many branching runners pressed against the wall face.
    main_count = 8 if not DRAFT else 4
    for v in range(main_count):
        x0 = rng.uniform(-1.04, 1.04)
        z0 = rng.uniform(-0.08, -0.42)
        length = rng.uniform(0.65, 1.25)
        pts = []
        steps = 7
        slant = rng.uniform(-0.16, 0.16)
        for i in range(steps):
            t = i / (steps - 1)
            x = x0 + slant * t + 0.05 * math.sin(t * 6.0 + v) + rng.uniform(-0.020, 0.020)
            y = rng.uniform(-0.030, -0.052) - 0.010 * t
            z = z0 - length * t + 0.04 * math.sin(t * 5.0 + v * 0.31)
            pts.append(Vector((x, y, z)))
        r0 = rng.uniform(0.006, 0.013)
        parts.append(_tube(kit, f"ClingingMainStem_{v:02d}", pts,
                           [r0 * (1.0 - 0.55 * i / (steps - 1)) for i in range(steps)], stem, 8, STEM_RECT))
        _rootlets(kit, parts, rng, pts[1], pts[-1] - pts[1], 8 if not DRAFT else 3, root, v)
        # Lateral branches, mostly horizontal/diagonal like a net under the leaves.
        for b in range(3 if not DRAFT else 1):
            t0 = rng.uniform(0.18, 0.82)
            anchor = pts[int(t0 * (steps - 1))]
            side = rng.choice((-1, 1))
            reach = rng.uniform(0.14, 0.30)
            branch = [anchor,
                      anchor + Vector((side * reach * 0.45, rng.uniform(-0.008, 0.010), rng.uniform(-0.10, 0.05))),
                      anchor + Vector((side * reach, rng.uniform(-0.018, 0.012), rng.uniform(-0.18, 0.08)))]
            parts.append(_tube(kit, f"SideBranch_{v:02d}_{b}", branch,
                               [r0 * 0.50, r0 * 0.36, r0 * 0.22], stem, 6, STEM_RECT))

    # Dense shingled wall leaves, but in natural clumps rather than a uniform curtain.
    leaf_index = 0
    clumps = []
    clump_count = 26 if not DRAFT else 12
    for c in range(clump_count):
        top = rng.uniform(-0.02, -0.45)
        if c % 11 == 0:
            drop = rng.uniform(1.25, 1.95)
        else:
            drop = rng.choice([rng.uniform(0.30, 0.70), rng.uniform(0.45, 1.05)])
        clumps.append((rng.uniform(-1.05, 1.05), top, min(-0.18, top - drop), rng.uniform(0.10, 0.28)))
    for ci, (cx, z_top, z_bottom, width) in enumerate(clumps):
        leaf_slots = (70 if not DRAFT else 30) if z_bottom > -1.15 else (84 if not DRAFT else 38)
        side_bias = rng.uniform(-0.18, 0.18)
        for j in range(leaf_slots):
            t = rng.random()
            # Shingled growth: denser lower half, but some upper bare stone remains.
            z = z_top + (z_bottom - z_top) * (t ** rng.uniform(0.80, 1.25))
            lower = min(1.0, max(0.0, -z / 2.0))
            spread = width * (0.45 + 0.95 * math.sin(math.pi * min(1.0, t)))
            x = cx + side_bias * t + rng.gauss(0.0, spread)
            if abs(x) > 1.20 or rng.random() < 0.10 * abs(x):
                continue
            y = rng.uniform(-0.070, -0.150) - 0.010 * lower
            normal = Vector((rng.uniform(-0.18, 0.18), -1.0, rng.uniform(0.04, 0.24))).normalized()
            # Most blades face outward and slightly upward toward the light; a minority droop.
            tip = Vector((rng.uniform(-0.55, 0.55), rng.uniform(-0.10, 0.08),
                          rng.uniform(0.15, 0.80) if rng.random() < 0.70 else rng.uniform(-0.60, 0.12))).normalized()
            young = (j > leaf_slots * 0.72 and rng.random() < 0.30) or rng.random() < 0.045
            old = (not young) and rng.random() < 0.16
            underside = rng.random() < 0.08
            size = rng.uniform(0.040, 0.100)
            mat = leaf_under if underside else (leaf_young if young else (leaf_bronze if old else leaf_dark))
            uv = LEAF_YOUNG if young else LEAF_DARK
            parts.append(_leaf(kit, f"IvyLeafFace_{leaf_index:04d}", (x, y, z + rng.uniform(-0.018, 0.018)),
                               normal, tip, size * (0.82 if young else 1.0),
                               mat, SEED + leaf_index, uv, young))
            petiole_base = Vector((x + rng.uniform(-0.010, 0.010), -0.030, z + rng.uniform(-0.020, 0.020)))
            petiole_tip = Vector((x, y, z))
            parts.append(_tube(kit, f"IvyPetioleFace_{leaf_index:04d}", [petiole_base, petiole_base.lerp(petiole_tip, 0.55), petiole_tip],
                               [0.0018, 0.0014, 0.0010], root, 4, ROOT_RECT))
            leaf_index += 1
        # A branching runner inside each clump, hugging the wall and visibly joining leaves.
        runner = []
        steps = 5
        for k in range(steps):
            t = k / (steps - 1)
            runner.append(Vector((cx + side_bias * t + 0.04 * math.sin(t * 5 + ci),
                                  rng.uniform(-0.042, -0.070),
                                  z_top + (z_bottom - z_top) * t)))
        parts.append(_tube(kit, f"ClumpRunner_{ci:02d}", runner,
                           [0.0045 * (1 - 0.45 * k / (steps - 1)) for k in range(steps)], stem, 6, STEM_RECT))
        _rootlets(kit, parts, rng, runner[0], runner[-1] - runner[0], 7 if not DRAFT else 3, root, 100 + ci)

    # Mound spilling over the wall head/top course (+Y over the 56 cm wall top).
    top_leaves = 250 if not DRAFT else 120
    for i in range(top_leaves):
        x = rng.uniform(-1.14, 1.14)
        y = rng.uniform(-0.020, 0.540)
        z = rng.uniform(-0.018, 0.145) - 0.030 * abs(x)
        normal = (Vector((rng.uniform(-0.12, 0.12), rng.uniform(-0.18, 0.18), 1.0))).normalized()
        tip = Vector((rng.uniform(-0.6, 0.6), rng.uniform(0.1, 1.0), rng.uniform(-0.2, 0.25))).normalized()
        young = rng.random() < 0.30
        mat = leaf_young if young else (leaf_bronze if rng.random() < 0.12 else leaf_dark)
        parts.append(_leaf(kit, f"IvyLeafTop_{leaf_index:04d}", (x, y, z), normal, tip,
                           rng.uniform(0.040, 0.095), mat, SEED + leaf_index,
                           LEAF_YOUNG if young else LEAF_DARK, young))
        parts.append(_tube(kit, f"IvyPetioleTop_{leaf_index:04d}", [Vector((x, max(0.0, y - 0.060), z - 0.010)), Vector((x, y, z))],
                           [0.0018, 0.0010], root, 4, ROOT_RECT))
        leaf_index += 1

    # A few loose edge shoots, deliberately sparse so the asset still reads as a mat.
    for e in range(3 if not DRAFT else 1):
        side = rng.choice((-1, 1))
        base = Vector((side * rng.uniform(1.02, 1.10), rng.uniform(-0.045, -0.08), rng.uniform(-0.05, -0.42)))
        pts = [base]
        for k in range(1, 4):
            pts.append(base + Vector((-side * rng.uniform(0.00, 0.035) * k,
                                      -0.02 * k + rng.uniform(-0.018, 0.012),
                                      -rng.uniform(0.07, 0.15) * k)))
        parts.append(_tube(kit, f"LooseEdgeShoot_{e}", pts, [0.0045 * (1 - 0.16 * k) for k in range(4)], stem, 6, STEM_RECT))
        for k in range(6 if not DRAFT else 2):
            p = pts[rng.randrange(1, len(pts))]
            normal = Vector((side * rng.uniform(0.25, 0.60), -1.0, rng.uniform(-0.15, 0.15))).normalized()
            tip = Vector((side * rng.uniform(0.2, 0.8), -0.15, -1.0)).normalized()
            parts.append(_leaf(kit, f"IvyLeafLoose_{leaf_index:04d}", tuple(p), normal, tip,
                               rng.uniform(0.040, 0.075), leaf_dark, SEED + leaf_index, LEAF_DARK, False))
            leaf_index += 1

    obj = kit.join(parts, "SM_RuinIvy", pivot=None, unwrap=False, reshade=True, smooth_angle=68)
    meshes = [obj]
    for idx, ratio in enumerate((0.35, 0.095), 1):
        lod = rocks.lod(kit, obj, f"SM_RuinIvy_LOD{idx}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=68))
    print("HOMESTEAD_LODS", [rocks.triangles(o) for o in meshes], "leaves", leaf_index)
    return meshes


def after_bake(kit, obj):
    if obj.name != "SM_RuinIvy" or "BeautyWallMock" in bpy.data.objects:
        return
    stone = kit.mats.granite("M_RuinIvyBeautyWallMockGranite", grain=0.003, scale=0.50, patina=0.75,
                             lichen=0.50, moss=0.25, soil=0.45, seed=71.0)
    wall = kit.box("BeautyWallMock", (2.85, 0.70, 2.50), location=(0.0, 0.35, -1.25),
                   material=stone, bevel=0.012, bevel_segments=2)
    wall.hide_select = True
