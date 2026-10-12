"""The mine house: a granite engine house on the mine yard (openspec open-estate-mine-shallow-level;
Jenny's D5 and her blockout review: "more engine-house: a bit taller and narrower (massive bob wall,
stack tight to a corner)").

    .\\Scripts\\Blender\\New-Prop.ps1 mine_house -BeautySamples 64        # HOMESTEAD_DRAFT=1 for quick bakes

Real-object research (see also ``mine/layout.py``): Cornish, Devon and Welsh engine and winding houses
were built of local granite or killas rubble brought to rough courses, 0.6-0.9 m thick, the bob wall
(the end that carried the beam) 1.2-1.8 m. Corners and openings got the best dressed granite: long-and-
short quoins, jambs, single-piece sills, and round arches turned in wedge voussoirs with a keystone.
Mortar was lime, recessed and dirtied. Roofs were Delabole slate in diminishing courses with clay or
granite ridge tiles; the stack rose from one corner of the house in granite and finished in brick
with a corbelled head. Inside, the count-house rooms were limewashed rubble with boarded floors and
joisted ceilings; doors were ledged planks, windows small-paned sashes painted in oxide colours.

Meshes (house frame, pivot = house origin on the yard; place at yaw 0 with place_mine.py):
SM_MineHouseWallE/W/N/S (masonry, inside and out), SM_MineHouseStack, SM_MineHouseRoof,
SM_MineHouseFloors, SM_MineHouseShaft, SM_MineHouseJoinery (frames, glazing bars, doors, boarded
arch), SM_MineHouseGlass (unbaked: Unreal assigns the glass material). The geometry is generated here; with
PHOTO the stone, mortar, limewash, slate and timber are shaded in Unreal with CC0 Poly Haven photoscans
(Assets/Props/MineSurfaces, credits in its CREDITS.json).
"""
import importlib
import math
import os
import sys
from pathlib import Path

import bpy
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

NAME = "MineHouse"
DESCRIPTION = ("Granite engine house (mine house): rubble walls with dressed quoins and arches, bob wall, corner "
               "stack, slate roof, boarded floors with the trapdoor's opening, the shaft below, joinery and glass.")
COLLISION = "none"            # import_props.COLLISION_OVERRIDES: complex (walk in through the door)
TRIANGLE_BUDGET = 3500000
PROVENANCE = ("Original project-authored procedural geometry; surfaces from CC0 Poly Haven photoscans "
              "(Assets/Props/MineSurfaces/CREDITS.json) or, with HOMESTEAD_MINE_PHOTO=0, procedural bakes.")
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = {"size": 1024 if DRAFT else 2048, "samples": 8 if DRAFT else 32, "repack": False}
# Masonry bakes a macro texture plus a "mask" (bare granite) that gates the shared 1 m GraniteDetail crystal
# tiles in Unreal (import_props.granite_parent, as the big rocks and the sea cave): close up the granite
# shows real crystals at any texel density.
_MASONRY = {"size": 1024 if DRAFT else 4096, "maps": ["basecolor", "roughness", "normal", "ao", "mask"]}
# Jenny (review 2): "Textures inside and out still need significant work to be photorealistic". A 4K atlas over a
# wall's few thousand stones gave about 100 px a metre; instead the stone, mortar, limewash, slate and timber meshes
# carry metric per-piece UVs and per-piece tints (photo_uvs) and Unreal shades them with tiling CC0 photoscans at
# full resolution (Scripts/Mine/mine_surfaces.py, which runs after import_props). PHOTO=0 restores the bakes.
PHOTO = os.environ.get("HOMESTEAD_MINE_PHOTO", "1") == "1"
PHOTO_MESHES = {"SM_MineHouseWallE", "SM_MineHouseWallW", "SM_MineHouseWallN", "SM_MineHouseWallS",
                "SM_MineHouseStack", "SM_MineHouseShaft", "SM_MineHouseRoof", "SM_MineHouseFloors"}
BAKE_MESHES = {"SM_MineHouseGlass": False,
               "SM_MineHouseWallE": _MASONRY, "SM_MineHouseWallW": _MASONRY,
               "SM_MineHouseWallN": _MASONRY, "SM_MineHouseWallS": _MASONRY,
               "SM_MineHouseStack": dict(_MASONRY, size=1024 if DRAFT else 2048),
               "SM_MineHouseShaft": dict(_MASONRY, size=1024 if DRAFT else 2048),
               "SM_MineHouseRoof": {"size": 1024 if DRAFT else 4096},
               "SM_MineHouseFloors": {"size": 1024 if DRAFT else 4096},
               "*": True}
if PHOTO:
    BAKE_MESHES.update({name: False for name in PHOTO_MESHES})
    BAKE_MESHES.update({f"{name}_LOD{i}": False for name in PHOTO_MESHES for i in (1, 2)})
    BAKE_MESHES["SM_MineHouseJoinery"] = False      # paint, weatherboard and iron slots; Unreal shades them triplanar
NOTES = {"detail_textures": [f"Assets/Props/GraniteDetail/Textures/T_GraniteDetail_{role}.png"
                             for role in ("basecolor", "normal", "roughness", "height")]}
DETAIL = {"folder": "Assets/Props/GraniteDetail/Textures", "stem": "GraniteDetail", "tile_m": 1.0, "strength": 0.7}
BEAUTY = {"pose": (0, 0, 0), "views": ["hero"], "meshes": {}}
REPORT = {"nanite": True, "pivot": "House origin on the mine yard (Anchor::MineEntrance); Unreal frame, yaw 0.",
          "unreal_frame": "Authored in Unreal axes; Blender Y is negated."}


def _modules():
    from stone_building import masonry, wall as kit_wall
    from mine import house, layout
    if not bpy.app.background:
        for module in (masonry, kit_wall, layout, house):
            importlib.reload(module)
    return masonry, kit_wall, house, layout


def U(x, y, z):
    return (x, -y, z)


def _finish(kit, masonry, obj, shrink=None):
    if PHOTO and obj.name in PHOTO_MESHES:
        photo_uvs(obj)
    else:
        masonry.unwrap(obj, shrink=shrink)
    return masonry.finish(kit, obj, lod_ratios=(0.35, 0.12))


def photo_uvs(obj, wrap=4.0):
    """UVs in metres for Unreal's tiling photo surfaces (Scripts/Mine/mine_surfaces.py), one layout per piece.

    Each face is projected along its dominant axis; within a piece (one stone, block, slate or board) U runs
    along the piece's longest dimension on that plane, so wood grain follows the board and a stone's bedding
    runs along its length. The piece's random stone_offset shifts it to its own patch of the photograph
    (wrapped into [0, wrap) m, which the textures tile over), and stone_tint goes into the vertex colour
    (divided by 1.5; the material multiplies it back)."""
    mesh = obj.data
    n_v = len(mesh.vertices)
    co = np.empty(n_v * 3, np.float32)
    mesh.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3).astype(np.float64)
    co[:, 1] *= -1.0                                   # back to the Unreal frame
    piece = np.empty(n_v, np.int32)
    mesh.attributes["piece"].data.foreach_get("value", piece)
    offset = np.empty(n_v * 3, np.float32)
    mesh.attributes["stone_offset"].data.foreach_get("vector", offset)
    offset = offset.reshape(-1, 3)
    tint = np.empty(n_v * 3, np.float32)
    mesh.attributes["stone_tint"].data.foreach_get("vector", tint)
    tint = tint.reshape(-1, 3)
    # Per piece: extent along X, Y, Z.
    count = piece.max() + 1
    lo = np.full((count, 3), np.inf)
    hi = np.full((count, 3), -np.inf)
    np.minimum.at(lo, piece, co)
    np.maximum.at(hi, piece, co)
    extent = hi - lo
    shift = np.mod(offset.astype(np.float64), wrap)    # per vertex, constant over a piece
    n_l = len(mesh.loops)
    loop_v = np.empty(n_l, np.int32)
    mesh.loops.foreach_get("vertex_index", loop_v)
    n_p = len(mesh.polygons)
    normals = np.empty(n_p * 3, np.float32)
    mesh.polygons.foreach_get("normal", normals)
    normals = normals.reshape(-1, 3).astype(np.float64)
    normals[:, 1] *= -1.0
    start = np.empty(n_p, np.int32)
    mesh.polygons.foreach_get("loop_start", start)
    total = np.empty(n_p, np.int32)
    mesh.polygons.foreach_get("loop_total", total)
    poly_of_loop = np.repeat(np.arange(n_p), total)
    axis = np.argmax(np.abs(normals), axis=1)[poly_of_loop]
    v = loop_v
    pc = piece[v]
    # The two in-plane axes for each projection axis, then the piece's longer one becomes U.
    plane = np.array([[1, 2], [0, 2], [0, 1]])[axis]
    ea = extent[pc, plane[:, 0]]
    eb = extent[pc, plane[:, 1]]
    u_axis = np.where(ea >= eb, plane[:, 0], plane[:, 1])
    v_axis = np.where(ea >= eb, plane[:, 1], plane[:, 0])
    rows = np.arange(n_l)
    p = co[v] + shift[v]
    uv = np.stack([p[rows, u_axis], p[rows, v_axis]], axis=1)
    layer = mesh.uv_layers.get("UVMap") or mesh.uv_layers.new(name="UVMap")
    layer.data.foreach_set("uv", uv.astype(np.float32).ravel())
    colour = mesh.color_attributes.get("Col") or mesh.color_attributes.new("Col", "BYTE_COLOR", "POINT")
    rgba = np.concatenate([np.clip(tint / 1.5, 0.0, 1.0), np.ones((n_v, 1), np.float32)], axis=1)
    colour.data.foreach_set("color", rgba.astype(np.float32).ravel())
    mesh.color_attributes.active_color = colour
    mesh.color_attributes.render_color_index = mesh.color_attributes.active_color_index
    return obj


# --------------------------------------------------------------------------- joinery (kit primitives)

def joinery(kit, L, H, paint, boards, iron):
    parts = []

    def wall_point(key, u, v, depth):
        return H.Wall(key).p(u, v, depth)

    def box_between(name, p0, p1, mat, bevel=0.004):
        p0, p1 = np.asarray(p0), np.asarray(p1)
        c = (p0 + p1) / 2
        s = np.abs(p1 - p0)
        parts.append(kit.box(name, tuple(s), U(*c), material=mat, bevel=bevel))

    def arc(name, key, c, spring, radius, depth, thick, mat, segs=20):
        w = H.Wall(key)
        pts = []
        for i in range(segs + 1):
            th = math.pi * i / segs
            q = w.p(c, spring, depth) + (w.u_dir() * math.cos(th) + np.array([0, 0, math.sin(th)])) * radius
            pts.append(U(*q))
        parts.append(kit.tube(name, pts, radius=thick, sides=4, material=mat, roll=math.pi / 4))

    glass_polys = []
    for o in L.WINDOWS:
        key, c, half, sill, spring = o["wall"], o["centre"], o["width"] / 2, o["sill"], o["spring"]
        w = H.Wall(key)
        if o.get("boarded"):
            depth = 0.32
            u = c - half
            while u < c + half - 1e-6:
                bw = min(0.16, c + half - u)
                top = spring + math.sqrt(max(half ** 2 - (u + bw / 2 - c) ** 2, 0.0))
                box_between("Board", w.p(u + 0.004, sill, depth), w.p(u + bw - 0.004, top, depth + 0.03), boards)
                u += bw
            for z in (sill + 0.25, spring - 0.1):
                box_between("BoardLedge", w.p(c - half + 0.05, z, depth + 0.03), w.p(c + half - 0.05, z + 0.12, depth + 0.06), boards)
            continue
        depth, t = 0.16, 0.035                      # frame set back in the reveal; member half-thickness
        for side in (-1, 1):
            u = c + side * (half - t)
            box_between("Jamb", w.p(u - t, sill, depth - 0.04), w.p(u + t, spring, depth + 0.04), paint)
        box_between("SillRail", w.p(c - half, sill, depth - 0.045), w.p(c + half, sill + 0.06, depth + 0.045), paint)
        arc("Head", key, c, spring, half - t, depth, t * 1.1, paint)
        bar = 0.012
        box_between("BarV", w.p(c - bar, sill, depth - 0.02), w.p(c + bar, spring + half - t, depth + 0.02), paint)
        box_between("Transom", w.p(c - half, spring - 0.025, depth - 0.03), w.p(c + half, spring + 0.025, depth + 0.03), paint)
        z = sill + 0.06
        rows = max(1, int(round((spring - sill) / 0.36)))
        for r in range(1, rows):
            zz = sill + (spring - sill) * r / rows
            box_between("BarH", w.p(c - half, zz - bar, depth - 0.02), w.p(c + half, zz + bar, depth + 0.02), paint)
        # Fan bars in the arched head.
        for th in (math.pi / 4, 3 * math.pi / 4):
            a = w.p(c, spring, depth)
            b = a + (w.u_dir() * math.cos(th) + np.array([0, 0, math.sin(th)])) * (half - t)
            parts.append(kit.tube("Fan", [U(*a), U(*b)], radius=bar, sides=4, material=paint))
        poly = [w.p(c - half + 0.01, sill + 0.03, depth)] + [w.p(c + half - 0.01, sill + 0.03, depth)]
        for i in range(0, 21):
            th = math.pi * i / 20
            poly.append(w.p(c, spring, depth) + (w.u_dir() * math.cos(th) + np.array([0, 0, math.sin(th)])) * (half - 0.01))
        glass_polys.append(poly)

    # Door: frame at the inner reveal, a boarded tympanum in the arch, two ledged leaves folded back
    # flat against the inside of the gable.
    d = L.DOOR
    key, c, half, spring = d["wall"], d["centre"], d["width"] / 2, d["spring"]
    w = H.Wall(key)
    depth = w.t - 0.10
    t = 0.045
    for side in (-1, 1):
        u = c + side * (half - t)
        box_between("DoorPost", w.p(u - t, L.FLOOR_Z, depth - 0.05), w.p(u + t, spring + 0.05, depth + 0.05), paint)
    box_between("DoorHead", w.p(c - half, spring - 0.03, depth - 0.05), w.p(c + half, spring + 0.07, depth + 0.05), paint)
    u = c - half
    while u < c + half - 1e-6:
        bw = min(0.15, c + half - u)
        top = spring + math.sqrt(max(half ** 2 - (u + bw / 2 - c) ** 2, 0.0))
        box_between("Tympanum", w.p(u + 0.003, spring + 0.07, depth - 0.015), w.p(u + bw - 0.003, top, depth + 0.015), boards)
        u += bw
    leaf_h = spring - L.FLOOR_Z - 0.03
    for side in (-1, 1):
        # Open leaf: hinged at the inner face beside the jamb, swung right round against the wall.
        y0 = c + side * (half + 0.02)
        x_face = L.IN_E - 0.012
        for k in range(5):
            ya, yb = sorted((y0 + side * k * 0.15, y0 + side * (k + 1) * 0.15))
            box_between("LeafBoard", (x_face - 0.04, ya + 0.002, L.FLOOR_Z + 0.02), (x_face, yb - 0.002, L.FLOOR_Z + 0.02 + leaf_h),
                        paint, bevel=0.006)
        for z in (0.35, leaf_h - 0.35):
            ya, yb = sorted((y0 + side * 0.04, y0 + side * 0.72))
            box_between("Strap", (x_face - 0.048, ya, L.FLOOR_Z + z), (x_face - 0.04, yb, L.FLOOR_Z + z + 0.045), iron, bevel=0.002)
            box_between("Ledge", (x_face + 0.0, ya, L.FLOOR_Z + z - 0.03), (x_face + 0.01, yb, L.FLOOR_Z + z + 0.09), paint)
    return parts, glass_polys


def glass(kit, polys, mat):
    verts, faces = [], []
    for poly in polys:
        i0 = len(verts)
        verts += [U(*p) for p in poly]
        faces.append(tuple(range(i0, i0 + len(poly))))
    obj = kit.mesh("SM_MineHouseGlass", verts, faces, mat)
    kit.recalc_normals(obj)
    return obj


def no_crystals(mat):
    """Masonry materials other than granite bake a zero 'mask' (no crystal detail on mortar, wash or brick)."""
    tree = mat.node_tree
    if any(n.label == "HOMESTEAD_MASK" for n in tree.nodes):
        return
    value = tree.nodes.new("ShaderNodeValue")
    value.outputs[0].default_value = 0.0
    value.label = "HOMESTEAD_MASK"


def after_bake(kit, obj):
    """Blender review only: layer the crystal tiles over the masonry's macro bake (Unreal does the same)."""
    mask = kit.ROOT / "Assets" / "Props" / NAME / "Textures" / f"T_{obj.name[3:]}_mask.png"
    if mask.exists():
        kit.layer_detail(obj, kit.ROOT / DETAIL["folder"], DETAIL["stem"], tile=DETAIL["tile_m"],
                         strength=DETAIL["strength"], mask=mask)


# --------------------------------------------------------------------------- build

def build(kit):
    masonry, kit_wall, H, L = _modules()
    stone = kit_wall.stone_material(kit, "M_MineHouseStone", 5, grains=False, lichen=0.55, moss=0.18, soil=0.5,
                                    patina=0.95)
    mortar = kit.mats.lime_mortar("M_MineHouseMortar", seed=5, grime=0.8)
    wash = kit.mats.lime_mortar("M_MineHouseLimewash", color=(0.50, 0.47, 0.41), dirt=(0.22, 0.20, 0.16), grime=0.35,
                                seed=9)
    brick = kit.mats.daub("M_MineHouseBrick", clay=(0.20, 0.075, 0.045), dry=(0.30, 0.13, 0.08), straw=(0.22, 0.09, 0.05))
    slate = kit.mats.slate("M_MineHouseSlate", dark=(0.035, 0.04, 0.046), light=(0.10, 0.105, 0.11), lichen=0.3,
                           moss=0.12, seed=4, offset_attr="stone_offset")
    deck = kit.mats.aged_wood("M_MineHouseDeck", light=(0.18, 0.13, 0.08), dark=(0.06, 0.045, 0.03), seed=2.0)
    ridge = kit.mats.daub("M_MineHouseRidge", clay=(0.24, 0.10, 0.055), dry=(0.32, 0.16, 0.09), straw=(0.25, 0.11, 0.06))
    board = kit.mats.aged_wood("M_MineHouseBoards", light=(0.26, 0.19, 0.12), dark=(0.09, 0.065, 0.04), saw=0.4,
                               grime=0.55, seed=6.0)
    beam = kit.mats.aged_wood("M_MineHouseBeams", light=(0.20, 0.145, 0.09), dark=(0.06, 0.042, 0.028), saw=0.6,
                              grime=0.5, seed=8.0)
    paint = kit.mats.painted_wood("M_MineHousePaint", paint=(0.075, 0.03, 0.022), under=(0.16, 0.10, 0.05), wear=0.5,
                                  grime=0.55, seed=3.0)
    weathered = kit.mats.aged_wood("M_MineHouseWeatherboard", light=(0.22, 0.20, 0.17), dark=(0.07, 0.065, 0.055),
                                   saw=0.3, grime=0.7, seed=12.0)
    tar = kit.material("M_MineHouseTarredDeck", (0.012, 0.010, 0.008), roughness=0.95)
    iron = kit.mats.wrought_iron("M_MineHouseIron", rust=0.6, wear=0.3, seed=4.0)
    glass_mat = kit.material("M_MineHouseGlass", (0.02, 0.025, 0.025), roughness=0.05)

    out = []
    for key, name, seed in (("+X", "SM_MineHouseWallE", 101), ("-X", "SM_MineHouseWallW", 102),
                            ("+Y", "SM_MineHouseWallN", 103), ("-Y", "SM_MineHouseWallS", 104)):
        m, _ = H.build_wall(key, seed)
        obj = m.build(name, [stone, mortar, wash, brick])
        out += _finish(kit, masonry, obj, shrink={H.MAT_MORTAR: 0.4})
        print(f"HOMESTEAD_MINE_HOUSE {name} tris={masonry.triangles(obj)}")
    obj = H.build_stack().build("SM_MineHouseStack", [stone, mortar, wash, brick])
    out += _finish(kit, masonry, obj, shrink={H.MAT_MORTAR: 0.4})
    obj = H.build_roof().build("SM_MineHouseRoof", [slate, deck, ridge])
    out += _finish(kit, masonry, obj)
    obj = H.build_floors().build("SM_MineHouseFloors", [board, beam, tar])
    out += _finish(kit, masonry, obj)
    shaft_stone = kit_wall.stone_material(kit, "M_MineHouseShaftStone", 7, grains=False, lichen=0.0, moss=0.05, soil=0.0,
                                          patina=0.5)
    for mat in (mortar, wash, brick):
        no_crystals(mat)
    obj = H.build_shaft().build("SM_MineHouseShaft", [shaft_stone, mortar])
    out += _finish(kit, masonry, obj, shrink={H.MAT_MORTAR: 0.4})
    parts, polys = joinery(kit, L, H, paint, weathered, iron)
    out.append(kit.join(parts, "SM_MineHouseJoinery", pivot=None, smooth_angle=35.0))
    out.append(glass(kit, polys, glass_mat))
    for obj in out:
        if not obj.name.endswith(("_LOD1", "_LOD2")):
            print(f"HOMESTEAD_MINE_HOUSE {obj.name} tris={masonry.triangles(obj)}")
    return out
