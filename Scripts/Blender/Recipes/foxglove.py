"""Foxglove (Digitalis purpurea) in flower: a tall purple spike for the lake trail's banks.

The real plant (what the geometry and textures follow):
- A biennial of Cornish hedge banks, woodland clearings and heaths, flowering June-July, 50-150 cm
  (here about 90 cm).
- A rosette of large, soft, downy, grey-green leaves, oval to lance-shaped, 15-30 cm, wrinkled with a
  network of sunk veins; up the single stem the leaves get smaller and stalkless.
- A one-sided spike over the top half: 20-80 tubular, drooping bells, 4-6 cm long, pinkish-purple
  outside, the paler lower lip spotted inside with dark purple rings on white; the lowest open first,
  so the tip is a taper of green-purple buds.

Game notes: walk-through (no collision). About 90 cm tall and 50 cm across; the purple spike reads
from far down the trail. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py, the same sway as the bluebell clump (the tall spike sways the most).
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "Foxglove"
MESH = "SM_Foxglove"
DESCRIPTION = ("A foxglove in flower, 90 cm tall: a rosette of large wrinkled grey-green leaves and a single "
               "one-sided spike of drooping purple bells, spotted inside, tapering to buds. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 14000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.5)}
REPORT = W.report(NAME)

SEED = 9116
HEIGHT = 0.92


def _paint_leaf(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.86
    # Lance-oval, finely round-toothed, deeply wrinkled by a sunk net of veins, softly downy.
    shape = F.ovate(width=w, widest=0.38, tip_sharp=1.0, base_round=0.7, base=0.02, tip=0.985,
                    teeth=26, tooth_depth=0.012)
    veins = F.pinnate_veins(count=9, angle=0.85, curve=0.6, reach=0.9, base=0.02, tip=0.985, width=w,
                            widest=0.38, start=0.08, stop=0.92, rng=nrng)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.025, vein_depth=0.0004,
                          puff=0.0010, tertiary=1.0, hair=0.7, trans=0.4, **extra)
    atlas.put(key, layer, meters_per_px=0.26 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.column("bell", 200)
    atlas.column("bud", 64)
    for key in ("leaf", "leaf2", "leaf_old"):
        atlas.tile(key, 340, 900)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    grey = dict(base=(0.055, 0.080, 0.040), tip=(0.060, 0.086, 0.044), vein=(0.085, 0.110, 0.060),
                margin=(0.055, 0.075, 0.040), brown=(0.09, 0.06, 0.03))
    _paint_leaf(atlas, "leaf", nrng, grey)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (0.92, 0.95, 0.9))) for k, v in grey.items()})
    _paint_leaf(atlas, "leaf_old", nrng, dict(grey, base=(0.07, 0.08, 0.035)), yellow=0.3, edge_burn=0.3, holes=0.2)
    W.paint_stem(atlas, "stem", nrng, (0.07, 0.08, 0.04), (0.10, 0.06, 0.07), hair=0.4, length_m=0.9)
    # The bell, u around and v from the pedicel to the mouth: rosy purple outside, the lower half of the
    # mouth pale and ringed with dark spots (the lower lip faces down, at u around 0.5).
    U, V = atlas.column_grid("bell")
    u = (U - U.min()) / max(U.max() - U.min(), 1e-6)
    n = F.noise(U.shape, nrng, freq=30.0, beta=1.6)
    bell = F.Layer(U.shape)
    col = F.lerp((0.36, 0.10, 0.28), (0.46, 0.16, 0.36), n)
    col = F.lerp(col, (0.30, 0.20, 0.10), (1.0 - F.smoothstep(0.0, 0.1, V)) * 0.6)
    lip = np.exp(-((u - 0.5) / 0.22) ** 2) * F.smoothstep(0.55, 0.9, V)
    col = F.lerp(col, (0.72, 0.60, 0.66), lip * 0.8)
    spots = np.zeros(U.shape)
    for k in range(14):
        cu = 0.5 + nrng.uniform(-0.18, 0.18)
        cv = nrng.uniform(0.6, 0.92)
        d = np.hypot((u - cu) * 4.0, (V - cv) * 1.2)
        spots = np.maximum(spots, np.clip(1.0 - d / 0.05, 0, 1))
        ring = np.clip(1.0 - np.abs(d - 0.06) / 0.015, 0, 1)
        col = F.lerp(col, (0.85, 0.82, 0.84), ring * lip)
    col = F.lerp(col, (0.16, 0.03, 0.10), spots * lip)
    bell.color = col
    bell.height = 0.00006 * n - 0.00004 * spots
    bell.rough = 0.5 + 0.1 * n
    bell.trans[...] = 0.55
    atlas.put("bell", bell, meters_per_px=0.06 / U.shape[0], opaque=True)
    U, V = atlas.column_grid("bud")
    n = F.noise(U.shape, nrng, freq=30.0, beta=1.6)
    bud = F.Layer(U.shape)
    bud.color = F.lerp((0.12, 0.13, 0.06), (0.26, 0.10, 0.20), F.smoothstep(0.3, 1.0, V) * (0.7 + 0.3 * n))
    bud.rough = 0.55 + 0.1 * n
    bud.trans[...] = 0.4
    atlas.put("bud", bud, meters_per_px=0.02 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    rosette = []
    for i in range(12):
        inner = i < 4
        rosette.append(dict(head=i * 2.39996 + rng.uniform(-0.25, 0.25),
                            length=rng.uniform(0.15, 0.20) if inner else rng.uniform(0.20, 0.27),
                            elev=math.radians(rng.uniform(35, 55) if inner else rng.uniform(10, 25)),
                            droop=rng.uniform(0.2, 0.5) if inner else rng.uniform(0.4, 0.8),
                            key="leaf" if inner else rng.choices(("leaf", "leaf2", "leaf_old"), (40, 40, 20))[0],
                            keep=rng.random(), curl=rng.uniform(0.1, 0.3)))
    side = rng.uniform(0, math.tau)
    bells = []
    count = 36
    for k in range(count):
        t = k / (count - 1)
        bells.append(dict(t=0.42 + 0.55 * t, az=side + rng.uniform(-1.1, 1.1),
                          length=(0.048 - 0.014 * t) * rng.uniform(0.92, 1.08), bud=t > 0.66, phase=rng.random()))
    return dict(rosette=rosette, stem=dict(lean=rng.uniform(0, math.tau), tilt=rng.uniform(0.03, 0.08)),
                bells=bells, side=side, stem_leaves=[dict(t=0.1 + 0.08 * k, az=k * 2.39996 + side,
                                                         length=0.14 - 0.022 * k) for k in range(5)])


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "stem": 0, "bells": 0}
    base = Vector((0, 0, 0.004))
    for lf in desc["rosette"]:
        if lod == 1 and lf["keep"] > 0.7 or lod == 2 and lf["keep"] > 0.45:
            continue
        h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        t0 = b.triangles
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.3).normalized(), lf["length"],
               lf["length"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]), rows=(5, 3, 1)[lod],
               cols=(2, 2, 1)[lod], fold=0.15, curl=lf["curl"] if lod == 0 else 0.0, droop=lf["droop"],
               phase=lf["head"] / math.tau, flutter=0.5)
        stats["leaves"] += b.triangles - t0
    st = desc["stem"]
    lean = Vector((math.cos(st["lean"]), math.sin(st["lean"]), 0.0))
    n = (12, 7, 4)[lod]
    length = 0.88
    pts = [base + (Vector((0, 0, 1)) * t + lean * st["tilt"] * t * t) * length for t in np.linspace(0, 1, n + 1)]
    t0 = b.triangles
    b.tube(pts, [0.0045 * (1 - 0.7 * i / n) for i in range(n + 1)], (5, 4, 3)[lod], atlas.uv("stem"),
           v_length=0.9, phase=0.37, flutter=0.15)
    stats["stem"] += b.triangles - t0
    acc = S.arclength(pts)
    t0 = b.triangles
    for sl in desc["stem_leaves"][: (5, 3, 1)[lod]]:
        node, _, _ = S.at(pts, acc, acc[-1] * sl["t"])
        h = Vector((math.cos(sl["az"]), math.sin(sl["az"]), 0))
        b.card(node, (h + Vector((0, 0, 0.7))).normalized(), Vector((0, 0, 1)), sl["length"],
               sl["length"] * S.tile_aspect(atlas, "leaf2"), atlas.uv("leaf2"), rows=(3, 2, 1)[lod],
               cols=(2, 1, 1)[lod], fold=0.2, droop=0.35, phase=0.37, flutter=0.6)
    stats["leaves"] += b.triangles - t0
    t0 = b.triangles
    for k, bl in enumerate(desc["bells"]):
        if lod == 1 and k % 3 == 2 or lod == 2 and k % 2:
            continue
        node, tan, _ = S.at(pts, acc, acc[-1] * bl["t"])
        out = Vector((math.cos(bl["az"]), math.sin(bl["az"]), 0))
        ped_end = node + out * 0.010 - Vector((0, 0, 0.002))
        if lod == 0:
            b.tube([node, ped_end], [0.0012, 0.0010], 3, atlas.uv("stem"), v_length=0.1, phase=bl["phase"], flutter=0.5)
        if bl["bud"]:
            down = (out * 0.6 - Vector((0, 0, 0.5))).normalized()
            size = 0.0045 + 0.007 * (0.97 - bl["t"]) / 0.32
            b.sphere(ped_end + down * size, size, atlas.uv("bud"), segs=(6, 4, 3)[lod], rings=4 if lod == 0 else 3,
                     stretch=1.7, axis=down, phase=bl["phase"])
            continue
        # The bell hangs out and down from its pedicel, the mouth flaring and the lower lip jutting.
        down = (out * 0.75 - Vector((0, 0, 0.66))).normalized()
        L = bl["length"]
        along = [0.0, 0.15, 0.45, 0.75, 0.95, 1.0]
        radii = [L * r for r in (0.07, 0.14, 0.20, 0.23, 0.26, 0.29)]
        ring = [ped_end + down * L * a for a in along]
        # The lower lip juts beyond the upper.
        ring[-1] = ring[-1] - Vector((0, 0, L * 0.06))
        sides = (8, 6, 4)[lod]
        roll = math.atan2(out.y, out.x)
        idx = (range(6) if lod == 0 else (0, 2, 5))
        b.tube([ring[i] for i in idx], [radii[i] for i in idx], sides, atlas.uv("bell"), v_length=L,
               phase=bl["phase"], flutter=0.4, cap=False, roll=roll)
    stats["bells"] += b.triangles - t0
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
