"""Wild daffodil (Narcissus pseudonarcissus), the Lent lily, in flower: the estate's "Wild daffodils" forage.

The real plant (what the geometry and textures follow):
- A native bulb of damp Cornish meadows, woodland edges and orchard grass, flowering February-April,
  picked by the armful for market in the 19th century.
- Each bulb has 2-4 erect, flat, strap-shaped leaves, 20-35 cm long and 6-12 mm wide, distinctly
  glaucous (grey-green with a bloom), blunt at the tip.
- One flower per bulb on a two-edged scape 20-35 cm tall, bent just below the flower so it faces
  out and a little down. A papery brown spathe sheaths the bend.
- The flower is 5-6 cm across: six pale primrose-yellow tepals spreading and twisting slightly
  round a deeper golden-yellow trumpet (corona) of the same length, with a flared, frilled rim.
- Clumps of several bulbs, the leaves standing up between the flowers.

Game notes: walk-through (no collision). About 30 cm tall and 30 cm across; the two-tone yellow
flowers are the cue. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per
homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "WildDaffodil"
MESH = "SM_WildDaffodilClump"
DESCRIPTION = ("A clump of wild daffodils in flower, 30 cm tall: erect glaucous strap leaves and nodding "
               "flowers with pale yellow tepals round a deeper golden trumpet. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 12000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.03, 0.0, 0.25)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip", "A": "1"},
    "material_notes": ("One material M_WildDaffodil: T_WildDaffodil_basecolor (sRGB, alpha = opacity mask, clip "
                       "0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 3302
HEIGHT = 0.33


def _paint_strap(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    shape = F.ovate(width=a * 0.9, widest=0.5, tip_sharp=0.35, base_round=0.2, base=0.0, tip=0.99)
    veins = [([(0.0, 0.0), (0.0, 0.99)], 0.6)] + [([(s * a * f, 0.02), (s * a * f * 0.7, 0.97)], 0.25)
                                                  for s in (-1, 1) for f in (0.3, 0.6)]
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=a * 0.06, vein_depth=0.00008,
                          puff=0.00004, tertiary=0.0, hair=0.0, trans=0.4, **extra)
    atlas.put(key, layer, meters_per_px=0.3 / X.shape[0])


def _paint_tepal(atlas, key, nrng):
    X, Y, px = atlas.grid(key)
    a = X.max()
    shape = F.ovate(width=a * 0.9, widest=0.45, tip_sharp=1.2, base_round=0.6, base=0.02, tip=0.98)
    veins = [([(0.0, 0.02), (0.0, 0.95)], 1.0)] + [([(s * a * 0.2, 0.05), (s * a * 0.35, 0.85)], 0.5) for s in (-1, 1)]
    pal = dict(base=(0.78, 0.66, 0.16), tip=(0.84, 0.74, 0.24), vein=(0.70, 0.56, 0.10),
               margin=(0.82, 0.72, 0.22))
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=a * 0.05, vein_depth=0.00004,
                          puff=0.00005, tertiary=0.2, trans=0.6)
    layer.color = F.lerp(layer.color, (0.30, 0.34, 0.08), (1 - F.smoothstep(0.02, 0.12, Y)) * 0.6)
    atlas.put(key, layer, meters_per_px=0.03 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.column("trumpet", 160)
    for key in ("leaf", "leaf2"):
        atlas.tile(key, 120, 900)
    atlas.tile("tepal", 220, 360)
    atlas.tile("spathe", 160, 260)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    # Glaucous grey-green with a waxy bloom.
    glaucous = dict(base=(0.060, 0.085, 0.055), tip=(0.068, 0.092, 0.060), vein=(0.075, 0.10, 0.068),
                    margin=(0.055, 0.075, 0.050), brown=(0.09, 0.07, 0.03))
    _paint_strap(atlas, "leaf", nrng, glaucous)
    _paint_strap(atlas, "leaf2", nrng, glaucous, edge_burn=0.25, yellow=0.15)
    _paint_tepal(atlas, "tepal", nrng)
    X, Y, px = atlas.grid("spathe")
    shape = F.ovate(width=X.max() * 0.8, widest=0.3, tip_sharp=1.4, base_round=0.5, base=0.0, tip=0.98)
    spathe = F.paint_blade(X, Y, nrng, shape, [], dict(base=(0.20, 0.15, 0.08), tip=(0.28, 0.22, 0.13),
                           vein=(0.16, 0.11, 0.06), margin=(0.30, 0.25, 0.16)), px, tertiary=0.3, trans=0.5)
    atlas.put("spathe", spathe, meters_per_px=0.03 / X.shape[0])
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.060, 0.090, 0.050), (0.070, 0.098, 0.058), st)
    layer.height = 0.00004 * st
    layer.rough = 0.5 + 0.08 * st
    layer.trans[...] = 0.3
    atlas.put("stem", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    # The trumpet, u around and v from its base to the frilled rim: golden yellow, ribbed.
    U, V = atlas.column_grid("trumpet")
    u = (U - U.min()) / max(U.max() - U.min(), 1e-6)
    n = F.noise(U.shape, nrng, freq=30.0, beta=1.6)
    rib = np.exp(-((np.mod(u * 12.0, 1.0) - 0.5) / 0.12) ** 2)
    tr = F.Layer(U.shape)
    col = F.lerp((0.81, 0.55, 0.06), (0.85, 0.59, 0.075), n)
    col = F.lerp(col, (0.74, 0.48, 0.04), rib * 0.25)
    col = F.lerp(col, (0.40, 0.42, 0.08), (1.0 - F.smoothstep(0.0, 0.15, V)) * 0.6)
    tr.color = col
    tr.height = 0.00008 * rib + 0.00003 * n
    tr.rough = 0.5 + 0.08 * n
    tr.trans[...] = 0.6
    atlas.put("trumpet", tr, meters_per_px=0.05 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    bulbs = []
    for i in range(9):
        az = rng.uniform(0, math.tau)
        r = 0.10 * rng.random() ** 0.5
        base = Vector((math.cos(az) * r, math.sin(az) * r, 0.0))
        leaves = []
        for k in range(rng.randint(3, 4)):
            head = rng.uniform(0, math.tau)
            leaves.append(dict(head=head, length=rng.uniform(0.20, 0.32), width=rng.uniform(0.009, 0.013),
                               elev=math.radians(rng.uniform(62, 82)), droop=rng.uniform(0.15, 0.5),
                               twist=rng.uniform(-0.6, 0.6), key=rng.choices(("leaf", "leaf2"), (70, 30))[0],
                               keep=rng.random()))
        bulbs.append(dict(base=base, leaves=leaves, scape=rng.uniform(0.22, 0.30), face=rng.uniform(0, math.tau),
                          droop=math.radians(rng.uniform(55, 95)), phase=rng.random(), spin=rng.uniform(0, math.tau),
                          flower=i < 7))
    return dict(bulbs=bulbs)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"leaves": 0, "scapes": 0, "flowers": 0}
    for bu in desc["bulbs"]:
        t0 = b.triangles
        for lf in bu["leaves"]:
            if lod == 1 and lf["keep"] > 0.8 or lod == 2 and lf["keep"] > 0.5:
                continue
            h = Vector((math.cos(lf["head"]), math.sin(lf["head"]), 0))
            d = (h * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
            b.card(bu["base"], d, (Vector((0, 0, 1)) - h * 0.5).normalized(), lf["length"], lf["width"],
                   atlas.uv(lf["key"]), rows=(5, 3, 2)[lod], cols=1, fold=0.2 if lod < 2 else 0.0,
                   droop=lf["droop"], twist=lf["twist"], phase=bu["phase"], flutter=0.8, flutter_base=0.0)
        stats["leaves"] += b.triangles - t0
        if not bu["flower"]:
            continue
        t0 = b.triangles
        n = (6, 4, 2)[lod]
        up = Vector((0, 0, 1))
        pts = [bu["base"] + up * bu["scape"] * i / n for i in range(n + 1)]
        b.tube(pts, [0.0022] * (n + 1), (4, 3, 3)[lod], atlas.uv("stem"), v_length=0.3, phase=bu["phase"], flutter=0.2)
        stats["scapes"] += b.triangles - t0
        t0 = b.triangles
        top = pts[-1]
        facing = Vector((math.cos(bu["face"]), math.sin(bu["face"]), 0))
        # The flower's axis: out from the bend and a little down.
        axis = (up * math.cos(bu["droop"]) + facing * math.sin(bu["droop"])).normalized()
        neck = top + axis * 0.012
        if lod < 2:
            b.card(top - up * 0.02, (up * 0.7 + facing * 0.3).normalized(), -facing, 0.035, 0.012,
                   atlas.uv("spathe"), rows=2, cols=1, fold=0.3, droop=0.3, phase=bu["phase"], flutter=0.3)
            b.tube([top, neck], [0.0022, 0.0028], 4, atlas.uv("stem"), v_length=0.3, phase=bu["phase"], flutter=0.3)
        # Six spreading tepals round the trumpet's base.
        for k in range(6 if lod < 2 else 3):
            ang = bu["spin"] + k * (math.tau / (6 if lod < 2 else 3))
            radial = (Matrix.Rotation(ang, 3, axis) @ axis.orthogonal().normalized()).normalized()
            d = (radial * 0.95 + axis * 0.3).normalized()
            b.card(neck, d, axis, 0.026, 0.026 * S.tile_aspect(atlas, "tepal"), atlas.uv("tepal"),
                   rows=2 if lod == 0 else 1, cols=1, fold=0.15, droop=-0.1, twist=0.25, phase=bu["phase"],
                   flutter=0.6, flutter_base=0.2)
        # The trumpet: a ribbed tube as long as the tepals, flaring to a frilled rim.
        along = [0.0, 0.3, 0.6, 0.85, 1.0]
        radii = [0.0055, 0.0065, 0.0075, 0.0095, 0.0135]
        ring = [neck + axis * 0.025 * a for a in along]
        if lod:
            ring, radii = [ring[0], ring[2], ring[-1]], [radii[0], radii[2], radii[-1]]
        b.tube(ring, radii, (10, 6, 5)[lod], atlas.uv("trumpet"), v_length=0.025, phase=bu["phase"], flutter=0.5)
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
