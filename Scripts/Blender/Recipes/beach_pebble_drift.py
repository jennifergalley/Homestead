"""Beach pebble drift: a ~1.5 x 0.8 m patch of rounded, wave-smoothed pebbles half-bedded in sand (scenery).

The real thing (what the geometry and textures follow):
- On a granite cove in south-west England the coarse fraction of the beach collects in patches and
  bands of pebbles 2-8 cm long on the sand, worn smooth and rounded by the swash: discs and flattened
  ovoids lie flat, rounder ones sit a third to half buried; small ones fill between the large ones.
- They are mostly the local granite: grey biotite granite with white feldspar laths and glassy
  quartz, some pink-tinged where orthoclase is stained, salted with milky white vein quartz (very
  round, slightly translucent, iron-stained in cracks), blue-black killas slate (flat discs, often
  with a faint banding) and a few dark greenstone (dolerite/elvan) pebbles. Dry pebbles are matt,
  sand clings round the waterline and fills the gaps between them.

Geometry: each pebble is a lumpy superellipsoid; a dense high mesh (granite crystals, quartz veins
and slate banding in procedural materials, per-pebble tint/offset attributes) is baked onto a light
low mesh (homestead_highbake, 2K) whose buried faces are dropped. Pebbles are bedded 15-60 % of
their height; the lowest kept faces reach ~1 cm below z = 0, so every pebble's visible skirt meets
the sand well below the ground line. Pivot bottom centre. Original procedural geometry and materials
(kit.mats.granite for the granite pebbles, as granite_hand_pile.py).
"""
import math
import os
import random
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import homestead_highbake as hb  # noqa: E402
import homestead_rocks as rocks  # noqa: E402
import homestead_sdf as sdf  # noqa: E402

NAME = "BeachPebbleDrift"
DESCRIPTION = ("A 1.5 x 0.8 m patch of rounded beach pebbles 2-8 cm (grey and pink granite, white quartz, "
               "dark slate and greenstone) half-bedded in sand. Ground-hugging scenery, no collision.")
COLLISION = "none"
TRIANGLE_BUDGET = 6000
PROVENANCE = "Original project-authored procedural geometry and materials; no third-party asset or texture."
DRAFT = os.environ.get("HOMESTEAD_DRAFT") == "1"
BAKE = None  # baked high-to-low inside build() (homestead_highbake)
BAKE_SIZE = 1024 if DRAFT else 2048
BEAUTY = {"pose": (0, 0, 0), "focus": (0.10, -0.05, 0.02), "ground": "origin", "detail_distance": 0.5,
          "detail_fstop": 11.0, "views": ["hero", "detail", "eye"], "eye_distance": 2.0}
REPORT = {"placement": "Ground line at z = 0 (pivot, bottom centre); pebble skirts reach ~1 cm below it. "
                       "Long axis along X. Random yaw, 0.8-1.2 scale."}
NOTES = {"sink_depth_m": 0.010}
SEED = 30851
LOD_RATIOS = (0.5, 0.22)
LOD0_TRIS = 6000
CUT_Z = -0.010           # faces entirely below this are buried and dropped

# kind: (weight, flatness range c/a, b/a range, superellipse power)
KINDS = {"granite": (0.46, (0.45, 0.75), (0.65, 0.9), 2.4), "pink": (0.14, (0.45, 0.72), (0.65, 0.9), 2.4),
         "quartz": (0.12, (0.6, 0.85), (0.72, 0.95), 2.1), "slate": (0.16, (0.18, 0.32), (0.55, 0.85), 2.8),
         "greenstone": (0.12, (0.45, 0.7), (0.6, 0.85), 2.3)}
SLOTS = ("granite", "quartz", "slate", "greenstone")


def pebble_points(dirs, radii, power, lumps, seed):
    """Surface points along unit directions: a superellipsoid with soft low-frequency lumps."""
    a, b, c = radii
    s = (np.abs(dirs[:, 0] / a) ** power + np.abs(dirs[:, 1] / b) ** power + np.abs(dirs[:, 2] / c) ** power) ** (-1.0 / power)
    s = s * (1.0 + lumps * sdf.fbm(dirs * 1.3 + seed * 0.37, 2, seed))
    return dirs * s[:, None]


def sphere_grid(nt, nphi):
    """Unit directions on a UV sphere (poles included) and its faces."""
    dirs = [(0.0, 0.0, 1.0)]
    for i in range(1, nphi):
        ph = math.pi * i / nphi
        for j in range(nt):
            th = 2 * math.pi * j / nt
            dirs.append((math.sin(ph) * math.cos(th), math.sin(ph) * math.sin(th), math.cos(ph)))
    dirs.append((0.0, 0.0, -1.0))
    faces = []
    last = len(dirs) - 1
    for j in range(nt):
        faces.append((0, 1 + j, 1 + (j + 1) % nt))
    for i in range(nphi - 2):
        r0, r1 = 1 + i * nt, 1 + (i + 1) * nt
        for j in range(nt):
            k = (j + 1) % nt
            faces.append((r0 + j, r1 + j, r1 + k, r0 + k))
    r0 = 1 + (nphi - 2) * nt
    for j in range(nt):
        faces.append((r0 + j, last, r0 + (j + 1) % nt))
    return np.array(dirs), faces


class Mesh:
    def __init__(self):
        self.verts, self.faces, self.mat, self.attrs = [], [], [], {}
        self.n = 0

    def add(self, P, faces, slot, attrs=None):
        keep = [f for f in faces if max(P[i][2] for i in f) > CUT_Z]
        used = sorted({i for f in keep for i in f})
        remap = {old: k for k, old in enumerate(used)}
        self.verts.append(P[used])
        self.faces.extend(tuple(self.n + remap[i] for i in f) for f in keep)
        self.mat.extend([slot] * len(keep))
        for key, value in (attrs or {}).items():
            self.attrs.setdefault(key, []).append(np.tile(value, (len(used), 1)))
        self.n += len(used)

    def build(self, name, materials):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(np.concatenate(self.verts).tolist(), [], self.faces)
        mesh.validate()
        mesh.polygons.foreach_set("material_index", self.mat)
        for key, chunks in self.attrs.items():
            arr = np.concatenate(chunks).astype(np.float32)
            if key.endswith("_tint"):
                attr = mesh.attributes.new(key, "FLOAT_COLOR", "POINT")
                attr.data.foreach_set("color", np.c_[arr, np.ones(len(arr), np.float32)].ravel())
            else:
                attr = mesh.attributes.new(key, "FLOAT_VECTOR", "POINT")
                attr.data.foreach_set("vector", arr.ravel())
        for mat in materials:
            mesh.materials.append(mat)
        mesh.update()
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


# ------------------------------------------------------------------ materials

def add_sand(g, color):
    """Sand clinging round each pebble's waterline and in the gaps."""
    pos = g.node("ShaderNodeTexCoord").outputs["Object"]
    pz = g.separate(pos)[2]
    grains = g.noise(pos, scale=700.0, detail=3.0).outputs["Fac"]
    edge = g.noise(pos, scale=90.0, detail=3.0).outputs["Fac"]
    line = g.math("ADD", pz, g.math("MULTIPLY", g.math("SUBTRACT", edge, 0.5), 0.006))
    mask = g.math("MULTIPLY", g.remap(line, 0.005, -0.001), g.remap(grains, 0.3, 0.55, 0.55, 1.0))
    sand = g.mix((0.30, 0.255, 0.185), (0.42, 0.365, 0.27), grains)
    return g.mix(color, sand, mask), mask


def finish(g, color, rough, height, relief=0.5):
    color, sand = add_sand(g, color)
    g.set("Base Color", color)
    g.set("Roughness", g.node("ShaderNodeMix", data_type="FLOAT", Factor=sand, A=rough, B=0.92).outputs[0])
    g.set("Normal", g.bump(height, strength=relief, distance=0.0004))
    g.bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return g.mat


def granite_pebble(kit):
    mat = kit.mats.granite("M_BeachPebbleGranite", grain=0.0014, scale=0.035, patina=0.25, lichen=0.0, moss=0.0,
                           iron=0.12, streaks=0.0, soil=0.0, enclaves=0.0, megacrysts=0.3, relief=0.45,
                           film=0.0, spots=0.0, seed=7.0, offset_attr="pb_off", tint_attr="pb_tint")
    tree = mat.node_tree
    bsdf = next(n for n in tree.nodes if n.type == "BSDF_PRINCIPLED")
    src = bsdf.inputs["Base Color"].links[0].from_socket
    g = kit.mats.Graph.__new__(kit.mats.Graph)
    g.mat, g.tree, g.bsdf = mat, tree, bsdf
    color, sand = add_sand(g, src)
    tree.links.new(color, bsdf.inputs["Base Color"])
    rough_src = bsdf.inputs["Roughness"].links[0].from_socket
    # Wave-polished: smoother than a weathered boulder.
    polished = g.math("MULTIPLY", rough_src, 0.85)
    tree.links.new(g.node("ShaderNodeMix", data_type="FLOAT", Factor=sand, A=polished, B=0.92).outputs[0],
                   bsdf.inputs["Roughness"])
    bsdf.inputs["Subsurface Weight"].default_value = 0.0
    return mat


def quartz_pebble(kit):
    g = kit.mats.Graph("M_BeachPebbleQuartz")
    p = g.vmath("ADD", g.coord(), g.node("ShaderNodeAttribute", attribute_name="pb_off",
                                          attribute_type="GEOMETRY").outputs["Vector"])
    cloud = g.noise(p, scale=55.0, detail=5.0, roughness=0.55).outputs["Fac"]
    cracks = g.voronoi(g.vmath("MULTIPLY", p, (1.0, 1.0, 1.0)), scale=40.0, feature="DISTANCE_TO_EDGE").outputs["Distance"]
    crack = g.math("MULTIPLY", g.remap(cracks, 0.0, 0.015, 1.0, 0.0), g.remap(g.noise(p, scale=14.0, detail=2.0).outputs["Fac"], 0.5, 0.62))
    color = g.mix((0.34, 0.335, 0.31), (0.46, 0.45, 0.42), cloud)
    stain = g.remap(g.noise(p, scale=18.0, detail=3.0).outputs["Fac"], 0.55, 0.72)
    color = g.mix(color, (0.42, 0.30, 0.17), g.math("MULTIPLY", g.math("MAXIMUM", crack, stain), 0.65))
    tint = g.node("ShaderNodeAttribute", attribute_name="pb_tint", attribute_type="GEOMETRY").outputs["Color"]
    color = g.mix(color, tint, 1.0, blend="MULTIPLY")
    g.set("Transmission Weight", 0.0)
    pits = g.noise(p, scale=400.0, detail=2.0).outputs["Fac"]
    return finish(g, color, g.remap(cloud, 0.3, 0.7, 0.38, 0.5),
                  g.math("SUBTRACT", g.math("MULTIPLY", pits, 0.3), g.math("MULTIPLY", crack, 0.6)), relief=0.3)


def slate_pebble(kit):
    g = kit.mats.Graph("M_BeachPebbleSlate")
    p = g.vmath("ADD", g.coord(), g.node("ShaderNodeAttribute", attribute_name="pb_off",
                                          attribute_type="GEOMETRY").outputs["Vector"])
    x, y, z = g.separate(p)
    warp = g.noise(p, scale=12.0, detail=3.0).outputs["Fac"]
    bands = g.wave(g.combine(x, g.math("ADD", y, g.math("MULTIPLY", warp, 0.01)), z), scale=28.0, distortion=4.0,
                   detail=3.0, kind="BANDS", direction="Y").outputs["Fac"]
    fine = g.noise(p, scale=300.0, detail=4.0).outputs["Fac"]
    color = g.mix((0.045, 0.049, 0.055), (0.072, 0.077, 0.083), g.math("ADD", g.math("MULTIPLY", bands, 0.3),
                                                                      g.math("MULTIPLY", fine, 0.7)))
    rust = g.remap(g.noise(p, scale=8.0, detail=3.0).outputs["Fac"], 0.62, 0.75)
    color = g.mix(color, (0.13, 0.085, 0.05), g.math("MULTIPLY", rust, 0.5))
    tint = g.node("ShaderNodeAttribute", attribute_name="pb_tint", attribute_type="GEOMETRY").outputs["Color"]
    color = g.mix(color, tint, 1.0, blend="MULTIPLY")
    return finish(g, color, g.remap(fine, 0.3, 0.7, 0.5, 0.62), g.math("ADD", fine, g.math("MULTIPLY", bands, 0.4)),
                  relief=0.35)


def greenstone_pebble(kit):
    g = kit.mats.Graph("M_BeachPebbleGreenstone")
    p = g.vmath("ADD", g.coord(), g.node("ShaderNodeAttribute", attribute_name="pb_off",
                                          attribute_type="GEOMETRY").outputs["Vector"])
    speck = g.voronoi(p, scale=900.0).outputs["Distance"]
    mott = g.noise(p, scale=30.0, detail=4.0).outputs["Fac"]
    color = g.mix((0.050, 0.060, 0.052), (0.095, 0.105, 0.088), mott)
    color = g.mix(color, (0.17, 0.17, 0.15), g.math("MULTIPLY", g.remap(speck, 0.0, 0.25, 1.0, 0.0), 0.5))
    vein = g.remap(g.math("ABSOLUTE", g.math("SUBTRACT", g.noise(p, scale=9.0, detail=2.0).outputs["Fac"], 0.5)),
                   0.0, 0.012, 1.0, 0.0)
    color = g.mix(color, (0.38, 0.38, 0.35), g.math("MULTIPLY", vein, 0.8))
    tint = g.node("ShaderNodeAttribute", attribute_name="pb_tint", attribute_type="GEOMETRY").outputs["Color"]
    color = g.mix(color, tint, 1.0, blend="MULTIPLY")
    return finish(g, color, g.remap(mott, 0.3, 0.7, 0.5, 0.6), g.math("ADD", mott, g.math("MULTIPLY", speck, 0.5)),
                  relief=0.3)


# ------------------------------------------------------------------ layout

def patch_density(x, y):
    """Elongated patch with a lobed, ragged edge and a few sandy gaps."""
    e = (x / 0.60) ** 2 + (y / 0.31) ** 2
    lobe = 0.15 * math.sin(3.0 * math.atan2(y, x) + 1.0) + 0.1 * math.sin(7.0 * math.atan2(y, x))
    gap = math.exp(-((x - 0.22) ** 2 + (y + 0.05) ** 2) / 0.006) + 0.7 * math.exp(-((x + 0.4) ** 2 + (y - 0.12) ** 2) / 0.004)
    return max(0.0, 1.0 + lobe - e) * (1.0 - 0.85 * min(gap, 1.0))


def layout(rng):
    stones = []
    for attempt in range(40000):
        if len(stones) >= 118:
            break
        x, y = rng.uniform(-0.62, 0.62), rng.uniform(-0.33, 0.33)
        d = patch_density(x, y)
        if rng.random() > min(1.0, d * 2.5):
            continue
        size = 0.022 + 0.058 * rng.random() ** (1.25 + 1.0 * (1 - min(d * 1.5, 1.0)))
        kind = rng.choices(list(KINDS), [v[0] for v in KINDS.values()])[0]
        _, flat, wide, power = KINDS[kind]
        a = size * 0.5
        radii = (a, a * rng.uniform(*wide), a * rng.uniform(*flat))
        foot = max(radii[0], radii[1])
        if any(math.hypot(x - s["x"], y - s["y"]) < (foot + s["foot"]) * 0.72 for s in stones):
            continue
        stones.append(dict(x=x, y=y, radii=radii, foot=foot, kind=kind, power=power, yaw=rng.uniform(0, math.tau),
                           tilt=(rng.gauss(0, 0.08), rng.gauss(0, 0.08)), bed=rng.uniform(0.15, 0.6),
                           seed=rng.randint(1, 10 ** 6), lumps=rng.uniform(0.02, 0.06)))
    return stones


TINTS = {"granite": [(0.80, 0.80, 0.80), (0.72, 0.72, 0.74), (0.85, 0.83, 0.80), (0.66, 0.66, 0.68)],
         "pink": [(0.96, 0.77, 0.69), (0.90, 0.74, 0.69), (1.0, 0.80, 0.72)],
         "quartz": [(1.0, 1.0, 1.0), (1.02, 0.97, 0.9), (0.94, 0.95, 0.96)],
         "slate": [(1.0, 1.0, 1.0), (0.85, 0.85, 0.9), (1.1, 1.08, 1.05)],
         "greenstone": [(1.0, 1.0, 1.0), (0.9, 0.95, 0.9)]}


def build(kit):
    rng = random.Random(SEED)
    stones = layout(rng)
    high, low = Mesh(), Mesh()
    hdirs, hfaces = sphere_grid(64, 32)
    counts = {}
    for st in stones:
        a = st["radii"][0]
        # Resolution by size so silhouettes stay round without a decimator (which faceted them):
        # 2 cm pebbles get 8 sides, 6-8 cm ones 14.
        nt = int(np.clip(round(4 + a * 2 * 160), 8, 14)) + (3 if st["kind"] == "slate" else 0)
        ldirs, lfaces = sphere_grid(nt, max(4, (nt + 1) // 2))
        c = st["radii"][2]
        z = c * (1.0 - 2.0 * st["bed"])
        xf = (Matrix.Translation((st["x"], st["y"], z)) @ Matrix.Rotation(st["yaw"], 4, "Z")
              @ Matrix.Rotation(st["tilt"][0], 4, "X") @ Matrix.Rotation(st["tilt"][1], 4, "Y"))
        M = np.array(xf)
        slot = SLOTS.index("granite" if st["kind"] == "pink" else st["kind"])
        off = np.array([rng.uniform(-50, 50) for _ in range(3)])
        tint = np.array(rng.choice(TINTS[st["kind"]])) * rng.uniform(0.92, 1.08)
        for target, dirs, faces in ((high, hdirs, hfaces), (low, ldirs, lfaces)):
            P = pebble_points(dirs, st["radii"], st["power"], st["lumps"], st["seed"] % 997)
            Pw = (np.c_[P, np.ones(len(P))] @ M.T)[:, :3]
            target.add(Pw, faces, slot, {"pb_off": off, "pb_tint": tint} if target is high else None)
        counts[st["kind"]] = counts.get(st["kind"], 0) + 1
    materials = [granite_pebble(kit), quartz_pebble(kit), slate_pebble(kit), greenstone_pebble(kit)]
    hi_obj = high.build("PebbleDriftHigh", materials)
    lo_obj = low.build("SM_" + NAME, [])
    total = rocks.triangles(lo_obj)
    if total > LOD0_TRIS:
        reduced = rocks.lod(kit, lo_obj, "PebbleLowReduced", LOD0_TRIS / total)
        bpy.data.objects.remove(lo_obj)
        lo_obj = reduced
        lo_obj.name = lo_obj.data.name = "SM_" + NAME
    kit.tag_coords(hi_obj.data)
    hi_obj.data.shade_smooth()
    lo, hi = kit.bounds(lo_obj)
    shift = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, 0.0))
    for obj in (lo_obj, hi_obj):
        obj.data.transform(Matrix.Translation(-shift))
        obj.data.update()
    kit.pack_uvs(lo_obj, margin=0.003)
    kit.finalize(lo_obj, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
    lo_obj["homestead_shift"] = list(shift)
    folder = Path(__file__).resolve().parents[3] / "Assets" / "Props" / NAME / "Textures"
    material, info = hb.bake(kit, hi_obj, lo_obj, folder, NAME, size=BAKE_SIZE, samples=16 if DRAFT else 64,
                             extrusion=0.004, distance=0.012)
    bpy.data.objects.remove(hi_obj)
    meshes = [lo_obj]
    for index, ratio in enumerate(LOD_RATIOS, start=1):
        lod = rocks.lod(kit, lo_obj, f"SM_{NAME}_LOD{index}", ratio)
        kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=180.0)
        lod.data.materials.clear()
        lod.data.materials.append(material)
        lod["homestead_shift"] = list(shift)
        meshes.append(lod)
    NOTES.update({"bake": info, "pebbles": len(stones), "kinds": counts,
                  "lods": {m.name: rocks.triangles(m) for m in meshes},
                  "material": "Baked from a dense high mesh with kit.mats.granite + quartz/slate/greenstone "
                              "materials (homestead_highbake); M_PropTextured."})
    print("HOMESTEAD_PEBBLES", len(stones), counts, NOTES["lods"], flush=True)
    return meshes
