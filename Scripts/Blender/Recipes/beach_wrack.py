"""Beach wrack: a ~2.5 m wavy strandline of dried and damp seaweed along the high-tide mark (scenery).

The real thing (what the geometry and textures follow):
- The strandline of a south-west English cove is mostly wrack torn off the rocks by swell:
  bladderwrack (Fucus vesiculosus: olive-brown, leathery fronds 15-40 cm, forked again and again,
  ~1-2 cm wide with a raised midrib and paired, pea-sized air bladders either side of it, swollen
  warty receptacles at the tips in spring), a little egg wrack, and pieces of oarweed/kelp
  (Laminaria: dark brown strap blades 6-12 cm wide with wavy, amber-translucent margins and torn
  ends, often crusted with white lacy sea-mat; cord-like stipes ~1-2 cm thick ending in a claw-like
  holdfast).
- It lies in a band 20-50 cm wide that wanders with the last tide, a few centimetres thick, heaped
  into tangles where it caught. The top layer dries brittle and nearly black with a faint white salt
  bloom; underneath it stays damp, olive-brown and glossy. Bits of shell (blue-black mussel, ribbed
  buff cockle, limpet) and grit get caught in it.

Geometry: draped alpha-masked frond, strap and tangle cards stacked on a low height field along a
wavy centre line, cord stipes with a holdfast, a few real air bladders on the top fronds, and small
shell-fragment cards; all on one 2K painted atlas (homestead_foliage). The `Wind` colour is written
with R and B ~ 0 so the importer's foliage material (two-sided, masked, translucent) doesn't sway it.
Pivot bottom centre; the lowest cards sit 6 mm below z = 0 so it hugs slopes. Original procedural
geometry and textures.
"""
import math
import random

import numpy as np
from mathutils import Vector

import homestead_foliage as F
import homestead_shrub as S

NAME = "BeachWrack"
MESH = "SM_BeachWrack"
DESCRIPTION = ("A 2.5 m wavy strandline of dried and damp bladderwrack and kelp with tangles, stipes and "
               "caught shell bits, a few centimetres thick. Walk-through scenery.")
COLLISION = "none"
TRIANGLE_BUDGET = 6000
PROVENANCE = "Original project-authored procedural geometry and numpy-painted textures; no third-party asset."
BEAUTY = {"pose": (0, 0, 0), "focus": (-0.35, 0.0, 0.03), "ground": "origin", "detail_distance": 0.55,
          "views": ["hero", "detail", "eye"], "eye_distance": 2.6}
REPORT = {
    "blocking": False,
    "wind": {"attribute": "Wind (vertex colour)", "R": "~0 (lies on the sand: no sway)", "G": "random",
             "B": "0 (no flutter)", "A": "1"},
    "material_notes": ("One material M_BeachWrack: T_BeachWrack_basecolor (sRGB, alpha = opacity mask, clip 0.5), "
                       "_normal (OpenGL, flip green for Unreal), _roughness (R roughness, G translucency mask, "
                       "B AO). Two-sided foliage, masked."),
    "placement": "Ground line at z = 0; lowest cards 6 mm below. Long axis along X. Random yaw, 0.8-1.2 scale.",
}
NOTES = {"sink_depth_m": 0.006}

SEED = 77013
SIZE = 2048
LENGTH = 2.5


# ------------------------------------------------------------------ painting

def fucus_design(rng, a):
    """Dichotomous bladderwrack frond in tile units (x in [-a, a], y 0 base .. 1 tip).
    Returns strap segments [(pts, w0, w1)], bladders [(x, y, rx, ry, angle)] and tips [(x, y, w, angle)]."""
    segs, bladders, tips = [], [], []

    def grow(p, ang, length, w, level, levels):
        n = 6
        pts = [tuple(p)]
        cur = np.array(p, dtype=float)
        curv = rng.uniform(-0.5, 0.5)
        for k in range(1, n + 1):
            ak = ang + curv * k / n * 0.5
            cur = cur + np.array([math.sin(ak), math.cos(ak)]) * length / n
            pts.append(tuple(cur))
        w1 = w * 0.9
        segs.append((pts, w, w1))
        if level < levels and rng.random() < 0.8:
            k = rng.randint(2, 4)
            (x0, y0), (x1, y1) = pts[k], pts[k + 1]
            ta = math.atan2(x1 - x0, y1 - y0)
            nx, ny = math.cos(ta), -math.sin(ta)
            for side in (-1, 1):
                bladders.append((x0 + nx * side * w * 0.52, y0 + ny * side * w * 0.52, w * 0.62, w * 0.95, ta))
        if level == levels:
            tips.append((cur[0], cur[1], w1, ak))
            return
        spread = rng.uniform(0.28, 0.5) * (1.0 - 0.1 * level)
        for sgn in (-1, 1):
            if level >= 2 and rng.random() < 0.2:
                continue
            grow(cur, ak + sgn * spread, length * rng.uniform(0.74, 0.88), w1, level + 1, levels)

    levels = rng.choice((3, 4))
    grow((0.0, 0.0), rng.uniform(-0.1, 0.1), 0.30, 0.040, 0, levels)
    pts = np.array([p for s in segs for p in s[0]])
    lo, hi = pts.min(0), pts.max(0)
    span = max((hi[0] - lo[0]) / (2 * a * 0.86), (hi[1] - lo[1]) / 0.92)
    cx = (lo[0] + hi[0]) / 2

    def fit(x, y):
        return ((x - cx) / span, 0.04 + (y - lo[1]) / span)
    ws = min(1.0 / span, 0.030 / 0.040)
    segs = [([fit(*q) for q in pts_], w0 * ws, w1 * ws) for pts_, w0, w1 in segs]
    bladders = [(*fit(x, y), rx * ws, ry * ws, ang) for x, y, rx, ry, ang in bladders]
    tips = [(*fit(x, y), w * ws, ang) for x, y, w, ang in tips]
    return segs, bladders, tips


def _strap(X, Y, segs, px):
    inside = np.full(X.shape, -1.0)
    rib = np.zeros(X.shape)
    for pts, w0, w1 in segs:
        d, along = F.polyline_distance(X, Y, pts, max_dist=max(w0, w1) * 2.2)
        w = w0 + (w1 - w0) * along
        inside = np.maximum(inside, w - d)
        rib = np.maximum(rib, np.exp(-(d / (w * 0.10 + px)) ** 2))
    return inside, rib


def _ellipse(X, Y, x, y, rx, ry, ang):
    c, s = math.cos(ang), math.sin(ang)
    dx, dy = X - x, Y - y
    u = (dx * c - dy * s) / rx
    v = (dx * s + dy * c) / ry
    return 1.0 - np.sqrt(u * u + v * v)


def paint_fucus(atlas, key, nrng, design, dry, length_m):
    X, Y, px = atlas.grid(key)
    segs, bladders, tips = design
    inside, rib = _strap(X, Y, segs, px)
    blad = np.zeros(X.shape)
    for x, y, rx, ry, ang in bladders:
        e = _ellipse(X, Y, x, y, rx, ry, ang)
        blad = np.maximum(blad, np.clip(e, 0, 1))
        inside = np.maximum(inside, e * rx)
    recept = np.zeros(X.shape)
    for x, y, w, ang in tips:
        e = _ellipse(X, Y, x + math.sin(ang) * w * 0.8, y + math.cos(ang) * w * 0.8, w * 1.25, w * 2.6, ang)
        recept = np.maximum(recept, np.clip(e, 0, 1))
        inside = np.maximum(inside, e * w)
    n_lo = F.noise(X.shape, nrng, freq=6.0, beta=2.2)
    n_mid = F.noise(X.shape, nrng, freq=30.0, beta=1.8)
    n_hi = F.noise(X.shape, nrng, freq=120.0, beta=1.4)
    # Ragged, slightly eroded margins.
    inside = inside - 0.004 * (n_mid - 0.5) - 0.002 * dry * (n_lo - 0.5)
    layer = F.Layer(X.shape)
    layer.alpha = np.clip(inside / (px * 1.5) + 0.5, 0, 1)
    edge = 1.0 - F.smoothstep(0.0, 0.012, inside)
    damp = F.lerp((0.060, 0.046, 0.014), (0.092, 0.070, 0.022), n_lo)
    damp = F.lerp(damp, (0.034, 0.025, 0.008), rib * 0.75)
    bl = np.clip(blad, 0, 1)
    damp = F.lerp(damp, F.lerp((0.05, 0.036, 0.011), (0.17, 0.125, 0.036), bl ** 0.5), np.clip(bl * 8, 0, 1))
    dots = F.smoothstep(0.62, 0.75, F.noise(X.shape, nrng, freq=160.0, beta=0.6))
    damp = F.lerp(damp, F.lerp((0.135, 0.105, 0.032), (0.07, 0.05, 0.015), dots), recept ** 0.4)
    damp = F.lerp(damp, (0.035, 0.026, 0.010), edge * 0.5)
    dried = F.lerp((0.020, 0.016, 0.011), (0.040, 0.031, 0.017), n_lo)
    dried = F.lerp(dried, (0.012, 0.010, 0.008), rib * 0.5 + edge * 0.4)
    salt = F.smoothstep(0.70, 0.86, F.noise(X.shape, nrng, freq=14.0, beta=2.0)) * F.smoothstep(0.5, 0.9, n_hi)
    dried = F.lerp(dried, (0.16, 0.155, 0.145), salt * 0.3)
    dried = F.lerp(dried, (0.05, 0.04, 0.02), np.clip(blad, 0, 1) * 0.4)
    layer.color = F.lerp(damp, dried, dry)
    layer.color = layer.color * (0.9 + 0.2 * n_mid)[..., None]
    crinkle = F.noise(X.shape, nrng, freq=18.0, beta=2.0)
    layer.height = (0.0006 * rib + 0.0040 * np.clip(blad, 0, 1) ** 0.6 + 0.0012 * recept ** 0.5
                    + 0.0004 * dots * recept + 0.0003 * n_mid + dry * 0.0006 * crinkle)
    layer.rough = np.clip(F.lerp(0.36 + 0.08 * n_mid, 0.80 + 0.1 * n_mid, dry) - 0.14 * np.clip(blad * 4, 0, 1) * (1 - dry), 0.2, 0.95)
    layer.trans = (1 - dry) * (0.38 - 0.2 * rib) * np.ones(X.shape)
    layer.ao = 1.0 - 0.25 * edge - 0.2 * np.clip(1 - blad * 3, 0, 1) * np.clip(blad * 6, 0, 1)
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def paint_kelp(atlas, key, nrng, length_m):
    X, Y, px = atlas.grid(key)
    a = X.max()
    n = F.noise(X.shape, nrng, freq=10.0, beta=2.0)
    wave = 0.06 * a * np.sin(Y * 38.0 + 4.0 * n) + 0.03 * a * np.sin(Y * 91.0)
    half = a * 0.8 * (0.85 + 0.15 * np.sin(Y * 5.0)) + wave
    tear_top = 0.97 - 0.05 * F.noise(X.shape, nrng, freq=10.0, beta=2.2) - 0.03 * np.abs(np.sin(X * 17.0))
    tear_bot = 0.03 + 0.03 * F.noise(X.shape, nrng, freq=10.0, beta=2.2)
    inside = np.minimum(half - np.abs(X), np.minimum(Y - tear_bot, tear_top - Y) * 2.0)
    split = np.exp(-((X - 0.15 * a * np.sin(Y * 3)) / (px * 2.5)) ** 2) * (Y > 0.62)
    layer = F.Layer(X.shape)
    layer.alpha = np.clip(inside / (px * 1.5) + 0.5, 0, 1) * (1 - split)
    edge = 1.0 - F.smoothstep(0.0, a * 0.18, inside)
    n_hi = F.noise(X.shape, nrng, freq=90.0, beta=1.5)
    col = F.lerp((0.026, 0.019, 0.008), (0.046, 0.033, 0.012), n)
    col = F.lerp(col, (0.080, 0.052, 0.016), edge * 0.6)
    mat = F.smoothstep(0.72, 0.78, F.noise(X.shape, nrng, freq=7.0, beta=2.4))
    lace = F.smoothstep(0.45, 0.6, F.noise(X.shape, nrng, freq=150.0, beta=0.8))
    col = F.lerp(col, (0.20, 0.19, 0.17), mat * lace * 0.45)
    col = col * (0.9 + 0.2 * n_hi)[..., None]
    layer.color = col
    layer.height = 0.0004 * np.sin(Y * 38.0 + 4.0 * n) * edge + 0.0002 * mat * lace + 0.00008 * n_hi
    layer.rough = np.clip(0.34 + 0.08 * n_hi + 0.4 * mat, 0.2, 0.95)
    layer.trans = 0.45 * (1 - mat) * (0.5 + 0.5 * edge)
    layer.ao = 1.0 - 0.15 * mat
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def paint_tangle(atlas, key, nrng, rng, length_m):
    """A matted mass of torn wrack pieces seen from above, ragged at its edges."""
    X, Y, px = atlas.grid(key)
    a = X.max()
    layer = F.Layer(X.shape)
    layer.color[...] = (0.03, 0.024, 0.012)
    blob = F.noise(X.shape, nrng, freq=4.0, beta=2.6)
    cx = np.clip(1.0 - np.abs(X) / a, 0, 1)
    cy = np.clip(1.0 - np.abs(Y - 0.5) / 0.5, 0, 1)
    mask_field = np.minimum(cx * 3.0, cy * 3.0) * (0.55 + 0.6 * blob)
    # A matted, half-buried underlayer of shredded wrack the loose pieces lie on.
    fib = F.noise(X.shape, nrng, freq=60.0, beta=1.5, aniso=(1.0, 3.0))
    under = F.smoothstep(0.62, 0.72, mask_field + 0.15 * (fib - 0.5))
    layer.alpha = under
    layer.color = F.lerp(np.array((0.018, 0.015, 0.010)), np.array((0.045, 0.034, 0.014)), fib)
    layer.height = 0.0006 * fib
    layer.rough = 0.55 + 0.2 * fib
    layer.trans = np.zeros(X.shape)
    layer.ao = 0.6 + 0.2 * fib
    count = 260
    for i in range(count):
        x0, y0 = rng.uniform(-a * 0.92, a * 0.92), rng.uniform(0.05, 0.95)
        ang = rng.uniform(0, math.tau)
        L = rng.uniform(0.04, 0.16)
        bend = rng.uniform(-0.8, 0.8)
        pts = []
        for k in range(6):
            t = k / 5
            aa = ang + bend * t
            pts.append((x0 + math.sin(aa) * L * t, y0 + math.cos(aa) * L * t))
        polys = [pts]
        if rng.random() < 0.35:
            fx, fy = pts[3]
            fa = ang + bend * 0.6 + rng.choice((-1, 1)) * rng.uniform(0.4, 0.7)
            polys.append([(fx + math.sin(fa) * L * 0.5 * t, fy + math.cos(fa) * L * 0.5 * t) for t in (0, 0.5, 1.0)])
        w = rng.uniform(0.006, 0.016)
        inside = np.full(X.shape, -1.0)
        rib = np.zeros(X.shape)
        for poly in polys:
            d, along = F.polyline_distance(X, Y, poly, max_dist=w * 2.5)
            inside = np.maximum(inside, w * (1 - 0.25 * along) - d)
            rib = np.maximum(rib, np.exp(-(d / (w * 0.15 + px)) ** 2))
        al = np.clip(inside / (px * 1.5) + 0.5, 0, 1) * F.smoothstep(0.4, 0.55, mask_field)
        if not al.any():
            continue
        dry = 1.0 if rng.random() < 0.45 else rng.uniform(0.0, 0.4)
        base = np.array(F.lerp((0.075, 0.058, 0.018), (0.022, 0.018, 0.011), dry))
        top = F.Layer(X.shape)
        top.alpha = al
        top.color = np.broadcast_to(base * rng.uniform(0.8, 1.2), X.shape + (3,)).copy()
        top.color = F.lerp(top.color, base * 0.55, rib * 0.6)
        top.height = 0.0008 * np.clip(inside / w, 0, 1) + 0.0004 * rib + 0.002 * (i / count)
        top.rough = np.full(X.shape, 0.36 + 0.36 * dry)
        top.trans = np.full(X.shape, 0.3 * (1 - dry))
        top.ao = np.full(X.shape, 0.7 + 0.3 * i / count)
        layer.over(top)
    grit = F.smoothstep(0.8, 0.9, F.noise(X.shape, nrng, freq=200.0, beta=0.5)) * layer.alpha
    layer.color = F.lerp(layer.color, (0.36, 0.31, 0.23), grit * 0.6)
    layer.alpha = np.clip(layer.alpha, 0, 1)
    atlas.put(key, layer, meters_per_px=length_m / X.shape[0])


def paint_shells(atlas, key, nrng, rng):
    """Four caught shell bits, one per quadrant: mussel half, broken cockle, limpet, white fragments."""
    X, Y, px = atlas.grid(key)
    a = X.max()
    layer = F.Layer(X.shape)
    n_hi = F.noise(X.shape, nrng, freq=120.0, beta=1.2)
    n_lo = F.noise(X.shape, nrng, freq=8.0, beta=2.0)
    q = 0.5

    def quad(ix, iy):
        cx, cy = -a + (ix + 0.5) * a, (iy + 0.5) * q
        return (X - cx) / (a * 0.5), (Y - cy) / (q * 0.5)
    # Mussel: blue-black teardrop with growth lines, the worn umbo violet-white.
    u, v = quad(0, 0)
    r = np.hypot(u / 0.55, (v + 0.1) / 0.9 - 0.25 * u)
    inside = 0.85 - r - 0.25 * np.clip(-v - 0.3, 0, 1)
    al = np.clip(inside / 0.02 + 0.5, 0, 1)
    ru = np.hypot(u * 1.4, v + 0.8)
    growth = 0.5 + 0.5 * np.sin(ru * 55.0 + 3.0 * n_lo)
    col = F.lerp((0.012, 0.014, 0.024), (0.03, 0.03, 0.05), growth * 0.6 + n_lo * 0.4)
    umbo = np.clip(1 - np.hypot(u, v + 0.75) / 0.35, 0, 1)
    col = F.lerp(col, (0.22, 0.18, 0.24), umbo)
    m = F.Layer(X.shape)
    m.alpha, m.color = al, col
    m.height, m.rough = 0.0008 * np.clip(inside, 0, 1) + 0.00005 * growth, 0.28 + 0.1 * n_hi
    m.ao = np.ones(X.shape)
    layer.over(m)
    # Broken cockle: ribbed, buff-white, a jagged break across it.
    u, v = quad(1, 0)
    rr = np.hypot(u, v + 0.15)
    th = np.arctan2(u, v + 0.15)
    ribs = 0.5 + 0.5 * np.cos(th * 26.0)
    brk = v - (0.15 + 0.12 * np.sin(u * 9.0) + 0.06 * (n_hi - 0.5))
    inside = np.minimum(0.82 - rr, -brk * 0.6)
    al = np.clip(inside / 0.02 + 0.5, 0, 1)
    col = F.lerp((0.30, 0.27, 0.21), (0.45, 0.41, 0.33), ribs * 0.6 + n_lo * 0.4)
    col = F.lerp(col, (0.25, 0.20, 0.13), np.clip(rr - 0.6, 0, 1) * 2.0 * (0.5 + 0.5 * np.sin(rr * 40)))
    m = F.Layer(X.shape)
    m.alpha, m.color = al, col
    m.height, m.rough = 0.0006 * np.clip(inside * 3, 0, 1) + 0.0003 * ribs, 0.55 + 0.1 * n_hi
    m.ao = 1.0 - 0.2 * (1 - ribs)
    layer.over(m)
    # Limpet: oval, radial ridges, grey-buff with a pale apex.
    u, v = quad(0, 1)
    rr = np.hypot(u / 0.85, v / 0.7)
    th = np.arctan2(u, v)
    ridges = 0.5 + 0.5 * np.cos(th * 34.0 + 3.0 * n_lo)
    inside = 0.95 - rr - 0.04 * ridges
    al = np.clip(inside / 0.02 + 0.5, 0, 1)
    col = F.lerp((0.17, 0.155, 0.13), (0.30, 0.28, 0.24), ridges * 0.5 + n_lo * 0.5)
    col = F.lerp(col, (0.36, 0.34, 0.30), np.clip(1 - rr / 0.2, 0, 1))
    m = F.Layer(X.shape)
    m.alpha, m.color = al, col
    m.height = 0.0025 * np.clip(1 - rr, 0, 1) + 0.0002 * ridges
    m.rough, m.ao = 0.6 + 0.1 * n_hi, 1.0 - 0.15 * (1 - ridges)
    layer.over(m)
    # White shell grit: a scatter of small angular fragments.
    u, v = quad(1, 1)
    frag = F.Layer(X.shape)
    for i in range(14):
        cx, cy = rng.uniform(-0.75, 0.75), rng.uniform(-0.75, 0.75)
        rad = rng.uniform(0.08, 0.2)
        ang = rng.uniform(0, math.tau)
        sides = rng.randint(3, 5)
        th = np.arctan2(u - cx, v - cy) + ang
        rr = np.hypot(u - cx, v - cy)
        poly = rad * np.cos(math.pi / sides) / np.maximum(np.cos(np.mod(th, 2 * math.pi / sides) - math.pi / sides), 0.3)
        inside = poly - rr
        al = np.clip(inside / 0.02 + 0.5, 0, 1)
        top = F.Layer(X.shape)
        tone = rng.uniform(0.0, 1.0)
        top.alpha = al
        top.color = np.broadcast_to(np.array(F.lerp((0.30, 0.28, 0.24), (0.46, 0.43, 0.38), tone)),
                                    X.shape + (3,)).copy()
        top.height = 0.0004 * np.clip(inside / rad, 0, 1)
        top.rough = np.full(X.shape, 0.55)
        frag.over(top)
    layer.over(frag)
    layer.trans[...] = 0.0
    atlas.put(key, layer, meters_per_px=0.10 / X.shape[0])


def paint_column(atlas, key, nrng, base, top, length_m, rough=0.45, trans=0.15, rings=0.0):
    U, V = atlas.column_grid(key)
    st = F.noise(U.shape, nrng, freq=50.0, beta=1.4, aniso=(1.0, 6.0))
    layer = F.Layer(U.shape)
    layer.color = F.lerp(np.array(base), np.array(top), st)
    if rings:
        ring = 0.5 + 0.5 * np.sin(V * 2 * math.pi * 40)
        layer.color = F.lerp(layer.color, np.array(base) * 0.7, ring * rings)
    layer.height = 0.00006 * st
    layer.rough = rough + 0.1 * st
    layer.trans[...] = trans
    atlas.put(key, layer, meters_per_px=length_m / U.shape[0], opaque=True)


TILES = {"fucus_a": (512, 1024), "fucus_b": (512, 1024), "fucus_dry": (512, 1024), "kelp": (320, 1024),
         "tangle": (512, 1024), "tangle2": (512, 1024), "shells": (512, 512), "fucus_c": (384, 512)}


def paint_atlas(designs):
    atlas = F.Atlas(NAME, size=SIZE, seed=SEED)
    atlas.column("stipe", 64)
    atlas.column("bladder", 64)
    for key, (w, h) in TILES.items():
        atlas.tile(key, w, h)
    if atlas.cached():
        return atlas
    nrng = np.random.default_rng(SEED)
    rng = random.Random(SEED + 1)
    paint_fucus(atlas, "fucus_a", nrng, designs["fucus_a"], 0.0, 0.34)
    paint_fucus(atlas, "fucus_b", nrng, designs["fucus_b"], 0.35, 0.30)
    paint_fucus(atlas, "fucus_dry", nrng, designs["fucus_dry"], 1.0, 0.30)
    paint_fucus(atlas, "fucus_c", nrng, designs["fucus_c"], 0.75, 0.16)
    paint_kelp(atlas, "kelp", nrng, 0.60)
    paint_tangle(atlas, "tangle", nrng, rng, 0.50)
    paint_tangle(atlas, "tangle2", nrng, rng, 0.50)
    paint_shells(atlas, "shells", nrng, rng)
    paint_column(atlas, "stipe", nrng, (0.040, 0.026, 0.012), (0.075, 0.050, 0.020), 0.25, rough=0.5, rings=0.15)
    paint_column(atlas, "bladder", nrng, (0.085, 0.064, 0.020), (0.12, 0.09, 0.028), 0.03, rough=0.28, trans=0.3)
    atlas.save()
    return atlas


def designs_for(atlas_aspects):
    rng = random.Random(SEED + 7)
    out = {}
    for key in ("fucus_a", "fucus_b", "fucus_dry", "fucus_c"):
        w, h = TILES[key]
        a = (w - 24) / (2.0 * (h - 24))
        out[key] = fucus_design(rng, a)
    return out


# ------------------------------------------------------------------ geometry

def centre_y(x):
    return 0.10 * math.sin(2.1 * x + 0.5) + 0.04 * math.sin(5.3 * x + 1.0)


def half_width(x):
    return 0.17 + 0.06 * math.sin(3.1 * x + 1.0) + 0.03 * math.sin(7.7 * x)


TANGLES = [(-0.55, 0.02, 0.16, 0.035), (0.35, -0.03, 0.13, 0.03), (0.95, 0.03, 0.10, 0.022),
           (-1.0, 0.0, 0.09, 0.018)]


def height(x, y):
    dy = (y - centre_y(x)) / half_width(x)
    h = 0.016 * math.exp(-dy * dy * 1.6) * min(density(x), 1.0)
    for tx, ty, tr, th in TANGLES:
        h += th * math.exp(-((x - tx) ** 2 + (y - centre_y(tx) - ty) ** 2) / (tr * tr))
    return h - 0.006


def drape(b, origin, yaw, length, width, rect, rows, cols, z0, rng, lift=0.0, curl=0.0, crinkle=0.003, phase=0.0,
          path=None, ruffle=0.0):
    """A card lying on the wrack height field: u across (width), v along (length) from ``origin``
    heading ``yaw``. ``path`` (list of xy) overrides the straight centre line (kelp straps)."""
    u0, u1, v0, v1 = rect
    idx = []
    if path is None:
        d = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        path = [Vector(origin) + d * length * i / rows for i in range(rows + 1)]
    seed = rng.random() * 100
    for i, c in enumerate(path):
        t = i / rows
        if i < rows:
            tan = (path[i + 1] - c).normalized()
        else:
            tan = (c - path[i - 1]).normalized()
        side = Vector((-tan.y, tan.x, 0.0))
        for j in range(cols + 1):
            s = j / cols * 2 - 1
            p = c + side * s * width * 0.5
            z = height(p.x, p.y) + z0 + crinkle * math.sin(p.x * 61 + seed) * math.sin(p.y * 47 + seed * 1.3)
            z += curl * s * s * width * 0.5 + lift * t * t * length * 0.25
            z += ruffle * s * s * math.sin(t * length * 70.0 + s * 2.0 + seed)
            idx.append(b._vert(Vector((p.x, p.y, z)), phase, 0.0))
    for i in range(rows):
        for j in range(cols):
            a_ = i * (cols + 1) + j
            b.faces.append((idx[a_], idx[a_ + 1], idx[a_ + cols + 2], idx[a_ + cols + 1]))
            b.uvs.append([(u0 + (u1 - u0) * (jj / cols), v0 + (v1 - v0) * (ii / rows))
                          for ii, jj in ((i, j), (i, j + 1), (i + 1, j + 1), (i + 1, j))])


def sub_rect(rect, ix, iy):
    u0, u1, v0, v1 = rect
    du, dv = (u1 - u0) / 2, (v1 - v0) / 2
    return (u0 + ix * du, u0 + (ix + 1) * du, v0 + iy * dv, v0 + (iy + 1) * dv)


CLUMPS = (-0.95, -0.52, -0.05, 0.38, 0.92)


def density(x):
    """Wrack heaps up in clumps along the line with thinner stretches between them."""
    return 0.22 + sum(math.exp(-((x - c) / 0.2) ** 2) for c in CLUMPS)


def describe(rng, designs):
    def along(spread=0.45):
        while True:
            x = rng.uniform(-LENGTH / 2 + 0.12, LENGTH / 2 - 0.12)
            if rng.random() * 1.25 < density(x):
                break
        y = centre_y(x) + rng.gauss(0, spread) * half_width(x) * (0.6 + 0.4 * min(density(x), 1.0))
        if rng.random() < 0.08:
            y = centre_y(x) + rng.choice((-1, 1)) * half_width(x) * rng.uniform(1.1, 1.7)
        tang = math.atan2(centre_y(x + 0.01) - centre_y(x - 0.01), 0.02)
        return x, y, tang
    fronds = []
    for i in range(170):
        x, y, tang = along()
        layer = rng.random()
        key = rng.choices(("fucus_a", "fucus_b", "fucus_dry", "fucus_c"),
                          (35 * (1 - layer) + 5, 25, 40 * layer + 5, 15))[0]
        length = rng.uniform(0.08, 0.14) if key == "fucus_c" else rng.uniform(0.20, 0.38)
        yaw = tang + rng.gauss(0, 0.7) + (math.pi if rng.random() < 0.5 else 0.0)
        fronds.append(dict(x=x - math.cos(yaw) * length * 0.5, y=y - math.sin(yaw) * length * 0.5, yaw=yaw,
                           length=length, key=key, layer=layer, keep=rng.random(), curl=rng.uniform(-0.15, 0.25),
                           lift=rng.uniform(0.0, 0.15) if layer > 0.7 else 0.0, phase=rng.random()))
    kelps = []
    for i in range(6):
        x, y, tang = along()
        length = rng.uniform(0.38, 0.62)
        width = rng.uniform(0.07, 0.11)
        yaw = tang + rng.gauss(0, 0.35) + (math.pi if rng.random() < 0.5 else 0.0)
        wig = rng.uniform(0.04, 0.09)
        ph = rng.uniform(0, 6)
        kelps.append(dict(x=x - math.cos(yaw) * length * 0.5, y=y - math.sin(yaw) * length * 0.5, yaw=yaw,
                          length=length, width=width, wig=wig, ph=ph, layer=rng.uniform(0.2, 0.9), keep=rng.random()))
    tangles = []
    for i in range(11):
        x = -LENGTH / 2 + 0.22 + (LENGTH - 0.44) * (i + rng.uniform(-0.3, 0.3)) / 10
        tang = math.atan2(centre_y(x + 0.01) - centre_y(x - 0.01), 0.02)
        length = rng.uniform(0.44, 0.60)
        tangles.append(dict(x=x, y=centre_y(x) + rng.gauss(0, 0.05), yaw=tang + rng.gauss(0, 0.2),
                            length=length, width=length * rng.uniform(0.5, 0.62),
                            key=rng.choice(("tangle", "tangle2"))))
    stipes = []
    for i in range(3):
        x, y, tang = along()
        length = rng.uniform(0.22, 0.42)
        u = rng.uniform(0, math.tau)
        yaw = tang + (u % math.pi - math.pi / 2) * 0.35 + (math.pi if u > math.pi else 0.0)
        x, y = x - math.cos(yaw) * length * 0.5, y - math.sin(yaw) * length * 0.5
        stipes.append(dict(x=x, y=y, yaw=yaw, length=length, r=rng.uniform(0.007, 0.011), bend=rng.uniform(-0.4, 0.4),
                           holdfast=i < 2, phase=rng.random()))
    shells = []
    for i in range(12):
        x, y, tang = along()
        shells.append(dict(x=x, y=y, size=rng.uniform(0.03, 0.06), q=(i % 2, (i // 2) % 2), spin=rng.uniform(0, math.tau),
                           tilt=Vector((rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 1.0)).normalized()))
    bladder_cards = sorted([f for f in fronds if f["key"] in ("fucus_a", "fucus_b")], key=lambda f: -f["layer"])[:10]
    return dict(fronds=fronds, kelps=kelps, tangles=tangles, stipes=stipes, shells=shells,
                bladder_cards=bladder_cards, designs=designs)


def emit(desc, atlas, lod):
    b = F.Batch(height=50.0)
    rng = random.Random(SEED + 100)
    stats = {}
    t0 = b.triangles
    for tg in desc["tangles"]:
        if lod == 2 and tg is desc["tangles"][-1]:
            continue
        d = Vector((math.cos(tg["yaw"]), math.sin(tg["yaw"]), 0))
        origin = Vector((tg["x"], tg["y"], 0)) - d * tg["length"] / 2
        r = (4, 3, 2)[lod]
        drape(b, origin, tg["yaw"], tg["length"], tg["width"], atlas.uv(tg["key"]), r, (4, 2, 2)[lod], 0.0005, rng,
              crinkle=0.004 if lod < 2 else 0.0)
    stats["tangles"] = b.triangles - t0
    t0 = b.triangles
    for kp in desc["kelps"]:
        if lod == 1 and kp["keep"] > 0.8 or lod == 2 and kp["keep"] > 0.5:
            continue
        rows = (14, 6, 3)[lod]
        d = Vector((math.cos(kp["yaw"]), math.sin(kp["yaw"]), 0))
        side = Vector((-d.y, d.x, 0))
        path = [Vector((kp["x"], kp["y"], 0)) + d * kp["length"] * i / rows
                + side * kp["wig"] * math.sin(i / rows * 5.0 + kp["ph"]) for i in range(rows + 1)]
        drape(b, None, 0, kp["length"], kp["width"], atlas.uv("kelp"), rows, (4, 2, 1)[lod],
              0.002 + 0.010 * kp["layer"], rng, curl=0.12, crinkle=0.003, path=path, ruffle=0.006)
    stats["kelp"] = b.triangles - t0
    t0 = b.triangles
    for fr in sorted(desc["fronds"], key=lambda f: f["layer"]):
        if lod == 1 and fr["keep"] > 0.9 or lod == 2 and fr["keep"] > 0.55:
            continue
        aspect = S.tile_aspect(atlas, fr["key"])
        rows = (4, 2, 1)[lod] if fr["length"] > 0.15 else (2, 1, 1)[lod]
        drape(b, (fr["x"], fr["y"], 0), fr["yaw"], fr["length"], fr["length"] * aspect, atlas.uv(fr["key"]),
              rows, (2, 1, 1)[lod], 0.002 + 0.013 * fr["layer"], rng, lift=fr["lift"], curl=fr["curl"],
              crinkle=0.003 if lod < 2 else 0.0, phase=fr["phase"])
    stats["fronds"] = b.triangles - t0
    t0 = b.triangles
    if lod < 2:
        for st in desc["stipes"]:
            # A dried kelp stipe never lies straight: it wanders in two or three gentle S-bends, sags
            # into the heap and rides up over the fronds it crosses, and its radius swells and pinches.
            n = (16, 7)[lod]
            d = Vector((math.cos(st["yaw"]), math.sin(st["yaw"]), 0))
            side = Vector((-d.y, d.x, 0))
            ph = st["phase"] * 6.283
            pts = []
            radii = []
            for i in range(n + 1):
                t = i / n
                lat = (st["bend"] * t * (1 - t) * 1.6
                       + 0.05 * math.sin(t * 4.2 + ph) * t
                       + 0.012 * math.sin(t * 9.0 + ph * 1.7))
                p = Vector((st["x"], st["y"], 0)) + d * st["length"] * t + side * lat * st["length"]
                ride = 0.5 + 0.5 * math.sin(t * 9.0 + ph * 2.3)
                p.z = height(p.x, p.y) + st["r"] * 0.8 + 0.004 + 0.006 * ride * math.sin(math.pi * t)
                pts.append(p)
                radii.append(st["r"] * (1.0 - 0.35 * t) * (1.0 + 0.12 * math.sin(t * 17.0 + ph)))
            b.tube(pts, radii, (8, 5)[lod], atlas.uv("stipe"), v_length=0.25, phase=st["phase"])
            if st["holdfast"] and lod == 0:
                # Laminaria holdfast: a tangled claw of short, knobbly haptera that branch once and
                # curl down to grip (here: the sand) - curved, not straight spokes.
                base = pts[0]
                for k in range(7):
                    a = k * math.tau / 7 + rng.uniform(-0.3, 0.3)
                    hd = (Vector((math.cos(a), math.sin(a), rng.uniform(0.0, 0.6))) - d * 0.7).normalized()
                    L = rng.uniform(0.03, 0.055)
                    curl = rng.uniform(0.5, 1.0)
                    hp = []
                    for m in range(4):
                        u = m / 3
                        q = base + hd * L * u + side * math.sin(u * 2.5 + a) * 0.006
                        q.z += 0.008 * math.sin(math.pi * u * 0.8) - curl * 0.012 * u * u
                        q.z = max(q.z, height(q.x, q.y) + 0.0015)
                        hp.append(q)
                    b.tube(hp, [st["r"] * 0.42, st["r"] * 0.32, st["r"] * 0.24, st["r"] * 0.15], 5, atlas.uv("stipe"),
                           v_length=0.25, phase=st["phase"], cap=True)
    stats["stipes"] = b.triangles - t0
    t0 = b.triangles
    if lod < 2:
        # Real air bladders on the top fronds, where the atlas painted them.
        for fr in desc["bladder_cards"][: (10, 4)[lod]]:
            segs, bladders, tips = desc["designs"][fr["key"]]
            aspect = S.tile_aspect(atlas, fr["key"])
            a = aspect * 0.5
            d = Vector((math.cos(fr["yaw"]), math.sin(fr["yaw"]), 0))
            side = Vector((-d.y, d.x, 0))
            width = fr["length"] * aspect
            for (bx, by, rx, ry, ang) in bladders[::2][:2]:
                for sgn in (0, 1):
                    bx2, by2 = bladders[bladders.index((bx, by, rx, ry, ang)) + sgn][:2]
                    p = Vector((fr["x"], fr["y"], 0)) + d * by2 * fr["length"] + side * (bx2 / (2 * a)) * width
                    rad = rx / (2 * a) * width * 0.9
                    z = (height(p.x, p.y) + 0.002 + 0.013 * fr["layer"] + fr["lift"] * by2 * by2 * fr["length"] * 0.25
                         + fr["curl"] * (bx2 / a) ** 2 * width * 0.5)
                    axis = d * math.cos(ang) + side * math.sin(ang)
                    b.sphere(Vector((p.x, p.y, z + rad * 0.45)), rad, atlas.uv("bladder"), segs=6, rings=3,
                             stretch=1.35, axis=axis, phase=fr["phase"])
    stats["bladders"] = b.triangles - t0
    t0 = b.triangles
    for sh in desc["shells"]:
        if lod == 2:
            break
        if lod == 1 and sh["size"] < 0.035:
            continue
        rect = sub_rect(atlas.uv("shells"), *sh["q"])
        z = height(sh["x"], sh["y"]) + 0.004 + 0.012 * ((sh["spin"] * 7.0) % 1.0)
        b.flat(Vector((sh["x"], sh["y"], z)), sh["tilt"], sh["size"], rect, spin=sh["spin"], cup=0.12, flutter=0.0,
               segs=(2, 1)[lod])
    stats["shells"] = b.triangles - t0
    b.wind = [(w[0], w[1], 0.0, 1.0) for w in b.wind]
    print("HOMESTEAD_TRIS", lod, b.triangles, stats, flush=True)
    return b


def build(kit):
    designs = designs_for(None)
    atlas = paint_atlas(designs)
    material = atlas.material()
    desc = describe(random.Random(SEED), designs)
    batches = [emit(desc, atlas, lod) for lod in range(3)]
    objs = F.finish_lods(kit, batches, MESH, material, smooth_angle=179.0)
    print("HOMESTEAD_LODS", F.lod_report(objs))
    return objs
