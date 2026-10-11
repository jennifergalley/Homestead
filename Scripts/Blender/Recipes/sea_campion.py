"""Sea campion (Silene uniflora) in flower: a low sprawling mat on cliff ledges and the beach-top shingle.

The real plant (what the geometry and textures follow):
- A cushion- or mat-forming perennial of sea cliffs, cliff-top turf and stabilised shingle round the
  coasts of Britain, flowering May to August; the mats are 20-40 cm across and only 5-12 cm high,
  the stems trailing over the ground with their tips turned up.
- Leaves in opposite pairs, 1-2.5 cm, lance-shaped to narrowly oval, thick and fleshy, waxy and
  glaucous: a blue-grey-green bloom; the older ones go dull purple-red at the tips in salt wind.
- Flowers usually single at the stem tips, 2-2.5 cm across: five broad white petals, each deeply
  cleft into two lobes, overlapping, with small white scales round the throat; behind them the
  calyx is a swollen bladder 12-18 mm long, pale green to cream, netted with pink-purple veins,
  that stays on as the flower fades.

Game notes: walk-through (no collision). About 11 cm tall and 32 cm across. One 2K atlas and
material (alpha-masked, two-sided). Wind vertex colours per homestead_foliage.py. Pivot bottom
centre; the stems' bases start 6 mm below z = 0 and the trailing stems lie on it.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_flowers as W
import homestead_foliage as F
import homestead_shrub as S

NAME = "SeaCampion"
MESH = "SM_SeaCampionClump"
DESCRIPTION = ("A mat of sea campion in flower, 11 cm tall and 32 cm across: trailing stems of waxy blue-green "
               "paired leaves and white bilobed flowers over inflated pink-veined bladder calyces. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 5000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.04, -0.03, 0.06), "ground": "origin", "detail_distance": 0.4,
          "detail_fstop": 9.0, "views": ["hero", "detail", "eye"], "eye_distance": 1.4}
REPORT = W.report(NAME)
REPORT["placement"] = "Ground line at z = 0 (pivot); stem bases start 6 mm below. Random yaw, 0.8-1.2 scale."
NOTES = {"sink_depth_m": 0.006}

SEED = 18517
HEIGHT = 0.11
SINK = 0.006


def _paint_leaf(atlas, key, nrng, pal, length_m, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.86
    # Lance-shaped, entire, thick: the midrib barely shows through the waxy bloom.
    shape = F.ovate(width=w, widest=0.42, tip_sharp=1.1, base_round=0.7, base=0.02, tip=0.985)
    veins = [([(0.0, 0.02), (0.0, 0.9)], 1.0)]
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.05, vein_depth=0.00008,
                          puff=0.00012, tertiary=0.0, gloss=0.0, trans=0.3, **extra)
    # Waxy bloom: a pale blue-grey film, thinner along the margin and where rubbed.
    bloom = F.smoothstep(0.25, 0.75, F.noise(X.shape, nrng, freq=10.0, beta=2.0))
    edge = 1.0 - F.smoothstep(0.0, 0.04, shape(X, Y)[0])
    layer.color = F.lerp(layer.color, np.array((0.115, 0.145, 0.135)), np.clip(0.45 + 0.25 * bloom - 0.3 * edge, 0, 1))
    layer.rough = np.clip(layer.rough + 0.12 * bloom, 0.3, 0.9)
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def _paint_calyx(atlas, key, nrng):
    """The inflated bladder unrolled: u round it, v from stalk (0) to mouth (1); pale green-cream
    with ~20 pink-purple longitudinal veins joined by a fine net."""
    X, Y, px = atlas.grid(key)
    a = X.max()
    u = (X + a) / (2 * a)
    v = np.clip(Y, 0, 1)
    n = F.noise(X.shape, nrng, freq=20.0, beta=1.8)
    long_ = np.exp(-((np.mod(u * 20.0 + 0.3 * (n - 0.5), 1.0) - 0.5) / 0.11) ** 2)
    net = F.smoothstep(0.8, 0.95, F.ridged(F.noise(X.shape, nrng, freq=60.0, beta=2.2)))
    vein = np.clip(long_ + 0.55 * net, 0, 1)
    col = F.lerp(np.array((0.26, 0.31, 0.19)), np.array((0.44, 0.42, 0.32)), F.smoothstep(0.1, 0.8, v))
    col = F.lerp(col, np.array((0.24, 0.05, 0.12)), np.clip(vein * (0.8 + 0.3 * v), 0, 1))
    teeth = F.smoothstep(0.9, 1.0, v)
    col = F.lerp(col, np.array((0.36, 0.20, 0.20)), teeth * 0.6)
    layer = F.Layer(X.shape)
    layer.alpha = np.ones(X.shape)
    layer.color = col
    layer.height = 0.00008 * vein
    layer.rough = 0.5 + 0.1 * n
    layer.trans = 0.55 * np.ones(X.shape)
    atlas.put(key, layer, meters_per_px=0.015 / X.shape[0], opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 40)
    for key in ("leaf", "leaf2", "leaf_old"):
        atlas.tile(key, 220, 520)
    for key in ("flower", "flower2", "flower_old"):
        atlas.tile(key, 400, 400)
    atlas.tile("calyx", 300, 300)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    glauc = dict(base=(0.060, 0.085, 0.066), tip=(0.066, 0.090, 0.070), vein=(0.085, 0.105, 0.085),
                 margin=(0.075, 0.095, 0.080), brown=(0.12, 0.05, 0.06), yellow=(0.16, 0.10, 0.10))
    _paint_leaf(atlas, "leaf", nrng, glauc, 0.022)
    _paint_leaf(atlas, "leaf2", nrng, {k: tuple(c * f for c, f in zip(v, (0.9, 0.95, 0.95))) for k, v in glauc.items()},
                0.022)
    _paint_leaf(atlas, "leaf_old", nrng, dict(glauc, brown=(0.13, 0.045, 0.06)), 0.022, edge_burn=0.6, yellow=0.25)
    W.paint_flower_face(atlas, "flower", nrng, petals=5, inner=(0.70, 0.70, 0.66), outer=(0.74, 0.74, 0.71),
                        eye=(0.66, 0.66, 0.62), centre=(0.55, 0.56, 0.45), size_m=0.024, notch=0.42,
                        notch_width=0.055, rim=0.96, waist=0.22, veins=0.12, eye_radius=0.2, centre_radius=0.06,
                        dots=10)
    W.paint_flower_face(atlas, "flower2", nrng, petals=5, inner=(0.68, 0.67, 0.64), outer=(0.72, 0.71, 0.68),
                        eye=(0.64, 0.62, 0.60), centre=(0.50, 0.40, 0.42), size_m=0.022, notch=0.38,
                        notch_width=0.06, rim=0.94, waist=0.26, veins=0.15, eye_radius=0.2, centre_radius=0.06,
                        dots=10)
    W.paint_flower_face(atlas, "flower_old", nrng, petals=5, inner=(0.52, 0.48, 0.42), outer=(0.50, 0.44, 0.38),
                        eye=(0.48, 0.42, 0.38), centre=(0.35, 0.30, 0.25), size_m=0.02, notch=0.4,
                        notch_width=0.07, rim=0.82, waist=0.38, veins=0.3, eye_radius=0.2, centre_radius=0.07)
    _paint_calyx(atlas, "calyx", nrng)
    W.paint_stem(atlas, "stem", nrng, (0.10, 0.07, 0.07), (0.085, 0.11, 0.09), hair=0.0, length_m=0.2, trans=0.25)
    atlas.save()
    return atlas


def describe(rng):
    stems = []
    count = 34
    for i in range(count):
        az = i * 2.39996 + rng.uniform(-0.3, 0.3)
        length = rng.uniform(0.08, 0.17) * (0.6 if i % 5 == 0 else 1.0)
        stems.append(dict(az=az, length=length, rise=rng.uniform(0.025, 0.07), wander=rng.uniform(-1.1, 1.1),
                          lift0=rng.uniform(0.0, 0.03), r0=rng.uniform(0.0, 0.03),
                          pairs=rng.randint(6, 9), flower=rng.random() < 0.75,
                          key=rng.choices(("flower", "flower2", "flower_old"), (45, 40, 15))[0],
                          size=rng.uniform(0.020, 0.026), spin=rng.uniform(0, math.tau), tilt=rng.uniform(0.3, 0.9),
                          phase=rng.random(), keep=rng.random(), leaf_spin=rng.uniform(0, math.pi)))
    return dict(stems=stems)


def stem_path(st, n):
    h = Vector((math.cos(st["az"]), math.sin(st["az"]), 0.0))
    side = Vector((-h.y, h.x, 0.0))
    pts = []
    for i in range(n + 1):
        t = i / n
        # Trailing and curving over the ground, then the tip turning up toward the light.
        bend = st["wander"] * st["length"] * t * t * 0.6
        p = h * (st["r0"] + st["length"] * t) + side * bend
        p.z = (-SINK + st["lift0"] * math.sin(math.pi * min(t * 1.6, 1.0)) * (1 - t)
               + st["rise"] * max(0.0, t - 0.5) ** 1.5 / 0.5 ** 1.5 + 0.004)
        pts.append(p)
    return pts

def emit(desc, atlas, lod):
    b = F.Batch(height=HEIGHT)
    stats = {"stems": 0, "leaves": 0, "flowers": 0}
    for st in desc["stems"]:
        if lod == 1 and st["keep"] > 0.75 or lod == 2 and st["keep"] > 0.45:
            continue
        n = (6, 4, 2)[lod]
        pts = stem_path(st, n)
        t0 = b.triangles
        b.tube(pts, [0.0015 * (1 - 0.35 * i / n) for i in range(n + 1)], 3, atlas.uv("stem"),
               v_length=0.2, phase=st["phase"], flutter=0.1)
        stats["stems"] += b.triangles - t0
        acc = S.arclength(pts)
        t0 = b.triangles
        pairs = st["pairs"] if lod == 0 else max(2, st["pairs"] // (2 if lod == 1 else 3))
        for p in range(pairs):
            frac = 0.1 + 0.88 * (p / max(pairs - 1, 1)) ** 0.8
            node, tan, _ = S.at(pts, acc, acc[-1] * frac)
            tan = Vector(tan).normalized()
            for s in (-1, 1):
                ang = st["leaf_spin"] * 0.3 + (p % 2) * 0.5
                side = tan.cross(Vector((0, 0, 1))).normalized() * s
                d = (side * math.cos(ang) + tan * 0.55 + Vector((0, 0, 0.35 + 0.2 * (p % 2)))).normalized()
                length = (0.019 + 0.010 * math.sin(math.pi * frac)) * (1.0, 1.2, 1.4)[lod]
                key = "leaf_old" if frac < 0.3 and (p + st["az"] * 10) % 3 < 1 else ("leaf" if (p + s) % 2 else "leaf2")
                b.card(node, d, Vector((0, 0, 1)), length, length * S.tile_aspect(atlas, key), atlas.uv(key),
                       rows=(2, 1, 1)[lod], cols=1, fold=0.35, droop=0.2, phase=st["phase"], flutter=0.4)
        stats["leaves"] += b.triangles - t0
        if not st["flower"] or lod == 2 and st["keep"] > 0.25:
            continue
        t0 = b.triangles
        top = pts[-1]
        tan = (pts[-1] - pts[-2]).normalized()
        face = (tan * (1 - st["tilt"]) + Vector((0, 0, 1)) * (0.5 + st["tilt"])).normalized()
        # The inflated bladder calyx, then the flower face at its mouth.
        clen = 0.014
        centre = top + face * clen * 0.5
        b.sphere(centre, clen * 0.36, atlas.uv("calyx"), segs=(7, 5, 4)[lod], rings=(4, 3, 3)[lod], stretch=1.35,
                 axis=face, phase=st["phase"])
        mouth = top + face * clen * 0.95
        b.flat(mouth, face, st["size"], atlas.uv(st["key"]), spin=st["spin"], cup=0.22 if lod == 0 else 0.10,
               phase=st["phase"], flutter=0.4, segs=(3, 1, 1)[lod])
        stats["flowers"] += b.triangles - t0
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
