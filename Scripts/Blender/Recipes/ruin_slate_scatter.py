"""Ruined manor detail: drift of broken Delabole roofing slates against a wall.

Real-object research: Delabole slate from north Cornwall is a blue-grey to grey-green roofing
stone, split thin (about 6-10 mm) along cleavage. Historic whole slates near the countess/
duchess range are roughly 40-56 cm long by 20-30 cm wide, with two nail holes near the head.
After a burnt roof collapses, slates slide and pile deepest against the wall, then thin outward:
mostly quadrilateral/irregular chunks broken along cleavage and straight-ish fracture lines,
with chipped stepped edges, moss/soil/leaf litter in the gaps, lichen on exposed faces and rust
stains around old holes.

Geometry (Unreal cm): a roughly 300 x 200 cm scatter, thickest along -X (wall side) to 15-25 cm
deep, thinning toward +X. Pivot is ground centre. Everything is original procedural geometry and
materials; no downloaded geometry or textures.
"""
import math
import os
import random
from pathlib import Path
import sys

from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_rocks as rocks

NAME = "RuinSlateScatter"
DESCRIPTION = "Ruined manor detail: stacked grey-green/blue-grey Delabole slate roof debris, thick against the -X wall side."
COLLISION = "none"
TRIANGLE_BUDGET = 200000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 4096, "samples": 16 if DRAFT else 96,
        "maps": ("basecolor", "roughness", "normal", "ao")}
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.55, -0.06, 0.15),
          "views": ["hero", "detail", "eye"], "eye_distance": 3.2}
REPORT = {
    "unreal_frame": "Extents are Unreal local cm: Blender (x, y, z) imports as Unreal (x, -y, z).",
    "pivot": "Ground centre of the scatter at (0, 0, 0).",
    "placement": "The -X side is deliberately thick and should sit against the ruin wall; debris thins toward +X into the room/yard.",
    "collision_note": "No collision: low walkable decoration.",
}

SEED = 184052


def _mat_frame(loc, yaw, pitch, roll):
    return (Matrix.Translation(Vector(loc)) @ Matrix.Rotation(yaw, 4, "Z") @
            Matrix.Rotation(pitch, 4, "X") @ Matrix.Rotation(roll, 4, "Y"))


def _poly(rng, whole, width, length):
    if whole:
        hw = width / 2
        pts = [(-hw, 0.0), (hw, rng.uniform(-0.010, 0.010)),
               (hw * rng.uniform(0.93, 1.03), length), (-hw * rng.uniform(0.93, 1.03), length + rng.uniform(-0.010, 0.012))]
    else:
        sides = rng.choice([4, 4, 4, 5, 6])
        pts = []
        # Broken roof slate chunks: straight-ish fracture edges, few true needles.
        for i in range(sides):
            a = -math.pi * 0.90 + math.tau * i / sides + rng.uniform(-0.22, 0.22)
            rx = width * rng.uniform(0.34, 0.56)
            ry = length * rng.uniform(0.34, 0.56)
            pts.append((math.cos(a) * rx, length * 0.50 + math.sin(a) * ry))
        pts.sort(key=lambda p: math.atan2(p[1] - length * 0.50, p[0]))
    return pts


def _subdivide_edge(a, b, cuts, rng):
    out = []
    va, vb = Vector((a[0], a[1], 0)), Vector((b[0], b[1], 0))
    edge = vb - va
    if edge.length < 1e-6:
        return [a]
    normal = Vector((-edge.y, edge.x, 0)).normalized()
    for i in range(cuts):
        t = i / cuts
        p = va.lerp(vb, t)
        chip = normal * rng.uniform(-0.006, 0.006)
        # Small stepped chips along split-cleavage edges.
        if rng.random() < 0.25:
            chip += edge.normalized() * rng.uniform(-0.006, 0.006)
        out.append((p.x + chip.x, p.y + chip.y))
    return out


def _boundary(poly, rng, whole):
    points = []
    for a, b in zip(poly, poly[1:] + poly[:1]):
        cuts = rng.randint(3, 6) if whole else rng.randint(2, 5)
        points.extend(_subdivide_edge(a, b, cuts, rng))
    return points


def _slate(kit, name, loc, yaw, width, length, thickness, whole, material, seed, pitch=0.0, roll=0.0):
    rng = random.Random(seed)
    base_poly = _poly(rng, whole, width, length)
    ring = _boundary(base_poly, rng, whole)
    center2 = Vector((sum(p[0] for p in ring) / len(ring), sum(p[1] for p in ring) / len(ring), 0))
    mat = _mat_frame(loc, yaw, pitch, roll)
    verts, coords = [], []

    def local_z(x, y, top):
        bend = 0.0016 * math.sin((x * 17.0 + y * 9.0 + seed) * 1.7)
        ripple = 0.0010 * math.sin((x * 43.0 - y * 31.0 + seed) * 0.6)
        return (thickness if top else 0.0) + (bend + ripple if top else -0.0006 * math.sin(x * 80 + seed))

    # Top and bottom centre/ring for fan faces.
    bottom_center_i = len(verts)
    verts.append(tuple(mat @ Vector((center2.x, center2.y, local_z(center2.x, center2.y, False)))))
    coords.append(Vector((center2.x, center2.y, 0.0)))
    top_center_i = len(verts)
    verts.append(tuple(mat @ Vector((center2.x, center2.y, local_z(center2.x, center2.y, True)))))
    coords.append(Vector((center2.x, center2.y, thickness)))
    bottom_ring, top_ring = [], []
    for x, y in ring:
        bottom_ring.append(len(verts))
        verts.append(tuple(mat @ Vector((x, y, local_z(x, y, False)))))
        coords.append(Vector((x, y, 0.0)))
        top_ring.append(len(verts))
        verts.append(tuple(mat @ Vector((x, y, local_z(x, y, True)))))
        coords.append(Vector((x, y, thickness)))

    faces = []
    n = len(ring)
    for i in range(n):
        j = (i + 1) % n
        faces.append((bottom_center_i, bottom_ring[j], bottom_ring[i]))
        faces.append((top_center_i, top_ring[i], top_ring[j]))

    # Real side thickness with split laminae: several stepped side quads through the 6-10 mm edge.
    side_layers = 5 if not DRAFT else 3
    side_indices = []
    for layer in range(side_layers + 1):
        t = layer / side_layers
        row = []
        for x, y in ring:
            step = rng.uniform(-0.0015, 0.0015) if layer not in (0, side_layers) else 0.0
            row.append(len(verts))
            verts.append(tuple(mat @ Vector((x + step, y - step * 0.3, thickness * t))))
            coords.append(Vector((x, y, thickness * t)))
        side_indices.append(row)
    for layer in range(side_layers):
        for i in range(n):
            j = (i + 1) % n
            faces.append((side_indices[layer][i], side_indices[layer][j], side_indices[layer + 1][j], side_indices[layer + 1][i]))

    obj = kit.mesh(name, verts, faces, material)
    kit.tag_coords(obj.data, coords)
    return obj


def _disc(kit, name, loc, yaw, pitch, roll, local_xy, z, radius, material, squash=(1.0, 1.0)):
    mat = _mat_frame(loc, yaw, pitch, roll)
    center = mat @ Vector((local_xy[0], local_xy[1], z))
    obj = kit.cylinder(name, radius, 0.0018, location=tuple(center), material=material, sides=16, bevel=0.0)
    obj.scale.x *= squash[0]
    obj.scale.y *= squash[1]
    obj.rotation_euler = (Matrix.Rotation(yaw, 4, "Z") @ Matrix.Rotation(pitch, 4, "X") @ Matrix.Rotation(roll, 4, "Y")).to_euler()
    return obj


def _ground_mesh(kit, name, rng, material):
    cols, rows = (52, 36) if not DRAFT else (26, 18)
    verts, faces, coords = [], [], []
    for iy in range(rows + 1):
        y = -0.98 + 1.96 * iy / rows
        for ix in range(cols + 1):
            x = -1.48 + 2.96 * ix / cols
            wall = max(0.0, 1.0 - (x + 1.48) / 2.96)
            mound = 0.022 * wall ** 1.8 * math.exp(-(y / 0.82) ** 4)
            relief = 0.006 * math.sin(ix * 0.9 + iy * 1.7) + 0.004 * math.sin(ix * 2.2 + rng.random())
            z = -0.010 + mound + relief * (0.35 + 0.65 * wall)
            verts.append((x, y, z))
            coords.append(Vector((x, y, z)))
    for iy in range(rows):
        for ix in range(cols):
            a = iy * (cols + 1) + ix
            faces.append((a, a + 1, a + cols + 2, a + cols + 1))
    obj = kit.mesh(name, verts, faces, material)
    kit.tag_coords(obj.data, coords)
    return obj


def _soil_patch(kit, name, loc, radius, material, seed):
    rng = random.Random(seed)
    n = 14
    verts = [(loc[0], loc[1], loc[2] + 0.004)]
    for i in range(n):
        a = math.tau * i / n
        r = radius * rng.uniform(0.45, 1.05)
        # Broken feathered outline, edges dipping below the placement plane.
        edge_z = loc[2] - rng.uniform(0.003, 0.012)
        verts.append((loc[0] + math.cos(a) * r * rng.uniform(0.8, 1.4),
                      loc[1] + math.sin(a) * r * rng.uniform(0.55, 1.05),
                      edge_z))
    faces = [(0, i, 1 + (i % n)) for i in range(1, n + 1)]
    return kit.mesh(name, verts, faces, material)


def _leaf(kit, name, loc, yaw, scale, material):
    pts = [(-0.040, 0.000), (-0.018, 0.030), (0.006, 0.045), (0.030, 0.026),
           (0.044, -0.004), (0.015, -0.030), (-0.014, -0.038), (-0.036, -0.020)]
    c, s = math.cos(yaw), math.sin(yaw)
    verts = [(loc[0] + scale * (x * c - y * s), loc[1] + scale * (x * s + y * c),
              loc[2] + 0.001 * math.sin((x + y) * 70.0)) for x, y in pts]
    return kit.mesh(name, verts, [tuple(range(len(verts)))], material)


def _settle(parts):
    lo = min((obj.matrix_world @ v.co).z for obj in parts for v in obj.data.vertices)
    for obj in parts:
        obj.location.z -= lo


def build(kit):
    rng = random.Random(SEED)
    m = kit.mats
    slate_mats = [
        m.slate("M_RuinSlateScatterDelaboleBlueGrey", dark=(0.020, 0.026, 0.032), light=(0.060, 0.074, 0.084),
                lichen=0.035, moss=0.0, seed=52.0),
        m.slate("M_RuinSlateScatterDelaboleGreenGrey", dark=(0.022, 0.032, 0.030), light=(0.058, 0.076, 0.070),
                lichen=0.025, moss=0.0, seed=53.0),
        m.slate("M_RuinSlateScatterDarkerWetSlate", dark=(0.014, 0.019, 0.024), light=(0.045, 0.055, 0.064),
                lichen=0.015, moss=0.0, seed=54.0),
    ]
    soil = m.soil("M_RuinSlateScatterLoamAndSlateDust", damp=(0.060, 0.047, 0.032),
                  dry=(0.160, 0.135, 0.092), seed=19.0)
    moss = kit.material("M_RuinSlateScatterGapMoss", (0.030, 0.040, 0.028), roughness=0.96)
    hole = kit.material("M_RuinSlateScatterNailHoleDark", (0.005, 0.004, 0.003), roughness=0.99)

    parts = []
    for i in range(14 if not DRAFT else 5):
        x = rng.uniform(-1.32, 0.20)
        y = rng.uniform(-0.82, 0.82)
        wall = max(0.0, 1.0 - (x + 1.38) / 2.38)
        z = -0.006 + rng.uniform(0.0, 0.025 * wall)
        mat = moss if rng.random() < 0.35 else soil
        parts.append(_soil_patch(kit, f"GapMossSoil_{i:02d}", (x, y, z), rng.uniform(0.035, 0.115), mat, SEED + 900 + i))

    total = 520 if not DRAFT else 240
    whole_target = 10 if not DRAFT else 4
    for i in range(total):
        # Bias hard into a wall-side talus fan; only a tail of fragments reaches +X.
        if rng.random() < 0.90:
            u = rng.random() ** 2.2
            x = -1.40 + u * 1.90
        else:
            x = rng.uniform(0.10, 1.05)
        wall = max(0.0, 1.0 - (x + 1.34) / 2.55)
        y_span = 0.92 - 0.20 * wall
        y = rng.uniform(-y_span, y_span)
        layers = int(2 + 11 * wall + rng.random() * 4 * wall)
        layer = rng.randrange(max(1, layers))
        z = 0.007 + layer * rng.uniform(0.007, 0.014) + wall * rng.uniform(0.004, 0.035)
        whole = i < whole_target and rng.random() < 0.82
        width = rng.uniform(0.18, 0.23) if whole else rng.uniform(0.060, 0.24)
        length = rng.uniform(0.28, 0.34) if whole else rng.uniform(0.10, 0.34)
        thickness = rng.uniform(0.008, 0.014)
        yaw = rng.uniform(-0.45, 0.45) if wall > 0.45 else rng.uniform(-math.pi, math.pi)
        pitch = rng.uniform(-0.08, 0.08) + wall * rng.uniform(-0.02, 0.16)
        roll = rng.uniform(-0.12, 0.12)
        if i < (42 if not DRAFT else 18):
            # Steeper wall-side slates leaning into the missing wall plane at -X.
            x = rng.uniform(-1.42, -1.20)
            y = rng.uniform(-0.72, 0.72)
            z = rng.uniform(0.018, 0.105)
            yaw = rng.uniform(-0.35, 0.35)
            pitch = rng.uniform(0.30, 0.58)
            roll = rng.uniform(-0.10, 0.10)
        mat = rng.choice(slate_mats)
        parts.append(_slate(kit, f"Slate_{i:03d}", (x, y, z), yaw, width, length, thickness,
                            whole, mat, SEED + i, pitch, roll))
        if whole:
            head_y = length * rng.uniform(0.72, 0.83)
            for j, sx in enumerate((-0.19, 0.19)):
                local = (sx * width + rng.uniform(-0.006, 0.006), head_y + rng.uniform(-0.012, 0.012))
                parts.append(_disc(kit, f"NailHole_{i:03d}_{j}", (x, y, z), yaw, pitch, roll,
                                   local, thickness + 0.002, rng.uniform(0.009, 0.013), hole))

    _settle(parts)
    obj = kit.join(parts, "SM_RuinSlateScatter", unwrap=True, reshade=True, smooth_angle=56)
    meshes = [obj]
    for idx, ratio in enumerate((0.42, 0.17), 1):
        lod = rocks.lod(kit, obj, f"SM_RuinSlateScatter_LOD{idx}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=56))
    print("HOMESTEAD_LODS", [rocks.triangles(o) for o in meshes])
    return meshes
