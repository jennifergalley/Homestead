"""Garden strawberry crop plot (Fragaria x ananassa / Keens' Seedling type), five growth stages.

Real plant research / art targets (written before modelling):
- Keens' Seedling (raised 1821) and other early garden strawberries are larger than woodland
  strawberry but still low, clumping crowns. Leaves are basal, trifoliate, toothed, on 10-15 cm
  hairy petioles; mature leaflets are commonly 5-8 cm long with impressed veins and pale undersides.
- Flowers are white, five-petalled, on arching scapes. Fruit hang on slender stalks around and just
  outside the crown: ripe berries are red, conical/ovoid, 2.5-3.5 cm with yellow achenes; some berries
  remain green-white during the pick-and-regrow stage.
- Period touch: a few straw strands under the crowns keep berries off the damp ridge. Plot matches the
  tilled bed: six crowns (2 per ridge) at y=-0.3, 0, +0.3, base z=0.045 inside the 1 m square.

One numpy-painted 2K atlas/material shared by all five stages. Alpha-masked two-sided foliage with
Wind vertex colours from homestead_foliage.py. No collision.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "CropStrawberry"
DESCRIPTION = ("One 1 m tilled-bed plot of six garden strawberry crowns across five crop stages, with "
               "large trifoliate leaves, straw mulch, white flowers and readable red berries.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
SEED = 1821_617

BEAUTY = {"pose": (0, 0, 0), "views": ["hero", "detail", "eye"], "eye_distance": 3.8,
          "meshes": {
              "SM_CropStrawberry_Growing": {"focus": (0.0, 0.0, 0.12), "eye_distance": 3.2},
              "SM_CropStrawberry_Ripe": {"focus": (0.0, 0.0, 0.11), "eye_distance": 3.6},
          }}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / crop height",
             "G": "per-crown phase", "B": "leaf flutter 0 at petiole -> 1 at leaflet tips", "A": "1"},
    "crop_layout": "Six crowns at x=-0.22,+0.22 on tilled-bed ridges y=-0.3,0,+0.3; base z=0.045 m.",
    "material_notes": "One M_CropStrawberry atlas: alpha in basecolor; roughness R, translucency G, AO B.",
}
NOTES = {
    "SM_CropStrawberry_Ripe": "Pick-and-regrow stage: red 2.5-3.5 cm berries hang over straw mulch; some berries remain pale.",
    "SM_CropStrawberry_Mature": "White flower scapes and green-white unripe berries, before red fruit colour.",
    "SM_CropStrawberry_Harvest": "Hand-held produce: three ripe berries on short stalks, grip at stalk top/origin, berries hang down -Z.",
}

RIDGES = (-0.30, 0.0, 0.30)
XS = (-0.22, 0.22)
BASE_Z = 0.045
STAGES = {
    "Sprout": dict(height=0.100, leaves=4, leaf=0.036, petiole=(0.040, 0.060), radius=0.080, flowers=0, berries=0, straw=0),
    "Young": dict(height=0.145, leaves=7, leaf=0.048, petiole=(0.070, 0.100), radius=0.130, flowers=0, berries=0, straw=3),
    "Growing": dict(height=0.260, leaves=8, leaf=0.068, petiole=(0.130, 0.180), radius=0.205, flowers=0, berries=0, straw=5),
    "Mature": dict(height=0.265, leaves=8, leaf=0.070, petiole=(0.130, 0.185), radius=0.215, flowers=2, berries=2, straw=6),
    "Ripe": dict(height=0.265, leaves=5, leaf=0.070, petiole=(0.130, 0.185), radius=0.220, flowers=0,
                 berries=4, unripe=0.5, straw=2),
}


def _crowns():
    out = []
    for j, y in enumerate(RIDGES):
        for i, x in enumerate(XS):
            rng = random.Random(SEED + j * 31 + i * 97)
            out.append(dict(pos=Vector((x + rng.uniform(-0.012, 0.012), y + rng.uniform(-0.012, 0.012), BASE_Z)),
                            heading=rng.uniform(0, math.tau), phase=rng.random(), seed=rng.randrange(1 << 20)))
    return out


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.035), brown=(0.065, 0.035, 0.015), dry=(0.13, 0.075, 0.030))
    return {
        "leaf": dict(base, base=(0.055, 0.100, 0.028), tip=(0.070, 0.122, 0.034),
                     vein=(0.110, 0.150, 0.055), margin=(0.044, 0.078, 0.024)),
        "leaf_dark": dict(base, base=(0.045, 0.082, 0.025), tip=(0.060, 0.104, 0.031),
                          vein=(0.095, 0.130, 0.048), margin=(0.038, 0.067, 0.022)),
        "leaf_light": dict(base, base=(0.073, 0.126, 0.035), tip=(0.090, 0.145, 0.042),
                           vein=(0.125, 0.165, 0.058), margin=(0.058, 0.104, 0.030)),
        "leaf_old": dict(base, base=(0.085, 0.068, 0.026), tip=(0.120, 0.055, 0.022),
                         vein=(0.120, 0.088, 0.040), margin=(0.070, 0.040, 0.018)),
    }


def _leaf_shape(rng):
    return F.ovate(width=0.34, widest=0.57, tip_sharp=0.62, base_round=1.05, base=0.02, tip=0.97,
                   teeth=16, tooth_depth=0.115, double=0.20, phase=rng.uniform(0, 1))


def _leaf_veins(nrng):
    shape = F.ovate(width=0.34, widest=0.57, tip_sharp=0.62, base_round=1.05, base=0.02, tip=0.97)
    veins = [([(0.0, 0.02), (0.0, 0.965)], 1.0)]
    for k in range(11):
        y0 = 0.075 + 0.073 * k + float(nrng.uniform(-0.006, 0.006))
        y1 = min(y0 + 0.13, 0.94)
        _, _, hw = shape(np.zeros(1), np.array([y1]))
        for side in (-1, 1):
            veins.append(([(0.0, y0), (side * hw[0] * 0.52, y0 + 0.065), (side * hw[0] * 0.96, y1)],
                          0.58 - 0.018 * k))
    return veins


def _paint_leaf(X, Y, px, nrng, rng, key, pals):
    old = key == "leaf_old"
    return F.paint_blade(X, Y, nrng, _leaf_shape(rng), _leaf_veins(nrng), pals[key], px,
                         vein_width=0.009, vein_depth=0.00017, puff=0.00017, tertiary=0.33,
                         damage=0.12 if old else 0.025, holes=0.07 if old else 0.0,
                         yellow=0.45 if old else 0.03, edge_burn=0.28 if old else 0.04,
                         gloss=0.12, hair=0.16, trans=0.62 if not old else 0.32)


def _paint_trifoliate(atlas, nrng, rng, pals):
    X, Y, px = atlas.grid("trifoliate")
    layer = F.Layer(X.shape)
    layer.color[...] = (0.036, 0.062, 0.024)
    for th, ln, key, ox in ((-1.08, 0.46, "leaf_dark", -0.05), (1.08, 0.46, "leaf", 0.05), (0.0, 0.55, "leaf", 0.0)):
        Xl, Yl = S.rotated(X, Y, (ox, 0.10), th, ln)
        layer.over(_paint_leaf(Xl, Yl, px / ln, nrng, rng, key, pals))
    atlas.put("trifoliate", layer, meters_per_px=0.16 / X.shape[0])


def _paint_flower(atlas, nrng, rng):
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    petal = F.ovate(width=0.52, widest=0.62, tip_sharp=0.34, base_round=1.4, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(72 * k + rng.uniform(-4, 4))
        Xl, Yl = S.rotated(X, Y, (0.0, 0.50), th, 0.48)
        pal = dict(base=(0.60, 0.60, 0.53), tip=(0.74, 0.73, 0.67), vein=(0.64, 0.64, 0.58),
                   margin=(0.76, 0.75, 0.70))
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, [([(0, 0.05), (0, 0.95)], 1.0)], pal, px / 0.48,
                                 vein_width=0.018, puff=0.00016, tertiary=0.0, trans=0.85, gloss=-0.1))
    r = np.hypot(X, Y - 0.50)
    n = F.noise(X.shape, nrng, freq=55.0, beta=1.4)
    disc = 1 - F.smoothstep(0.105, 0.140, r)
    ring = F.smoothstep(0.10, 0.12, r) * (1 - F.smoothstep(0.16, 0.19, r))
    layer.color = F.lerp(layer.color, F.lerp((0.32, 0.33, 0.055), (0.48, 0.43, 0.075), n), disc)
    layer.color = F.lerp(layer.color, (0.56, 0.45, 0.075), ring * (n > 0.45))
    layer.height += 0.00035 * disc + 0.00010 * ring * n
    atlas.put("flower", layer, meters_per_px=0.028 / X.shape[0])


def _paint_calyx(atlas, nrng):
    X, Y, px = atlas.grid("calyx")
    layer = F.Layer(X.shape)
    sepal = F.ovate(width=0.24, widest=0.36, tip_sharp=1.25, base=0.0, tip=1.0)
    pal = dict(base=(0.052, 0.095, 0.030), tip=(0.065, 0.112, 0.036), vein=(0.095, 0.130, 0.050),
               margin=(0.042, 0.073, 0.024))
    for k in range(10):
        th = math.radians(36 * k)
        ln = 0.48 if k % 2 == 0 else 0.34
        Xl, Yl = S.rotated(X, Y, (0.0, 0.50), th, ln)
        layer.over(F.paint_blade(Xl, Yl, nrng, sepal, [([(0, 0), (0, 1)], 1.0)], pal, px / ln,
                                 vein_width=0.028, tertiary=0.0, hair=0.2, trans=0.55))
    atlas.put("calyx", layer, meters_per_px=0.030 / X.shape[0])


def _paint_berries(atlas, nrng):
    for key, flesh, hi, seed_col, trans in (
        ("berry", (0.42, 0.026, 0.024), (0.58, 0.050, 0.036), (0.66, 0.50, 0.095), 0.22),
        ("unripe", (0.35, 0.43, 0.23), (0.50, 0.50, 0.32), (0.44, 0.36, 0.080), 0.32),
        ("pale", (0.56, 0.43, 0.34), (0.66, 0.08, 0.055), (0.55, 0.42, 0.080), 0.25),
    ):
        X, Y, px = atlas.grid(key)
        U, V = X / (2 * max(X.max(), 1e-6)) + 0.5, Y
        layer = F.Layer(X.shape)
        n = F.noise(X.shape, nrng, freq=18.0, beta=2.0)
        col = F.lerp(flesh, hi, n)
        shoulder = 1 - F.smoothstep(0.08, 0.35, V)
        col = F.lerp(col, np.asarray(col) * 0.62, shoulder * 0.55)
        cells = np.sin(U * math.tau * 9.5) * np.sin(V * math.pi * 12.5 + U * 2.5)
        seeds = F.smoothstep(0.80, 0.94, cells)
        pits = F.smoothstep(0.50, 0.92, cells)
        layer.color = F.lerp(col, seed_col, seeds)
        layer.alpha[...] = 1
        layer.height = -0.00028 * pits + 0.00018 * seeds
        layer.rough = (0.34 if key == "berry" else 0.42) + 0.20 * seeds
        layer.trans[...] = trans
        atlas.put(key, layer, meters_per_px=0.030 / X.shape[0], opaque=True)


def _paint_columns(atlas, nrng):
    U, V = atlas.column_grid("petiole")
    n = F.noise(U.shape, nrng, freq=80.0, beta=1.5, aniso=(1, 7))
    hairs = F.smoothstep(0.78, 0.93, F.noise(U.shape, nrng, freq=165.0, beta=1.0, aniso=(1, 3)))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.070, 0.095, 0.035), (0.13, 0.055, 0.035), F.smoothstep(0.45, 0.85, n) * 0.50)
    layer.color = F.lerp(layer.color, (0.25, 0.27, 0.18), hairs * 0.45)
    layer.height = 0.00007 * n + 0.00005 * hairs
    layer.rough = 0.58 + 0.14 * hairs
    layer.trans[...] = 0.36
    layer.alpha[...] = 1
    atlas.put("petiole", layer, meters_per_px=0.35 / U.shape[0], wrap=True, opaque=True)
    U, V = atlas.column_grid("straw")
    n = F.noise(U.shape, nrng, freq=65.0, beta=1.7, aniso=(1, 10))
    stripe = F.ridged(np.sin((U * 8.0 + 0.15 * np.sin(V * 20)) * math.tau) * 0.5 + 0.5)
    straw = F.Layer(U.shape)
    straw.color = F.lerp((0.29, 0.22, 0.090), (0.55, 0.43, 0.18), 0.45 * n + 0.35 * stripe)
    straw.color = F.lerp(straw.color, (0.12, 0.08, 0.035), F.smoothstep(0.88, 0.98, n) * 0.28)
    straw.height = 0.00008 * n
    straw.rough = 0.78 + 0.10 * stripe
    straw.trans[...] = 0.05
    straw.alpha[...] = 1
    atlas.put("straw", straw, meters_per_px=0.40 / U.shape[0], wrap=True, opaque=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=12)
    atlas.column("petiole", 58)
    atlas.column("straw", 46)
    for key in ("leaf", "leaf_dark", "leaf_light", "leaf_old"):
        atlas.tile(key, 390, 560)
    atlas.tile("trifoliate", 700, 700)
    atlas.tile("flower", 250, 250)
    atlas.tile("calyx", 180, 180)
    atlas.tile("berry", 210, 220)
    atlas.tile("unripe", 210, 220)
    atlas.tile("pale", 210, 220)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in ("leaf", "leaf_dark", "leaf_light", "leaf_old"):
        X, Y, px = atlas.grid(key)
        atlas.put(key, _paint_leaf(X, Y, px, nrng, rng, key, pals), meters_per_px=0.080 / X.shape[0])
    _paint_trifoliate(atlas, nrng, rng, pals)
    _paint_flower(atlas, nrng, rng)
    _paint_calyx(atlas, nrng)
    _paint_berries(atlas, nrng)
    _paint_columns(atlas, nrng)
    atlas.save()
    return atlas


def _arc(p0, heading, elev, length, droop, n=4):
    pts = [Vector(p0)]
    side = heading.cross(Vector((0, 0, 1))).normalized()
    d = (heading * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    for _ in range(n):
        d = (Matrix.Rotation(-droop / n, 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def _add_leaf(b, atlas, crown, az, petiole_len, leaflet_len, phase, lod, rng, old=False):
    heading = Vector((math.cos(az), math.sin(az), 0))
    elev = math.radians(rng.uniform(52, 76))
    pts = _arc(crown, heading, elev, petiole_len, elev * rng.uniform(0.32, 0.52), n=3 if lod == 0 else 2)
    stem_pts = pts if lod == 0 else [pts[0], pts[-1]]
    b.tube(stem_pts, [0.00135, 0.00105, 0.00085, 0.00070][:len(stem_pts)], 3, atlas.uv("petiole"),
           v_length=0.32, phase=phase, flutter=0.15)
    top = pts[-1]
    up = (Vector((0, 0, 1)) + heading * rng.uniform(0.10, 0.30)).normalized()
    if lod:
        b.card(top - heading * leaflet_len * 0.10, heading, up,
               leaflet_len * (0.88 if lod == 1 else 1.10), leaflet_len * (0.48 if lod == 1 else 0.65),
               atlas.uv("trifoliate"), rows=1, cols=2 if lod == 1 else 1, fold=0.0, droop=0.10, phase=phase,
               flutter=0.95, flutter_base=0.25)
        return
    keys = [rng.choices(["leaf", "leaf_dark", "leaf_light", "leaf_old"], [46, 32, 16, 6])[0] for _ in range(3)]
    if old:
        keys = ["leaf_old", "leaf_dark", "leaf_old"]
    for k, (ang, scale) in enumerate(((0.0, 1.0), (-1.12, 0.90), (1.12, 0.90))):
        d = Matrix.Rotation(ang + rng.uniform(-0.10, 0.10), 3, up) @ heading
        d = (d - up * d.dot(up) * 0.60 + Vector((0, 0, 0.10))).normalized()
        ln = leaflet_len * scale * rng.uniform(0.94, 1.07)
        rect = atlas.uv(keys[k])
        if ang > 0:
            rect = (rect[1], rect[0], rect[2], rect[3])
        b.card(top + d * leaflet_len * 0.025, d, up, ln, ln * S.tile_aspect(atlas, keys[k]), rect,
               rows=1, cols=2 if lod == 0 else 1,
               fold=0.25 if lod == 0 else 0.0, curl=rng.uniform(-0.06, 0.08),
               droop=rng.uniform(0.06, 0.20) + (0.18 if old else 0), twist=rng.uniform(-0.10, 0.10),
               phase=phase, flutter=1.0, flutter_base=0.25)


def _add_straw(b, atlas, crown, radius, count, phase, lod, rng):
    if count <= 0 or lod == 2:
        return
    for _ in range(count):
        az = rng.uniform(0, math.tau)
        length = rng.uniform(radius * 0.75, radius * 1.25)
        mid = crown + Vector((math.cos(az), math.sin(az), 0)) * rng.uniform(0.02, radius * 0.45)
        d = Vector((math.cos(az + rng.uniform(-0.45, 0.45)), math.sin(az + rng.uniform(-0.45, 0.45)), 0))
        p0 = mid - d * length * 0.5
        p1 = mid + d * length * 0.5
        p0.x, p0.y, p1.x, p1.y = [max(-0.48, min(0.48, v)) for v in (p0.x, p0.y, p1.x, p1.y)]
        z = BASE_Z + 0.003 + rng.uniform(-0.001, 0.002)
        pts = [Vector((p0.x, p0.y, z)), Vector(((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5, z + rng.uniform(0, 0.006))),
               Vector((p1.x, p1.y, z))]
        b.tube(pts, [0.00095, 0.00080, 0.00055], 3, atlas.uv("straw"), v_length=0.32,
               phase=phase, flutter=0.02, flatten=0.50, roll=rng.uniform(0, math.tau))


def _add_flower_or_berry(b, atlas, base, az, phase, lod, rng, berry_kind=None, edge=False):
    heading = Vector((math.cos(az), math.sin(az), 0))
    if berry_kind and edge:
        scape_len = rng.uniform(0.135, 0.195) * (0.82 if lod else 1.0)
        elev = math.radians(36)
        droop = 1.18
    else:
        scape_len = rng.uniform(0.085, 0.135) * (0.78 if lod else 1.0)
        elev = math.radians(62)
        droop = 0.82
    scape = _arc(base, heading, elev, scape_len, droop, n=4 if lod == 0 else 2)
    stem_pts = scape if lod == 0 else [scape[0], scape[-1]]
    b.tube(stem_pts, [0.00085] * len(stem_pts), 3, atlas.uv("petiole"), v_length=0.32, phase=phase, flutter=0.35)
    top = scape[-1]
    normal = (heading * 0.55 + Vector((0, 0, 0.65))).normalized()
    if berry_kind is None:
        b.flat(top + normal * 0.002, normal, rng.uniform(0.025, 0.032), atlas.uv("flower"),
               spin=rng.uniform(0, math.tau), cup=0.10 if lod == 0 else 0.0, phase=phase,
               flutter=0.50, segs=2 if lod == 0 else 1)
    else:
        if lod == 2 and berry_kind != "berry":
            return
        down = (heading * 0.35 + Vector((0, 0, -1))).normalized()
        rad = rng.uniform(0.0125, 0.0155) if berry_kind == "berry" else rng.uniform(0.008, 0.012)
        segs = 7 if (lod == 0 and berry_kind == "berry") else (5 if lod == 0 else 5)
        rings = 4 if (lod == 0 and berry_kind == "berry") else 3
        b.sphere(top + down * rad * 0.95, rad, atlas.uv(berry_kind, inset=False), segs=segs,
                 rings=rings, stretch=rng.uniform(1.15, 1.35), axis=down, phase=phase)
        if lod == 0:
            b.flat(top + down * rad * 0.12, -down, rad * 2.5, atlas.uv("calyx"), spin=rng.uniform(0, math.tau),
                   cup=-0.18, phase=phase, flutter=0.30, segs=1)


def _emit_stage(stage, atlas, lod):
    cfg = STAGES[stage]
    b = F.Batch(height=cfg["height"])
    for crown_index, crown in enumerate(_crowns()):
        rng = random.Random(crown["seed"] + lod * 2003 + sum(ord(c) for c in stage))
        base = crown["pos"]
        phase = crown["phase"]
        _add_straw(b, atlas, base, cfg["radius"], cfg["straw"], phase, lod, rng)
        if lod < 2:
            for k in range(3):
                az = crown["heading"] + k * 2.094 + rng.uniform(-0.25, 0.25)
                pts = [base, base + Vector((math.cos(az), math.sin(az), 0.35)) * 0.010]
                b.tube(pts, [0.0020, 0.0011], 4, atlas.uv("petiole"), v_length=0.32, phase=phase, flutter=0.05)
        for i in range(cfg["leaves"]):
            az = crown["heading"] + i * 2.39996 + rng.uniform(-0.22, 0.22)
            old = stage == "Ripe" and i in (cfg["leaves"] - 1, cfg["leaves"] - 2)
            _add_leaf(b, atlas, base, az, rng.uniform(*cfg["petiole"]), cfg["leaf"] * rng.uniform(0.88, 1.10),
                      phase, lod, rng, old=old)
        for s in range(cfg["flowers"]):
            if lod == 2 and s > 0:
                break
            _add_flower_or_berry(b, atlas, base + Vector((0, 0, 0.008)),
                                 crown["heading"] + 0.9 + s * 2.6 + rng.uniform(-0.25, 0.25),
                                 phase, lod, rng, berry_kind=None)
        for s in range(cfg["berries"]):
            if lod == 2 and s >= max(1, cfg["berries"] // 2):
                break
            kind = rng.choice(["unripe", "unripe", "pale"]) if stage == "Mature" else "berry"
            _add_flower_or_berry(b, atlas, base + Vector((0, 0, 0.004)),
                                 crown["heading"] - 0.9 + s * 1.38 + rng.uniform(-0.28, 0.28),
                                 phase, lod, rng, berry_kind=kind, edge=(stage == "Ripe"))
        if stage == "Ripe" and crown_index % 2 == 0 and lod < 2:
            _add_flower_or_berry(b, atlas, base + Vector((0, 0, 0.004)),
                                 crown["heading"] + 2.8 + rng.uniform(-0.25, 0.25),
                                 phase, lod, rng, berry_kind=rng.choice(["unripe", "pale"]), edge=True)
    print("HOMESTEAD_TRIS", stage, lod, b.triangles)
    return b


def _emit_harvest(atlas):
    b = F.Batch(height=0.14)
    rng = random.Random(SEED + 9017)
    origin = Vector((0, 0, 0))
    for k, az in enumerate((-0.55, 0.18, 0.74)):
        heading = Vector((math.sin(az) * 0.55, math.cos(az) * 0.55, -1.0)).normalized()
        pts = [origin,
               origin + heading * rng.uniform(0.030, 0.040) + Vector((0, 0, -0.006)),
               origin + heading * rng.uniform(0.055, 0.075) + Vector((0, 0, -0.020))]
        b.tube(pts, [0.0010, 0.0008, 0.00055], 3, atlas.uv("petiole"), v_length=0.32,
               phase=0.29, flutter=0.10)
        down = (heading * 0.35 + Vector((0, 0, -1))).normalized()
        rad = rng.uniform(0.0135, 0.0170)
        b.sphere(pts[-1] + down * rad * 0.90, rad, atlas.uv("berry", inset=False), segs=10, rings=6,
                 stretch=rng.uniform(1.25, 1.45), axis=down, phase=0.29)
        b.flat(pts[-1] + down * rad * 0.08, -down, rad * 2.55, atlas.uv("calyx"),
               spin=rng.uniform(0, math.tau), cup=-0.18, phase=0.29, flutter=0.30, segs=1)
    print("HOMESTEAD_TRIS Harvest 0", b.triangles)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    objs = []
    for stage in ("Sprout", "Young", "Growing", "Mature", "Ripe"):
        batches = [_emit_stage(stage, atlas, lod) for lod in range(3)]
        made = F.finish_lods(kit, batches, f"SM_{NAME}_{stage}", material, smooth_angle=179.0)
        print("HOMESTEAD_LODS", stage, F.lod_report(made))
        objs.extend(made)
    harvest = _emit_harvest(atlas).build(f"SM_{NAME}_Harvest", material)
    harvest = kit.join([harvest], f"SM_{NAME}_Harvest", pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    harvest.data.color_attributes.active_color = harvest.data.color_attributes["Wind"]
    objs.append(harvest)
    return objs
