"""Woodland strawberry (Fragaria vesca): an ankle-high groundcover patch of trifoliate leaves.

The real plant (what the geometry and textures follow):
- Common in the shady mixed-conifer and oak woodland of the Sierra Nevada foothills around
  Yosemite; spreads by thin reddish runners (stolons) that root into daughter plantlets, so it
  forms loose mats 10-25 cm tall.
- Leaves basal, on hairy petioles 5-15 cm long, each of three leaflets 2-6 cm long: obovate,
  coarsely toothed, the veins deeply impressed and parallel (a quilted look), bright yellow-green
  and slightly glossy above, paler and silky below; held nearly level. The lateral leaflets are
  lopsided. Old leaves turn bronze-red or brown and lie on the ground.
- Flowers 1.2-2 cm, five round white petals around a domed yellow-green centre, two to four on a
  slender stalk that rises about as high as the leaves; fruit a small conical red strawberry with
  the seeds on the surface, nodding on its stalk, some still white-green.

Game notes: walk-through (no collision). One 2K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G per-plantlet phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "WildStrawberry"
DESCRIPTION = ("Woodland strawberry groundcover patch, 15 cm tall and 70 cm across: trifoliate toothed leaves "
               "on hairy petioles, runners with daughter plantlets, white flowers and a few red berries. "
               "Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.05, 0.06)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-plantlet random phase", "B": "leaf flutter 0 at the petiole top -> 1 at leaflet tips",
             "A": "1"},
    "material_notes": ("One material M_WildStrawberry: T_WildStrawberry_basecolor (sRGB, alpha = opacity mask, "
                       "clip 0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G "
                       "translucency mask, B AO). Two-sided foliage, masked."),
}

SEED = 4417
LEAFLETS = ("green", "green2", "light", "red", "dry")


def _palettes():
    base = dict(yellow=(0.20, 0.17, 0.035), brown=(0.07, 0.035, 0.014), stalk=(0.10, 0.09, 0.04))
    return {
        "green": dict(base, base=(0.070, 0.112, 0.026), tip=(0.074, 0.114, 0.027), vein=(0.10, 0.14, 0.045),
                      margin=(0.060, 0.094, 0.022)),
        "green2": dict(base, base=(0.060, 0.098, 0.022), tip=(0.066, 0.104, 0.024), vein=(0.092, 0.13, 0.04),
                       margin=(0.052, 0.084, 0.020)),
        "light": dict(base, base=(0.090, 0.135, 0.032), tip=(0.098, 0.140, 0.034), vein=(0.125, 0.165, 0.055),
                      margin=(0.080, 0.118, 0.028)),
        "red": dict(base, base=(0.13, 0.045, 0.022), tip=(0.16, 0.05, 0.022), vein=(0.10, 0.07, 0.03),
                    margin=(0.10, 0.03, 0.016), yellow=(0.19, 0.10, 0.025), brown=(0.06, 0.025, 0.012)),
        "dry": dict(base, base=(0.10, 0.06, 0.025), tip=(0.12, 0.065, 0.028), vein=(0.14, 0.09, 0.04),
                    margin=(0.07, 0.04, 0.018), dry=(0.15, 0.08, 0.032)),
    }


PARAMS = {
    "green": dict(gloss=0.25, hair=0.1),
    "green2": dict(gloss=0.2, hair=0.1, damage=0.05, edge_burn=0.05),
    "light": dict(gloss=0.2, hair=0.12, trans=0.72),
    "red": dict(gloss=0.1, yellow=0.35, damage=0.35, edge_burn=0.3),
    "dry": dict(dry=1.0, edge_burn=0.5, holes=0.4, trans=0.25),
}


def _leaflet_shape(rng):
    return F.ovate(width=0.36, widest=0.62, tip_sharp=0.55, base_round=1.1, base=0.02, tip=0.97,
                   teeth=13, tooth_depth=0.13, double=0.15, phase=rng.uniform(0, 1))


def _leaflet_veins(nrng):
    # Straight, closely parallel secondaries running into the teeth: the quilted strawberry look.
    shape = F.ovate(width=0.36, widest=0.62, tip_sharp=0.55, base_round=1.1, base=0.02, tip=0.97)
    veins = [([(0.0, 0.02), (0.0, 0.96)], 1.0)]
    for k in range(10):
        y0 = 0.08 + 0.075 * k + float(nrng.uniform(-0.01, 0.01))
        y1 = min(y0 + 0.13, 0.95)
        _, _, hw = shape(np.zeros(1), np.array([y1]))
        for side in (-1, 1):
            yy = y0 + (0.012 if side > 0 else 0.0)
            veins.append(([(0.0, yy), (side * hw[0] * 0.55, yy + (y1 - y0) * 0.5), (side * hw[0] * 0.93, y1)],
                          0.55 - 0.02 * k))
    return veins


def _leaflet(X, Y, px, nrng, rng, key, pals):
    return F.paint_blade(X, Y, nrng, _leaflet_shape(rng), _leaflet_veins(nrng), pals[key], px,
                         vein_width=0.010, vein_depth=0.00013, puff=0.00013, tertiary=0.3, stalk=0.02,
                         **PARAMS[key])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("petiole", 48)
    for key in LEAFLETS:
        atlas.tile(key, 400, 560)
    atlas.tile("trifoliate", 700, 700)
    atlas.tile("flower", 240, 240)
    atlas.tile("calyx", 160, 160)
    atlas.tile("berry", 200, 200)
    atlas.tile("unripe", 200, 200)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    for key in LEAFLETS:
        X, Y, px = atlas.grid(key)
        atlas.put(key, _leaflet(X, Y, px, nrng, rng, key, pals), meters_per_px=0.045 / X.shape[0])
    # Whole trifoliate leaf seen from above for the far LOD (petiole top at the bottom centre).
    X, Y, px = atlas.grid("trifoliate")
    layer = F.Layer(X.shape)
    layer.color[...] = pals["green"]["margin"]
    for th, ln, key in ((-1.15, 0.46, "green2"), (1.15, 0.46, "green"), (0.0, 0.56, "green")):
        Xl, Yl = S.rotated(X, Y, (0.0, 0.08), th, ln)
        layer.over(_leaflet(Xl, Yl, px / ln, nrng, rng, key, pals))
    atlas.put("trifoliate", layer, meters_per_px=0.11 / X.shape[0])
    # Flower: five round white petals, domed yellow-green receptacle ringed by stamens.
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    petal = F.ovate(width=0.52, widest=0.62, tip_sharp=0.35, base_round=1.4, base=0.0, tip=1.0)
    for k in range(5):
        th = math.radians(72 * k + rng.uniform(-6, 6))
        Xl, Yl = S.rotated(X, Y, (0.0, 0.5), th, 0.47)
        streaks = [([(0, 0.05), (0.25 * s, 0.95)], 0.3) for s in (-1, -0.5, 0, 0.5, 1)]
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, streaks,
                                 dict(base=(0.60, 0.60, 0.52), tip=(0.72, 0.72, 0.66), vein=(0.64, 0.64, 0.58),
                                      margin=(0.74, 0.74, 0.70)), px / 0.47, vein_width=0.02, vein_depth=0.0001,
                                 puff=0.0002, tertiary=0.0, trans=0.85, gloss=-0.1))
    r = np.hypot(X, Y - 0.5)
    n = F.noise(X.shape, nrng, freq=60.0, beta=1.3)
    ring = F.smoothstep(0.11, 0.13, r) * (1 - F.smoothstep(0.16, 0.19, r))
    disc = 1 - F.smoothstep(0.12, 0.14, r)
    layer.color = F.lerp(layer.color, F.lerp((0.45, 0.36, 0.04), (0.60, 0.50, 0.10), n), ring * (n > 0.45))
    layer.color = F.lerp(layer.color, F.lerp((0.28, 0.30, 0.05), (0.40, 0.38, 0.08), n), disc)
    layer.height = layer.height + 0.0004 * disc * (1 - r / 0.13) + 0.0001 * ring * n
    atlas.put("flower", layer, meters_per_px=0.018 / X.shape[0])
    # Calyx: a green ten-pointed star (sepals and bracteoles) that sits on top of the berry.
    X, Y, px = atlas.grid("calyx")
    layer = F.Layer(X.shape)
    sepal = F.ovate(width=0.28, widest=0.35, tip_sharp=1.3, base=0.0, tip=1.0)
    for k in range(10):
        th = math.radians(36 * k)
        ln = 0.46 if k % 2 == 0 else 0.32
        Xl, Yl = S.rotated(X, Y, (0.0, 0.5), th, ln)
        layer.over(F.paint_blade(Xl, Yl, nrng, sepal, [([(0, 0), (0, 1)], 1.0)], pals["green2"], px / ln,
                                 vein_width=0.03, tertiary=0.0, hair=0.2))
    atlas.put("calyx", layer, meters_per_px=0.014 / X.shape[0])
    # Berries: glossy red flesh (or white-green unripe) with yellow seeds in small pits.
    for key, flesh, flesh2 in (("berry", (0.30, 0.018, 0.016), (0.42, 0.03, 0.022)),
                               ("unripe", (0.36, 0.40, 0.22), (0.46, 0.46, 0.30))):
        X, Y, px = atlas.grid(key)
        U, V = X / (2 * X.max()) + 0.5, Y
        cells = (np.sin(U * math.tau * 9) * np.sin(V * math.pi * 11 + U * 3.0))
        seeds = F.smoothstep(0.80, 0.93, cells)
        pits = F.smoothstep(0.55, 0.95, cells)
        n = F.noise(X.shape, nrng, freq=20.0, beta=2.0)
        layer = F.Layer(X.shape)
        col = F.lerp(flesh, flesh2, n)
        col = F.lerp(col, np.asarray(col) * 0.6, 1 - F.smoothstep(0.0, 0.35, V))   # paler/darker near stalk
        layer.color = F.lerp(col, (0.42, 0.32, 0.06), seeds)
        layer.alpha[...] = 1
        layer.height = -0.00025 * pits + 0.00015 * seeds
        layer.rough = 0.32 + 0.3 * seeds
        layer.trans[...] = 0.25
        atlas.put(key, layer, meters_per_px=0.012 / X.shape[0], opaque=True)
    # Petiole / stalk / runner: hairy green with reddish tint (v runs along).
    U, V = atlas.column_grid("petiole")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 6.0))
    hairs = F.smoothstep(0.78, 0.92, F.noise(U.shape, nrng, freq=160.0, beta=1.0, aniso=(1.0, 3.0)))
    layer = F.Layer(U.shape)
    col = F.lerp((0.085, 0.10, 0.035), (0.13, 0.05, 0.03), F.smoothstep(0.3, 0.8, st) * 0.6)
    layer.color = F.lerp(col, (0.26, 0.28, 0.20), hairs * 0.6)
    layer.height = 0.00008 * st + 0.00005 * hairs
    layer.rough = 0.55 + 0.15 * hairs
    layer.trans[...] = 0.3
    atlas.put("petiole", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    """Plantlets (crowns) linked by runners; each carries leaves, flower stalks and berries."""
    plants = [dict(pos=Vector((0.0, 0.0, 0.0)), size=1.0, leaves=12, heading=0.0),
              dict(pos=Vector((0.15, 0.09, 0.0)), size=0.9, leaves=10, heading=1.0),
              dict(pos=Vector((-0.06, 0.16, 0.0)), size=0.8, leaves=8, heading=2.2)]
    runners = []
    for parent, az, dist in ((0, 3.5, 0.20), (0, 4.7, 0.17), (1, 0.3, 0.18), (1, 5.6, 0.15), (2, 2.3, 0.17)):
        p0 = plants[parent]["pos"]
        tip = p0 + Vector((math.cos(az), math.sin(az), 0)) * dist
        runners.append((p0, tip))
        plants.append(dict(pos=tip, size=rng.uniform(0.5, 0.7), leaves=rng.randint(3, 5), heading=az))
    for p in plants:
        p["phase"] = rng.random()
        p["leaf_list"] = []
        n = p["leaves"]
        for i in range(n):
            az = p["heading"] + i * 2.39996 + rng.uniform(-0.3, 0.3)
            old = i >= n - 2 and p["size"] > 0.8
            elev = math.radians(rng.uniform(15, 30) if old else rng.uniform(58, 80))
            pet = p["size"] * (rng.uniform(0.05, 0.07) if old else rng.uniform(0.06, 0.12))
            p["leaf_list"].append(dict(az=az, elev=elev, pet=pet, old=old, leaflet=p["size"] * rng.uniform(0.045, 0.06),
                                       roll=rng.uniform(-0.25, 0.25), tilt=rng.uniform(-0.15, 0.25),
                                       seed=rng.randrange(1 << 20)))
    fruit = []
    for p in plants[:2]:
        for j in range(2):
            az = p["heading"] + 1.3 + j * 3.1 + rng.uniform(-0.4, 0.4)
            fruit.append(dict(plant=p, az=az, height=rng.uniform(0.09, 0.14), n=rng.randint(2, 4),
                              seed=rng.randrange(1 << 20)))
    return plants, runners, fruit


def _arc(p0, heading, elev, length, droop, n=5):
    """Arching stalk from ``p0``: starts at ``elev`` and bows down by ``droop`` radians."""
    pts = [p0.copy()]
    side = heading.cross(Vector((0, 0, 1))).normalized()
    d = (heading * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    for i in range(n):
        d = (Matrix.Rotation(-droop / n, 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def emit(plants, runners, fruit, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"stalks": 0, "leaves": 0, "fruit": 0}
    for p0, tip in runners:
        if lod == 2:
            break
        t0 = b.triangles
        mid = p0.lerp(tip, 0.5) + Vector((0, 0, 0.018))
        pts = [p0 + Vector((0, 0, 0.004)), p0.lerp(mid, 0.5) + Vector((0, 0, 0.006)), mid,
               mid.lerp(tip, 0.5) + Vector((0, 0, 0.004)), tip + Vector((0, 0, 0.002))]
        if lod == 1:
            pts = pts[::2]
        b.tube(pts, [0.0012] * len(pts), 3, atlas.uv("petiole"), v_length=0.3, phase=0.0, flutter=0.1)
        stats["stalks"] += b.triangles - t0
    for p in plants:
        for lf in p["leaf_list"]:
            rng = random.Random(lf["seed"])
            heading = Vector((math.cos(lf["az"]), math.sin(lf["az"]), 0))
            pts = _arc(p["pos"], heading, lf["elev"], lf["pet"], lf["elev"] * 0.55, n=3)
            t0 = b.triangles
            idx = ([0, 1, 2, 3], [0, 3], [0, 3])[lod]
            b.tube([pts[i] for i in idx], [0.0011, 0.0009, 0.0008, 0.0007][:len(idx)], 3,
                   atlas.uv("petiole"), v_length=0.3, phase=p["phase"], flutter=0.15)
            stats["stalks"] += b.triangles - t0
            t0 = b.triangles
            top = pts[-1]
            # Leaflets held nearly level, facing up and slightly toward the light.
            up = (Vector((0, 0, 1)) + heading * lf["tilt"]).normalized()
            up = Matrix.Rotation(lf["roll"], 3, heading) @ up
            if lf["old"]:
                keys = ["dry" if rng.random() < 0.5 else "red"] * 3
            else:
                keys = [rng.choices(["green", "green2", "light", "red"], [44, 36, 16, 4])[0] for _ in range(3)]
            L = lf["leaflet"]
            if lod == 2:
                size = L * 2.1
                b.card(top - heading * L * 0.12, heading, up, size, size, atlas.uv("trifoliate"), rows=1, cols=1,
                       fold=0.0, droop=0.1, phase=p["phase"], flutter=1.0, flutter_base=0.3)
                stats["leaves"] += b.triangles - t0
                continue
            for k, (ang, scale) in enumerate(((0.0, 1.0), (-1.2, 0.88), (1.2, 0.88))):
                d = Matrix.Rotation(ang + rng.uniform(-0.12, 0.12), 3, up) @ heading
                d = (d - up * d.dot(up) * 0.7 + Vector((0, 0, 0.12))).normalized()
                ln = L * scale * rng.uniform(0.92, 1.06)
                rect = atlas.uv(keys[k])
                if ang > 0:
                    rect = (rect[1], rect[0], rect[2], rect[3])    # mirror lateral leaflets
                width = ln * S.tile_aspect(atlas, keys[k])
                b.card(top + d * L * 0.03, d, up, ln, width, rect, rows=(2, 1)[lod], cols=(2, 1)[lod],
                       fold=(0.32 if not lf["old"] else 0.1) if lod == 0 else 0.0,
                       curl=(0.1 if lf["old"] else -0.04) if lod == 0 else 0.0,
                       droop=rng.uniform(0.1, 0.3) + (0.3 if lf["old"] else 0.0), twist=rng.uniform(-0.1, 0.1),
                       phase=p["phase"], flutter=1.0, flutter_base=0.3)
            stats["leaves"] += b.triangles - t0
    for fr in fruit:
        rng = random.Random(fr["seed"])
        p = fr["plant"]
        heading = Vector((math.cos(fr["az"]), math.sin(fr["az"]), 0))
        t0 = b.triangles
        stalk = _arc(p["pos"], heading, math.radians(70), fr["height"], 0.7, n=4)
        idx = (list(range(5)), [0, 2, 4], [0, 4])[lod]
        b.tube([stalk[i] for i in idx], [0.0009] * len(idx), 3, atlas.uv("petiole"), v_length=0.3,
               phase=p["phase"], flutter=0.3)
        top = stalk[-1]
        for j in range(fr["n"]):
            az = fr["az"] + (j - (fr["n"] - 1) / 2) * 0.9 + rng.uniform(-0.2, 0.2)
            hd = Vector((math.cos(az), math.sin(az), 0))
            kind = rng.choices(["flower", "berry", "unripe"], [50, 32, 18])[0]
            if kind == "flower":
                ped = _arc(top, hd, math.radians(40), 0.025, 0.2, n=2)
                if lod < 2:
                    b.tube(ped if lod == 0 else [ped[0], ped[-1]], [0.0006] * (len(ped) if lod == 0 else 2), 3,
                           atlas.uv("petiole"), v_length=0.3, phase=p["phase"], flutter=0.4)
                face = (Vector((0, 0, 1)) + hd * rng.uniform(0.3, 0.8)).normalized()
                b.flat(ped[-1] + face * 0.002, face, rng.uniform(0.016, 0.02), atlas.uv("flower"),
                       spin=rng.uniform(0, 6.28), cup=0.12 if lod == 0 else 0.0, phase=p["phase"],
                       segs=2 if lod == 0 else 1)
            else:
                if lod == 2:
                    continue
                ped = _arc(top, hd, math.radians(10), 0.03, 1.3, n=3)
                b.tube(ped if lod == 0 else [ped[0], ped[-1]], [0.0006] * (len(ped) if lod == 0 else 2), 3,
                       atlas.uv("petiole"), v_length=0.3, phase=p["phase"], flutter=0.4)
                down = (ped[-1] - ped[-2]).normalized()
                down = (down + Vector((0, 0, -0.8))).normalized()
                r = rng.uniform(0.0045, 0.006) * (0.8 if kind == "unripe" else 1.0)
                b.sphere(ped[-1] + down * r * 1.1, r, atlas.uv(kind, inset=False), segs=(6, 4)[lod],
                         rings=(4, 3)[lod], stretch=1.3, axis=down, phase=p["phase"])
                if lod == 0:
                    b.flat(ped[-1] + down * r * 0.15, -down, r * 3.2, atlas.uv("calyx"), spin=rng.uniform(0, 6.28),
                           cup=-0.25, phase=p["phase"], segs=1)
        stats["fruit"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    plants, runners, fruit = describe(random.Random(SEED))
    height = 0.15
    batches = [emit(plants, runners, fruit, atlas, lod, height) for lod in range(3)]
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
