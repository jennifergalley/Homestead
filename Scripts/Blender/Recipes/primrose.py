"""Primrose (Primula vulgaris) in flower: the estate's "Primroses" forage.

The real plant (what the geometry and textures follow):
- A low perennial of Cornish hedge banks, lane verges and woodland edges, flowering from February
  to May; one of the first spring flowers picked for posies and sold in town.
- A basal rosette of 8-15 leaves, 5-20 cm long, obovate (widest two-thirds of the way up) and
  tapering into a winged stalk, the blade deeply wrinkled between the veins, bright to mid green,
  paler and downy beneath; the youngest leaves stand up in the middle, the older ones lie outward.
- Flowers borne singly on soft, hairy pale stalks 6-12 cm long rising from the rosette: 2-4 cm
  across, five broad, notched, pale sulphur-yellow petals, a deeper egg-yellow star at the eye
  and a green-throated tube.

Game notes: walk-through (no collision). About 14 cm tall and 35 cm across; the pale yellow flowers
are the cue from a distance. One 2K atlas and material (alpha-masked, two-sided). Wind vertex
colours per homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "Primrose"
MESH = "SM_PrimroseClump"
DESCRIPTION = ("A primrose in flower, 14 cm tall and 35 cm across: a rosette of wrinkled obovate leaves "
               "with pale sulphur-yellow five-petalled flowers on soft stalks. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 9000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.07)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip", "A": "1"},
    "material_notes": ("One material M_Primrose: T_Primrose_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency mask, "
                       "B AO). Two-sided foliage, masked."),
}

SEED = 6120
HEIGHT = 0.15


def _paint_leaf(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.86
    # Obovate, tapering into a winged stalk, the margin finely and irregularly toothed.
    shape = F.ovate(width=w, widest=0.68, tip_sharp=0.55, base_round=1.6, base=0.02, tip=0.985,
                    teeth=22, tooth_depth=0.025, double=0.4)
    veins = F.pinnate_veins(count=8, angle=0.95, curve=0.6, reach=0.9, base=0.02, tip=0.985, width=w,
                            widest=0.68, start=0.18, stop=0.9, rng=nrng)
    # Deeply wrinkled: sunk veins and a strongly puffed lamina between them.
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.03, vein_depth=0.0003,
                          puff=0.0009, tertiary=1.0, hair=0.25, trans=0.45, **extra)
    atlas.put(key, layer, meters_per_px=0.16 / X.shape[0])


def _paint_flower(atlas, key, nrng):
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    r = np.hypot(Xc, Yc) / 0.5
    th = np.arctan2(Xc, Yc)
    # Five broad petals, each notched at its tip (a heart-shaped rim), meeting at the eye.
    lobe = np.mod(th / (2 * math.pi) * 5 + 0.5, 1.0) - 0.5
    rim = 0.92 - 0.10 * np.abs(lobe) * 2 - 0.12 * np.exp(-(lobe / 0.06) ** 2) - 0.06 * np.abs(lobe) ** 2
    n = F.noise(X.shape, nrng, freq=24.0, beta=1.8)
    inside = rim + 0.02 * (n - 0.5) - r
    layer = F.Layer(X.shape)
    layer.alpha = np.clip(inside / (px * 2.0) + 0.5, 0, 1)
    pale = F.lerp((0.80, 0.64, 0.12), (0.90, 0.74, 0.18), n)
    # The egg-yellow star at the eye, and fine veins running out along the petals.
    star = np.clip(1.0 - (r - 0.22 - 0.07 * np.cos(th * 5)) / 0.08, 0, 1)
    veins = np.exp(-((np.mod(th / (2 * math.pi) * 25, 1.0) - 0.5) / 0.1) ** 2) * F.smoothstep(0.2, 0.7, r) * 0.25
    col = F.lerp(pale, (0.78, 0.40, 0.03), star)
    col = F.lerp(col, pale * 0.85, veins)
    col = F.lerp(col, (0.20, 0.28, 0.07), np.clip(1.0 - r / 0.07, 0, 1))
    layer.color = col
    layer.height = 0.00012 * (1 - F.smoothstep(0.0, 0.5, r)) - 0.00005 * veins + 0.00003 * n
    layer.rough = 0.55 + 0.1 * n
    layer.trans = 0.55 * np.ones(X.shape)
    atlas.put(key, layer, meters_per_px=0.035 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    for key in ("leaf", "leaf2", "leaf_old"):
        atlas.tile(key, 300, 780)
    for key in ("flower", "flower2"):
        atlas.tile(key, 400, 400)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    green = dict(base=(0.060, 0.105, 0.030), tip=(0.070, 0.115, 0.034), vein=(0.085, 0.13, 0.045),
                 margin=(0.055, 0.095, 0.028), brown=(0.09, 0.06, 0.025))
    _paint_leaf(atlas, "leaf", nrng, green)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (0.9, 0.92, 0.9))) for k, v in green.items()})
    _paint_leaf(atlas, "leaf_old", nrng, dict(green, base=(0.07, 0.09, 0.028)), yellow=0.3, edge_burn=0.25, holes=0.3)
    _paint_flower(atlas, "flower", nrng)
    _paint_flower(atlas, "flower2", nrng)
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    # Soft, pale, hairy flower stalks, flushed pink near the base.
    layer.color = F.lerp((0.16, 0.10, 0.08), (0.14, 0.17, 0.08), F.smoothstep(0.0, 0.4, V))
    layer.color = F.lerp(layer.color, (0.30, 0.30, 0.24), F.smoothstep(0.7, 0.95, st) * 0.4)
    layer.height = 0.00004 * st
    layer.rough = 0.6 + 0.08 * st
    layer.trans[...] = 0.35
    atlas.put("stem", layer, meters_per_px=0.15 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    leaves = []
    for i in range(14):
        az = i * 2.39996 + rng.uniform(-0.25, 0.25)
        inner = i < 4
        leaves.append(dict(head=az, length=rng.uniform(0.09, 0.13) if inner else rng.uniform(0.13, 0.19),
                           elev=math.radians(rng.uniform(40, 65) if inner else rng.uniform(8, 25)),
                           droop=rng.uniform(0.0, 0.2) if inner else rng.uniform(0.2, 0.5),
                           key="leaf" if inner else rng.choices(("leaf", "leaf2", "leaf_old"), (40, 40, 20))[0],
                           keep=rng.random(), curl=rng.uniform(0.1, 0.35)))
    flowers = []
    for i in range(11):
        az = rng.uniform(0, math.tau)
        tilt = rng.uniform(0.25, 0.7)
        flowers.append(dict(head=az, tilt=tilt, length=rng.uniform(0.07, 0.12), size=rng.uniform(0.028, 0.036),
                            key=rng.choice(("flower", "flower2")), spin=rng.uniform(0, math.tau),
                            phase=rng.random(), keep=rng.random()))
    return dict(leaves=leaves, flowers=flowers)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "flowers": 0}
    base = Vector((0, 0, 0.004))
    for lf in desc["leaves"]:
        if lod == 1 and lf["keep"] > 0.7 or lod == 2 and lf["keep"] > 0.45:
            continue
        h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        t0 = b.triangles
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.3).normalized(), lf["length"],
               lf["length"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]), rows=(4, 2, 1)[lod],
               cols=(2, 2, 1)[lod], fold=0.12, curl=lf["curl"] if lod == 0 else 0.0, droop=lf["droop"],
               phase=lf["head"] / math.tau, flutter=0.6, flutter_base=0.0)
        stats["leaves"] += b.triangles - t0
    for fl in desc["flowers"]:
        if lod == 1 and fl["keep"] > 0.75 or lod == 2 and fl["keep"] > 0.5:
            continue
        h = Vector((math.cos(fl["head"]), math.sin(fl["head"]), 0))
        d = (Vector((0, 0, 1)) + h * fl["tilt"]).normalized()
        n = (4, 3, 2)[lod]
        # The soft stalk arches outward a little under the flower's weight.
        pts = [base + (d * t + h * 0.02 * t * t / fl["length"]) * fl["length"] for t in np.linspace(0, 1, n + 1)]
        t0 = b.triangles
        b.tube(pts, [0.0010] * (n + 1), (4, 3, 3)[lod], atlas.uv("stem"), v_length=0.15, phase=fl["phase"], flutter=0.4)
        face = (h * 0.8 + Vector((0, 0, 1)) * 0.9).normalized()
        b.flat(pts[-1] + face * 0.003, face, fl["size"], atlas.uv(fl["key"]), spin=fl["spin"], cup=0.12,
               phase=fl["phase"], flutter=0.6, segs=2 if lod == 0 else 1)
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
