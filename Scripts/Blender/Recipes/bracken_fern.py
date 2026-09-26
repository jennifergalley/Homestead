"""Western bracken fern (Pteridium aquilinum var. pubescens): a knee-to-waist-high colony clump.

The real plant (what the geometry and textures follow):
- The commonest fern of Sierra Nevada foothill and Yosemite woodland floors and clearings,
  spreading from deep rhizomes into loose colonies; each frond rises on its own.
- Stipe (stalk) erect, 30-100 cm, grooved, straw-green above and dark brown to black near the
  ground; the blade then bends back to lie almost horizontal like a table.
- Blade broadly triangular, 40-100 cm, bipinnate to tripinnate: opposite pinnae shortening toward
  the tip, each with many oblong, blunt pinnules whose lower ones are lobed; margins rolled under,
  mid to yellow-green, slightly hairy beneath. By late summer the oldest fronds yellow and brown.

Game notes: walk-through (no collision). One 4K atlas and material (alpha-masked, two-sided).
Wind vertex colours per homestead_foliage.py (R height, G per-frond phase, B leaf flutter).
"""
import math
import random

import numpy as np
from mathutils import Matrix, Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "BrackenFern"
DESCRIPTION = ("Bracken fern colony clump, 0.6-0.7 m tall and 1.6 m across: erect dark-based stipes carrying "
               "horizontal triangular bipinnate blades, a few yellowing and dead fronds. Walk-through.")
COLLISION = "none"
TRIANGLE_BUDGET = 25000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (0.0, -0.35, 0.6)}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "height above ground / plant height",
             "G": "per-frond random phase", "B": "flutter 0 at the stipe -> 1 at pinna tips",
             "A": "1"},
    "material_notes": ("One material M_BrackenFern: T_BrackenFern_basecolor (sRGB, alpha = opacity mask, clip "
                       "0.5), _normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency "
                       "mask, B AO). Two-sided foliage, masked."),
}

SEED = 8123
PINNAE = ("pinna_a", "pinna_b", "pinna_c", "pinna_yellow", "pinna_dry")


def _palettes():
    brown = (0.07, 0.035, 0.012)
    return {
        "a": dict(base=(0.060, 0.098, 0.022), tip=(0.070, 0.108, 0.024), vein=(0.085, 0.120, 0.034),
                  margin=(0.046, 0.074, 0.018), brown=brown, yellow=(0.20, 0.17, 0.04)),
        "b": dict(base=(0.050, 0.086, 0.019), tip=(0.058, 0.095, 0.021), vein=(0.075, 0.108, 0.030),
                  margin=(0.040, 0.066, 0.016), brown=brown, yellow=(0.20, 0.17, 0.04)),
        "c": dict(base=(0.072, 0.112, 0.026), tip=(0.085, 0.118, 0.028), vein=(0.098, 0.13, 0.04),
                  margin=(0.060, 0.090, 0.022), brown=brown, yellow=(0.20, 0.17, 0.04)),
        "yellow": dict(base=(0.13, 0.13, 0.03), tip=(0.19, 0.15, 0.035), vein=(0.17, 0.15, 0.05),
                       margin=(0.12, 0.09, 0.025), brown=brown, yellow=(0.24, 0.18, 0.04)),
        "dry": dict(base=(0.14, 0.075, 0.03), tip=(0.12, 0.06, 0.025), vein=(0.17, 0.10, 0.045),
                    margin=(0.08, 0.04, 0.018), brown=(0.06, 0.03, 0.012), dry=(0.14, 0.075, 0.03)),
    }


def _pinnule_shape(rng, lobed):
    return F.ovate(width=0.15, widest=0.30, tip_sharp=0.6, base_round=0.9, base=0.0, tip=1.0,
                   teeth=6 if lobed else 0, tooth_depth=0.28 if lobed else 0.0, phase=rng.uniform(0, 1))


def _paint_pinna(atlas, key, nrng, rng, pal, extra):
    """One pinna: a rachis with opposite oblong pinnules shortening toward a lobed tip."""
    X, Y, px = atlas.grid(key)
    layer = F.Layer(X.shape)
    layer.color[...] = pal["margin"]
    axis = [(0.008 * math.sin(math.pi * t), 0.0 + 0.97 * t) for t in np.linspace(0, 1, 12)]
    d, along = F.polyline_distance(X, Y, axis)
    rach = F.Layer(X.shape)
    rach.color[...] = (0.10, 0.11, 0.04)
    rach.alpha = np.clip(-(d - 0.006 * (1 - 0.6 * along)) / px + 0.5, 0, 1)
    rach.height = 0.0001 * rach.alpha
    rach.trans[...] = 0.2
    layer.over(rach)
    pairs = 15
    veins = [([(0, 0.02), (0, 0.96)], 1.0)] + [([(0, 0.15 + 0.12 * k), (s * 0.2, 0.25 + 0.12 * k)], 0.18)
                                              for k in range(6) for s in (-1, 1)]
    for k in range(pairs):
        t = (k + 0.4) / (pairs + 0.8)
        y = 0.02 + 0.93 * t
        ln = 0.23 * (1 - 0.72 * t ** 1.2) * rng.uniform(0.93, 1.05)
        for side in (-1, 1):
            th = side * math.radians(rng.uniform(66, 78))
            o = (0.008 * math.sin(math.pi * t) + side * 0.004, y)
            sq = rng.uniform(0.85, 1.0)
            shape = _pinnule_shape(rng, t < 0.55)
            ctr = (o[0] + 0.5 * ln * math.sin(th), o[1] + 0.5 * ln * math.cos(th))
            S.over_window(layer, X, Y, ctr, ln * 0.6,
                          lambda Xs, Ys, o=o, th=th, ln=ln, sq=sq, shape=shape: F.paint_blade(
                              *S.rotated(Xs, Ys, o, th, ln, sq), nrng, shape, veins, pal, px / ln,
                              vein_width=0.016, vein_depth=0.00008, puff=0.00008, tertiary=0.0, **extra))
    ln = 0.10
    Xl, Yl = S.rotated(X, Y, axis[-1], 0.0, ln, 0.9)
    layer.over(F.paint_blade(Xl, Yl, nrng, _pinnule_shape(rng, False), veins, pal, px / ln, vein_width=0.016,
                             vein_depth=0.00008, puff=0.00008, tertiary=0.0, **extra))
    atlas.put(key, layer, meters_per_px=0.25 / X.shape[0])


def _paint_frond(atlas, nrng, rng, pal):
    """Whole triangular blade seen from above (far LOD)."""
    X, Y, px = atlas.grid("frond")
    layer = F.Layer(X.shape)
    layer.color[...] = pal["margin"]
    axis = [(0.0, 0.02 + 0.96 * t) for t in np.linspace(0, 1, 8)]
    d, along = F.polyline_distance(X, Y, axis)
    rach = F.Layer(X.shape)
    rach.color[...] = (0.10, 0.10, 0.04)
    rach.alpha = np.clip(-(d - 0.006 * (1 - 0.6 * along)) / px + 0.5, 0, 1)
    layer.over(rach)
    pinna = F.ovate(width=0.10, widest=0.22, tip_sharp=0.8, base=0.0, tip=1.0, teeth=16, tooth_depth=0.45)
    veins = [([(0, 0.0), (0, 1.0)], 1.0)]
    for k in range(11):
        t = (k + 0.3) / 11.5
        y = 0.02 + 0.9 * t
        ln = 0.58 * (1 - 0.85 * t ** 0.9)
        for side in (-1, 1):
            th = side * math.radians(62 - 12 * t)
            Xl, Yl = S.rotated(X, Y, (0.0, y), th, ln)
            layer.over(F.paint_blade(Xl, Yl, nrng, pinna, veins, pal, px / ln, vein_width=0.02, tertiary=0.0,
                                     puff=0.0002))
    atlas.put("frond", layer, meters_per_px=0.8 / X.shape[0])


def paint_atlas():
    atlas = F.Atlas(NAME, size=4096, seed=SEED)
    atlas.column("stipe", 128)
    for key in PINNAE:
        atlas.tile(key, 900, 1800)
    atlas.tile("frond", 1800, 1600)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED)
    pals = _palettes()
    extras = {"pinna_a": ("a", dict(hair=0.15)), "pinna_b": ("b", dict(damage=0.05, edge_burn=0.1)),
              "pinna_c": ("c", dict(trans=0.7)), "pinna_yellow": ("yellow", dict(damage=0.6, edge_burn=0.4)),
              "pinna_dry": ("dry", dict(dry=1.0, edge_burn=0.5, trans=0.25))}
    for key, (pk, extra) in extras.items():
        _paint_pinna(atlas, key, nrng, rng, pals[pk], extra)
    _paint_frond(atlas, nrng, rng, pals["a"])
    # Stipe: grooved straw-green, darkening to black-brown at the base (v = 0 at the ground).
    U, V = atlas.column_grid("stipe")
    st = F.noise(U.shape, nrng, freq=30.0, beta=1.8, aniso=(1.0, 10.0))
    groove = np.abs(np.cos(U * math.pi)) ** 6
    layer = F.Layer(U.shape)
    col = F.lerp((0.030, 0.020, 0.012), (0.10, 0.11, 0.04), F.smoothstep(0.02, 0.10, V))
    col = F.lerp(col, (0.13, 0.12, 0.05), F.smoothstep(0.6, 1.0, V) * 0.6)
    layer.color = col * (0.85 + 0.3 * st)[..., None]
    layer.color = F.lerp(layer.color, np.asarray(layer.color) * 0.6, groove)
    layer.height = -0.0003 * groove + 0.00005 * st
    layer.rough = 0.5 + 0.1 * st
    layer.trans[...] = 0.1
    atlas.put("stipe", layer, meters_per_px=1.6 / U.shape[0], opaque=True)
    atlas.save()
    return atlas


def describe(rng):
    fronds = []
    # Fronds come up along a few rhizome runs through the clump.
    runs = [(rng.uniform(0, math.tau), rng.uniform(0.45, 0.75)) for _ in range(5)]
    for i in range(26):
        az, run_len = runs[i % len(runs)]
        dist = run_len * rng.random() ** 0.7
        base = Vector((math.cos(az) * dist + rng.uniform(-0.08, 0.08), math.sin(az) * dist + rng.uniform(-0.08, 0.08), -0.01))
        state = "green"
        r = rng.random()
        if i >= 22:
            state = "dry" if i >= 24 else "yellow"
        size = rng.uniform(0.5, 1.15)
        stipe_h = rng.uniform(0.32, 0.66) * size
        blade_len = rng.uniform(0.48, 0.70) * size
        heading = rng.uniform(0, math.tau)
        lean = Vector((math.cos(heading), math.sin(heading), 0))
        if state == "dry":
            stipe_h *= 0.6
        fronds.append(dict(base=base, lean=lean, stipe_h=stipe_h, blade_len=blade_len, state=state,
                           tilt=math.radians(rng.uniform(4, 26) if state != "dry" else rng.uniform(-8, 6)),
                           splay=rng.uniform(0.08, 0.32), phase=rng.random(), roll=rng.uniform(-0.2, 0.2), seed=rng.randrange(1 << 20)))
    return fronds


def _frond_geometry(fr):
    """Stipe points and the blade rachis (points + tangents) of one frond."""
    lean, h = fr["lean"], fr["stipe_h"]
    base = fr["base"]
    d0 = (lean * math.cos(fr["tilt"]) + Vector((0, 0, math.sin(fr["tilt"])))).normalized()
    up = Vector((0, 0, 1))
    # Erect stipe that bends over smoothly into the level blade (no hard knee).
    stipe = [base.copy()]
    n = 9
    for i in range(n):
        t = (i + 0.5) / n
        w = 0.6 * float(F.smoothstep(0.55, 1.0, t))
        stipe.append(stipe[-1] + (up + lean * fr["splay"]).lerp(d0, w).normalized() * (h / n))
    top = stipe[-1]
    rach = [top]
    d = up.lerp(d0, 0.8).normalized()
    step = fr["blade_len"] / 10
    for k in range(10):
        # Knee between stipe and blade, then a slight downward arch toward the tip.
        side = lean.cross(Vector((0, 0, 1))).normalized()
        if k < 2:
            d = d.lerp(d0, 0.55).normalized()
        d = (Matrix.Rotation(-0.035 * (1 + k * 0.15), 3, side) @ d).normalized()
        rach.append(rach[-1] + d * step)
    return stipe, rach


def emit(fronds, atlas, lod, height):
    b = F.Batch(height=height)
    stats = {"stipes": 0, "blades": 0}
    for fr in fronds:
        rng = random.Random(fr["seed"])
        stipe, rach = _frond_geometry(fr)
        pts = stipe + rach[1:]
        n = len(pts)
        radii = [0.0045 * (1 - 0.75 * i / (n - 1)) for i in range(n)]
        t0 = b.triangles
        idx = list(range(0, n, (1, 2, 3)[lod]))
        if idx[-1] != n - 1:
            idx.append(n - 1)
        b.tube([pts[i] for i in idx], [radii[i] for i in idx], (4, 3, 3)[lod], atlas.uv("stipe"), v_length=1.6,
               phase=fr["phase"], flutter=0.0)
        stats["stipes"] += b.triangles - t0
        t0 = b.triangles
        acc = S.arclength(rach)
        L = acc[-1]
        side0 = fr["lean"].cross(Vector((0, 0, 1))).normalized()
        blade_up = Matrix.Rotation(fr["roll"], 3, fr["lean"]) @ Vector((0, 0, 1))
        if lod == 2:
            key = "frond" if fr["state"] == "green" else ("pinna_yellow" if fr["state"] == "yellow" else "pinna_dry")
            width = L * S.tile_aspect(atlas, key) * (1.0 if key == "frond" else 2.2)
            b.card(rach[0], (rach[-1] - rach[0]).normalized(), blade_up, L * 1.05, width, atlas.uv(key), rows=2,
                   cols=1, fold=-0.1, droop=0.2, phase=fr["phase"], flutter=1.0, flutter_base=0.2)
            stats["blades"] += b.triangles - t0
            continue
        pairs = 11 if lod == 0 else 7
        for k in range(pairs):
            t = (k + 0.2) / (pairs + 0.3)
            node, tan, _ = S.at(rach, acc, L * t * 0.93)
            ln = L * (0.62 * (1 - t) ** 0.95 + 0.07) * rng.uniform(0.92, 1.05)
            if lod == 1:
                ln *= 1.1
            for side in (-1, 1):
                sd = (side0 * side).normalized()
                ang = math.radians(rng.uniform(58, 70) - 10 * t)
                d = (tan * math.cos(ang) + sd * math.sin(ang) + Vector((0, 0, -0.08))).normalized()
                if fr["state"] == "green":
                    key = rng.choices(["pinna_a", "pinna_b", "pinna_c", "pinna_yellow"], [40, 34, 20, 6])[0]
                else:
                    key = "pinna_" + ("dry" if fr["state"] == "dry" else "yellow")
                    if fr["state"] == "yellow" and rng.random() < 0.3:
                        key = "pinna_dry"
                width = ln * S.tile_aspect(atlas, key)
                up = (blade_up + sd * 0.15 * -side + Vector((rng.uniform(-.1, .1), rng.uniform(-.1, .1), 0))).normalized()
                droop = rng.uniform(0.25, 0.5) + (0.2 if fr["state"] == "dry" else 0.0)
                b.card(node, d, up, ln, width, atlas.uv(key), rows=(3, 1)[lod], cols=(2, 1)[lod],
                       fold=-0.12 if lod == 0 else 0.0, curl=(0.25 if fr["state"] == "dry" else -0.06) if lod == 0 else 0.0,
                       droop=droop, twist=rng.uniform(-0.2, 0.2) if lod == 0 else 0.0, phase=fr["phase"],
                       flutter=1.0, flutter_base=0.35 + 0.4 * t)
        # Lobed tip of the blade.
        tip_len = L * 0.16
        node, tan, _ = S.at(rach, acc, L * 0.9)
        key = "pinna_a" if fr["state"] == "green" else "pinna_" + fr["state"]
        b.card(node, tan, blade_up, tip_len, tip_len * S.tile_aspect(atlas, key), atlas.uv(key), rows=(2, 1)[lod],
               cols=(2, 1)[lod], droop=0.3, phase=fr["phase"], flutter=1.0, flutter_base=0.7)
        stats["blades"] += b.triangles - t0
    print("HOMESTEAD_TRIS", lod, stats)
    return b


def build(kit):
    atlas = paint_atlas()
    material = atlas.material()
    fronds = describe(random.Random(SEED))
    height = max(fr["stipe_h"] + math.sin(fr["tilt"]) * fr["blade_len"] * 0.5 for fr in fronds)
    batches = [emit(fronds, atlas, lod, height) for lod in range(3)]
    objs = F.finish_lods(kit, batches, "SM_" + NAME, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
