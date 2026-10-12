"""Mine nodes: the rock face, coal seam and copper vein she works with her pickaxe on the shallow level,
each with a worked-out (spent) state, as six meshes. (No gem pocket: gems turn up at random; Jenny, 2026-10-11.)

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

NAME = "MineNodes"
DESCRIPTION = ("Mine nodes: rock face, coal seam and copper vein, each with a spent state; pivot on "
               "the floor at the node socket, +X out of the wall.")
COLLISION = "box"
TRIANGLE_BUDGET = 900000
PROVENANCE = ("Original project-authored procedural geometry; surfaces from CC0 Poly Haven photoscans in Unreal "
              "(Assets/Props/MineSurfaces/CREDITS.json).")
BAKE = None                  # shaded in Unreal with photoscans (Scripts/Mine/mine_surfaces.py)
BEAUTY = {"pose": (0, 0, -90), "ground": "origin", "views": ["hero", "detail"],
          "meshes": {f"SM_Mine{n}": {"focus": (0.1, 0.0, 0.9), "eye_distance": 3.2}
                     for n in ("RockFace", "RockFaceSpent", "CoalSeam", "CoalSeamSpent", "CopperVein",
                               "CopperVeinSpent")}}
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


def fractured(node, cell=0.2, depth=0.045, seed=0):
    """Break a block field's faces into small joint facets (each Voronoi cell knocked back by its own
    amount) so blocks read as broken rock, not sawn boxes."""
    def fn(p):
        f1, f2, r1, r2, c1, c2 = rocks.voronoi(p, cell, seed)
        edge = rocks.smoothstep(0.0, 0.025, (f2 - f1) * 0.5)
        return depth * (0.15 + 0.85 * r1) * (0.6 + 0.4 * edge) * rocks.smoothstep(FLOOR_Z, 0.06, p[:, 2])
    return sdf.displaced(node, lambda p: -fn(p), depth)


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
        out.append(sdf.box((front - hx, y, z), (hx, hy, hz), rotation=_tilt(rng, 16.0), rounding=0.018))
    return out


def backing(front=WALL_X - 0.22):
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
        wet = np.clip(0.55 + 0.35 * rocks.smoothstep(0.5, 0.0, points[:, 2])
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


# ------------------------------------------------------------------ the eight nodes (review 8)
#
# Jenny (08-tunnels): "Mineable nodes don't read as real: each is pale pillowy beige blocks stuck flat on the
# wall ... Rebuild all four from the same rock as the tunnel, cut back into the face, with the ore as
# colour/texture in the stone, not shapes stuck on." So each node is now one worked patch of the gallery wall:
# a face a few cm proud at its middle that sinks back behind the wall line at its edges, shaded in Unreal
# with the tunnel's own granite photoscan (Scripts/Mine/mine_surfaces.py maps the slots), and the ore is a
# region of that same surface in another material: a dark glossy coal band or a green-stained copper streak.
# Spent states are the same faces worked back: pick-scarred, the band or streak cut out as a slot.

ZC, HZ = 0.95, 0.98          # the face's centre height and half height
SLOT_ROCK, SLOT_ORE, SLOT_HALO = 0, 1, 2


def face(seed, proud=0.07, cell=0.3, depth=0.07):
    """A worked patch of the wall: proud at the middle, sunk behind the wall line at the edges, broken into
    quarried facets (Voronoi cells knocked back) with a little rock relief."""
    front = WALL_X
    hx = (front - BACK_X) / 2
    slab = sdf.box((front - hx, 0.0, ZC), (hx, HALF_W + 0.25, HZ + 0.2), rounding=0.02)

    def shape(p):
        # A rounded, ragged outline (a squarish one read as a slab stuck on the wall).
        e = ((np.abs(p[:, 1]) / HALF_W) ** 2.6 + (np.abs(p[:, 2] - ZC) / HZ) ** 2.6) ** (1 / 2.6)
        e = e + 0.22 * sdf.fbm(p / 0.45, 3, seed=seed)
        return proud * (1.0 - rocks.smoothstep(0.55, 1.0, e)) - 0.12 * rocks.smoothstep(0.9, 1.15, e) \
            + 0.025 * sdf.fbm(p / 0.25, 3, seed=seed + 1)
    shaped = sdf.displaced(slab, shape, proud + 0.16)
    return fractured(shaped, cell=cell, depth=depth, seed=seed)


def band_box(lo, hi, front=0.6, wave=0.0, seed=0):
    """A bed between lo and hi; ``wave`` (m) lets its edges pinch and swell like a real seam."""
    box = sdf.box(((front + BACK_X) / 2, 0.0, (lo + hi) / 2), ((front - BACK_X) / 2, HALF_W + 0.4, (hi - lo) / 2))
    return sdf.displaced(box, lambda q: wave * sdf.fbm(q / 0.35, 3, seed=seed), wave) if wave else box


def dipping(centre_z, width, dip_deg, front=0.6, wave=0.0, seed=0):
    dip = math.radians(dip_deg)
    n = np.array([0.0, math.sin(dip), math.cos(dip)])
    rot = sdf.rotation(axis_x=(1.0, 0.0, 0.0), axis_y=n, axis_z=np.cross((1.0, 0.0, 0.0), n))
    box = sdf.box(((front + BACK_X) / 2, 0.0, centre_z), ((front - BACK_X) / 2, width, 1.8), rotation=rot)
    return sdf.displaced(box, lambda q: wave * sdf.fbm(q / 0.3, 3, seed=seed), wave) if wave else box


def mesh_regions(kit, name, field, materials, regions=(), extra=()):
    """Mesh ``field``; faces inside each region node (in order, later wins) take that region's slot."""
    obj = sdf.mesh(name, _clip(field), voxel=VOXEL, material=materials[0])
    for m in materials[1:]:
        obj.data.materials.append(m)
    if regions:
        p = rocks.coords(obj)
        polys = obj.data.polygons
        idx = np.empty(sum(len(f.vertices) for f in polys), dtype=np.int64)
        polys.foreach_get("vertices", idx)
        counts = np.array([len(f.vertices) for f in polys])
        starts = np.r_[0, np.cumsum(counts)[:-1]]
        cent = np.add.reduceat(p[idx], starts) / counts[:, None]
        slot = np.zeros(len(polys), dtype=np.int32)
        for node, margin, index in regions:
            slot[_eval(node, cent) < margin] = index
        polys.foreach_set("material_index", slot)
    rocks.unwrap(obj, angle=55.0, buried=(0.0, 0.4))
    kit.subdivide(obj, levels=1)
    rocks.displace(obj, lambda pts, nrm: (rocks.relief(pts, nrm, 77, [(0.09, 0.004), (0.025, 0.0012)]), {}))
    if extra:
        obj = rocks.join([obj, *extra], name)
        _repack(obj)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=50.0)
    meshes = [obj]
    for i, ratio in enumerate(LOD_RATIOS, start=1):
        meshes.append(kit.finalize(rocks.lod(kit, obj, f"{name}_LOD{i}", ratio), pivot=None, unwrap=False,
                                   reshade=True, smooth_angle=50.0))
    print(f"HOMESTEAD_MINE_NODE {name} tris={rocks.triangles(obj)}")
    return meshes


def build(kit):
    # Slot materials are placeholders: Unreal shades them with photoscans (mine_surfaces.py WHOLE_MESHES).
    rock = kit.material("M_MineNodeRock", (0.30, 0.29, 0.27), roughness=0.85)
    coal = kit.material("M_MineNodeCoal", (0.02, 0.02, 0.022), roughness=0.25)
    copper = kit.material("M_MineNodeCopper", (0.12, 0.30, 0.22), roughness=0.6)
    stain = kit.material("M_MineNodeStain", (0.20, 0.27, 0.22), roughness=0.7)
    out = []

    # Rock face: a quarried breast in big facets; spent: worked back flatter, finely pick-scarred, spalls at the foot.
    out += mesh_regions(kit, "SM_MineRockFace", face(11, proud=0.09, cell=0.32, depth=0.09), [rock])
    spent = sdf.union([face(12, proud=0.03, cell=0.13, depth=0.03)] + rubble(13, 10))
    out += mesh_regions(kit, "SM_MineRockFaceSpent", spent, [rock])

    # Coal seam: a dark glossy band 0.4 m thick through the face, a little cleated; spent: the band cut out as a slot.
    lo, hi = 0.62, 1.02
    seam = fractured(face(21, proud=0.08, cell=0.3, depth=0.06), cell=0.09, depth=0.015, seed=29)
    out += mesh_regions(kit, "SM_MineCoalSeam", seam, [rock, coal], [(band_box(lo, hi, wave=0.06, seed=27), 0.0, SLOT_ORE)])
    slot = sdf.subtract(face(22, proud=0.07, cell=0.3, depth=0.06), band_box(lo + 0.03, hi - 0.03, 0.6), 0.03)
    slot = sdf.union([slot, sdf.box((WALL_X - 0.18, 0.0, (lo + hi) / 2), (0.05, HALF_W, (hi - lo) / 2))] + rubble(23, 7))
    out += mesh_regions(kit, "SM_MineCoalSeamSpent", slot, [rock, coal],
                        [(band_box(lo - 0.02, hi + 0.02, wave=0.04, seed=28), 0.0, SLOT_ORE)])

    # Copper vein: a steep streak 12 cm wide, green with malachite, with a fading stain either side;
    # spent: a groove cut along it, the stain left behind.
    vein, halo = dipping(0.9, 0.06, 68.0, wave=0.035, seed=35), dipping(0.9, 0.2, 68.0, wave=0.09, seed=36)
    out += mesh_regions(kit, "SM_MineCopperVein", face(31, proud=0.08), [rock, copper, stain],
                        [(halo, 0.0, SLOT_HALO), (vein, 0.0, SLOT_ORE)])
    groove = sdf.subtract(face(32, proud=0.07), dipping(0.9, 0.08, 68.0, 0.6), 0.025)
    out += mesh_regions(kit, "SM_MineCopperVeinSpent", sdf.union([groove] + rubble(34, 6, size=(0.03, 0.07))),
                        [rock, copper, stain], [(halo, 0.0, SLOT_HALO), (dipping(0.9, 0.1, 68.0), 0.0, SLOT_ORE)])

    # No gem pocket: quartz and amethyst turn up at random from rock faces and veins (Jenny, 2026-10-11).
    return out