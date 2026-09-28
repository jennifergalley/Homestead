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
    side_layers = 4 if not DRAFT else 2
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
        m.slate("M_RuinSlateScatterDelaboleBlueGrey", dark=(0.032, 0.040, 0.046), light=(0.105, 0.118, 0.124),
                lichen=0.46, moss=0.18, seed=52.0),
        m.slate("M_RuinSlateScatterDelaboleGreenGrey", dark=(0.034, 0.046, 0.043), light=(0.105, 0.125, 0.112),
                lichen=0.40, moss=0.24, seed=53.0),
        m.slate("M_RuinSlateScatterDarkerWetSlate", dark=(0.018, 0.026, 0.032), light=(0.075, 0.088, 0.096),
                lichen=0.22, moss=0.30, seed=54.0),
    ]
    soil = m.soil("M_RuinSlateScatterLoamAndSlateDust", damp=(0.060, 0.047, 0.032),
                  dry=(0.160, 0.135, 0.092), seed=19.0)
    moss = kit.material("M_RuinSlateScatterGapMoss", (0.035, 0.068, 0.024), roughness=0.96)
    rust = kit.material("M_RuinSlateScatterRustStain", (0.092, 0.038, 0.014), roughness=0.94)
    hole = kit.material("M_RuinSlateScatterNailHoleDark", (0.005, 0.004, 0.003), roughness=0.99)
    leaf_mat = kit.material("M_RuinSlateScatterOakLeafLitter", (0.070, 0.043, 0.018), roughness=0.92)

    parts = [_ground_mesh(kit, "SoilMossReliefUnderSlate", rng, soil)]
    for i in range(26 if not DRAFT else 9):
        x = rng.uniform(-1.38, 0.85)
        y = rng.uniform(-0.90, 0.90)
        wall = max(0.0, 1.0 - (x + 1.38) / 2.38)
        z = 0.004 + rng.uniform(0.0, 0.055 * wall)
        mat = moss if rng.random() < 0.60 else soil
        parts.append(kit.sphere(f"GapMossSoil_{i:02d}", rng.uniform(0.018, 0.060), location=(x, y, z),
                                material=mat, segments=8, rings=4, scale=(1.0, 0.75, 0.12)))

    total = 175 if not DRAFT else 70
    whole_target = 24 if not DRAFT else 10
    for i in range(total):
        # Bias x toward the wall side, then stack with more layers there.
        u = rng.random() ** 1.75
        x = -1.34 + u * 2.55
        wall = max(0.0, 1.0 - (x + 1.34) / 2.55)
        y_span = 0.88 - 0.18 * wall
        y = rng.uniform(-y_span, y_span)
        layers = int(2 + 7 * wall + rng.random() * 3 * wall)
        layer = rng.randrange(max(1, layers))
        z = 0.010 + layer * rng.uniform(0.014, 0.025) + wall * rng.uniform(0.0, 0.055)
        whole = i < whole_target and rng.random() < 0.82
        width = rng.uniform(0.20, 0.30) if whole else rng.uniform(0.09, 0.30)
        length = rng.uniform(0.40, 0.56) if whole else rng.uniform(0.16, 0.48)
        thickness = rng.uniform(0.006, 0.010)
        yaw = rng.uniform(-math.pi, math.pi)
        pitch = rng.uniform(-0.10, 0.10) + wall * rng.uniform(-0.08, 0.10)
        roll = rng.uniform(-0.18, 0.18)
        mat = rng.choice(slate_mats)
        parts.append(_slate(kit, f"Slate_{i:03d}", (x, y, z), yaw, width, length, thickness,
                            whole, mat, SEED + i, pitch, roll))
        if whole:
            head_y = length * rng.uniform(0.72, 0.83)
            for j, sx in enumerate((-0.19, 0.19)):
                local = (sx * width + rng.uniform(-0.006, 0.006), head_y + rng.uniform(-0.012, 0.012))
                parts.append(_disc(kit, f"NailHole_{i:03d}_{j}", (x, y, z), yaw, pitch, roll,
                                   local, thickness + 0.002, rng.uniform(0.009, 0.013), hole))
                if rng.random() < 0.58:
                    parts.append(_disc(kit, f"RustBloom_{i:03d}_{j}", (x, y, z), yaw, pitch, roll,
                                       (local[0] + rng.uniform(-0.010, 0.018), local[1] - rng.uniform(0.010, 0.045)),
                                       thickness + 0.003, rng.uniform(0.014, 0.030), rust,
                                       squash=(rng.uniform(1.2, 2.4), rng.uniform(0.34, 0.85))))

    for i in range(50 if not DRAFT else 15):
        x = rng.uniform(-1.35, 1.15)
        y = rng.uniform(-0.85, 0.85)
        wall = max(0.0, 1.0 - (x + 1.34) / 2.55)
        z = rng.uniform(0.004, 0.045 + wall * 0.08)
        parts.append(_leaf(kit, f"LeafLitter_{i:02d}", (x, y, z), rng.uniform(0, math.tau),
                           rng.uniform(0.45, 1.15), leaf_mat))

    _settle(parts)
    obj = kit.join(parts, "SM_RuinSlateScatter", unwrap=True, reshade=True, smooth_angle=56)
    meshes = [obj]
    for idx, ratio in enumerate((0.42, 0.17), 1):
        lod = rocks.lod(kit, obj, f"SM_RuinSlateScatter_LOD{idx}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=56))
    print("HOMESTEAD_LODS", [rocks.triangles(o) for o in meshes])
    return meshes
