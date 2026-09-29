"""Native bluebell (Hyacinthoides non-scripta) in flower: the estate's "Bluebells" forage.

The real plant (what the geometry and textures follow):
- A bulb of Cornish woodland edges, hedge banks and bracken slopes, flowering April-May in drifts.
- Each bulb sends up 3-6 linear, glossy, dark green leaves, 20-45 cm long and 7-15 mm wide, with a
  hooded tip and a pale groove down the middle; they flop outward and lie half over by flowering.
- One flower scape per bulb, 20-50 cm, smooth and green, nodding over strongly at the top so the
  flowers hang down on one side (the native bluebell's tell against the upright Spanish kind).
- 5-12 narrow, tubular bells, 14-20 mm, deep violet-blue, the six tepals curling back at the tips,
  each hanging from a short pedicel with a small bract; cream anthers inside.
- Clumps of several bulbs make a low mound of leaves with the flower scapes arching over it.

Game notes: walk-through (no collision). About 35 cm tall and 45 cm across; the violet-blue bells
are the cue from a distance. One 2K atlas and material (alpha-masked, two-sided). Wind vertex
colours per homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "Bluebell"
MESH = "SM_BluebellClump"
DESCRIPTION = ("A clump of native bluebells in flower, 35 cm tall and 45 cm across: glossy strap leaves "
               "flopping outward and nodding scapes hung on one side with deep violet-blue bells. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 16000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.04, 0.0, 0.24)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip", "A": "1"},
    "material_notes": ("One material M_Bluebell: T_Bluebell_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency mask, "
                       "B AO). Two-sided foliage, masked."),
}

SEED = 4417
HEIGHT = 0.36


def _paint_strap(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    shape = F.ovate(width=a * 0.92, widest=0.55, tip_sharp=0.45, base_round=0.25, base=0.0, tip=0.99)
    veins = [([(0.0, 0.0), (0.0, 0.99)], 1.0)] + [([(s * a * 0.45, 0.02), (s * a * 0.3, 0.97)], 0.3)
                                                  for s in (-1, 1)]
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=a * 0.12, vein_depth=0.00015,
                          puff=0.00006, tertiary=0.0, gloss=0.8, trans=0.45, **extra)
    atlas.put(key, layer, meters_per_px=0.35 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.column("bell", 160)
    for key in ("leaf", "leaf2", "leaf_old"):
        atlas.tile(key, 120, 900)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    green = dict(base=(0.030, 0.060, 0.022), tip=(0.034, 0.066, 0.024), vein=(0.060, 0.090, 0.040),
                 margin=(0.028, 0.052, 0.020), brown=(0.08, 0.05, 0.02))
    _paint_strap(atlas, "leaf", nrng, green)
    _paint_strap(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (1.1, 1.08, 0.95))) for k, v in green.items()})
    _paint_strap(atlas, "leaf_old", nrng, green, yellow=0.25, edge_burn=0.3)
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.050, 0.075, 0.030), (0.045, 0.070, 0.035), st)
    layer.height = 0.00004 * st
    layer.rough = 0.45 + 0.08 * st
    layer.trans[...] = 0.3
    atlas.put("stem", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    # The bell, u around and v from the pedicel to the mouth: deep violet-blue with darker
    # mid-vein stripes on each tepal, paler at the recurved tips, a green flush at the top.
    U, V = atlas.column_grid("bell")
    u = (U - U.min()) / max(U.max() - U.min(), 1e-6)
    n = F.noise(U.shape, nrng, freq=30.0, beta=1.6)
    stripe = np.exp(-((np.mod(u * 6.0, 1.0) - 0.5) / 0.08) ** 2)
    bell = F.Layer(U.shape)
    col = F.lerp((0.070, 0.050, 0.25), (0.105, 0.075, 0.33), n)
    col = F.lerp(col, (0.045, 0.028, 0.16), stripe * 0.55)
    col = F.lerp(col, (0.17, 0.13, 0.40), F.smoothstep(0.8, 1.0, V) * 0.6)
    col = F.lerp(col, (0.05, 0.08, 0.04), (1.0 - F.smoothstep(0.0, 0.12, V)) * 0.8)
    bell.color = col
    bell.height = 0.00006 * n - 0.00008 * stripe
    bell.rough = 0.55 + 0.1 * n
    bell.trans[...] = 0.6
    atlas.put("bell", bell, meters_per_px=0.06 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    bulbs = []
    for i in range(6):
        az = rng.uniform(0, math.tau)
        r = 0.11 * rng.random() ** 0.5
        base = Vector((math.cos(az) * r, math.sin(az) * r, 0.0))
        leaves = []
        for k in range(rng.randint(4, 6)):
            head = rng.uniform(0, math.tau)
            leaves.append(dict(head=head, length=rng.uniform(0.20, 0.36), width=rng.uniform(0.014, 0.020),
                               elev=math.radians(rng.uniform(30, 60)), droop=rng.uniform(0.8, 1.4),
                               twist=rng.uniform(-0.8, 0.8), key=rng.choices(("leaf", "leaf2", "leaf_old"), (50, 35, 15))[0],
                               keep=rng.random()))
        lean = rng.uniform(0, math.tau)
        bulbs.append(dict(base=base, leaves=leaves, scape=rng.uniform(0.24, 0.36), lean=lean,
                          nod=rng.uniform(1.0, 1.6), bells=rng.randint(6, 10), phase=rng.random(),
                          seed=rng.randrange(1 << 20)))
    return dict(bulbs=bulbs)


def _scape(base, length, lean, nod, n):
    """Straight and upright for two-thirds, then nodding over toward ``lean``."""
    heading = Vector((math.cos(lean), math.sin(lean), 0.0))
    d = (Vector((0, 0, 1)) + heading * 0.12).normalized()
    side = heading.cross(Vector((0, 0, 1))).normalized()
    pts = [base.copy()]
    for i in range(n):
        t = (i + 0.5) / n
        if t > 0.6:
            d = (Matrix.Rotation(nod / (n * 0.4), 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "scapes": 0, "bells": 0}
    for bu in desc["bulbs"]:
        rng = random.Random(bu["seed"])
        t0 = b.triangles
        for lf in bu["leaves"]:
            if lod == 1 and lf["keep"] > 0.75 or lod == 2 and lf["keep"] > 0.45:
                continue
            h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
            d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
            b.card(bu["base"], d, (Vector((0, 0, 1)) - h * 0.5).normalized(), lf["length"], lf["width"],
                   atlas.uv(lf["key"]), rows=(6, 4, 2)[lod], cols=(2, 1, 1)[lod], fold=0.35 if lod < 2 else 0.0,
                   droop=lf["droop"], twist=lf["twist"], phase=bu["phase"], flutter=0.9, flutter_base=0.0)
        stats["leaves"] += b.triangles - t0
        t0 = b.triangles
        n = (8, 5, 3)[lod]
        pts = _scape(bu["base"], bu["scape"], bu["lean"], bu["nod"], n)
        b.tube(pts, [0.0022 * (1 - 0.4 * i / n) for i in range(n + 1)], (4, 3, 3)[lod], atlas.uv("stem"),
               v_length=0.4, phase=bu["phase"], flutter=0.2)
        stats["scapes"] += b.triangles - t0
        t0 = b.triangles
        acc = S.arclength(pts)
        heading = Vector((math.cos(bu["lean"]), math.sin(bu["lean"]), 0.0))
        count = bu["bells"] if lod == 0 else max(3, bu["bells"] // (2 if lod == 1 else 3))
        for k in range(count):
            # Bells along the top third, the youngest (smallest) at the tip, all hanging to one side.
            t = 0.58 + 0.42 * k / max(count - 1, 1)
            node, tan, _ = S.at(pts, acc, acc[-1] * min(t, 0.995))
            size = (0.018 - 0.006 * (k / max(count - 1, 1))) * rng.uniform(0.9, 1.1) * (1.0, 1.1, 1.3)[lod]
            side = heading * 0.8 + Vector((rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 0))
            ped_end = node + side.normalized() * 0.008 - Vector((0, 0, 0.004))
            if lod == 0:
                b.tube([node, ped_end], [0.0007, 0.0006], 3, atlas.uv("stem"), v_length=0.4, phase=bu["phase"], flutter=0.5)
            down = (Vector((0, 0, -1)) + side.normalized() * 0.35).normalized()
            radii = [size * r for r in (0.10, 0.20, 0.22, 0.24, 0.34, 0.50)]
            along = [0.0, 0.18, 0.45, 0.72, 0.9, 0.96]
            ring = [ped_end + down * size * a for a in along]
            # The recurved tepal tips: the last ring steps back up round the mouth.
            ring[-1] = ring[-1] - down * size * 0.08
            sides = (6, 5, 4)[lod]
            b.tube(ring if lod == 0 else [ring[0], ring[2], ring[-1]],
                   radii if lod == 0 else [radii[0], radii[2], radii[-1]], sides, atlas.uv("bell"),
                   v_length=size, phase=bu["phase"], flutter=0.6, cap=False)
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
