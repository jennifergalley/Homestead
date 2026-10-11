"""Thrift / sea pink (Armeria maritima) in flower: a dense cushion on the cliff and beach-top turf.

The real plant (what the geometry and textures follow):
- A tough, salt-tolerant perennial of sea cliffs, rock ledges and the grassy tops of Cornish and
  Devon beaches, flowering April to July (peak May); it grows in dense, domed cushions 10-25 cm
  across that join into mats.
- Leaves are basal, very many, narrow and grass-like: 2-15 cm long, 1-3 mm wide, one-veined,
  fleshy, dark to mid green and a little glaucous; old dead leaves stay brown and straw-coloured
  inside the base of the cushion.
- Flowers in dense globular heads 1.5-3 cm across on leafless, slightly downy stalks 5-30 cm tall
  (here 10-20 cm), 8-15 per cushion: many small five-petalled florets, pale to rose pink (some
  darker magenta-pink, a few nearly white), with papery, chaffy bracts; under each head a short
  brownish papery sheath wraps the top of the stalk. Old heads fade to papery buff-brown.

Game notes: walk-through (no collision). About 20 cm tall and 22 cm across; the pink heads float
over the dark green cushion. One 2K atlas and material (alpha-masked, two-sided). Wind vertex
colours per homestead_foliage.py. Pivot bottom centre; the cushion skirt starts 8 mm below
z = 0 so it never shows a gap on slopes.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_flowers as W
import homestead_foliage as F

NAME = "Thrift"
MESH = "SM_ThriftClump"
DESCRIPTION = ("A cushion of thrift (sea pinks), 20 cm tall and 22 cm across: dense narrow dark-green leaves "
               "with rose-pink globular flower heads on leafless stalks. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 5000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, 0.0, 0.12), "ground": "origin", "detail_distance": 0.45,
          "detail_fstop": 9.0, "views": ["hero", "detail", "eye"], "eye_distance": 1.4}
REPORT = W.report(NAME)
REPORT["placement"] = "Ground line at z = 0 (pivot); the leaf skirt starts 8 mm below. Random yaw, 0.8-1.2 scale."
NOTES = {"sink_depth_m": 0.008}

SEED = 18513
HEIGHT = 0.20
SINK = 0.008


def _paint_leaf(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.85
    # Linear, blunt-tipped, one-veined, fleshy.
    shape = F.ovate(width=w, widest=0.45, tip_sharp=0.35, base_round=0.15, base=0.0, tip=0.99)
    veins = [([(0.0, 0.0), (0.0, 0.97)], 1.0)]
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.18, vein_depth=0.00004,
                          puff=0.00004, tertiary=0.0, gloss=0.15, trans=0.45, **extra)
    atlas.put(key, layer, meters_per_px=0.12 / X.shape[0])


def _paint_head(atlas, key, nrng, rng, inner, outer, faded=0.0):
    """A dense globular head seen from the side: florets packed on a dome, papery bracts and calyces
    showing between them, pale toward the light."""
    X, Y, px = atlas.grid(key)
    Xc, Yc = X, Y - 0.5
    layer = F.Layer(X.shape)
    alpha = np.zeros(X.shape)
    col = np.zeros(X.shape + (3,))
    height = np.zeros(X.shape)
    rr = np.hypot(Xc, Yc)
    base = np.clip(1.0 - rr / 0.45, 0, 1)
    paper = F.lerp(np.array((0.36, 0.26, 0.20)), np.array((0.46, 0.38, 0.30)), F.noise(X.shape, nrng, freq=40.0))
    alpha = np.clip((0.40 - rr) / (px * 2) + 0.5, 0, 1)
    col[...] = paper * 0.7
    order = sorted(range(70), key=lambda k: rng.random())
    for k in order:
        ang = rng.uniform(0, 2 * math.pi)
        dist = 0.38 * math.sqrt(rng.random())
        cx, cy = dist * math.sin(ang), dist * math.cos(ang)
        size = rng.uniform(0.055, 0.085) * (1.0 - 0.3 * dist / 0.38)
        dx, dy = Xc - cx, Yc - cy
        d = np.hypot(dx, dy) / size
        th = np.arctan2(dx, dy) + rng.uniform(0, 6.28)
        lobe = np.mod(th / (2 * math.pi) * 5 + 0.5, 1.0) - 0.5
        edge = 1.0 - 0.28 * np.abs(lobe) * 2 - d
        a = np.clip(edge / (px * 2.0 / size) + 0.5, 0, 1)
        if not a.any():
            continue
        tone = rng.uniform(0.0, 1.0)
        c = F.lerp(np.array(inner), np.array(outer), tone)
        c = F.lerp(c, np.array((0.40, 0.33, 0.27)), faded * rng.uniform(0.6, 1.0))
        petal = F.lerp(c * 1.12, c * 0.82, np.clip(d, 0, 1))
        eye = np.clip(1.0 - d / 0.25, 0, 1)
        petal = F.lerp(petal, np.array((0.50, 0.40, 0.42)), eye * 0.6)
        col = F.lerp(col, petal, a)
        height = height * (1 - a) + a * (0.0006 * np.clip(1 - d, 0, 1) + 0.0004 * (1 - dist / 0.38))
        alpha = np.maximum(alpha, a)
    ragged = F.noise(X.shape, nrng, freq=30.0, beta=1.5)
    alpha = alpha * np.clip((0.47 - rr - 0.05 * ragged) / (px * 2) + 0.5, 0, 1)
    layer.alpha = alpha
    layer.color = col
    layer.height = height + 0.0003 * base
    layer.rough = 0.55 + 0.1 * ragged
    layer.trans = 0.5 * np.ones(X.shape)
    layer.ao = 0.75 + 0.25 * base
    atlas.put(key, layer, meters_per_px=0.03 / X.shape[0])


def _paint_sheath(atlas, key, nrng):
    U, V = atlas.column_grid(key)
    n = F.noise(U.shape, nrng, freq=40.0, beta=1.5, aniso=(1.0, 4.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp(np.array((0.20, 0.13, 0.08)), np.array((0.36, 0.28, 0.20)), n)
    layer.height = 0.00004 * n
    layer.rough = 0.65 + 0.1 * n
    layer.trans[...] = 0.35
    atlas.put(key, layer, meters_per_px=0.03 / U.shape[0], opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stalk", 40)
    atlas.column("sheath", 40)
    for key in ("leaf", "leaf2", "leaf_dead", "leaf_dry"):
        atlas.tile(key, 72, 900)
    for key in ("head", "head2", "head3", "head_pale", "head_old"):
        atlas.tile(key, 380, 380)
    for key in ("floret", "floret2", "floret3", "floret_old"):
        atlas.tile(key, 170, 170)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    green = dict(base=(0.030, 0.056, 0.020), tip=(0.038, 0.062, 0.024), vein=(0.046, 0.072, 0.030),
                 margin=(0.028, 0.050, 0.020), brown=(0.10, 0.07, 0.035))
    _paint_leaf(atlas, "leaf", nrng, green)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (1.12, 1.1, 1.05))) for k, v in green.items()},
                edge_burn=0.25)
    _paint_leaf(atlas, "leaf_dead", nrng, dict(green, dry=(0.20, 0.14, 0.075), brown=(0.09, 0.06, 0.03)), dry=1.0)
    _paint_leaf(atlas, "leaf_dry", nrng, dict(green, dry=(0.30, 0.25, 0.15), brown=(0.15, 0.11, 0.06)), dry=1.0)
    _paint_head(atlas, "head", nrng, rng, (0.62, 0.24, 0.36), (0.52, 0.13, 0.27))
    _paint_head(atlas, "head2", nrng, rng, (0.70, 0.34, 0.45), (0.58, 0.20, 0.33))
    _paint_head(atlas, "head3", nrng, rng, (0.55, 0.14, 0.30), (0.42, 0.07, 0.20))
    _paint_head(atlas, "head_pale", nrng, rng, (0.74, 0.52, 0.58), (0.66, 0.40, 0.48))
    _paint_head(atlas, "head_old", nrng, rng, (0.50, 0.30, 0.34), (0.40, 0.25, 0.25), faded=0.8)
    # Single florets (5 rounded petals, ~7 mm) that are set all over each head's ball.
    for key, inner, outer in (("floret", (0.64, 0.26, 0.38), (0.54, 0.15, 0.29)),
                              ("floret2", (0.72, 0.38, 0.48), (0.62, 0.24, 0.36)),
                              ("floret3", (0.56, 0.15, 0.31), (0.44, 0.08, 0.21)),
                              ("floret_old", (0.46, 0.34, 0.33), (0.40, 0.30, 0.27))):
        W.paint_flower_face(atlas, key, nrng, petals=5, inner=inner, outer=outer,
                            eye=tuple(c * 0.85 for c in inner), centre=(0.55, 0.45, 0.40), size_m=0.007,
                            notch=0.06, notch_width=0.08, rim=0.94, waist=0.22, veins=0.18, eye_radius=0.22,
                            centre_radius=0.09, dots=5)
    W.paint_stem(atlas, "stalk", nrng, (0.09, 0.09, 0.045), (0.07, 0.085, 0.04), hair=0.35, length_m=0.2)
    _paint_sheath(atlas, "sheath", nrng)
    atlas.save()
    return atlas


def describe(rng):
    leaves = []
    # A domed cushion: very many short, narrow leaves from a tight crown, the outer ones arching
    # out and down over the rim, a few dead ones caught inside.
    n_leaves = 450
    for i in range(n_leaves):
        u = math.sqrt((i + 0.5) / n_leaves)
        az = i * 2.39996 + rng.uniform(-0.2, 0.2)
        r0 = 0.05 * u + rng.uniform(0, 0.008)
        elev = math.radians(86 - 56 * u ** 1.1 + rng.uniform(-10, 10))
        length = rng.uniform(0.035, 0.075) * (1.0 - 0.15 * u)
        key = rng.choices(("leaf", "leaf2", "leaf_dead", "leaf_dry"), (46, 38, 10, 6) if u > 0.45 else (56, 40, 3, 1))[0]
        leaves.append(dict(az=az, r0=r0, elev=elev, length=length, key=key, droop=0.1 + rng.uniform(0.3, 0.8) * u,
                           twist=rng.uniform(-0.8, 0.8), keep=rng.random(), phase=rng.random()))
    heads = []
    count = 12
    for i in range(count):
        az = i * 2.39996 + rng.uniform(-0.4, 0.4)
        r0 = 0.03 * math.sqrt(rng.random())
        length = rng.uniform(0.10, 0.19)
        tilt = rng.uniform(0.05, 0.32)
        key = rng.choices(("head", "head2", "head3", "head_pale", "head_old"), (30, 30, 18, 10, 12))[0]
        heads.append(dict(az=az, r0=r0, length=length, tilt=tilt, size=rng.uniform(0.019, 0.026), key=key,
                          bend=rng.uniform(-0.03, 0.03), spin=rng.uniform(0, math.tau), phase=rng.random(),
                          keep=rng.random()))
    return dict(leaves=leaves, heads=heads)


def _sub(rect, a0, a1, b0, b1):
    u0, u1, v0, v1 = rect
    return (u0 + (u1 - u0) * a0, u0 + (u1 - u0) * a1, v0 + (v1 - v0) * b0, v0 + (v1 - v0) * b1)


FLORET = {"head": "floret", "head2": "floret2", "head3": "floret3", "head_pale": "floret2", "head_old": "floret_old"}


def emit(desc, atlas, lod):
    b = F.Batch(height=HEIGHT)
    stats = {"leaves": 0, "stalks": 0, "heads": 0, "core": 0}
    # The cushion's packed interior: a low dome of leaf texture under the leaves so no sand shows
    # through the middle of the mound (real cushions are solid with old leaf bases).
    t0 = b.triangles
    b.sphere(Vector((0, 0, -0.012)), 0.054, _sub(atlas.uv("leaf2"), 0.38, 0.62, 0.25, 0.75),
             segs=(12, 8, 6)[lod], rings=(6, 4, 3)[lod], stretch=0.6, phase=0.0)
    stats["core"] = b.triangles - t0
    t0 = b.triangles
    for lf in desc["leaves"]:
        if lod == 1 and lf["keep"] > 0.42 or lod == 2 and lf["keep"] > 0.14:
            continue
        h = Vector((math.cos(lf["az"]), math.sin(lf["az"]), 0.0))
        base = h * lf["r0"] + Vector((0, 0, -SINK + 0.006 * (1 - lf["r0"] / 0.07)))
        d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        width = lf["length"] * (72 - 24) / (900 - 24)
        b.card(base, d, (Vector((0, 0, 1)) - h * 0.4).normalized(), lf["length"], width * (1.0, 1.7, 2.6)[lod],
               atlas.uv(lf["key"]), rows=(3 if lf["length"] > 0.055 else 2, 2, 1)[lod], cols=1, fold=0.3, curl=0.0, droop=lf["droop"],
               twist=lf["twist"], phase=lf["phase"], flutter=0.5)
    stats["leaves"] = b.triangles - t0
    for hd in desc["heads"]:
        if lod == 1 and hd["keep"] > 0.8 or lod == 2 and hd["keep"] > 0.55:
            continue
        h = Vector((math.cos(hd["az"]), math.sin(hd["az"]), 0.0))
        side = Vector((-h.y, h.x, 0.0))
        base = h * hd["r0"] + Vector((0, 0, -SINK))
        n = (8, 4, 2)[lod]
        pts = []
        for i in range(n + 1):
            t = i / n
            pts.append(base + Vector((0, 0, hd["length"] * t)) + h * hd["tilt"] * hd["length"] * t * t
                       + side * hd["bend"] * math.sin(t * math.pi))
        t0 = b.triangles
        b.tube(pts, [0.0011 * (1 - 0.2 * i / n) for i in range(n + 1)], (5, 3, 3)[lod], atlas.uv("stalk"),
               v_length=0.2, phase=hd["phase"], flutter=0.1)
        stats["stalks"] += b.triangles - t0
        top = pts[-1]
        axis = (pts[-1] - pts[-2]).normalized()
        t0 = b.triangles
        if lod == 0:
            # The papery sheath wrapping the top of the stalk just below the head.
            b.tube([top - axis * 0.015, top - axis * 0.007, top], [0.0013, 0.0015, 0.0019], 6, atlas.uv("sheath"),
                   v_length=0.03, phase=hd["phase"], flutter=0.2)
        R = hd["size"] * 0.5
        centre = top + axis * R * 0.8
        e1 = h - axis * h.dot(axis)
        e1 = e1.normalized() if e1.length > 1e-6 else axis.orthogonal().normalized()
        e2 = axis.cross(e1).normalized()
        # A ball of papery bracts (core) set all over with outward-facing florets: reads round from
        # every side, unlike crossed cards.
        b.sphere(centre, R * 0.66, _sub(atlas.uv(hd["key"]), 0.32, 0.68, 0.32, 0.68), segs=(8, 6, 5)[lod],
                 rings=(5, 4, 3)[lod], stretch=0.95, axis=axis, phase=hd["phase"])
        nflor = (20, 9, 0)[lod]
        frect = atlas.uv(FLORET[hd["key"]])
        for k in range(nflor):
            zf = 1.0 - 1.7 * (k + 0.5) / nflor
            rr = math.sqrt(max(0.0, 1.0 - zf * zf))
            ang = k * 2.39996 + hd["spin"]
            nrm = axis * zf + e1 * (rr * math.cos(ang)) + e2 * (rr * math.sin(ang))
            b.flat(centre + nrm * R * 0.66, nrm, R * 0.78, frect, spin=ang * 1.7, cup=0.0, phase=hd["phase"],
                   flutter=0.3, segs=1)
        stats["heads"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, b.triangles, stats, flush=True)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    desc = describe(random.Random(SEED))
    batches = [emit(desc, atlas, lod) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESH, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
