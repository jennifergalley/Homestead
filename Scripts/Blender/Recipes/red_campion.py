"""Red campion (Silene dioica) in flower: rose-pink hedgerow flowers along the lake trail.

The real plant (what the geometry and textures follow):
- A softly hairy perennial of Cornish hedge banks, woodland edges and cliff slopes, flowering
  May-June (and on and off through summer), 30-90 cm tall (here about 50 cm).
- A rosette of oval, pointed, hairy basal leaves (6-12 cm), and upright stems with opposite pairs of
  smaller, unstalked oval leaves.
- Loose, forked clusters at the stem tops of rose-pink flowers 18-25 mm across: five petals, each
  deeply cleft into two lobes, a paler throat, from a hairy red-purple tubular calyx. Buds are dark
  red-purple and plump; after flowering the calyx swells round the seed capsule.

Game notes: walk-through (no collision). About 50 cm tall and 40 cm across; the pink flowers float
over the green. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py, the same sway as the bluebell clump.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "RedCampion"
MESH = "SM_RedCampionClump"
DESCRIPTION = ("A clump of red campion in flower, 50 cm tall and 40 cm across: hairy upright stems with paired "
               "oval leaves and loose clusters of rose-pink flowers with deeply cleft petals. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 12000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.03, 0.44)}
REPORT = W.report(NAME)

SEED = 8302
HEIGHT = 0.52


def _paint_leaf(atlas, key, nrng, pal, length_m, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.88
    # Oval and pointed, entire-margined, softly hairy with a few looped side veins.
    shape = F.ovate(width=w, widest=0.42, tip_sharp=1.2, base_round=0.8, base=0.02, tip=0.985)
    veins = F.pinnate_veins(count=5, angle=0.8, curve=0.7, reach=0.9, base=0.02, tip=0.985, width=w,
                            widest=0.42, start=0.12, stop=0.8, rng=nrng)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.03, vein_depth=0.00025,
                          puff=0.0003, tertiary=0.4, hair=0.6, trans=0.45, **extra)
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.column("calyx", 64)
    for key in ("leaf", "leaf_old"):
        atlas.tile(key, 300, 640)
    for key in ("flower", "flower2"):
        atlas.tile(key, 400, 400)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    green = dict(base=(0.040, 0.075, 0.025), tip=(0.046, 0.082, 0.028), vein=(0.065, 0.10, 0.040),
                 margin=(0.040, 0.070, 0.024), brown=(0.09, 0.05, 0.02))
    _paint_leaf(atlas, "leaf", nrng, green, 0.10)
    _paint_leaf(atlas, "leaf_old", nrng, dict(green, base=(0.05, 0.075, 0.025)), 0.10, yellow=0.25, holes=0.2)
    W.paint_flower_face(atlas, "flower", nrng, petals=5, inner=(0.50, 0.12, 0.26), outer=(0.50, 0.04, 0.17),
                        eye=(0.60, 0.26, 0.36), centre=(0.55, 0.44, 0.46), size_m=0.024, notch=0.38,
                        notch_width=0.05, rim=0.95, waist=0.42, veins=0.25, eye_radius=0.22, centre_radius=0.07)
    W.paint_flower_face(atlas, "flower2", nrng, petals=5, inner=(0.54, 0.15, 0.30), outer=(0.55, 0.06, 0.21),
                        eye=(0.62, 0.28, 0.38), centre=(0.56, 0.45, 0.47), size_m=0.022, notch=0.34,
                        notch_width=0.05, rim=0.93, waist=0.40, veins=0.25, eye_radius=0.22, centre_radius=0.07)
    W.paint_stem(atlas, "stem", nrng, (0.10, 0.05, 0.04), (0.06, 0.09, 0.035), hair=0.35, length_m=0.5)
    # The calyx tube: hairy red-purple with darker ribs.
    U, V = atlas.column_grid("calyx")
    u = (U - U.min()) / max(U.max() - U.min(), 1e-6)
    rib = np.exp(-((np.mod(u * 10.0, 1.0) - 0.5) / 0.12) ** 2)
    n = F.noise(U.shape, nrng, freq=40.0, beta=1.5)
    layer = F.Layer(U.shape)
    layer.color = F.lerp(F.lerp((0.20, 0.05, 0.08), (0.14, 0.03, 0.06), rib), (0.30, 0.28, 0.24), n * 0.2)
    layer.height = 0.00005 * rib
    layer.rough = 0.6 + 0.08 * n
    layer.trans[...] = 0.3
    atlas.put("calyx", layer, meters_per_px=0.02 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    rosette = []
    for i in range(9):
        rosette.append(dict(head=i * 2.39996 + rng.uniform(-0.3, 0.3), length=rng.uniform(0.08, 0.12),
                            elev=math.radians(rng.uniform(15, 40)), droop=rng.uniform(0.2, 0.5),
                            key=rng.choices(("leaf", "leaf_old"), (75, 25))[0], keep=rng.random()))
    stems = []
    for i in range(8):
        az = i * 2.39996 + rng.uniform(-0.3, 0.3)
        r = 0.03 + 0.05 * rng.random() ** 0.5
        heads = []
        for k in range(rng.randint(4, 7)):
            heads.append(dict(az=rng.uniform(0, math.tau), out=rng.uniform(0.02, 0.06), up=rng.uniform(0.01, 0.06),
                              size=rng.uniform(0.019, 0.025), key=rng.choice(("flower", "flower2")),
                              bud=rng.random() < 0.25, tilt=rng.uniform(0.3, 0.8), spin=rng.uniform(0, math.tau)))
        stems.append(dict(base=Vector((math.cos(az) * r, math.sin(az) * r, 0.0)), length=rng.uniform(0.36, 0.50),
                          lean=az + rng.uniform(-0.4, 0.4), tilt=rng.uniform(0.10, 0.28), pairs=rng.randint(3, 4),
                          spin=rng.uniform(0, math.pi), heads=heads, phase=rng.random(), keep=rng.random()))
    return dict(rosette=rosette, stems=stems)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "stems": 0, "flowers": 0}
    base = Vector((0, 0, 0.004))
    for lf in desc["rosette"]:
        if lod == 1 and lf["keep"] > 0.7 or lod == 2 and lf["keep"] > 0.45:
            continue
        h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        t0 = b.triangles
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.3).normalized(), lf["length"],
               lf["length"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]), rows=(4, 2, 1)[lod],
               cols=(2, 2, 1)[lod], fold=0.15, droop=lf["droop"], phase=lf["head"] / math.tau, flutter=0.6)
        stats["leaves"] += b.triangles - t0
    for st in desc["stems"]:
        if lod == 2 and st["keep"] > 0.7:
            continue
        lean = Vector((math.cos(st["lean"]), math.sin(st["lean"]), 0.0))
        n = (8, 5, 3)[lod]
        pts = [st["base"] + (Vector((0, 0, 1)) * t + lean * st["tilt"] * t * t) * st["length"]
               for t in np.linspace(0, 1, n + 1)]
        t0 = b.triangles
        b.tube(pts, [0.0022 * (1 - 0.45 * i / n) for i in range(n + 1)], (4, 3, 3)[lod], atlas.uv("stem"),
               v_length=0.5, phase=st["phase"], flutter=0.2)
        stats["stems"] += b.triangles - t0
        # Opposite leaf pairs up the stem, each pair turned a quarter from the last.
        t0 = b.triangles
        acc = S.arclength(pts)
        for p in range(st["pairs"]):
            node, _, _ = S.at(pts, acc, acc[-1] * (0.16 + 0.17 * p))
            for s in (0, math.pi):
                a = st["spin"] + p * math.pi / 2 + s
                h = Vector((math.cos(a), math.sin(a), 0))
                d = (h + Vector((0, 0, 0.55))).normalized()
                length = 0.085 - 0.014 * p
                b.card(node, d, Vector((0, 0, 1)), length, length * S.tile_aspect(atlas, "leaf"), atlas.uv("leaf"),
                       rows=(3, 2, 1)[lod], cols=(2, 1, 1)[lod], fold=0.2, droop=0.3, phase=st["phase"], flutter=0.7)
        stats["leaves"] += b.triangles - t0
        # The loose forked flower cluster at the top: short pedicels, a calyx tube and the open face.
        t0 = b.triangles
        top = pts[-1]
        heads = st["heads"] if lod == 0 else st["heads"][: (3, 2)[lod - 1]]
        for hd in heads:
            out = Vector((math.cos(hd["az"]), math.sin(hd["az"]), 0))
            tip = top + out * hd["out"] + Vector((0, 0, hd["up"]))
            b.tube([top, tip], [0.0011, 0.0009], 3, atlas.uv("stem"), v_length=0.1, phase=st["phase"], flutter=0.5)
            face = (Vector((0, 0, 1)) + out * hd["tilt"]).normalized()
            calyx_end = tip + face * 0.012
            b.tube([tip, calyx_end], [0.0028, 0.0035], (5, 4, 3)[lod], atlas.uv("calyx"), v_length=0.012,
                   phase=st["phase"], flutter=0.5, cap=False)
            if hd["bud"]:
                b.sphere(calyx_end, 0.004, atlas.uv("calyx"), segs=(5, 4, 3)[lod], rings=3, stretch=1.4,
                         axis=face, phase=st["phase"])
            else:
                b.flat(calyx_end + face * 0.001, face, hd["size"], atlas.uv(hd["key"]), spin=hd["spin"], cup=0.1,
                       phase=st["phase"], flutter=0.6, segs=2 if lod == 0 else 1)
        stats["flowers"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    desc = describe(random.Random(SEED))
    batches = [emit(desc, atlas, lod, HEIGHT) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESH, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
