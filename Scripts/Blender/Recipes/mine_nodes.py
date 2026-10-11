"""Mine nodes: the rock face, coal seam, copper vein and gem pocket she works with her pickaxe on the
shallow level, each with a worked-out (spent) state, as eight meshes.

    .\\Scripts\\Blender\\New-Prop.ps1 mine_nodes -BeautySamples 96

Real-object research (see also ``mine/materials.py``):
- A working face in hard granite country rock is a "breast" of blocky rock already loosened along
  its joints by earlier blasting and pick work: angular blocks 20-60 cm across standing a little
  proud of the drive's wall, with fresh, paler fracture faces where pieces have come away and a few
  spalls at the foot. Mined out, it is a flatter, pale, pick-scarred scar with a little rubble.
- A coal seam shows as a dark band 30-50 cm thick between rock above and below, broken into cubic
  blocks by its cleat; worked, it leaves a slot cut back into the wall with crumbs on the floor.
- A copper lode is a steep quartz vein 10-15 cm wide crossing the face, green with malachite and
  rusty with gossan; worked, a groove where the vein was cut out.
- A gem pocket is a vug in the vein, the size of a small cupboard, lined with quartz and amethyst
  points pointing into the hollow; worked, broken crystal stumps.

Frame (Blender = Unreal here; the meshes are symmetric enough that the importer's Y mirror is
harmless): the pivot is the node's socket in HomesteadMineLayout.inc, on the gallery floor 10 cm in
front of the wall line; +X points out of the wall into the gallery, so placing a mesh with the
socket's yaw stands it against the wall. The back is buried in the wall to x = -0.75 m, below the
floor to z = -0.06 m. Everything is generated here: no scanned or downloaded geometry or textures.
"""
import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import homestead_rocks as rocks  # noqa: E402
import homestead_sdf as sdf  # noqa: E402
from mine import materials as minemats  # noqa: E402

NAME = "MineNodes"
DESCRIPTION = ("Mine nodes: rock face, coal seam, copper vein and gem pocket, each with a spent state; pivot on "
               "the floor at the node socket, +X out of the wall.")
COLLISION = "box"
TRIANGLE_BUDGET = 900000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
BAKE = {"size": 2048, "samples": 48, "repack": False}
BEAUTY = {"pose": (0, 0, -90), "ground": "origin", "views": ["hero", "detail"],
          "meshes": {f"SM_Mine{n}": {"focus": (0.1, 0.0, 0.9), "eye_distance": 3.2}
                     for n in ("RockFace", "RockFaceSpent", "CoalSeam", "CoalSeamSpent", "CopperVein",
                               "CopperVeinSpent", "GemPocket", "GemPocketSpent")}}
REPORT = {"pivot": "Node socket on the floor; +X faces into the gallery (place with the socket's yaw).",
          "extent_m": {"x": [-0.75, 0.35], "y": [-1.0, 1.0], "z": [-0.06, 1.9]}}

WALL_X = -0.10               # the gallery wall's line (HomesteadMineLayout: sockets are 10 cm proud)
BACK_X, FLOOR_Z = -0.75, -0.06
HALF_W, TOP = 0.85, 1.75     # the breast's half width and height
VOXEL = 0.022
LOD_RATIOS = (0.3, 0.08)


# ------------------------------------------------------------------ fields

def _eval(node, points):
    return np.asarray(node(np.asarray(points, dtype=np.float64), 1.0), dtype=np.float64)


def _clip(node):
    """Keep the node between the floor and the buried back."""
    def fn(p, band):
        d = node(p, band)
        return np.maximum(np.maximum(d, FLOOR_Z - p[:, 2]), BACK_X - p[:, 0]).astype(np.float32)
    lo, hi = node.lo.copy(), node.hi.copy()
    lo[0], lo[2] = max(lo[0], BACK_X - 0.05), max(lo[2], FLOOR_Z - 0.05)
    return sdf.Node(fn, lo, hi, node.k)


def _tilt(rng, amount):
    axis = rng.normal(size=3)
    return sdf.axis_angle(axis / np.linalg.norm(axis), math.radians(rng.uniform(-amount, amount)))


def blocks(seed, count, proud=(0.08, 0.24), zone=((-HALF_W, HALF_W), (0.0, TOP)), size=(0.16, 0.34),
           skip=None):
    """Joint-bounded blocks standing proud of the wall line, front faces between proud[0] and proud[1]."""
    rng = np.random.default_rng(seed)
    out, tries = [], 0
    while len(out) < count and tries < count * 20:
        tries += 1
        y = rng.uniform(*zone[0])
        z = rng.uniform(*zone[1])
        if skip is not None and skip(y, z):
            continue
        hy, hz = rng.uniform(*size), rng.uniform(size[0] * 0.7, size[1] * 0.8)
        # Taper toward the breast's edge so it melts into the wall.
        edge = max(abs(y) / (HALF_W + 0.1), abs(z - TOP * 0.45) / (TOP * 0.62))
        front = WALL_X + rng.uniform(*proud) * max(0.0, 1.0 - edge ** 3)
        hx = rng.uniform(0.22, 0.32)
        out.append(sdf.box((front - hx, y, z), (hx, hy, hz), rotation=_tilt(rng, 9.0), rounding=0.018))
    return out


def backing(front=WALL_X - 0.04):
    hx = (front - BACK_X) / 2
    return sdf.box((front - hx, 0.0, TOP / 2), (hx, HALF_W + 0.25, TOP / 2 + 0.2), rounding=0.05)


def rubble(seed, count, reach=(0.05, 0.6), spread=0.75, size=(0.04, 0.11)):
    rng = np.random.default_rng(seed)
    out = []
    for _ in range(count):
        r = rng.uniform(*size)
        x, y = rng.uniform(*reach), rng.uniform(-spread, spread)
        out.append(sdf.box((x, y, r * 0.55 - 0.01), (r * rng.uniform(0.8, 1.3), r * rng.uniform(0.7, 1.2), r * 0.6),
                           rotation=_tilt(rng, 30.0), rounding=r * 0.25))
    return out


# ------------------------------------------------------------------ crystals

def crystal_mesh(name, specs, materials):
    """Hexagonal prisms with pyramidal terminations: specs of (base, axis, radius, length, material
    index, broken). ``tip`` point attribute 0 at the base to 1 at the point."""
    verts, faces, mats, tips = [], [], [], []
    for base, axis, radius, length, mi, broken in specs:
        axis = np.asarray(axis) / np.linalg.norm(axis)
        u = np.cross(axis, (0.0, 0.0, 1.0) if abs(axis[2]) < 0.9 else (1.0, 0.0, 0.0))
        u /= np.linalg.norm(u)
        v = np.cross(axis, u)
        base = np.asarray(base) - axis * 0.02
        ring = [u * math.cos(a) + v * math.sin(a) for a in np.arange(6) * math.pi / 3 + 0.3]
        i0 = len(verts)
        for r in ring:
            verts.append(base + r * radius)
            tips.append(0.0)
        for r in ring:
            verts.append(base + axis * length + r * radius * (0.97 if not broken else 1.0))
            tips.append(0.75 if not broken else 0.35)
        for k in range(6):
            faces.append((i0 + k, i0 + (k + 1) % 6, i0 + 6 + (k + 1) % 6, i0 + 6 + k))
            mats.append(mi)
        if broken:
            faces.append(tuple(i0 + 6 + k for k in range(6)))
            mats.append(mi)
        else:
            verts.append(base + axis * (length + radius * 1.15))
            tips.append(1.0)
            apex = len(verts) - 1
            for k in range(6):
                faces.append((i0 + 6 + k, i0 + 6 + (k + 1) % 6, apex))
                mats.append(mi)
    data = bpy.data.meshes.new(name)
    data.from_pydata([tuple(p) for p in verts], [], faces)
    data.update()
    for m in materials:
        data.materials.append(m)
    data.polygons.foreach_set("material_index", np.asarray(mats, dtype=np.int32))
    attr = data.attributes.new("tip", "FLOAT", "POINT")
    attr.data.foreach_set("value", np.asarray(tips, dtype=np.float32))
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def lining(cavity_centre, radii, seed, count, amethyst_share, broken):
    """Crystals rooted on the vug's wall, pointing roughly at its middle."""
    rng = np.random.default_rng(seed)
    c, r = np.asarray(cavity_centre), np.asarray(radii)
    specs = []
    while len(specs) < count:
        d = rng.normal(size=3)
        d /= np.linalg.norm(d)
        if d[0] > 0.35:          # the open front has no wall
            continue
        p = c + d * r * 0.97
        inward = c - p + rng.normal(scale=0.08, size=3) * np.linalg.norm(r)
        deep = -d[0]             # the back of the hollow holds the amethyst
        mi = 1 if rng.random() < amethyst_share * (0.6 + 0.8 * max(deep, 0.0)) else 0
        radius = rng.uniform(0.010, 0.026) * (1.25 if mi == 1 else 1.0)
        length = rng.uniform(0.03, 0.11) if not broken else rng.uniform(0.006, 0.022)
        specs.append((p, inward, radius, length, mi, broken))
    return specs


# ------------------------------------------------------------------ meshing

def mesh_node(kit, name, field, special=None, granite=None, special_mat=None, extra=()):
    """Mesh ``field``; faces nearer ``special`` than the rest go to ``special_mat``. ``extra`` objects
    (crystals) join after the rock is unwrapped and detailed."""
    obj = sdf.mesh(name, _clip(field), voxel=VOXEL, material=granite)
    if special is not None:
        obj.data.materials.append(special_mat)
        p = rocks.coords(obj)
        polys = obj.data.polygons
        idx = np.empty(sum(len(f.vertices) for f in polys), dtype=np.int64)
        polys.foreach_get("vertices", idx)
        counts = np.array([len(f.vertices) for f in polys])
        starts = np.r_[0, np.cumsum(counts)[:-1]]
        cent = np.add.reduceat(p[idx], starts) / counts[:, None]
        d_special = _eval(special, cent)
        polys.foreach_set("material_index", (d_special < 0.012).astype(np.int32))
    rocks.unwrap(obj, angle=55.0, buried=(0.0, 0.4))
    kit.subdivide(obj, levels=1)

    def detail(points, normal):
        grain = rocks.relief(points, normal, 77, [(0.35, 0.010), (0.09, 0.004), (0.025, 0.0012)])
        out = normal[:, 0] > 0.45
        front = rocks.smoothstep(WALL_X - 0.02, WALL_X + 0.08, points[:, 0])
        fresh = np.clip(out * front * rocks.smoothstep(0.35, 0.65, sdf.fbm(points / 0.22, 3, seed=5) * 0.5 + 0.5)
                        + 0.25 * front, 0.0, 1.0)
        wet = np.clip(0.25 + 0.5 * rocks.smoothstep(0.5, 0.0, points[:, 2])
                      + 0.3 * rocks.smoothstep(0.1, 0.5, sdf.fbm(points * np.array([1.0, 1.0, 0.15]) / 0.4, 3, seed=9)),
                      0.0, 0.9)
        return grain, {"fresh": fresh, "wet": wet}
    rocks.displace(obj, detail)
    if extra:
        obj = rocks.join([obj, *extra], name)
        _repack(obj)
    kit.tag_coords(obj.data)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=50.0)
    meshes = [obj]
    for i, ratio in enumerate(LOD_RATIOS, start=1):
        meshes.append(kit.finalize(rocks.lod(kit, obj, f"{name}_LOD{i}", ratio), pivot=None, unwrap=False,
                                   reshade=True, smooth_angle=50.0))
    print(f"HOMESTEAD_MINE_NODE {name} tris={rocks.triangles(obj)}")
    return meshes


def _repack(obj):
    """Give the crystal faces their own smart-projected islands and repack everything."""
    rocks._select(obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bm = bmesh.from_edit_mesh(obj.data)
    tip = bm.verts.layers.float.get("tip")
    for f in bm.faces:
        f.select = tip is not None and any(v[tip] > 0.0 for v in f.verts) or f.material_index >= 2
    bmesh.update_edit_mesh(obj.data)
    bpy.ops.uv.smart_project(angle_limit=math.radians(50.0), island_margin=0.003, scale_to_bounds=False)
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.average_islands_scale()
    bpy.ops.uv.pack_islands(margin=0.003, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")


# ------------------------------------------------------------------ the eight nodes

def build(kit):
    granite = kit.mats.granite("M_MineNodeGranite", grains=True, grain=0.004, scale=1.5, patina=0.25, lichen=0.0,
                               moss=0.0, iron=0.4, streaks=0.35, soil=0.0, enclaves=0.4, film=0.2, seed=19.0,
                               cave=True)
    coal = minemats.coal()
    vein = minemats.copper_vein()
    quartz, amethyst = minemats.quartz(), minemats.amethyst()
    out = []

    # Rock face: a breast of loosened blocks; spent: a pale, flatter scar with spalls at the foot.
    face = sdf.smooth_union(blocks(11, 26) + [backing()], 0.015)
    out += mesh_node(kit, "SM_MineRockFace", face, granite=granite)
    spent = sdf.smooth_union(blocks(12, 7, proud=(0.0, 0.07), size=(0.14, 0.26)) + [backing(WALL_X + 0.01)]
                             + rubble(13, 9), 0.012)
    out += mesh_node(kit, "SM_MineRockFaceSpent", spent, granite=granite)

    # Coal seam: a cleated band 0.4 m thick at 0.55-0.95 m, between blocky rock.
    band_lo, band_hi = 0.55, 0.95

    def coal_band(front, seed, crumbs=0):
        """Cleat blocks: columns 9-16 cm wide across the band, each split once by a bedding parting."""
        rng = np.random.default_rng(seed)
        nodes, y = [], -HALF_W - 0.1
        while y < HALF_W + 0.1:
            w = rng.uniform(0.09, 0.16)
            mid = rng.uniform(band_lo + 0.12, band_hi - 0.12)
            for z0, z1 in ((band_lo, mid), (mid, band_hi)):
                f = front - rng.uniform(0.0, 0.05)
                nodes.append(sdf.box(((f + BACK_X) / 2, y + w / 2, (z0 + z1) / 2),
                                     ((f - BACK_X) / 2, w / 2 - 0.006, (z1 - z0) / 2 - 0.006), rounding=0.006))
            y += w
        nodes += [sdf.box((x, yy, 0.04), (0.05, 0.06, 0.04), rotation=_tilt(rng, 30), rounding=0.01)
                  for x, yy in zip(rng.uniform(0.05, 0.45, crumbs), rng.uniform(-0.6, 0.6, crumbs))]
        return sdf.union(nodes)
    def off_band(y, z):
        return band_lo - 0.12 < z < band_hi + 0.12

    seam = coal_band(WALL_X + 0.12, 21)
    rock = sdf.smooth_union(blocks(22, 18, skip=off_band) + [backing()], 0.015)
    out += mesh_node(kit, "SM_MineCoalSeam", sdf.union([rock, seam]), special=seam, granite=granite, special_mat=coal)
    seam_spent = coal_band(WALL_X - 0.16, 23, crumbs=7)
    rock_spent = sdf.smooth_union(blocks(24, 16, skip=off_band) + [backing()], 0.015)
    slot_back = WALL_X - 0.16
    rock_spent = sdf.subtract(rock_spent, sdf.box(((slot_back + 1.0) / 2, 0.0, (band_lo + band_hi) / 2),
                                                  ((1.0 - slot_back) / 2, HALF_W + 0.4, (band_hi - band_lo) / 2 + 0.02)), 0.02)
    out += mesh_node(kit, "SM_MineCoalSeamSpent", sdf.union([rock_spent, seam_spent]), special=seam_spent,
                     granite=granite, special_mat=coal)

    # Copper vein: a quartz lode dipping 68 degrees across the face, standing 3 cm proud.
    dip = math.radians(68.0)
    normal = np.array([0.0, math.sin(dip), math.cos(dip)])
    rot = sdf.rotation(axis_x=(1.0, 0.0, 0.0), axis_y=normal, axis_z=np.cross((1.0, 0.0, 0.0), normal))

    def lode(front, width=0.07):
        hx = (front - BACK_X) / 2
        return sdf.box((front - hx, 0.0, 0.85), (hx, width, 1.6), rotation=rot, rounding=0.01)

    def off_vein(y, z):
        return abs(normal[1] * y + normal[2] * (z - 0.85)) < 0.16

    vein_node = sdf.displaced(lode(WALL_X + 0.15), lambda q: 0.012 * sdf.fbm(q / 0.06, 3, seed=31), 0.012)
    rock = sdf.smooth_union(blocks(32, 20, skip=off_vein) + [backing()], 0.015)
    out += mesh_node(kit, "SM_MineCopperVein", sdf.union([rock, vein_node]), special=vein_node, granite=granite,
                     special_mat=vein)
    vein_spent = lode(WALL_X - 0.12, 0.05)
    rock = sdf.subtract(sdf.smooth_union(blocks(33, 20, skip=off_vein) + [backing()], 0.015),
                        lode(0.6, 0.11), 0.02)
    out += mesh_node(kit, "SM_MineCopperVeinSpent", sdf.union([rock, vein_spent] + rubble(34, 6, size=(0.03, 0.07))),
                     special=vein_spent, granite=granite, special_mat=vein)

    # Gem pocket: a vug in a quartz boss at chest height, lined with quartz and amethyst points.
    centre, radii = (WALL_X + 0.08, 0.0, 1.05), (0.30, 0.36, 0.26)
    boss = sdf.box((WALL_X - 0.15, 0.0, 1.05), (0.33, 0.55, 0.45), rotation=_tilt(np.random.default_rng(41), 6),
                   rounding=0.12)
    boss = sdf.displaced(boss, lambda q: 0.02 * sdf.fbm(q / 0.12, 3, seed=42), 0.02)
    hollow = sdf.ellipsoid(centre, radii)

    def off_boss(y, z):
        return abs(y) < 0.62 and abs(z - 1.05) < 0.52
    for spent_state, nm in ((False, "SM_MineGemPocket"), (True, "SM_MineGemPocketSpent")):
        rock = sdf.smooth_union(blocks(43 + spent_state, 18, skip=off_boss) + [backing()], 0.015)
        shell = sdf.subtract(boss, hollow, 0.02)
        field = sdf.union([rock, shell] + (rubble(45, 6, size=(0.02, 0.05)) if spent_state else []))
        specs = lining(centre, radii, 46 + spent_state, 70 if not spent_state else 40, 0.45, spent_state)
        crystals = crystal_mesh(nm + "Crystals", specs, [quartz, amethyst])
        # Crystal slots follow the rock's two (granite, quartz boss) in the joined mesh.
        out += mesh_node(kit, nm, field, special=shell, granite=granite, special_mat=vein, extra=[crystals])
    return out
