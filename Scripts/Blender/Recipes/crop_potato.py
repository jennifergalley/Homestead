"""Procedural 1 m plot of Cornish potato crop stages for Homestead.

Real plant research (written before modeling):
- Potatoes were a Cornwall staple crop by the mid-19th century; early varieties were grown in
  hand-hoed, earthed-up ridges. A garden plant sits on a ridge top, throws several soft angular
  haulm stems from the crown, and spreads into a loose bush rather than a single upright stalk.
- Leaves are odd-pinnate/compound: 5-9 ovate leaflets plus much smaller interstitial leaflets,
  alternate along a hairy petiole. The leaf surface is matt dark green, slightly crinkled and
  hairy, with wavy margins and a clear midrib/secondary venation. Healthy foliage height is about
  12-18 cm young, 30-40 cm in active growth, and 45-55 cm when mature.
- Flowers are small five-pointed stars in terminal clusters, white to pale mauve with yellow
  anthers. As tubers ripen, haulm dies back: stems flop, leaves yellow to straw and brown, and
  exposed tubers near the ridge surface are golden-buff, 5-8 cm long, with shallow eyes.
- This recipe represents one 1 m tilled-bed plot: three plants at x=0, y=-0.3/0/+0.3 m, growing
  from ridge tops at z=0.045 m. The pivot remains the plot bottom-centre and collision is none.

All geometry/textures are original procedural work. One painted 2K atlas, one two-sided masked
foliage material, Wind vertex colours, and five stage meshes each with LOD1/LOD2.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "CropPotato"
DESCRIPTION = ("One 1 m garden plot of earthed-up Cornish potato plants in five crop stages: "
               "sprout, young, growing, mature/flowering and ripe die-back; produce is drawn "
               "by the game as separate anchored tuber clusters.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "views": ["hero"], "eye_distance": 5.0, "meshes": {
    "SM_CropPotato_Growing": {"focus": (0.0, 0.0, 0.24), "eye_distance": 5.0, "views": ["hero", "detail"]},
    "SM_CropPotato_Ripe": {"focus": (0.0, 0.0, 0.18), "eye_distance": 5.2, "views": ["hero", "detail"]},
    "SM_CropPotato_Produce": {"focus": (0.0, 0.0, -0.005), "eye_distance": 0.65,
                              "detail_distance": 0.28, "views": ["hero", "detail"]},
    "SM_CropPotato_Harvest": {"focus": (0.0, 0.0, -0.07), "eye_distance": 1.4, "views": ["hero", "detail"]},
}}
REPORT = {
    "blocking": False,
    "plot": "1 m square, pivot bottom-centre; plant crowns stand on tilled-bed ridge tops at z≈0.045 m.",
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above plot ground / stage height",
             "G": "per-stem random phase", "B": "leaf flutter mask; produce mesh is static", "A": "1"},
    "material_notes": ("One material M_CropPotato: 2K alpha-masked foliage atlas with dark matt potato leaflets, "
                       "yellow die-back leaves, star flowers, buff tubers and muted stem/ridge details. "
                       "SM_CropPotato_Produce uses material slot M_CropPotatoProduce with the same atlas."),
}

SEED = 1851
BASE_Z = 0.045
RIDGES = (-0.30, 0.0, 0.30)
STAGES = ("Sprout", "Young", "Growing", "Mature", "Ripe")


def _produce_anchor_lists():
    scales = (0.92, 1.05, 0.88)
    yaws = (-18.0, 11.0, 27.0)
    x_offsets = (-0.036, 0.042, -0.026)
    anchors = []
    for idx, y in enumerate(RIDGES):
        anchors.append([round(x_offsets[idx], 3), round(y, 3), round(BASE_Z, 3), yaws[idx], scales[idx]])
    return {stage: [list(a) for a in anchors] for stage in ("Young", "Growing", "Mature", "Ripe")}


REPORT["produce"] = {
    "mesh": "SM_CropPotato_Produce",
    "anchors": _produce_anchor_lists(),
}


def _palettes():
    return {
        "leaf": dict(base=(0.045, 0.082, 0.030), tip=(0.055, 0.100, 0.036),
                     vein=(0.074, 0.118, 0.045), margin=(0.035, 0.065, 0.025),
                     yellow=(0.22, 0.19, 0.055), brown=(0.08, 0.045, 0.020), stalk=(0.050, 0.075, 0.030)),
        "leaf2": dict(base=(0.040, 0.073, 0.027), tip=(0.052, 0.092, 0.033),
                      vein=(0.068, 0.106, 0.040), margin=(0.032, 0.058, 0.023),
                      yellow=(0.22, 0.19, 0.055), brown=(0.08, 0.045, 0.020), stalk=(0.050, 0.070, 0.030)),
        "shoot": dict(base=(0.035, 0.045, 0.025), tip=(0.065, 0.075, 0.034),
                      vein=(0.095, 0.070, 0.045), margin=(0.030, 0.032, 0.020),
                      brown=(0.055, 0.025, 0.014), stalk=(0.065, 0.040, 0.030)),
        "yellow": dict(base=(0.205, 0.175, 0.060), tip=(0.250, 0.215, 0.075),
                       vein=(0.235, 0.230, 0.105), margin=(0.135, 0.105, 0.040),
                       brown=(0.095, 0.055, 0.022), dry=(0.175, 0.125, 0.045), stalk=(0.120, 0.090, 0.040)),
        "dry": dict(base=(0.125, 0.078, 0.034), tip=(0.150, 0.095, 0.038),
                    vein=(0.190, 0.135, 0.055), margin=(0.070, 0.043, 0.020),
                    brown=(0.050, 0.027, 0.013), dry=(0.170, 0.105, 0.040), stalk=(0.100, 0.070, 0.035)),
    }


def _leaf_layer(X, Y, px, nrng, key, phase=0.0):
    pals = _palettes()
    shape = F.ovate(width=0.30, widest=0.48, tip_sharp=0.95, base_round=0.72,
                    base=0.025, tip=0.97, teeth=10, tooth_depth=0.030, double=0.20, phase=phase)
    veins = F.pinnate_veins(count=7, angle=0.78, curve=0.48, reach=0.88, width=0.30,
                            widest=0.48, rng=nrng, midrib_bend=0.018 * math.sin(phase * math.tau))
    params = dict(vein_width=0.010, vein_depth=0.00018, puff=0.00015, tertiary=0.42,
                  hair=0.25, stalk=0.018, trans=0.48)
    if key == "leaf2":
        params.update(gloss=-0.05, damage=0.03)
    elif key == "yellow":
        params.update(yellow=0.70, damage=0.38, edge_burn=0.32, trans=0.28)
    elif key == "dry":
        params.update(dry=0.92, damage=0.45, holes=0.18, edge_burn=0.55, trans=0.15)
    elif key == "shoot":
        params.update(vein_width=0.016, tertiary=0.1, hair=0.35, trans=0.35)
    return F.paint_blade(X, Y, nrng, shape, veins, pals[key], px, **params)


def _paint_compound(atlas, key, nrng, rng, dry=False):
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    layer.color[...] = (0.030, 0.050, 0.022)
    slots = [(0.00, 0.16, 0.32), (-0.25, 0.32, 0.23), (0.25, 0.42, 0.25),
             (-0.22, 0.57, 0.25), (0.22, 0.68, 0.26), (0.00, 0.82, 0.31)]
    for i, (x, y, ln) in enumerate(slots):
        theta = (0.35 if x >= 0 else -0.35) + rng.uniform(-0.10, 0.10)
        Xl, Yl = S.rotated(X, Y, (x, y), theta, ln)
        k = "dry" if dry and i > 1 and rng.random() < 0.45 else ("yellow" if dry else rng.choice(["leaf", "leaf2"]))
        layer.over(_leaf_layer(Xl, Yl, px / ln, nrng, k, phase=rng.random()))
    atlas.put(key, layer, meters_per_px=0.13 / X.shape[0])


def _paint_flower(atlas, nrng, rng):
    X, Y, px = atlas.grid("flower")
    layer = F.Layer(X.shape)
    petal = F.ovate(width=0.22, widest=0.42, tip_sharp=1.7, base_round=0.35, base=0.0, tip=1.0)
    pal = dict(base=(0.58, 0.55, 0.62), tip=(0.72, 0.69, 0.76), vein=(0.48, 0.43, 0.55),
               margin=(0.76, 0.74, 0.78), brown=(0.10, 0.07, 0.03))
    for k in range(5):
        th = math.radians(72 * k + 18)
        Xl, Yl = S.rotated(X, Y, (0.0, 0.49), th, 0.44)
        layer.over(F.paint_blade(Xl, Yl, nrng, petal, [([(0, 0.02), (0, 0.95)], 0.5)], pal, px / 0.44,
                                 vein_width=0.018, vein_depth=0.00004, puff=0.00006, tertiary=0.0,
                                 trans=0.85, gloss=-0.2))
    r = np.hypot(X, Y - 0.50)
    n = F.noise(X.shape, nrng, freq=48, beta=1.6)
    centre = 1 - F.smoothstep(0.075, 0.095, r)
    anthers = (1 - F.smoothstep(0.12, 0.145, r)) * F.smoothstep(0.065, 0.095, r) * (n > 0.38)
    layer.color = F.lerp(layer.color, (0.50, 0.38, 0.040), centre)
    layer.color = F.lerp(layer.color, (0.78, 0.58, 0.060), anthers)
    layer.alpha = np.maximum(layer.alpha, np.maximum(centre, anthers))
    layer.height = layer.height + 0.00035 * centre + 0.00018 * anthers
    atlas.put("flower", layer, meters_per_px=0.025 / X.shape[0])


def _paint_tuber(atlas, nrng):
    X, Y, px = atlas.grid("tuber")
    n1 = F.noise(X.shape, nrng, freq=13, beta=2.0)
    n2 = F.noise(X.shape, nrng, freq=60, beta=1.3)
    layer = F.Layer(X.shape)
    base = F.lerp((0.265, 0.205, 0.110), (0.365, 0.285, 0.155), n1)
    freckles = F.smoothstep(0.77, 0.94, n2)
    eyes = F.smoothstep(0.965, 0.992, F.noise(X.shape, nrng, freq=28, beta=2.8, aniso=(1.7, 1.0)))
    layer.color = F.lerp(base, (0.135, 0.085, 0.040), freckles * 0.35 + eyes * 0.9)
    layer.alpha[...] = 1
    layer.height = -0.00035 * eyes + 0.00008 * n2
    layer.rough = 0.74 + 0.12 * freckles
    layer.trans[...] = 0.03
    atlas.put("tuber", layer, meters_per_px=0.065 / X.shape[0], opaque=True)


def _paint_soil(atlas, nrng):
    X, Y, px = atlas.grid("soil")
    n = F.noise(X.shape, nrng, freq=20, beta=1.5)
    grit = F.smoothstep(0.70, 0.94, F.noise(X.shape, nrng, freq=95, beta=1.0))
    layer = F.Layer(X.shape)
    layer.color = F.lerp((0.060, 0.042, 0.026), (0.145, 0.105, 0.060), n)
    layer.color = F.lerp(layer.color, (0.030, 0.023, 0.017), grit * 0.45)
    layer.alpha[...] = 1
    layer.height = 0.00022 * n + 0.00012 * grit
    layer.rough = 0.86 + 0.10 * grit
    layer.trans[...] = 0.0
    atlas.put("soil", layer, meters_per_px=0.050 / X.shape[0], opaque=True)


def _paint_stem_column(atlas, nrng):
    U, V = atlas.column_grid("stem")
    n = F.noise(U.shape, nrng, freq=42, beta=1.4, aniso=(1.0, 8.0))
    hair = F.smoothstep(0.76, 0.93, F.noise(U.shape, nrng, freq=150, beta=1.0, aniso=(1.0, 4.0)))
    layer = F.Layer(U.shape)
    col = F.lerp((0.050, 0.075, 0.030), (0.105, 0.075, 0.040), F.smoothstep(0.35, 0.9, n))
    layer.color = F.lerp(col, (0.26, 0.28, 0.20), hair * 0.45)
    layer.alpha[...] = 1
    layer.height = 0.00007 * n + 0.00005 * hair
    layer.rough = 0.58 + 0.18 * hair
    layer.trans[...] = 0.28
    atlas.put("stem", layer, meters_per_px=0.4 / U.shape[0], opaque=True, wrap=True)


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED, padding=12)
    atlas.column("stem", 64)
    for key in ("leaf", "leaf2", "yellow", "dry", "shoot"):
        atlas.tile(key, 330, 520)
    atlas.tile("compound", 380, 720)
    atlas.tile("compound_dry", 380, 720)
    atlas.tile("flower", 260, 260)
    atlas.tile("tuber", 360, 360)
    atlas.tile("soil", 220, 220)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    for key in ("leaf", "leaf2", "yellow", "dry", "shoot"):
        X, Y, px = atlas.grid(key)
        atlas.put(key, _leaf_layer(X, Y, px, nrng, key, phase=rng.random()), meters_per_px=0.070 / X.shape[0])
    _paint_compound(atlas, "compound", nrng, rng, dry=False)
    _paint_compound(atlas, "compound_dry", nrng, rng, dry=True)
    _paint_flower(atlas, nrng, rng)
    _paint_tuber(atlas, nrng)
    _paint_soil(atlas, nrng)
    _paint_stem_column(atlas, nrng)
    atlas.save()
    return atlas


def _arc(p0, heading, elev, length, droop, n=4):
    pts = [Vector(p0)]
    side = heading.cross(Vector((0, 0, 1)))
    if side.length < 1e-4:
        side = Vector((1, 0, 0))
    side.normalize()
    d = (heading * math.cos(elev) + Vector((0, 0, math.sin(elev)))).normalized()
    for _ in range(n):
        d = (Matrix.Rotation(-droop / max(n, 1), 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def _compound_leaf(b, atlas, base, heading, length, key, lod, phase, rng, floppy=0.0):
    elev = math.radians(rng.uniform(48, 82)) * (1.0 - 0.70 * floppy) + math.radians(5) * floppy
    droop = rng.uniform(0.08, 0.42) + 1.35 * floppy
    pts = _arc(base, heading, elev, length * rng.uniform(0.85, 1.1), droop, n=(4, 3, 2)[lod])
    idx = pts if lod == 0 else (pts[::2] if lod == 1 and len(pts) > 3 else [pts[0], pts[-1]])
    b.tube(idx, [0.0019 * (1 - 0.55 * i / max(len(idx) - 1, 1)) for i in range(len(idx))], 4,
           atlas.uv("stem"), v_length=0.35, phase=phase, flutter=0.12)
    top = pts[-1]
    up = (Vector((0, 0, 1)) - heading * 0.15).normalized()
    if lod == 2:
        rect = atlas.uv("compound_dry" if key in ("yellow", "dry") else "compound")
        b.card(pts[1] if len(pts) > 2 else base, heading, up, length * 0.72, length * 0.30, rect,
               rows=1, cols=1, fold=0.010, droop=0.20 + floppy, twist=rng.uniform(-0.2, 0.2),
               phase=phase, flutter=1.0, flutter_base=0.25)
        return
    spray_rect = atlas.uv("compound_dry" if key in ("yellow", "dry") else "compound")
    spray_dir = (top - pts[1]).normalized() if len(pts) > 2 and (top - pts[1]).length > 1e-4 else heading
    b.card(pts[1] if len(pts) > 2 else base, spray_dir, up, length * 0.74, length * 0.34, spray_rect,
           rows=(2, 1)[lod], cols=1, fold=0.010, droop=0.12 + 0.25 * floppy,
           twist=rng.uniform(-0.15, 0.15), phase=phase, flutter=0.85, flutter_base=0.15)
    side = heading.cross(up)
    if side.length < 1e-4:
        side = heading.orthogonal()
    side.normalize()
    count = 5 if lod == 0 and length > 0.10 else 4
    for k in range(count):
        t = 0.25 + 0.72 * k / max(count - 1, 1)
        mid = pts[0].lerp(top, t)
        terminal = (k == count - 1)
        sign = -1 if k % 2 else 1
        if terminal:
            sign = 0
        leaf_len = length * (0.21 if terminal else rng.uniform(0.15, 0.20)) * (1.15 if terminal else 1.0)
        if k in (1, 4) and not terminal:
            leaf_len *= 0.58
        d = (heading * (0.85 if terminal else 0.35) + side * (0.72 * sign) + Vector((0, 0, 0.12 - 0.22 * floppy))).normalized()
        rect = atlas.uv(key if (key in ("yellow", "dry") or rng.random() > 0.20) else "leaf2")
        if sign < 0:
            rect = (rect[1], rect[0], rect[2], rect[3])
        aspect_key = key if key in ("yellow", "dry") else "leaf"
        b.card(mid, d, up, leaf_len, leaf_len * S.tile_aspect(atlas, aspect_key),
               rect, rows=(2, 2)[lod], cols=1, fold=0.012, curl=rng.uniform(-0.04, 0.04),
               droop=rng.uniform(0.05, 0.22) + 0.40 * floppy, twist=rng.uniform(-0.20, 0.20),
               phase=phase, flutter=1.0, flutter_base=0.20)


def _flower_cluster(b, atlas, base, heading, phase, lod, rng):
    if lod == 2:
        return
    stem = _arc(base, heading, math.radians(55), 0.08, 0.15, n=3 if lod == 0 else 2)
    b.tube(stem, [0.0011, 0.0009, 0.0007, 0.0006][:len(stem)], 3, atlas.uv("stem"), v_length=0.35,
           phase=phase, flutter=0.15)
    top = stem[-1]
    flowers = 5 if lod == 0 else 3
    for i in range(flowers):
        az = i * math.tau / flowers + rng.uniform(-0.25, 0.25)
        off = Vector((math.cos(az), math.sin(az), 0)) * rng.uniform(0.010, 0.026)
        normal = (Vector((0, 0, 1)) + off.normalized() * 0.45).normalized() if off.length else Vector((0, 0, 1))
        b.flat(top + off + Vector((0, 0, rng.uniform(-0.004, 0.006))), normal, rng.uniform(0.020, 0.027),
               atlas.uv("flower"), spin=rng.uniform(0, math.tau), cup=0.08, phase=phase, flutter=0.6,
               segs=2 if lod == 0 else 1)


def _tuber(b, atlas, center, radius, axis, phase, lod):
    b.sphere(center, radius, atlas.uv("tuber", inset=False), segs=(8, 6, 5)[lod], rings=(5, 4, 3)[lod],
             stretch=1.35, axis=axis, phase=phase)


def _stage_description(stage, rng):
    plants = []
    for y in RIDGES:
        plants.append({"base": Vector((0.0, y, BASE_Z)), "phase": rng.random(), "seed": rng.randrange(1 << 20)})
    cfg = {
        "Sprout": dict(height=0.06, leaves=0, shoots=5, spread=0.035),
        "Young": dict(height=0.18, leaves=7, length=0.135, spread=0.13),
        "Growing": dict(height=0.40, leaves=13, length=0.210, spread=0.20),
        "Mature": dict(height=0.54, leaves=12, length=0.255, spread=0.21, flowers=True),
        "Ripe": dict(height=0.38, leaves=10, length=0.220, spread=0.23, ripe=True),
    }[stage]
    return plants, cfg


def emit(stage, atlas, lod):
    rng = random.Random(SEED + 101 * STAGES.index(stage))
    plants, cfg = _stage_description(stage, rng)
    b = F.Batch(height=max(cfg["height"], 0.08))
    stats = {"stems": 0, "leaves": 0, "flowers": 0, "tubers": 0}
    for pidx, p in enumerate(plants):
        prng = random.Random(p["seed"] + lod * 37)
        base = p["base"]
        if stage == "Sprout":
            for _ in range(cfg["shoots"] - lod):
                az = prng.uniform(0, math.tau)
                hd = Vector((math.cos(az), math.sin(az), 0))
                ln = prng.uniform(0.025, 0.052) * (1.0 - 0.15 * lod)
                pts = _arc(base + hd * prng.uniform(0.0, 0.012), hd, math.radians(prng.uniform(65, 86)), ln,
                           prng.uniform(0.08, 0.25), n=2)
                t0 = b.triangles
                b.tube(pts, [0.0016, 0.0012, 0.0008], 3, atlas.uv("stem"), v_length=0.30,
                       phase=p["phase"], flutter=0.15)
                b.card(pts[-1], hd, Vector((0, 0, 1)), ln * 0.75, ln * 0.30, atlas.uv("shoot"),
                       rows=1, cols=1, fold=0.010, droop=0.04, phase=p["phase"], flutter=0.7)
                stats["stems"] += b.triangles - t0
            continue
        leaf_total = cfg["leaves"]
        if lod == 1:
            leaf_total = int(leaf_total * 0.58)
        elif lod == 2:
            leaf_total = int(leaf_total * 0.34)
        for i in range(leaf_total):
            az = i * 2.39996 + prng.uniform(-0.42, 0.42) + pidx * 0.4
            hd = Vector((math.cos(az), math.sin(az), 0))
            if abs(base.y + hd.y * cfg["spread"]) > 0.515:
                hd.y *= 0.45
                hd.normalize()
            key = prng.choice(["leaf", "leaf2"])
            floppy = 0.0
            if cfg.get("ripe"):
                key = prng.choices(["yellow", "dry", "leaf2"], [52, 36, 12])[0]
                floppy = prng.uniform(0.55, 1.0)
            length = cfg["length"] * prng.uniform(0.82, 1.18)
            node_z = 0.004
            if stage == "Young":
                node_z += prng.uniform(0.015, 0.060)
            elif stage == "Growing":
                node_z += prng.uniform(0.050, 0.160)
            elif stage == "Mature":
                node_z += prng.uniform(0.070, 0.285)
            elif cfg.get("ripe"):
                node_z += prng.uniform(0.020, 0.140)
            t0 = b.triangles
            leaf_base = base + Vector((0, 0, node_z))
            if node_z > 0.018 and lod < 2:
                b.tube([base, leaf_base], [0.0023, 0.0015], 4, atlas.uv("stem"), v_length=0.35,
                       phase=p["phase"], flutter=0.08)
            _compound_leaf(b, atlas, leaf_base, hd, length, key, lod, p["phase"], prng, floppy)
            stats["leaves"] += b.triangles - t0
        if cfg.get("flowers"):
            for j in range(3 if lod == 0 else 1):
                hd = Vector((math.cos(j * 2.1 + pidx), math.sin(j * 2.1 + pidx), 0))
                t0 = b.triangles
                _flower_cluster(b, atlas, base + Vector((0, 0, 0.32 + 0.07 * prng.random())), hd,
                                p["phase"], lod, prng)
                stats["flowers"] += b.triangles - t0
    print("HOMESTEAD_TRIS", stage, lod, stats)
    return b


def _restore_plot_origin(objs):
    for obj in objs:
        shift = Vector(obj.get("homestead_shift", (0, 0, 0)))
        if shift.length > 1e-8:
            obj.data.transform(Matrix.Translation(shift))
            obj["homestead_shift"] = [0.0, 0.0, 0.0]
            obj.data.update()
    return objs


def harvest(kit, atlas, material):
    """Hand-held produce: three small tubers hanging from a broken haulm stem.

    Pivot/GRIP is at the origin where her fist closes on the snapped stem; the clump hangs down -Z.
    """
    rng = random.Random(SEED + 900)
    b = F.Batch(height=0.16)
    grip = Vector((0, 0, 0))
    stem = [grip, Vector((0.002, -0.004, -0.030)), Vector((-0.006, 0.006, -0.055))]
    b.tube(stem, [0.0032, 0.0024, 0.0016], 5, atlas.uv("stem"), v_length=0.28, phase=0.4, flutter=0.05)
    for i, (off, radius) in enumerate(((Vector((-0.026, 0.000, -0.085)), 0.030),
                                       (Vector((0.018, 0.018, -0.078)), 0.025),
                                       (Vector((0.012, -0.022, -0.112)), 0.028))):
        joint = stem[-1]
        b.tube([joint, joint.lerp(off, 0.45), off + Vector((0, 0, radius * 0.6))],
               [0.0013, 0.0009, 0.0005], 3, atlas.uv("stem"), v_length=0.28,
               phase=0.4 + i * 0.13, flutter=0.05)
        axis = (Vector((rng.uniform(-0.35, 0.35), rng.uniform(-0.2, 0.2), -1.0))).normalized()
        _tuber(b, atlas, off, radius, axis, 0.35 + 0.15 * i, 0)
    for i, az in enumerate((0.4, 2.7)):
        hd = Vector((math.cos(az), math.sin(az), -0.55)).normalized()
        b.card(Vector((0, 0, -0.030)), hd, Vector((0, 0, 1)), 0.055, 0.018,
               atlas.uv("dry" if i else "yellow"), rows=1, cols=1, fold=0.004, droop=0.35,
               phase=0.5, flutter=0.5)
    obj = b.build("SM_CropPotato_Harvest", material)
    obj = kit.join([obj], "SM_CropPotato_Harvest", pivot=None, unwrap=False, reshade=True, smooth_angle=179.0)
    print("HOMESTEAD_LODS Harvest", F.lod_report([obj]))
    return obj


def _static_wind(obj):
    attr = obj.data.color_attributes.get("Wind") or obj.data.attributes.get("Wind")
    if attr:
        for datum in attr.data:
            datum.color = (0.0, 0.0, 0.0, 1.0)
        obj.data.color_attributes.active_color = attr
    return obj


def produce(kit, atlas, material):
    """Separate ripe-size potato produce cluster; pivot is the soil line."""
    m2 = material.copy()
    m2.name = "M_CropPotatoProduce"
    b = F.Batch(height=0.14)
    tubers = (
        (Vector((-0.030, -0.008, -0.020)), 0.034, Vector((1.00, 0.20, 0.20))),
        (Vector((0.026, 0.010, -0.023)), 0.030, Vector((0.82, -0.55, 0.25))),
        (Vector((0.000, 0.031, -0.026)), 0.026, Vector((0.25, 0.96, 0.20))),
    )
    for i, (center, radius, axis) in enumerate(tubers):
        b.sphere(center, radius, atlas.uv("tuber", inset=False), segs=12, rings=6,
                 stretch=1.32, axis=axis.normalized(), phase=0.07 * i)
    rng = random.Random(SEED + 1200)
    for i in range(8):
        az = rng.uniform(0, math.tau)
        dist = rng.uniform(0.018, 0.061)
        loc = Vector((math.cos(az) * dist, math.sin(az) * dist * 0.72, rng.uniform(-0.002, 0.012)))
        b.sphere(loc, rng.uniform(0.004, 0.008), atlas.uv("soil", inset=False), segs=5, rings=3,
                 stretch=rng.uniform(0.65, 1.15),
                 axis=Vector((rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 1)).normalized(),
                 phase=0.0)
    obj = b.build("SM_CropPotato_Produce", m2)
    obj = kit.join([obj], "SM_CropPotato_Produce", pivot=None, unwrap=False, reshade=True, smooth_angle=65.0)
    _static_wind(obj)
    print("HOMESTEAD_LODS Produce", F.lod_report([obj]))
    return obj


def build(kit):
    atlas = paint_atlas()
    material = atlas.material(translucent=(1.05, 1.10, 0.55))
    out = []
    for stage in STAGES:
        batches = [emit(stage, atlas, lod) for lod in range(3)]
        objs = F.finish_lods(kit, batches, f"SM_{NAME}_{stage}", material, smooth_angle=179.0)
        _restore_plot_origin(objs)
        out.extend(objs)
        print("HOMESTEAD_LODS", stage, F.lod_report(objs))
    out.append(produce(kit, atlas, material))
    out.append(harvest(kit, atlas, material))
    return out
