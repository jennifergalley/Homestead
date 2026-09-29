"""Stinging nettle (Urtica dioica) in spring: the clear-out's "Nettles" patch.

The real plant (what the geometry and textures follow):
- A rhizomatous perennial that colonises rich, disturbed ground: middens, farmyards, the foot of
  old walls and neglected kitchen gardens (nitrogen from ash and dung). It spreads into patches
  a metre or more across; on the 1850s estate it would be thick round the yard and outbuildings.
- In spring (the game opens on Spring day 1) the new shoots are 25-60 cm tall, unbranched, square
  in section, green flushed purple low down, bristly with stinging hairs. Last year's dead stalks
  (to 1.2 m, grey-brown, fibrous, leafless or with a few shrivelled leaves) still stand among them.
- Leaves opposite and decussate on stalks a third to half the blade, heart-shaped at the base and
  drawn out to a long point, 4-10 cm long, coarsely and regularly serrated (large forward teeth),
  dark dull green, finely hairy; the youngest leaves crowd into a bright, slightly glossy rosette
  at each shoot tip (the "tops" picked for nettle soup).
- No flowers yet (the green catkin-like strands come in June).

Game notes: walk-through (no collision). About 60 cm tall and 75 cm across, so a patch reads above
the pasture and among the clear-out's weeds; the dark, toothed leaves and grey dead stalks are the
cue. One 2K atlas and material (alpha-masked, two-sided). Wind vertex colours per homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "Nettle"
MESH = "SM_NettlePatch"
DESCRIPTION = ("A spring patch of stinging nettle, 60 cm tall and 75 cm across: square bristly shoots "
               "flushed purple at the base, opposite heart-shaped coarsely toothed dark leaves, bright "
               "young tip rosettes, and last year's grey-brown dead stalks standing among them. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 22000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.05, -0.03, 0.40)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip",
             "A": "1"},
    "material_notes": ("One material M_Nettle: T_Nettle_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 8812
HEIGHT = 0.62
LEAVES = ("leaf", "leaf2", "leaf_young", "leaf_old")


def _paint_leaf(atlas, key, nrng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.78
    # Cordate base, long acuminate tip, big regular forward teeth.
    shape = F.ovate(width=w, widest=0.30, tip_sharp=1.7, base_round=0.55, base=0.10, tip=0.985,
                    teeth=10, tooth_depth=0.13, double=0.0, cordate=0.28)
    veins = F.pinnate_veins(count=5, angle=0.7, curve=0.7, reach=0.85, base=0.10, tip=0.985, width=w,
                            widest=0.30, start=0.10, stop=0.68, rng=nrng)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.04, vein_depth=0.00012,
                          puff=0.00014, tertiary=0.45, hair=0.45, stalk=a * 0.05, trans=0.4, **extra)
    atlas.put(key, layer, meters_per_px=0.08 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    atlas.column("dead", 48)
    for key in LEAVES:
        atlas.tile(key, 360, 520)
    atlas.tile("leaf_dead", 300, 440)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    # Dull, dark nettle green; the young tips brighter and a touch yellower.
    dark = dict(base=(0.026, 0.046, 0.016), tip=(0.030, 0.052, 0.018), vein=(0.040, 0.062, 0.024),
                margin=(0.024, 0.040, 0.014), stalk=(0.070, 0.050, 0.032), brown=(0.08, 0.045, 0.02))
    _paint_leaf(atlas, "leaf", nrng, dark)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (1.1, 1.12, 1.0))) for k, v in dark.items()},
                damage=0.12, holes=0.2)
    _paint_leaf(atlas, "leaf_young", nrng, dict(dark, base=(0.042, 0.080, 0.020), tip=(0.052, 0.092, 0.022),
                                                margin=(0.040, 0.070, 0.018)), gloss=0.5)
    _paint_leaf(atlas, "leaf_old", nrng, dict(dark, base=(0.040, 0.050, 0.016), tip=(0.055, 0.052, 0.016)),
                yellow=0.3, edge_burn=0.35, holes=0.4)
    _paint_leaf(atlas, "leaf_dead", nrng, dict(dark, dry=(0.13, 0.095, 0.060), brown=(0.07, 0.05, 0.03)), dry=1.0)
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    bristle = F.smoothstep(0.86, 0.97, F.noise(U.shape, nrng, freq=180.0, beta=1.1))
    layer = F.Layer(U.shape)
    # Purple-flushed at the base, green above, flecked with pale bristles.
    layer.color = F.lerp((0.075, 0.030, 0.040), (0.036, 0.060, 0.020), F.smoothstep(0.05, 0.45, V))
    layer.color = F.lerp(layer.color, (0.20, 0.22, 0.17), bristle * 0.45)
    layer.height = 0.00006 * st + 0.00008 * bristle
    layer.rough = 0.58 + 0.08 * st
    layer.trans[...] = 0.2
    atlas.put("stem", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    # Last year's stalk: fibrous grey-brown, weathered paler on the ribs.
    U, V = atlas.column_grid("dead")
    fib = F.noise(U.shape, nrng, freq=90.0, beta=1.2, aniso=(1.0, 14.0))
    dead = F.Layer(U.shape)
    dead.color = F.lerp((0.085, 0.065, 0.045), (0.17, 0.145, 0.11), F.smoothstep(0.35, 0.85, fib))
    dead.color = F.lerp(dead.color, (0.05, 0.04, 0.03), F.smoothstep(0.0, 0.2, V) * 0.5)
    dead.height = 0.0001 * fib
    dead.rough = 0.8 + 0.1 * fib
    dead.trans[...] = 0.05
    atlas.put("dead", dead, meters_per_px=0.5 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def _stalk(base, d, length, bend, n, wobble=0.0, rng=None):
    side = d.cross(Vector((0, 0, 1)))
    side = side.normalized() if side.length > 1e-4 else Vector((1, 0, 0))
    pts = [base.copy()]
    for i in range(n):
        d = (Matrix.Rotation(-bend / n, 3, side) @ d).normalized()
        if wobble and rng:
            d = (d + Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)) * wobble).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def describe(rng):
    shoots = []
    # A rhizome patch: shoots in loose drifts rather than one crown.
    drifts = [Vector((rng.uniform(-0.2, 0.2), rng.uniform(-0.2, 0.2), 0)) for _ in range(6)]
    for i in range(34):
        c = drifts[i % len(drifts)]
        az = rng.uniform(0, math.tau)
        base = c + Vector((math.cos(az), math.sin(az), 0)) * 0.12 * rng.random() ** 0.5
        base.z = 0.0
        lean = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)) * rng.uniform(0.04, 0.2)
        length = rng.uniform(0.24, 0.56)
        spin = rng.uniform(0, math.pi)
        count = max(5, int(length / 0.045))
        nodes = []
        for k in range(count):
            t = 0.10 + 0.87 * (k + rng.uniform(-0.1, 0.1)) / count
            # Largest leaves a third of the way up, tapering to the tip rosette.
            size = (0.065 + 0.06 * math.sin(math.pi * min(1.0, t * 1.4))) * (0.75 + 0.5 * length / 0.56)
            key = "leaf_old" if t < 0.2 and rng.random() < 0.6 else rng.choices(("leaf", "leaf2"), (60, 40))[0]
            nodes.append(dict(t=t, size=size * rng.uniform(0.85, 1.12), key=key, keep=rng.random(),
                              elev=math.radians(rng.uniform(20, 45)), droop=rng.uniform(0.35, 0.8)))
        shoots.append(dict(base=base, dir=(Vector((0, 0, 1)) + lean).normalized(), length=length,
                           bend=rng.uniform(0.05, 0.25), nodes=nodes, phase=rng.random(), spin=spin))
    dead = []
    for i in range(4):
        c = drifts[i % len(drifts)]
        base = c + Vector((rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12), 0))
        base.z = 0.0
        lean = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)) * rng.uniform(0.05, 0.45)
        # Some stand tall, some are snapped and hang over.
        snapped = rng.random() < 0.4
        dead.append(dict(base=base, dir=(Vector((0, 0, 1)) + lean).normalized(),
                         length=rng.uniform(0.32, 0.5) if snapped else rng.uniform(0.48, 0.72),
                         bend=rng.uniform(0.9, 1.6) if snapped else rng.uniform(0.05, 0.3), phase=rng.random(),
                         leaves=[(rng.uniform(0.3, 0.8), rng.uniform(0, math.tau)) for _ in range(rng.randint(0, 3))],
                         seed=rng.randrange(1 << 20)))
    return dict(shoots=shoots, dead=dead)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"stems": 0, "leaves": 0, "dead": 0}
    for sh in desc["shoots"]:
        n = (6, 4, 2)[lod]
        pts = _stalk(sh["base"], sh["dir"], sh["length"], sh["bend"], n)
        t0 = b.triangles
        b.tube(pts, [0.0024 * (1 - 0.5 * i / n) for i in range(n + 1)], (4, 4, 3)[lod], atlas.uv("stem"),
               v_length=0.5, phase=sh["phase"], flutter=0.12)
        stats["stems"] += b.triangles - t0
        acc = S.arclength(pts)
        t0 = b.triangles
        for k, nd in enumerate(sh["nodes"]):
            if lod == 1 and nd["keep"] > 0.7 or lod == 2 and nd["keep"] > 0.4:
                continue
            node, tan, _ = S.at(pts, acc, acc[-1] * nd["t"])
            side = Matrix.Rotation(sh["spin"] + k * 1.5708, 3, tan) @ tan.orthogonal().normalized()
            for s in (1, -1) if lod < 2 else (1,):
                hd = (side * s).normalized()
                d = (hd * math.cos(nd["elev"]) + tan * math.sin(nd["elev"])).normalized()
                up = (tan - hd * 0.4).normalized()
                ln = nd["size"] * (1.0, 1.08, 1.25)[lod]
                b.card(node, d, up, ln, ln * S.tile_aspect(atlas, nd["key"]), atlas.uv(nd["key"]),
                       rows=(3, 2, 1)[lod], cols=(2, 1, 1)[lod], fold=0.2 if lod == 0 else 0.0, droop=nd["droop"],
                       phase=sh["phase"], flutter=1.0, flutter_base=0.1)
        # The young tip rosette: two small bright pairs, folded up round the growing point.
        top, tan, _ = S.at(pts, acc, acc[-1])
        for k in range(2 if lod < 2 else 1):
            side = Matrix.Rotation(sh["spin"] + (k + 0.5) * 1.5708, 3, tan) @ tan.orthogonal().normalized()
            for s in (1, -1):
                hd = (side * s).normalized()
                d = (hd * 0.45 + tan * 0.9).normalized()
                ln = (0.040 - 0.010 * k) * (1.0, 1.1, 1.3)[lod]
                b.card(top - tan * 0.01 * k, d, (tan - hd * 0.2).normalized(), ln,
                       ln * S.tile_aspect(atlas, "leaf_young"), atlas.uv("leaf_young"), rows=2 if lod == 0 else 1,
                       cols=1, fold=0.35 if lod == 0 else 0.0, droop=-0.1, phase=sh["phase"], flutter=0.8,
                       flutter_base=0.1)
        stats["leaves"] += b.triangles - t0
    t0 = b.triangles
    for dd in desc["dead"]:
        if lod == 2:
            continue
        rng = random.Random(dd["seed"])
        n = (7, 4)[lod]
        pts = _stalk(dd["base"], dd["dir"], dd["length"], dd["bend"], n, wobble=0.09, rng=rng)
        b.tube(pts, [0.0045 * (1 - 0.45 * i / n) for i in range(n + 1)], (4, 3)[lod], atlas.uv("dead"),
               v_length=0.6, phase=dd["phase"], flutter=0.05)
        if lod == 0:
            acc = S.arclength(pts)
            for t, az in dd["leaves"]:
                node, tan, _ = S.at(pts, acc, acc[-1] * t)
                hd = Vector((math.cos(az), math.sin(az), 0))
                d = (hd * 0.4 - tan * 0.6).normalized()  # Shrivelled and hanging.
                b.card(node, d, (hd - tan * 0.2).normalized(), 0.04, 0.04 * S.tile_aspect(atlas, "leaf_dead"),
                       atlas.uv("leaf_dead"), rows=2, cols=1, fold=0.5, curl=0.4, droop=0.3, phase=dd["phase"],
                       flutter=0.4, flutter_base=0.1)
    stats["dead"] += b.triangles - t0
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
