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

from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_rocks as rocks

NAME = "RuinIvy"
DESCRIPTION = "Ruined manor detail: dense common-ivy wall mat with woody clinging runners, rootlet fuzz and thousands of alpha-free lobed leaves."
COLLISION = "none"
TRIANGLE_BUDGET = 500000
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
    left = [(0.50, 0.000), (0.435, 0.065), (0.315, 0.115), (0.175, 0.215),
            (0.315, 0.300), (0.405, 0.365)]
    if lobes == 5:
        left += [(0.105, 0.505), (0.355, 0.590), (0.245, 0.790), (0.430, 0.750)]
    else:
        left += [(0.135, 0.510), (0.365, 0.615), (0.385, 0.755)]
    left += [(0.50, 1.000)]
    return left + [(1.0 - u, v) for u, v in reversed(left[:-1])]


def _leaf(kit, name, origin, normal, tip_dir, length, material, seed, uv_rect, young=False):
    rng = random.Random(seed)
    lobes = 3 if rng.random() < (0.42 if young else 0.18) else 5
    outline = _ivy_outline(lobes)
    center = (0.50 + rng.uniform(-0.012, 0.012), 0.47 + rng.uniform(-0.020, 0.020))
    width = length * (0.74 if lobes == 5 else 0.60)
    verts, coords, uvs = [], [], []

    def local(u, v):
        # Convex, leathery leaf: shallow V around the midrib plus curled edges.
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


def _make_mats(kit):
    m = kit.mats
    leaf_dark = m.leaf_pcoord("M_RuinIvyDarkGlossLeaves", color=(0.024, 0.060, 0.018),
                              vein=(0.085, 0.125, 0.050), tip=(0.036, 0.082, 0.023),
                              roughness=0.52, translucency=0.16, rugose=0.82, serrate_dark=0.14)
    leaf_young = m.leaf_pcoord("M_RuinIvyYoungNewLeaves", color=(0.055, 0.112, 0.032),
                               vein=(0.120, 0.160, 0.064), tip=(0.080, 0.145, 0.044),
                               roughness=0.50, translucency=0.26, rugose=0.60, serrate_dark=0.06)
    stem = m.bark("M_RuinIvyWoodyFlattenedStems", light=(0.125, 0.096, 0.064),
                  dark=(0.040, 0.030, 0.020), scale=0.45, roughness=0.86, lichen=0.22)
    root = m.bark("M_RuinIvyAdventitiousRootFuzz", light=(0.112, 0.086, 0.055),
                  dark=(0.036, 0.027, 0.018), scale=0.28, roughness=0.92, lichen=0.12)
    return leaf_dark, leaf_young, stem, root


def build(kit):
    rng = random.Random(SEED)
    leaf_dark, leaf_young, stem, root = _make_mats(kit)
    parts = []

    # Flattened woody scaffold: many branching runners pressed against the wall face.
    main_count = 36 if not DRAFT else 16
    for v in range(main_count):
        x0 = rng.uniform(-1.04, 1.04)
        z0 = rng.uniform(-0.08, -0.42)
        length = rng.uniform(1.15, 1.95)
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

    # Dense shingled wall leaves: thousands, clinging close to the face with ragged coverage.
    leaf_index = 0
    cols, rows = ((74, 58) if not DRAFT else (42, 34))
    for row in range(rows):
        z = -0.10 - 1.86 * row / max(1, rows - 1)
        lower = row / max(1, rows - 1)
        for col in range(cols):
            x = -1.22 + 2.44 * (col + 0.5 + rng.uniform(-0.38, 0.38)) / cols
            edge = abs(x) / 1.22
            top_thin = max(0.0, 0.30 - lower) / 0.30
            ragged = rng.random() < (0.05 + 0.22 * edge + 0.28 * top_thin)
            if ragged:
                continue
            # Heavier/darker lower coverage and damp side clusters.
            if rng.random() > (0.90 + 0.10 * lower - 0.10 * edge):
                continue
            y = rng.uniform(-0.110, -0.205) - 0.020 * lower
            origin = (x, y, z + rng.uniform(-0.020, 0.020))
            normal = (Vector((rng.uniform(-0.08, 0.08), -1.0, rng.uniform(-0.10, 0.10)))).normalized()
            tip = Vector((rng.uniform(-0.32, 0.32), rng.uniform(-0.07, -0.02), -1.0 + rng.uniform(-0.20, 0.18))).normalized()
            young = (lower < 0.22 and rng.random() < 0.28) or rng.random() < 0.045
            size = rng.uniform(0.060, 0.122) * (0.86 if young else 1.0)
            parts.append(_leaf(kit, f"IvyLeafFace_{leaf_index:04d}", origin, normal, tip, size,
                               leaf_young if young else leaf_dark, SEED + leaf_index,
                               LEAF_YOUNG if young else LEAF_DARK, young))
            leaf_index += 1

    # Mound spilling over the wall head/top course (+Y over the 56 cm wall top).
    top_leaves = 900 if not DRAFT else 260
    for i in range(top_leaves):
        x = rng.uniform(-1.14, 1.14)
        y = rng.uniform(0.010, 0.540)
        z = rng.uniform(0.000, 0.140) - 0.035 * abs(x)
        normal = (Vector((rng.uniform(-0.12, 0.12), rng.uniform(-0.18, 0.18), 1.0))).normalized()
        tip = Vector((rng.uniform(-0.6, 0.6), rng.uniform(0.1, 1.0), rng.uniform(-0.2, 0.25))).normalized()
        young = rng.random() < 0.30
        parts.append(_leaf(kit, f"IvyLeafTop_{leaf_index:04d}", (x, y, z), normal, tip,
                           rng.uniform(0.042, 0.090), leaf_young if young else leaf_dark,
                           SEED + leaf_index, LEAF_YOUNG if young else LEAF_DARK, young))
        leaf_index += 1

    # A few loose edge shoots, deliberately sparse so the asset still reads as a mat.
    for e in range(10 if not DRAFT else 3):
        side = rng.choice((-1, 1))
        base = Vector((side * rng.uniform(1.02, 1.10), rng.uniform(-0.055, -0.10), rng.uniform(-0.12, -1.35)))
        pts = [base]
        for k in range(1, 6):
            pts.append(base + Vector((-side * rng.uniform(0.00, 0.035) * k,
                                      -0.02 * k + rng.uniform(-0.018, 0.012),
                                      -rng.uniform(0.07, 0.15) * k)))
        parts.append(_tube(kit, f"LooseEdgeShoot_{e}", pts, [0.0045 * (1 - 0.12 * k) for k in range(6)], stem, 6, STEM_RECT))
        for k in range(10 if not DRAFT else 3):
            p = pts[rng.randrange(1, len(pts))]
            normal = Vector((side * rng.uniform(0.25, 0.60), -1.0, rng.uniform(-0.15, 0.15))).normalized()
            tip = Vector((side * rng.uniform(0.2, 0.8), -0.15, -1.0)).normalized()
            parts.append(_leaf(kit, f"IvyLeafLoose_{leaf_index:04d}", tuple(p), normal, tip,
                               rng.uniform(0.045, 0.080), leaf_dark, SEED + leaf_index, LEAF_DARK, False))
            leaf_index += 1

    obj = kit.join(parts, "SM_RuinIvy", pivot=None, unwrap=False, reshade=True, smooth_angle=68)
    meshes = [obj]
    for idx, ratio in enumerate((0.38, 0.15), 1):
        lod = rocks.lod(kit, obj, f"SM_RuinIvy_LOD{idx}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=68))
    print("HOMESTEAD_LODS", [rocks.triangles(o) for o in meshes], "leaves", leaf_index)
    return meshes
