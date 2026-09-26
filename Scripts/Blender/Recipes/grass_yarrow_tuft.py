"""Bunchgrass tuft with yarrow: an ankle-high woodland-floor clump of grass blades and forbs.

The real plants (what the geometry and textures follow):
- Blue wildrye (Elymus glaucus) and similar cool-season bunchgrasses carpet the open oak and pine
  floors of the Sierra Nevada foothills around Yosemite: tufts of flat, finely ribbed blades
  15-35 cm long, 4-8 mm wide, arching outward, glaucous green; by late summer many blades cure to
  straw with browned tips, and slender culms carry narrow spikes of awned spikelets.
- Common yarrow (Achillea millefolium) grows among them: rosettes of soft, grey-green, finely
  dissected feathery leaves 5-15 cm long, and stiff hairy stems 20-40 cm tall topped by flat,
  dense corymbs 4-8 cm across of tiny white five-rayed flower heads with cream centres, some
  fading to buff.

Game notes: walk-through (no collision). One 2K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G per-blade/stem phase, B blade flutter).
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "GrassYarrowTuft"
DESCRIPTION = ("Bunchgrass and yarrow tuft, 35 cm tall and 60 cm across: arching ribbed grass blades, part "
               "cured to straw, awned seed spikes, feathery yarrow rosettes and flat white flower heads. "
               "Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 3000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.11, -0.07, 0.13)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-blade / per-stem random phase", "B": "flutter 0 at blade base -> 1 at blade tip",
             "A": "1"},
    "material_notes": ("One material M_GrassYarrowTuft: T_GrassYarrowTuft_basecolor (sRGB, alpha = opacity mask, "
                       "clip 0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G "
                       "translucency mask, B AO). Two-sided foliage, masked."),
}

SEED = 5531
BLADES = ("blade", "blade2", "blade_tip", "blade_straw")


def _paint_blade_tile(atlas, key, nrng, rng):
    X, Y, px = atlas.grid(key)
    a = X.max()
    shape = F.ovate(width=a * 0.32, widest=0.55, tip_sharp=1.25, base_round=0.25, base=0.0, tip=1.0)
    hw = a * 0.32
    veins = [([(0.0, 0.0), (0.0, 0.99)], 1.0)] + [([(s * hw * f, 0.0), (s * hw * f * 0.9, 0.55), (s * hw * f * 0.05, 0.97)], 0.45)
                                                 for f in (0.35, 0.68) for s in (-1, 1)]
    green = dict(base=(0.058, 0.080, 0.030), tip=(0.066, 0.086, 0.034), vein=(0.080, 0.104, 0.044),
                 margin=(0.054, 0.072, 0.028), yellow=(0.24, 0.20, 0.08), brown=(0.10, 0.07, 0.035))
    if key == "blade2":
        green = {k: tuple(c * f for c, f in zip(v, (1.1, 1.12, 0.95))) for k, v in green.items()}
    layer = F.paint_blade(X, Y, nrng, shape, veins, green, px, vein_width=hw * 0.09, vein_depth=0.00005,
                          puff=0.00004, tertiary=0.0, gloss=0.1, trans=0.6)
    straw = np.asarray((0.30, 0.25, 0.12))
    n = F.noise(X.shape, nrng, freq=30.0, beta=1.6, aniso=(1.0, 8.0))
    if key in ("blade_tip", "blade_straw"):
        m = F.smoothstep(0.45, 0.85, Y + 0.1 * (n - 0.5)) if key == "blade_tip" else np.ones(X.shape)
        scol = straw * (0.75 + 0.45 * n)[..., None]
        scol = F.lerp(scol, (0.16, 0.11, 0.05), F.smoothstep(0.93, 1.0, Y) * 0.8)
        layer.color = F.lerp(layer.color, scol, m)
        layer.trans = layer.trans * (1 - 0.6 * m)
        layer.rough = layer.rough + 0.15 * m
    atlas.put(key, layer, meters_per_px=0.3 / X.shape[0])


def _paint_spike(atlas, key, nrng, rng, cured):
    """Narrow spike of overlapping spikelets with long awns (Elymus)."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    col = (0.26, 0.22, 0.11) if cured else (0.085, 0.10, 0.045)
    pal = dict(base=col, tip=tuple(c * 1.15 for c in col), vein=tuple(c * 1.3 for c in col),
               margin=tuple(c * 0.85 for c in col), brown=(0.10, 0.07, 0.035))
    axis = [(0.0, 0.0), (0.0, 1.0)]
    d, along = F.polyline_distance(X, Y, axis)
    rach = F.Layer(X.shape)
    rach.color[...] = pal["margin"]
    rach.alpha = np.clip(-(d - 0.006) / px + 0.5, 0, 1)
    layer.over(rach)
    glume = F.ovate(width=0.22, widest=0.35, tip_sharp=1.6, base=0.0, tip=1.0)
    awn = F.ovate(width=0.02, widest=0.05, tip_sharp=0.6, base=0.0, tip=1.0)
    for k in range(20):
        y = 0.18 + 0.66 * k / 19
        for side in (-1, 1):
            th = side * math.radians(rng.uniform(12, 22))
            o = (side * 0.012, y)
            ln = 0.075 * rng.uniform(0.9, 1.1)
            S.over_window(layer, X, Y, (o[0] + ln * 0.5 * math.sin(th), o[1] + ln * 0.5), ln * 0.7,
                          lambda Xs, Ys, o=o, th=th, ln=ln: F.paint_blade(
                              *S.rotated(Xs, Ys, o, th, ln), nrng, glume, [([(0, 0), (0, 1)], 1.0)], pal, px / ln,
                              vein_width=0.05, vein_depth=0.0001, puff=0.0001, tertiary=0.0, trans=0.4))
            al = 0.16 * rng.uniform(0.8, 1.1)
            ao = (o[0] + ln * math.sin(th), o[1] + ln * math.cos(th))
            S.over_window(layer, X, Y, (ao[0] + al * 0.5 * math.sin(th * 0.6), ao[1] + al * 0.5), al * 0.6,
                          lambda Xs, Ys, ao=ao, th=th, al=al: F.paint_blade(
                              *S.rotated(Xs, Ys, ao, th * 0.6, al), nrng, awn, [], pal, px / al,
                              tertiary=0.0, trans=0.3))
    atlas.put(key, layer, meters_per_px=0.12 / X.shape[0])


def _paint_yarrow_leaf(atlas, key, nrng, rng, pal):
    """Feathery bipinnatifid leaf: a rachis with many short segments, each split into fine lobes."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    layer.color[...] = pal["margin"]
    d, along = F.polyline_distance(X, Y, [(0.0, 0.0), (0.0, 0.97)])
    rach = F.Layer(X.shape)
    rach.color[...] = pal["vein"]
    rach.alpha = np.clip(-(d - 0.006 * (1 - 0.6 * along)) / px + 0.5, 0, 1)
    layer.over(rach)
    lobe = F.ovate(width=0.075, widest=0.35, tip_sharp=0.9, base=0.0, tip=1.0)
    a = X.max()
    for k in range(24):
        t = (k + 0.5) / 24
        y = 0.04 + 0.92 * t
        seg = a * 0.9 * math.sin(math.pi * min(0.15 + t * 0.95, 1.0)) ** 0.7
        for side in (-1, 1):
            base_th = side * math.radians(rng.uniform(55, 75))
            o = (side * 0.004, y)
            for j, (f, dth) in enumerate(((0.0, 0.0), (0.35, 0.55), (0.6, -0.5), (0.35, -0.5), (0.6, 0.5))):
                ln = seg * (1.0 if j == 0 else 0.4) * rng.uniform(0.85, 1.05)
                oo = (o[0] + seg * f * math.sin(base_th), o[1] + seg * f * math.cos(base_th))
                th = base_th + side * dth
                S.over_window(layer, X, Y, (oo[0] + ln * 0.5 * math.sin(th), oo[1] + ln * 0.5 * math.cos(th)),
                              ln * 0.65, lambda Xs, Ys, oo=oo, th=th, ln=ln: F.paint_blade(
                                  *S.rotated(Xs, Ys, oo, th, ln), nrng, lobe, [([(0, 0), (0, 1)], 1.0)], pal,
                                  px / ln, vein_width=0.05, vein_depth=0.00005, puff=0.00008, tertiary=0.0,
                                  hair=0.45, trans=0.5))
    atlas.put(key, layer, meters_per_px=0.12 / X.shape[0])


def _paint_corymb(atlas, key, nrng, rng, fade):
    """Flat-topped yarrow corymb from above: many tiny five-rayed heads packed in a dome."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    white = (0.62, 0.61, 0.54) if not fade else (0.42, 0.38, 0.28)
    petal = F.ovate(width=0.55, widest=0.5, tip_sharp=0.4, base_round=1.0, base=0.0, tip=1.0,
                    teeth=3, tooth_depth=0.15)
    pal = dict(base=white, tip=tuple(c * 1.05 for c in white), vein=tuple(c * 0.92 for c in white),
               margin=tuple(c * 1.02 for c in white), brown=(0.16, 0.12, 0.07))
    # Dark bracts/stalk gaps between heads.
    r0 = np.hypot(X, Y - 0.5)
    base = F.Layer(X.shape)
    n = F.noise(X.shape, nrng, freq=40.0, beta=1.6)
    base.color = F.lerp((0.05, 0.06, 0.03), (0.09, 0.09, 0.05), n)
    base.alpha = np.clip((0.40 - r0) / px + 0.5, 0, 1) * F.smoothstep(0.35, 0.55, n + 0.25 * (1 - r0 / 0.4))
    layer.over(base)
    heads = []
    for _ in range(400):
        c = (rng.uniform(-0.42, 0.42), rng.uniform(0.08, 0.92))
        if math.hypot(c[0], c[1] - 0.5) > 0.42:
            continue
        if all(math.hypot(c[0] - h[0], c[1] - h[1]) > 0.058 for h in heads):
            heads.append(c)
    for c in heads:
        s = rng.uniform(0.028, 0.036)
        for k in range(5):
            th = math.radians(72 * k + rng.uniform(-10, 10))
            S.over_window(layer, X, Y, (c[0] + 0.5 * s * math.sin(th), c[1] + 0.5 * s * math.cos(th)), s * 0.8,
                          lambda Xs, Ys, th=th, s=s, c=c: F.paint_blade(
                              *S.rotated(Xs, Ys, c, th, s), nrng, petal, [], pal, px / s, tertiary=0.0,
                              puff=0.00005, damage=0.4 if fade else 0.0, trans=0.8))
        rr = np.hypot(X - c[0], Y - c[1])
        disc = np.clip((s * 0.35 - rr) / px + 0.5, 0, 1)
        dc = (0.40, 0.34, 0.16) if not fade else (0.20, 0.14, 0.07)
        layer.color = F.lerp(layer.color, dc, disc)
        layer.alpha = np.maximum(layer.alpha, disc)
        layer.height = layer.height + 0.00015 * disc
    atlas.put(key, layer, meters_per_px=0.065 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    for key in BLADES:
        atlas.tile(key, 110, 1400)
    atlas.tile("spike", 200, 900)
    atlas.tile("spike_straw", 200, 900)
    atlas.tile("yarrow", 300, 1100)
    atlas.tile("yarrow2", 300, 1100)
    atlas.tile("yarrow_dry", 300, 1100)
    atlas.tile("corymb", 500, 500)
    atlas.tile("corymb_fade", 500, 500)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    for key in BLADES:
        _paint_blade_tile(atlas, key, nrng, rng)
    _paint_spike(atlas, "spike", nrng, rng, False)
    _paint_spike(atlas, "spike_straw", nrng, rng, True)
    ypal = dict(base=(0.058, 0.080, 0.034), tip=(0.062, 0.084, 0.036), vein=(0.07, 0.09, 0.04),
                margin=(0.05, 0.07, 0.03), brown=(0.08, 0.05, 0.025))
    _paint_yarrow_leaf(atlas, "yarrow", nrng, rng, ypal)
    _paint_yarrow_leaf(atlas, "yarrow2", nrng, rng, {k: tuple(c * 1.15 for c in v) for k, v in ypal.items()})
    _paint_yarrow_leaf(atlas, "yarrow_dry", nrng, rng, dict(base=(0.14, 0.10, 0.05), tip=(0.12, 0.08, 0.04),
                                                           vein=(0.15, 0.11, 0.06), margin=(0.10, 0.07, 0.035)))
    _paint_corymb(atlas, "corymb", nrng, rng, False)
    _paint_corymb(atlas, "corymb_fade", nrng, rng, True)
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp((0.07, 0.085, 0.035), (0.20, 0.17, 0.08), F.smoothstep(0.4, 0.9, st) * 0.5)
    layer.height = 0.00008 * st
    layer.rough = 0.55 + 0.1 * st
    layer.trans[...] = 0.3
    atlas.put("stem", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    blades, culms, rosettes, stems = [], [], [], []
    for tuft, (cx, cy, count, scale) in enumerate(((0.0, 0.0, 120, 1.0), (-0.17, 0.12, 50, 0.75))):
        for i in range(count):
            az = rng.uniform(0, math.tau)
            r = 0.035 * scale * rng.random() ** 0.5
            base = Vector((cx + math.cos(az) * r, cy + math.sin(az) * r, -0.005))
            heading = Vector((math.cos(az + rng.uniform(-0.5, 0.5)), math.sin(az + rng.uniform(-0.5, 0.5)), 0))
            fallen = rng.random() < 0.08
            elev = math.radians(rng.uniform(4, 14) if fallen else rng.uniform(55, 86))
            key = rng.choices(BLADES, [34, 26, 24, 16])[0]
            if fallen:
                key = "blade_straw"
            blades.append(dict(base=base, heading=heading, elev=elev, length=scale * rng.uniform(0.18, 0.34),
                               droop=rng.uniform(0.8, 1.9) * (0.3 if fallen else 1.0), twist=rng.uniform(-1.2, 1.2),
                               key=key, phase=rng.random(), keep=rng.random(), roll=rng.uniform(-0.7, 0.7)))
        for i in range(5 if tuft == 0 else 2):
            az = rng.uniform(0, math.tau)
            lean = Vector((math.cos(az), math.sin(az), 0)) * rng.uniform(0.1, 0.3)
            culms.append(dict(base=Vector((cx, cy, 0)) + lean * 0.05, dir=(Vector((0, 0, 1)) + lean).normalized(),
                              length=scale * rng.uniform(0.25, 0.34), nod=rng.uniform(0.2, 0.6),
                              key=rng.choice(["spike", "spike_straw", "spike_straw"]), phase=rng.random()))
    for cx, cy, n_leaves, n_stems in ((0.14, -0.08, 9, 2), (-0.02, 0.2, 7, 1)):
        base = Vector((cx, cy, 0))
        leaves = []
        for i in range(n_leaves):
            az = i * 2.39996 + rng.uniform(-0.3, 0.3)
            leaves.append(dict(heading=Vector((math.cos(az), math.sin(az), 0)), elev=math.radians(rng.uniform(25, 60)),
                               length=rng.uniform(0.08, 0.14), key=rng.choices(["yarrow", "yarrow2", "yarrow_dry"],
                                                                                 [45, 40, 15])[0],
                               droop=rng.uniform(0.2, 0.5), phase=rng.random()))
        rosettes.append(dict(base=base, leaves=leaves))
        for j in range(n_stems):
            az = rng.uniform(0, math.tau)
            lean = Vector((math.cos(az), math.sin(az), 0)) * rng.uniform(0.05, 0.2)
            stems.append(dict(base=base + lean * 0.05, dir=(Vector((0, 0, 1)) + lean).normalized(),
                              length=rng.uniform(0.22, 0.30), head=rng.uniform(0.05, 0.07),
                              key="corymb" if rng.random() < 0.7 else "corymb_fade", phase=rng.random(),
                              spin=rng.uniform(0, 6.28), seed=rng.randrange(1 << 20)))
    return dict(blades=blades, culms=culms, rosettes=rosettes, stems=stems)


def _stalk(base, d, length, bend, n):
    side = d.cross(Vector((0, 0, 1)))
    side = side.normalized() if side.length > 1e-4 else Vector((1, 0, 0))
    pts = [base.copy()]
    for i in range(n):
        d = (Matrix.Rotation(-bend / n, 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"blades": 0, "culms": 0, "yarrow": 0}
    t0 = b.triangles
    for bl in desc["blades"]:
        if lod == 1 and bl["keep"] > 0.7:
            continue
        if lod == 2 and bl["keep"] > 0.4:
            continue
        d = (bl["heading"] * math.cos(bl["elev"]) + Vector((0, 0, math.sin(bl["elev"])))).normalized()
        up = (Vector((0, 0, 1)) - bl["heading"] * 1.2).normalized()
        up = Matrix.Rotation(bl["roll"], 3, d) @ up      # arch partly sideways so blades curve in plan
        widen = (1.0, 1.1, 1.35)[lod]
        b.card(bl["base"], d, up, bl["length"], bl["length"] * S.tile_aspect(atlas, bl["key"]) * widen,
               atlas.uv(bl["key"]), rows=(7, 3, 2)[lod], cols=1, fold=0.0, droop=bl["droop"] * (1.0, 0.85, 0.7)[lod],
               twist=bl["twist"] if lod < 2 else 0.0, phase=bl["phase"], flutter=1.0, flutter_base=0.0)
    stats["blades"] = b.triangles - t0
    t0 = b.triangles
    for cu in desc["culms"]:
        n = (4, 2, 1)[lod]
        pts = _stalk(cu["base"], cu["dir"], cu["length"], cu["nod"] * 0.5, n)
        b.tube(pts, [0.0011 * (1 - 0.4 * i / n) for i in range(n + 1)], 3, atlas.uv("stem"), v_length=0.3,
               phase=cu["phase"], flutter=0.3)
        tip, tan = pts[-1], (pts[-1] - pts[-2]).normalized()
        sl = 0.085
        up = tan.orthogonal().normalized()
        for k in range(2 if lod < 2 else 1):
            upk = Matrix.Rotation(1.57 * k, 3, tan) @ up
            b.card(tip - tan * 0.004, tan, upk, sl, sl * S.tile_aspect(atlas, cu["key"]), atlas.uv(cu["key"]),
                   rows=(2, 1, 1)[lod], cols=1, fold=0.0, droop=cu["nod"] * (1.0, 0.5, 0.0)[lod], phase=cu["phase"], flutter=1.0,
                   flutter_base=0.5)
    stats["culms"] = b.triangles - t0
    t0 = b.triangles
    for ro in desc["rosettes"]:
        for i, lf in enumerate(ro["leaves"]):
            if lod == 2 and i % 2:
                continue
            d = (lf["heading"] * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
            up = (Vector((0, 0, 1)) - lf["heading"] * 0.6).normalized()
            b.card(ro["base"], d, up, lf["length"], lf["length"] * S.tile_aspect(atlas, lf["key"]) * (1, 1, 1.3)[lod],
                   atlas.uv(lf["key"]), rows=(3, 1, 1)[lod], cols=1, fold=0.0, droop=lf["droop"] * (1.0, 0.5, 0.5)[lod],
                   phase=lf["phase"], flutter=1.0, flutter_base=0.1)
    for st in desc["stems"]:
        rng = random.Random(st["seed"])
        n = (5, 3, 1)[lod]
        pts = _stalk(st["base"], st["dir"], st["length"], 0.15, n)
        b.tube(pts, [0.0016 * (1 - 0.35 * i / n) for i in range(n + 1)], 3, atlas.uv("stem"), v_length=0.3,
               phase=st["phase"], flutter=0.1)
        top = pts[-1]
        if lod < 2:
            # Small feathery stem leaves.
            acc = S.arclength(pts)
            for k in range(3 if lod == 0 else 2):
                node, tan, _ = S.at(pts, acc, acc[-1] * (0.25 + 0.2 * k))
                az = k * 2.4 + rng.uniform(0, 1)
                hd = Vector((math.cos(az), math.sin(az), 0))
                dd = (hd + Vector((0, 0, 0.9))).normalized()
                ln = 0.05 * (1 - 0.25 * k)
                b.card(node, dd, (Vector((0, 0, 1)) - hd).normalized(), ln, ln * S.tile_aspect(atlas, "yarrow"),
                       atlas.uv("yarrow2"), rows=1, cols=1, droop=0.2, phase=st["phase"], flutter=1.0,
                       flutter_base=0.2)
            # Corymb rays holding the flat head.
            for k in range(5 if lod == 0 else 3):
                az = st["spin"] + k * 1.26
                ray_end = top + Vector((math.cos(az) * st["head"] * 0.3, math.sin(az) * st["head"] * 0.3, 0.016))
                b.tube([top - Vector((0, 0, 0.004)), ray_end], [0.0007, 0.0005], 3, atlas.uv("stem"), v_length=0.3,
                       phase=st["phase"], flutter=0.2)
        b.flat(top + Vector((0, 0, 0.018)), Vector((0, 0, 1)), st["head"], atlas.uv(st["key"]), spin=st["spin"],
               cup=-0.12 if lod < 2 else 0.0, phase=st["phase"], flutter=0.3, segs=(3, 2, 1)[lod])
    stats["yarrow"] = b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    desc = describe(random.Random(SEED))
    batches = [emit(desc, atlas, lod, 0.36) for lod in range(3)]
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
