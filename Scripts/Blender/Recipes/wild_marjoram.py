"""Wild marjoram (Origanum vulgare) in flower: the forageable "meadow herb" clump.

The real plant (what the geometry and textures follow):
- A bushy perennial of dry grassland, hedge banks and field edges on Cornish soils, flowering
  July-September; the fragrant leaves and flowering tops were cut for seasoning, tea and posies.
- Clumps of 15-30 upright, square, downy stems 30-60 cm tall, reddish-purple low down and on
  the sunny side, branching only near the top.
- Leaves opposite and decussate on short stalks, broadly ovate, 1-4 cm long, entire or faintly
  toothed, dark green with a soft down and pale undersides; upper leaves smaller and often
  flushed purple.
- Flowers in dense rounded heads 1-2.5 cm across, gathered into a loose flat-topped panicle:
  small two-lipped pale pink to rosy-mauve corollas (6-8 mm) poking out of deep red-purple
  bracts and calyces, so the heads read rosy-purple from a distance and darker up close.
- A low mat of overwintering leafy shoots sits around the base.

Game notes: walk-through (no collision). About 50 cm tall and 45 cm across, so a herb patch reads
above pasture grass from across the field; the rosy-purple tops are the visual cue. One 2K atlas
and material (alpha-masked, two-sided). Wind vertex colours per homestead_foliage.py.
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "WildMarjoram"
DESCRIPTION = ("Wild marjoram in flower, 50 cm tall and 45 cm across: square downy red-purple stems, "
               "opposite ovate leaves, and loose flat-topped panicles of dense rosy-mauve flower heads "
               "in dark purple bracts, over a low leafy mat. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 18000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.03, -0.02, 0.44)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-stem random phase", "B": "flutter 0 at leaf base -> 1 at leaf tip",
             "A": "1"},
    "material_notes": ("One material M_WildMarjoram: T_WildMarjoram_basecolor (sRGB, alpha = opacity mask, "
                       "clip 0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G "
                       "translucency mask, B AO). Two-sided foliage, masked."),
}

SEED = 7703
HEIGHT = 0.56
LEAVES = ("leaf", "leaf2", "leaf_purple", "leaf_old")
HEADS = ("head_skin", "head_skin2", "head_skin_bud")


def _paint_leaf(atlas, key, nrng, rng, pal, **extra):
    X, Y, px = atlas.grid(key)
    a = X.max()
    w = a * 0.9
    shape = F.ovate(width=w, widest=0.36, tip_sharp=0.9, base_round=0.75, base=0.12, tip=0.98,
                    teeth=5, tooth_depth=0.012)
    veins = F.pinnate_veins(count=4, angle=0.75, curve=0.6, reach=0.8, base=0.12, tip=0.98, width=w,
                            widest=0.36, start=0.16, stop=0.72, rng=nrng)
    layer = F.paint_blade(X, Y, nrng, shape, veins, pal, px, vein_width=w * 0.045, vein_depth=0.00008,
                          puff=0.00008, tertiary=0.25, hair=0.4, stalk=a * 0.07, trans=0.45, **extra)
    atlas.put(key, layer, meters_per_px=0.035 / X.shape[0])


def _paint_head(atlas, key, nrng, rng, bud, mode="side"):
    """A dense rounded flower head: from the side (a dome on its calyx base), from above (a disc),
    or as the "skin" wrapped round a head's solid core (u around, v from stalk to crown): florets
    poking from dark bracts."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    skin = mode == "skin"
    # Overlapping purple bracts and calyces, a little ragged at the rim.
    n = F.noise(X.shape, nrng, freq=26.0, beta=1.7)
    if mode == "top":
        r = np.hypot(X, Y - 0.5) / 0.46
    elif skin:
        r = np.zeros(X.shape)
    else:
        r = np.hypot(X / 0.44, (Y - 0.46) / np.where(Y > 0.46, 0.46, 0.34))
    dome = np.clip((1.0 - r + 0.08 * (n - 0.5)) / (px * 2.5) + 0.5, 0, 1)
    body = F.Layer(X.shape)
    body.color = F.lerp((0.075, 0.018, 0.035), (0.20, 0.045, 0.085), n)
    if mode != "top":
        body.color = F.lerp(body.color, (0.05, 0.06, 0.025), (1.0 - F.smoothstep(0.05, 0.25, Y)) * 0.6)
    body.alpha = dome
    body.height = 0.0003 * n
    body.rough[...] = 0.62
    body.trans[...] = 0.25
    layer.over(body)
    # Bract scales: small pointed ovals shingled over the dome.
    bract = F.ovate(width=0.36, widest=0.45, tip_sharp=1.1, base_round=0.7, base=0.0, tip=1.0)
    bpal = dict(base=(0.16, 0.035, 0.07), tip=(0.24, 0.055, 0.10), vein=(0.10, 0.02, 0.05),
                margin=(0.09, 0.02, 0.045))
    def inside(c):
        if skin:
            return 0.0
        if mode == "top":
            return math.hypot(c[0], c[1] - 0.5) / 0.44
        return math.hypot(c[0] / 0.42, (c[1] - 0.46) / (0.44 if c[1] > 0.46 else 0.30))

    def outward(c):
        if skin:
            return rng.uniform(-0.6, 0.6)
        return math.atan2(c[0], c[1] - 0.5) if mode == "top" else math.atan2(c[0], max(c[1] - 0.22, 0.05))

    xr = float(X.max()) - 0.02 if skin else 0.40
    for _ in range(220 if skin else 70):
        c = (rng.uniform(-xr, xr), rng.uniform(0.10, 0.90))
        if inside(c) > 0.92:
            continue
        s = rng.uniform(0.10, 0.15)
        th = outward(c) + rng.uniform(-0.4, 0.4)
        S.over_window(layer, X, Y, c, s, lambda Xs, Ys, c=c, th=th, s=s: F.paint_blade(
            *S.rotated(Xs, Ys, (c[0] - 0.5 * s * math.sin(th), c[1] - 0.5 * s * math.cos(th)), th, s), nrng, bract,
            [([(0, 0), (0, 1)], 1.0)], bpal, px / s, vein_width=0.04, vein_depth=0.00005, puff=0.00005,
            tertiary=0.0, hair=0.3, trans=0.3))
    if bud:
        atlas.put(key, layer, meters_per_px=0.02 / X.shape[0])
        return
    # Florets: two-lipped tubular corollas, pale pink to rosy mauve, facing out of the dome.
    lobe = F.ovate(width=0.42, widest=0.55, tip_sharp=0.35, base_round=1.0, base=0.0, tip=1.0)
    tones = ((0.44, 0.15, 0.30), (0.52, 0.22, 0.38), (0.34, 0.10, 0.24), (0.56, 0.32, 0.46))
    placed = []
    xr = float(X.max()) - 0.02 if skin else 0.40
    for _ in range(3000 if skin else 1500):
        c = (rng.uniform(-xr, xr), rng.uniform(0.14 if skin else 0.10 if mode == "top" else 0.30, 0.98 if skin else 0.92))
        if inside(c) > 0.95:
            continue
        if any(math.hypot(c[0] - p[0], c[1] - p[1]) < 0.095 for p in placed):
            continue
        placed.append(c)
        col = rng.choices(tones, (40, 30, 18, 12))[0]
        pal = dict(base=col, tip=tuple(min(1.0, v * 1.12) for v in col), vein=tuple(v * 0.8 for v in col),
                   margin=tuple(v * 1.05 for v in col))
        out = outward(c) + rng.uniform(-0.5, 0.5)
        s = rng.uniform(0.065, 0.09)
        # Upper lip (one broad lobe) and the three-lobed lower lip.
        for k, (dth, ln) in enumerate(((0.0, 1.0), (-1.1, 0.8), (1.1, 0.8), (math.pi, 0.9))):
            th = out + dth + rng.uniform(-0.2, 0.2)
            ll = s * ln
            S.over_window(layer, X, Y, (c[0] + 0.5 * ll * math.sin(th), c[1] + 0.5 * ll * math.cos(th)), ll * 0.8,
                          lambda Xs, Ys, th=th, ll=ll, c=c, pal=pal: F.paint_blade(
                              *S.rotated(Xs, Ys, c, th, ll), nrng, lobe, [], pal, px / ll, tertiary=0.0,
                              puff=0.00006, trans=0.7))
        rr = np.hypot(X - c[0], Y - c[1])
        throat = np.clip((s * 0.28 - rr) / px + 0.5, 0, 1)
        layer.color = F.lerp(layer.color, (0.20, 0.05, 0.10), throat * 0.8)
        layer.height = layer.height + 0.0002 * throat
    atlas.put(key, layer, meters_per_px=0.02 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=2048, seed=SEED)
    atlas.column("stem", 48)
    for key in LEAVES:
        atlas.tile(key, 300, 460)
    for key in HEADS:
        atlas.tile(key, 640, 320)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    green = dict(base=(0.040, 0.064, 0.022), tip=(0.046, 0.070, 0.026), vein=(0.058, 0.084, 0.034),
                 margin=(0.034, 0.052, 0.020), stalk=(0.11, 0.045, 0.040), brown=(0.09, 0.05, 0.025))
    _paint_leaf(atlas, "leaf", nrng, rng, green)
    _paint_leaf(atlas, "leaf2", nrng, rng, {k: tuple(c * f for c, f in zip(v, (1.12, 1.1, 0.95)))
                                             for k, v in green.items()}, damage=0.25)
    _paint_leaf(atlas, "leaf_purple", nrng, rng, dict(green, base=(0.050, 0.045, 0.026), tip=(0.085, 0.035, 0.045),
                                                     margin=(0.075, 0.025, 0.035)))
    _paint_leaf(atlas, "leaf_old", nrng, rng, dict(green, base=(0.055, 0.060, 0.022), tip=(0.075, 0.062, 0.022)),
                yellow=0.35, edge_burn=0.4)
    _paint_head(atlas, "head_skin", nrng, rng, False, mode="skin")
    _paint_head(atlas, "head_skin2", nrng, rng, False, mode="skin")
    _paint_head(atlas, "head_skin_bud", nrng, rng, True, mode="skin")
    U, V = atlas.column_grid("stem")
    st = F.noise(U.shape, nrng, freq=60.0, beta=1.4, aniso=(1.0, 8.0))
    layer = F.Layer(U.shape)
    # Red-purple low down, greener toward the top, with a downy sheen.
    layer.color = F.lerp((0.12, 0.035, 0.040), (0.055, 0.070, 0.028), F.smoothstep(0.25, 0.95, V))
    layer.color = F.lerp(layer.color, (0.20, 0.18, 0.15), F.smoothstep(0.75, 0.98, st) * 0.25)
    layer.height = 0.00006 * st
    layer.rough = 0.62 + 0.08 * st
    layer.trans[...] = 0.25
    atlas.put("stem", layer, meters_per_px=0.3 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def _stalk(base, d, length, bend, n, side_hint=None):
    side = d.cross(Vector((0, 0, 1)))
    side = side.normalized() if side.length > 1e-4 else Vector((1, 0, 0))
    pts = [base.copy()]
    for i in range(n):
        d = (Matrix.Rotation(-bend / n, 3, side) @ d).normalized()
        pts.append(pts[-1] + d * (length / n))
    return pts


def describe(rng):
    stems = []
    for i in range(26):
        az = rng.uniform(0, math.tau)
        r = 0.075 * rng.random() ** 0.5
        flowering = i < 18
        lean_amt = rng.uniform(0.06, 0.28) if flowering else rng.uniform(0.3, 0.6)
        lean = Vector((math.cos(az), math.sin(az), 0)) * lean_amt
        length = rng.uniform(0.34, 0.50) if flowering else rng.uniform(0.10, 0.18)
        spin = rng.uniform(0, math.pi)
        nodes = []
        count = rng.randint(9, 11) if flowering else rng.randint(6, 7)
        top = 0.68 if flowering else 0.95
        for k in range(count):
            t = 0.06 + (top - 0.06) * (k + rng.uniform(-0.12, 0.12)) / count
            size = (0.034 - 0.012 * t) * rng.uniform(0.85, 1.15)
            key = rng.choices(LEAVES, [36, 30, 8 + 30 * t, 26 * (1 - t)])[0]
            # Leafy side shoots in the lower axils bulk the clump out, as on the real plant.
            axil = flowering and t < 0.5 and rng.random() < 0.6
            nodes.append(dict(t=t, size=size, key=key, keep=rng.random(), elev=math.radians(rng.uniform(30, 55)),
                              droop=rng.uniform(0.2, 0.55), axil=axil))
        if flowering:
            # Small, purple-flushed leaves carry on up into the panicle, one pair under each branch pair.
            for t in (0.70, 0.79, 0.88, 0.95):
                nodes.append(dict(t=t, size=rng.uniform(0.011, 0.017), key=rng.choice(("leaf_purple", "leaf2")),
                                  keep=rng.random(), elev=math.radians(rng.uniform(25, 45)),
                                  droop=rng.uniform(0.1, 0.3), axil=False))
        branches = []
        if flowering:
            # A loose, flat-topped panicle: opposite branch pairs from the top leaf axils, longer
            # lower down, so their heads come level with the terminal cluster.
            for level, t in enumerate((0.70, 0.79, 0.88)):
                for side in (0, 1):
                    if rng.random() < 0.2:
                        continue
                    up = math.radians(rng.uniform(38, 58))
                    reach = (1.0 - t) * length / math.sin(up) * rng.uniform(0.85, 1.1)
                    branches.append(dict(t=t, az=spin + level * 1.5708 + side * math.pi + rng.uniform(-0.3, 0.3),
                                         length=min(reach, 0.12), up=up, heads=rng.randint(2, 3),
                                         keep=rng.random(), seed=rng.randrange(1 << 20)))
        stems.append(dict(base=Vector((math.cos(az) * r, math.sin(az) * r, 0.0)),
                          dir=(Vector((0, 0, 1)) + lean).normalized(), length=length,
                          bend=rng.uniform(0.1, 0.3), nodes=nodes,
                          branches=branches, flowering=flowering, phase=rng.random(), spin=spin,
                          heads=rng.randint(3, 4), seed=rng.randrange(1 << 20)))
    mat = []
    for i in range(30):
        az = i * 2.39996 + rng.uniform(-0.3, 0.3)
        mat.append(dict(r=rng.uniform(0.02, 0.13), heading=Vector((math.cos(az), math.sin(az), 0)),
                        elev=math.radians(rng.uniform(8, 28)), size=rng.uniform(0.024, 0.036),
                        key=rng.choices(LEAVES, [40, 30, 5, 25])[0], droop=rng.uniform(0.1, 0.4), phase=rng.random(),
                        keep=rng.random()))
    return dict(stems=stems, mat=mat)


def _head(b, atlas, lod, top, tan, size, key, spin, phase):
    """Rounded flower head: a solid knobbly core wrapped in florets and bracts."""
    b.sphere(top + tan * size * 0.18, size * 0.38, atlas.uv(key), segs=(6, 5, 4)[lod],
             rings=(4, 3, 3)[lod], stretch=0.85, axis=tan, phase=phase)


def emit(desc, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"stems": 0, "leaves": 0, "heads": 0}
    for st in desc["stems"]:
        if lod == 2 and not st["flowering"]:
            continue
        rng = random.Random(st["seed"])
        n = (7, 4, 2)[lod]
        pts = _stalk(st["base"], st["dir"], st["length"], st["bend"], n)
        t0 = b.triangles
        b.tube(pts, [0.0024 * (1 - 0.55 * i / n) for i in range(n + 1)], (4, 4, 3)[lod], atlas.uv("stem"),
               v_length=0.5, phase=st["phase"], flutter=0.15)
        stats["stems"] += b.triangles - t0
        acc = S.arclength(pts)
        t0 = b.triangles
        for k, nd in enumerate(st["nodes"]):
            if lod == 1 and nd["keep"] > 0.65 or lod == 2 and nd["keep"] > 0.3:
                continue
            node, tan, _ = S.at(pts, acc, acc[-1] * nd["t"])
            side = Matrix.Rotation(st["spin"] + k * 1.5708, 3, tan) @ tan.orthogonal().normalized()
            for s in (1, -1) if lod < 2 else (1,):
                hd = (side * s).normalized()
                d = (hd * math.cos(nd["elev"]) + tan * math.sin(nd["elev"])).normalized()
                up = (tan - hd * 0.4).normalized()
                ln = nd["size"] * (1.0, 1.1, 1.3)[lod]
                b.card(node, d, up, ln, ln * S.tile_aspect(atlas, nd["key"]), atlas.uv(nd["key"]),
                       rows=(3, 2, 1)[lod], cols=1, fold=0.15 if lod == 0 else 0.0, droop=nd["droop"],
                       phase=st["phase"], flutter=1.0, flutter_base=0.1)
                if nd["axil"] and lod == 0:
                    # A short leafy side shoot: a smaller pair turned a quarter from the main pair.
                    ad = (Matrix.Rotation(0.785 * s, 3, tan) @ hd * math.cos(nd["elev"] * 0.7)
                          + tan * math.sin(nd["elev"] * 0.7 + 0.35)).normalized()
                    al = ln * 0.6
                    b.card(node + tan * 0.004, ad, (tan - hd * 0.3).normalized(), al,
                           al * S.tile_aspect(atlas, "leaf2"), atlas.uv("leaf2"), rows=2, cols=1, fold=0.1,
                           droop=nd["droop"] * 0.7, phase=st["phase"], flutter=1.0, flutter_base=0.1)
        stats["leaves"] += b.triangles - t0
        if not st["flowering"]:
            continue
        t0 = b.triangles
        top, tan, _ = S.at(pts, acc, acc[-1])
        # Terminal cluster: a few heads crowded on short stalks at the stem tip.
        for h in range(st["heads"] if lod < 2 else 1):
            az = st["spin"] + h * 2.4
            off = Vector((math.cos(az), math.sin(az), 0)) * (0.0 if h == 0 else 0.012)
            size = rng.uniform(0.020, 0.028) * (1.0, 1.1, 1.35)[lod]
            key = rng.choices(HEADS, [50, 44, 6])[0]
            _head(b, atlas, lod, top + off + tan * (0.004 if h == 0 else -0.004), tan, size, key, az, st["phase"])
        # Side branchlets from the upper leaf axils, each ending in one to three heads.
        for br in st["branches"]:
            if lod == 1 and br["keep"] > 0.7 or lod == 2 and br["keep"] > 0.35:
                continue
            brng = random.Random(br["seed"])
            node, stan, _ = S.at(pts, acc, acc[-1] * br["t"])
            hd = Vector((math.cos(br["az"]), math.sin(br["az"]), 0))
            d = (hd * math.cos(br["up"]) + stan * math.sin(br["up"])).normalized()
            bpts = _stalk(node, d, br["length"], -0.4, (3, 2, 1)[lod])
            b.tube(bpts, [0.0012, 0.0009] + [0.0008] * (len(bpts) - 2), 3, atlas.uv("stem"), v_length=0.5,
                   phase=st["phase"], flutter=0.3)
            btop = bpts[-1]
            btan = (bpts[-1] - bpts[-2]).normalized()
            for h in range(br["heads"] if lod == 0 else 1):
                az = br["az"] + h * 2.1
                off = Vector((math.cos(az), math.sin(az), 0)) * (0.0 if h == 0 else 0.010)
                size = brng.uniform(0.015, 0.022) * (1.0, 1.15, 1.4)[lod]
                key = brng.choices(HEADS, [48, 44, 8])[0]
                _head(b, atlas, lod, btop + off, btan, size, key, az, st["phase"])
        stats["heads"] += b.triangles - t0
    t0 = b.triangles
    for lf in desc["mat"]:
        if lod == 1 and lf["keep"] > 0.6 or lod == 2:
            continue
        base = lf["heading"] * lf["r"]
        d = (lf["heading"] * math.cos(lf["elev"]) + Vector((0, 0, math.sin(lf["elev"])))).normalized()
        up = (Vector((0, 0, 1)) - lf["heading"] * 0.3).normalized()
        b.card(base, d, up, lf["size"], lf["size"] * S.tile_aspect(atlas, lf["key"]), atlas.uv(lf["key"]),
               rows=2 if lod == 0 else 1, cols=1, fold=0.1, droop=lf["droop"], phase=lf["phase"], flutter=0.8,
               flutter_base=0.1)
    stats["leaves"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    desc = describe(random.Random(SEED))
    batches = [emit(desc, atlas, lod, HEIGHT) for lod in range(3)]
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
