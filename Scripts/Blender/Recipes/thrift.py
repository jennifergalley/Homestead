"""Thrift / sea pink (Armeria maritima) in flower: cushions on the cove's cliff ledges (Balance: sea pinks on the
ledges by the sea cave; card "[Balance] Rock face for the cove cliff").

The real plant (what the geometry and textures follow):
- A low, dense evergreen cushion of narrow, grass-like, slightly fleshy dark green leaves (2-10 cm long, 1-3 mm
  wide), rooted in cliff crevices and on sea-washed turf; old leaves brown and wither in the cushion.
- In April-July, many leafless, faintly downy stalks (10-25 cm) rise above it, each topped by one dense round
  head 15-25 mm across of tiny five-petalled pink florets (rose to pale lilac-pink), papery brownish bracts and a
  short papery sheath just under the head. Faded heads go straw-brown.

Game notes: walk-through (no collision). About 22 cm tall and 22 cm across; the pink heads float over the dark
cushion. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "Thrift"
MESH = "SM_ThriftClump"
DESCRIPTION = ("A cushion of thrift (sea pink) in flower, 22 cm tall: a dense tuft of narrow dark leaves under "
               "round pink flower heads on leafless stalks. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 14000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.14)}
REPORT = W.report(NAME)

SEED = 9410
HEIGHT = 0.24


def _paint_leaf(atlas, key, nrng, pal, length_m, **extra):
    X, Y, px = atlas.grid(key)
    w = X.max() * 0.9
    shape = F.ovate(width=w, widest=0.35, tip_sharp=0.9, base_round=0.3, base=0.01, tip=0.99)
    # Linear leaves: only the midrib.
    veins = [([(0.0, 0.01 + 0.98 * t) for t in np.linspace(0, 1, 12)], 1.0)]
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.05, vein_depth=0.0001,
                          puff=0.0002, tertiary=0.0, hair=0.0, trans=0.35, **extra)
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stalk", 48)
    atlas.column("sheath", 48)
    for key in ("leaf", "leaf_old"):
        atlas.tile(key, 90, 720)
    for key in ("head", "head2", "head_old"):
        atlas.tile(key, 420, 420)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    green = dict(base=(0.030, 0.055, 0.022), tip=(0.036, 0.062, 0.026), vein=(0.045, 0.075, 0.030),
                 margin=(0.030, 0.052, 0.020), brown=(0.10, 0.07, 0.04))
    _paint_leaf(atlas, "leaf", nrng, green, 0.08)
    _paint_leaf(atlas, "leaf_old", nrng, dict(green, base=(0.07, 0.06, 0.03), tip=(0.12, 0.09, 0.05)), 0.07,
                yellow=0.5)
    W.paint_umbellet(atlas, "head", nrng, rng, florets=34, colour=(0.62, 0.28, 0.42), size_m=0.022)
    W.paint_umbellet(atlas, "head2", nrng, rng, florets=30, colour=(0.66, 0.34, 0.48), size_m=0.020)
    W.paint_umbellet(atlas, "head_old", nrng, rng, florets=28, colour=(0.30, 0.24, 0.17), size_m=0.020)
    W.paint_stem(atlas, "stalk", nrng, (0.07, 0.09, 0.04), (0.10, 0.12, 0.06), hair=0.25, length_m=0.2)
    W.paint_stem(atlas, "sheath", nrng, (0.30, 0.24, 0.17), (0.40, 0.33, 0.24), hair=0.0, length_m=0.02, trans=0.5)
    atlas.save()
    return atlas


def describe(rng):
    leaves = []
    for i in range(150):
        az = i * 2.39996 + rng.uniform(-0.2, 0.2)
        r = 0.075 * math.sqrt(rng.random())
        leaves.append(dict(az=az, r=r, length=rng.uniform(0.05, 0.09) * (1.0 - 0.3 * r / 0.075),
                           elev=math.radians(rng.uniform(35, 80) - 30 * r / 0.075), droop=rng.uniform(0.2, 0.6),
                           key=rng.choices(("leaf", "leaf_old"), (82, 18))[0], keep=rng.random()))
    stalks = []
    for i in range(rng.randint(10, 14)):
        az = rng.uniform(0, math.tau)
        r = 0.05 * math.sqrt(rng.random())
        stalks.append(dict(base=Vector((math.cos(az) * r, math.sin(az) * r, 0.03)), length=rng.uniform(0.12, 0.21),
                           lean=az + rng.uniform(-0.5, 0.5), tilt=rng.uniform(0.05, 0.3),
                           size=rng.uniform(0.018, 0.024),
                           key=rng.choices(("head", "head2", "head_old"), (62, 34, 4))[0],
                           spin=rng.uniform(0, math.tau), phase=rng.random(), keep=rng.random()))
    return dict(leaves=leaves, stalks=stalks)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "stalks": 0, "heads": 0}
    for lf in desc["leaves"]:
        if lod == 1 and lf["keep"] > 0.6 or lod == 2 and lf["keep"] > 0.3:
            continue
        h = Vector((math.cos(lf["az"]), math.sin(lf["az"]), 0))
        base = Vector((math.cos(lf["az"]) * lf["r"], math.sin(lf["az"]) * lf["r"], 0.002))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        t0 = b.triangles
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.4).normalized(), lf["length"],
               lf["length"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]), rows=(3, 2, 1)[lod], cols=1,
               fold=0.3, droop=lf["droop"], phase=lf["az"] / math.tau, flutter=0.4)
        stats["leaves"] += b.triangles - t0
    for st in desc["stalks"]:
        if lod == 2 and st["keep"] > 0.6:
            continue
        lean = Vector((math.cos(st["lean"]), math.sin(st["lean"]), 0.0))
        n = (6, 4, 2)[lod]
        pts = [st["base"] + (Vector((0, 0, 1)) * t + lean * st["tilt"] * t * t) * st["length"]
               for t in np.linspace(0, 1, n + 1)]
        t0 = b.triangles
        b.tube(pts, [0.0013] * (n + 1), (4, 3, 3)[lod], atlas.uv("stalk"), v_length=0.2, phase=st["phase"],
               flutter=0.3)
        stats["stalks"] += b.triangles - t0
        t0 = b.triangles
        top = pts[-1]
        axis = (pts[-1] - pts[-2]).normalized()
        # The papery sheath under the head, then the round head as three crossed florets faces.
        b.tube([top - axis * 0.012, top], [0.0022, 0.0030], (5, 4, 3)[lod], atlas.uv("sheath"), v_length=0.02,
               phase=st["phase"], flutter=0.5, cap=False)
        centre = top + axis * st["size"] * 0.35
        faces = [axis] if lod == 2 else [axis] + [(axis + Vector((math.cos(st["spin"] + k * 2.1),
                                                                   math.sin(st["spin"] + k * 2.1), 0)) * 0.9).normalized()
                                              for k in range((3, 2)[lod])]
        for k, f in enumerate(faces):
            b.flat(centre - f * st["size"] * 0.12, f, st["size"] * (1.0 if k == 0 else 0.92), atlas.uv(st["key"]),
                   spin=st["spin"] + k, cup=0.35, phase=st["phase"], flutter=0.6, segs=2 if lod == 0 else 1)
        stats["heads"] += b.triangles - t0
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
