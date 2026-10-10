"""Procedural PBR maps for the footwear, synthesised per texel in each pair's packed UV atlas.

Per-texel inputs are rasterized from the mesh: the part id (which material the texel belongs to),
the rest position (3D noise is continuous across UV seams), the unpacked meter-scale parameter UVs
("Param": along/around a thong, around/down an upper) and per-vertex construction attributes
(distance to the back seam, tongue mask, sole weave row phase, fur zone, ...). Each part kind
writes linear albedo, a height field in millimetres and roughness; the tangent-space normal map
(OpenGL, +Y) is the gradient of the height in texel space, and AO multiplies a cavity term with a
Cycles-baked geometric AO. Albedo stays in plausible linear ranges: smoked hide 0.06-0.25,
sheepskin wool up to ~0.5, sagebrush bark fibre ~0.2-0.3.
"""
import math

import bpy
import numpy as np

from outfit import textures as T
from . import modern as MD
from . import pairs as PR

S = T.smoothstep


# ------------------------------------------------------------------ noise

def fbm(p, scale, octaves=3, seed=0):
    return T.fbm3(np.asarray(p, np.float64) / scale, octaves, seed)


def cells(p, scale, seed=0, jitter=0.9):
    """3D cellular noise: F1, F2 distances (in cell units) and a per-cell random id in [0,1)."""
    q = np.asarray(p, np.float64) / scale
    i = np.floor(q)
    f1 = np.full(q.shape[:-1], 9.0)
    f2 = np.full(q.shape[:-1], 9.0)
    cid = np.zeros(q.shape[:-1])
    cvec = np.zeros(q.shape)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                c = i + np.array([dx, dy, dz], np.float64)
                h = c[..., 0] * 127.1 + c[..., 1] * 311.7 + c[..., 2] * 74.7
                jit = np.stack([T.hash1(h, seed + 1), T.hash1(h, seed + 2), T.hash1(h, seed + 3)], -1)
                pt = c + 0.5 + jitter * (jit - 0.5)
                d = np.linalg.norm(pt - q, axis=-1)
                closer = d < f1
                f2 = np.where(closer, f1, np.minimum(f2, d))
                cid = np.where(closer, T.hash1(h, seed + 4), cid)
                cvec = np.where(closer[..., None], pt - q, cvec)
                f1 = np.where(closer, d, f1)
    return f1, f2, cid, cvec


def mix(a, b, t):
    t = np.asarray(t, np.float64)
    if t.ndim == 1:
        t = t[:, None]
    return a * (1 - t) + b * t


def col(*rgb):
    return np.array(rgb, np.float64)


# ------------------------------------------------------------ part kinds
# Each returns (albedo (n,3), height_mm (n,), roughness (n,)). F: dict of per-texel arrays.

def suede(F, seed):
    """Smoke-tanned flesh side of the sheepskin (boot outside), with the gathered hide sole."""
    P = F["pos"]
    z = P[:, 2]
    smoke = fbm(P, 0.06, 4, seed)
    mott = fbm(P, 0.012, 3, seed + 1)
    nap = fbm(P, 0.0006, 2, seed + 2)
    streak = fbm(P * np.array([1.0, 1.0, 0.18]), 0.0012, 2, seed + 3)
    base = mix(col(0.155, 0.105, 0.068), col(0.235, 0.165, 0.105), S(smoke, 0.35, 0.65))
    base = base * (0.9 + 0.2 * mott)[:, None] * (0.93 + 0.1 * nap)[:, None]
    h = 0.035 * nap + 0.025 * streak + 0.18 * (fbm(P * np.array([1, 1, 0.3]), 0.02, 3, seed + 4) - 0.5)
    rough = 0.9 + 0.05 * (nap - 0.5)
    # flex creases where the ankle bends: horizontal ridges across the front of the shaft
    front = S(F["dback"], 0.05, 0.10)
    band = S(z, 0.035, 0.06) * (1 - S(z, 0.12, 0.16))
    cr = ridge(P * np.array([0.25, 0.25, 1.0]), 0.012, 3, seed + 11) ** 7 * front * band
    cr2 = ridge(P * np.array([0.5, 0.5, 1.0]), 0.004, 2, seed + 12) ** 9 * front * band
    h = h - 0.35 * cr - 0.12 * cr2
    base = base * (1 - 0.18 * cr - 0.08 * cr2)[:, None]
    rough = rough - 0.04 * cr
    # the hide's own character: faint healed scratches and paler rubbed patches
    scr = ridge(P * np.array([1.0, 0.4, 0.7]), 0.02, 2, seed + 13) ** 24
    rub = S(fbm(P, 0.025, 3, seed + 14), 0.6, 0.8)
    base = mix(base, base * 1.25, 0.5 * rub) * (1 - 0.1 * scr)[:, None]
    h = h - 0.05 * scr        # sole: thicker smoked hide gathered up to the seam at 21 mm
    zseam = 0.021
    sole = 1 - S(z, zseam - 0.001, zseam + 0.0015)
    wear = fbm(P, 0.008, 3, seed + 5)
    sole_col = mix(col(0.075, 0.052, 0.034), col(0.13, 0.095, 0.065), S(wear, 0.4, 0.75))
    ground = 1 - S(z, -0.0115, -0.004)
    sole_col = mix(sole_col, col(0.16, 0.135, 0.105), 0.45 * ground * S(fbm(P, 0.004, 2, seed + 6), 0.45, 0.7))
    base = mix(base, sole_col, sole)
    h = h + sole * (0.06 * (fbm(P, 0.0015, 3, seed + 7) - 0.5) - 0.12 * ground * (fbm(P, 0.003, 2, seed + 8) - 0.5))
    rough = mix(rough[:, None], np.full((len(z), 1), 0.82), sole)[:, 0]
    # whip-stitched sinew along the sole seam
    ang = np.arctan2(P[:, 0] - F["foot_x"], P[:, 1] - F["foot_y"])
    arc = ang * 0.07
    ph = arc / 0.0065 + (z - zseam) / 0.0045
    fr = ph - np.floor(ph)
    band = np.exp(-((z - zseam) / 0.0024) ** 2)
    st = band * np.exp(-((fr - 0.5) / 0.14) ** 2)
    h = h + 0.45 * st - 0.12 * np.exp(-((z - zseam) / 0.0008) ** 2)
    base = mix(base, col(0.30, 0.25, 0.17) * (0.9 + 0.2 * mott)[:, None], S(st, 0.35, 0.7))
    # back seam, whip-stitched up the shaft
    db = F["dback"]
    bs = np.exp(-(db / 0.0022) ** 2) * S(z, zseam, zseam + 0.01)
    ph2 = P[:, 2] / 0.0062 + db / 0.004
    st2 = bs * np.exp(-(((ph2 - np.floor(ph2)) - 0.5) / 0.14) ** 2)
    h = h + 0.4 * st2 - 0.10 * np.exp(-(db / 0.0007) ** 2) * S(z, zseam, zseam + 0.01)
    base = mix(base, col(0.28, 0.23, 0.16), S(st2, 0.35, 0.7))
    # soil and wet darkening toward the ground, scuffs on the toe
    soil = (1 - S(z, 0.0, 0.06)) * S(fbm(P, 0.02, 3, seed + 9), 0.42, 0.7)
    base = base * (1 - 0.35 * soil)[:, None] + col(0.09, 0.075, 0.055) * (0.2 * soil)[:, None]
    rough = rough + 0.03 * soil
    return base, h, rough


def wool(F, seed, clean=0.0):
    """Sheepskin wool (hair side): crimped locks separated by deep partings, creamy with
    greyed, dirtier tips on the cuff; the lining is cleaner."""
    P = F["pos"]
    # domain-warped so lock outlines curve instead of forming Voronoi polygons
    warp = np.stack([fbm(P, 0.011, 2, seed + 10 + k) for k in range(3)], -1) - 0.5
    Q = P + 0.007 * warp
    dome = np.zeros(len(P))
    curl = np.zeros(len(P))
    lock = np.zeros(len(P))
    for k, (sc, w) in enumerate(((0.0068, 1.0), (0.0046, 0.8), (0.0031, 0.55))):
        f1, _, cid, cv = cells(Q, sc, seed + 7 * k, jitter=1.0)
        blob = np.clip(1 - (f1 / 0.72) ** 2, 0, 1) ** 0.7 * w
        # each lock is a tight crimped spiral: rings about its centre, twisted by the angle
        ang = np.arctan2(cv[:, 2], cv[:, 0] + 0.7 * cv[:, 1])
        sp = 0.5 + 0.5 * np.sin(6.2832 * (3.2 * f1 + cid) + 2.0 * ang)
        top = blob > dome
        dome = np.where(top, blob, dome)
        curl = np.where(top, sp, curl)
        lock = np.where(top, cid, lock)
    fib = fbm(Q, 0.00032, 2, seed + 4)
    fluff = fbm(P, 0.0016, 3, seed + 3)
    h = 1.5 * dome + 0.28 * curl * dome + 0.22 * fib + 0.3 * fluff - 1.0
    root = 1 - S(dome, 0.02, 0.45)
    tip = mix(col(0.50, 0.43, 0.32), col(0.43, 0.36, 0.26), lock)
    tip = mix(tip, col(0.56, 0.50, 0.40), clean)
    dirt = S(fbm(P, 0.03, 3, seed + 5), 0.5, 0.75) * (1 - clean)
    tip = mix(tip, col(0.29, 0.23, 0.16), 0.4 * dirt)
    base = mix(tip, col(0.21, 0.16, 0.11), 0.55 * root)
    base = base * (0.88 + 0.2 * curl * dome)[:, None] * (0.9 + 0.16 * fib)[:, None] * (0.94 + 0.1 * fluff)[:, None]
    rough = 0.86 + 0.08 * root
    return base, h, rough


def cuff(F, seed):
    fur = F["fur"] > 0.5
    a1, h1, r1 = wool(F, seed + 20)
    a2, h2, r2 = flesh_side(F, seed + 21)
    m = fur.astype(float)
    return mix(a2, a1, m), h2 * (1 - m) + h1 * m, r2 * (1 - m) + r1 * m


def flesh_side(F, seed):
    """Paler flesh side of hide (inside of the cuff, lining of the turnshoe)."""
    P = F["pos"]
    m = fbm(P, 0.02, 3, seed)
    nap = fbm(P, 0.0005, 2, seed + 1)
    base = mix(col(0.25, 0.18, 0.12), col(0.33, 0.25, 0.17), S(m, 0.35, 0.65)) * (0.92 + 0.12 * nap)[:, None]
    return base, 0.04 * nap + 0.08 * (fbm(P, 0.006, 2, seed + 2) - 0.5), 0.9 + 0.04 * nap


def thong(F, seed, color=(0.115, 0.07, 0.04)):
    """Cut leather thong: burnished grain, darker cut edges, little twist creases."""
    P = F["pos"]
    u, v = F["pu"], F["pv"]
    per = F["per"]
    ang = (u / np.maximum(per, 1e-4)) * 6.2832
    edge = np.abs(np.cos(ang)) ** 6
    c = col(*color)
    m = fbm(P, 0.01, 3, seed)
    base = c * (0.8 + 0.4 * m)[:, None] * (1 - 0.35 * edge)[:, None]
    crease = np.abs(np.sin(v / 0.0045 * 3.1416 + 2 * fbm(P, 0.006, 2, seed + 1))) ** 12
    h = -0.08 * crease + 0.03 * fbm(P, 0.0005, 2, seed + 2) - 0.05 * edge
    rough = 0.62 + 0.12 * edge + 0.05 * (m - 0.5)
    return base, h, rough


def ridge(p, scale, octaves=3, seed=0):
    """Ridged noise: 1 on thin crease lines, falling off to 0."""
    return 1 - np.abs(2 * fbm(p, scale, octaves, seed) - 1)


def turn_outer(F, seed):
    """Vegetable-tanned calf, grain out, dressed with tallow: mid brown with smoky mottling, fine
    pores and a network of grain breaks, irregular flex creases across the ball, burnished toe and
    heel with scuffs, stitched heel and medial seams, awl holes, darker thicker sole."""
    P = F["pos"]
    z = P[:, 2]
    y = P[:, 1]
    tone = fbm(P, 0.045, 4, seed)
    mott = fbm(P, 0.007, 3, seed + 1)
    base = mix(col(0.075, 0.046, 0.027), col(0.155, 0.098, 0.057), S(tone, 0.28, 0.72))
    base = base * (0.84 + 0.32 * mott)[:, None]
    f1, _, _, _ = cells(P, 0.00032, seed + 2)
    pore = 1 - S(f1, 0.0, 0.22)
    brk = S(ridge(P, 0.0028, 3, seed + 3), 0.9, 0.995)
    fine = fbm(P, 0.0007, 2, seed + 14)
    h = -0.018 * pore - 0.03 * brk + 0.025 * (fine - 0.5) + 0.04 * (fbm(P, 0.0035, 2, seed + 4) - 0.5)
    base = base * (1 - 0.08 * brk)[:, None] * (0.96 + 0.08 * fine)[:, None]
    rough = 0.50 + 0.05 * pore + 0.06 * brk + 0.06 * (mott - 0.5)
    # flex creases: irregular ridges running across the vamp at the ball
    ball = np.exp(-((y - F["ball_y"]) / 0.016) ** 2) * S(z, 0.010, 0.028)
    q = P * np.array([0.22, 1.0, 0.5])
    crease = S(ridge(q, 0.0045, 3, seed + 5), 0.86, 0.99) * ball
    h = h - 0.16 * crease
    base = base * (1 - 0.35 * crease)[:, None]
    rough = rough + 0.08 * crease
    # burnished toe and heel counter, with scuffs on the toe
    toe = S(-(y - (F["toe_y"] + 0.04)), 0.0, 0.035)
    heel = S(y - (F["heel_y"] - 0.03), 0.0, 0.025) * S(z, 0.0, 0.02)
    burn = np.clip(toe + heel, 0, 1) * S(fbm(P, 0.012, 2, seed + 6), 0.3, 0.6)
    base = mix(base, base * 1.35, 0.5 * burn)
    rough = rough - 0.12 * burn
    scr = S(ridge(P * np.array([1.0, 0.25, 1.0]), 0.002, 2, seed + 7), 0.93, 1.0) * toe
    scr = scr * S(fbm(P, 0.006, 2, seed + 8), 0.5, 0.65)
    base = mix(base, col(0.23, 0.165, 0.11), 0.7 * scr)
    rough = rough + 0.35 * scr
    h = h - 0.03 * scr
    # sole leather below the turn seam bead
    sole = 1 - S(z, F["floor_out"] + 0.0028, F["floor_out"] + 0.0036)
    sole_col = mix(col(0.052, 0.034, 0.021), col(0.10, 0.068, 0.042), S(fbm(P, 0.006, 3, seed + 9), 0.35, 0.7))
    ground = 1 - S(z, F["floor_out"] + 0.0003, F["floor_out"] + 0.0012)
    sole_col = mix(sole_col, col(0.15, 0.125, 0.095), 0.55 * ground * S(fbm(P, 0.003, 2, seed + 10), 0.4, 0.7))
    base = mix(base, sole_col, sole)
    rough = rough * (1 - sole) + (0.8 + 0.1 * ground) * sole
    h = h + sole * 0.06 * (fbm(P, 0.0012, 2, seed + 11) - 0.5)
    # seams: heel (back) and medial side, butted and stitched with dark waxed linen
    for dist in (F["dback"], F["dside"]):
        on = S(z, F["floor_out"] + 0.005, F["floor_out"] + 0.008) * (z < F["top_z"] - 0.002)
        groove = np.exp(-(dist / 0.00045) ** 2) * on
        ph = z / 0.0042 + 0.15 * fbm(P, 0.01, 2, seed + 12)
        stitch = np.exp(-(((ph - np.floor(ph)) - 0.5) / 0.17) ** 2) * np.exp(-(np.abs(dist) / 0.0016) ** 2) * on
        h = h - 0.14 * groove + 0.12 * stitch
        base = mix(base, col(0.042, 0.03, 0.02), S(stitch, 0.35, 0.75))
        base = base * (1 - 0.3 * groove)[:, None]
    # tongue (inset between the flaps) is a little darker; flap edges are darkened cut edges
    tg = F["tongue"]
    base = mix(base, base * 0.82, tg)
    fe = np.exp(-((tg - 0.5) / 0.18) ** 2)
    base = base * (1 - 0.5 * fe)[:, None]
    # awl-punched lace holes
    hd = np.full(len(P), 1.0)
    for hp in F["holes"]:
        hd = np.minimum(hd, np.linalg.norm(P - hp, axis=1))
    hole = 1 - S(hd, 0.0010, 0.0014)
    rim = np.exp(-((hd - 0.0015) / 0.0005) ** 2)
    base = base * (1 - 0.92 * hole)[:, None] * (1 - 0.25 * rim)[:, None]
    h = h - 0.6 * hole + 0.05 * rim
    rough = rough + 0.4 * hole
    # grime along the sole seam
    grime = (1 - S(z, F["floor_out"] + 0.003, F["floor_out"] + 0.014)) * (1 - sole)
    grime = grime * S(fbm(P, 0.01, 2, seed + 13), 0.3, 0.7)
    base = base * (1 - 0.3 * grime)[:, None]
    rough = rough + 0.1 * grime
    return base, h, rough


def turn_rim(F, seed):
    P = F["pos"]
    m = fbm(P, 0.004, 2, seed)
    return col(0.085, 0.05, 0.028) * (0.85 + 0.3 * m)[:, None], 0.02 * m, 0.55 + 0.1 * m


def plant_sole(F, seed):
    """Twined sagebrush-bark sole: weft rows across the foot, each a pair of twisted strands
    crossing the warps; the corded rim is wrapped by the wefts; worn and darkened underfoot."""
    P = F["pos"]
    kind = F["kind"]
    row = F["row"]
    fr = row - np.floor(row)
    across = np.sin(np.pi * fr)
    rid = np.floor(row)
    x = P[:, 0] * F["sgn"]
    slant = np.where(np.mod(rid, 2) == 0, 1.0, -1.0)
    tw = (x / 0.0052 + slant * (fr - 0.5) * 1.3)
    twf = tw - np.floor(tw)
    strand = np.sin(np.pi * twf) ** 0.6
    fib = fbm(P * np.array([1.0, 3.0, 1.0]), 0.0008, 2, seed)
    shred = fbm(P * np.array([4.0, 1.0, 1.0]), 0.0025, 3, seed + 1)
    h = 0.55 * across ** 0.7 + 0.25 * strand * across + 0.12 * fib
    tone = fbm(P, 0.02, 3, seed + 2)
    base = mix(col(0.17, 0.145, 0.105), col(0.29, 0.245, 0.175), S(tone + 0.25 * (shred - 0.5), 0.3, 0.7))
    base = base * (0.72 + 0.38 * strand * across)[:, None] * (0.9 + 0.2 * fib)[:, None]
    gap = 1 - S(across, 0.08, 0.3)
    base = mix(base, col(0.075, 0.06, 0.045), 0.7 * gap)
    rough = 0.86 + 0.08 * gap
    # rim: wefts wrapping the edge warp (param u along the outline)
    rim = kind > 0.5
    rim &= kind < 1.5
    ru = F["pu"] / 0.0068 + F["pv"] / 0.006
    rfr = ru - np.floor(ru)
    wrap = np.sin(np.pi * rfr) ** 0.8
    h = np.where(rim, 0.5 * wrap + 0.12 * fib, h)
    base = np.where(rim[:, None], mix(col(0.16, 0.135, 0.095), col(0.27, 0.225, 0.16), S(tone, 0.3, 0.7))
                    * (0.7 + 0.4 * wrap)[:, None], base)
    # wear: top compressed and darkened under the heel and ball; bottom soiled
    top = kind > 1.5
    y = P[:, 1]
    press = (np.exp(-((y - F["heel_y"]) / 0.03) ** 2) + np.exp(-((y - F["ball_y"]) / 0.03) ** 2)) * top
    base = base * (1 - 0.35 * np.clip(press, 0, 1))[:, None]
    rough = rough - 0.12 * np.clip(press, 0, 1)
    h = h * (1 - 0.4 * np.clip(press, 0, 1))
    bot = kind < 0.5
    soil = bot * S(fbm(P, 0.01, 3, seed + 3), 0.3, 0.6)
    base = mix(base, col(0.13, 0.105, 0.078), 0.6 * soil)
    return base, h, rough


def cord(F, seed):
    """Two-ply S-twisted plant-fibre cord."""
    P = F["pos"]
    u, v = F["pu"], F["pv"]
    per = np.maximum(F["per"], 1e-4)
    ph = v / 0.0075 + 2 * u / per
    frp = ph - np.floor(ph)
    ply = np.sin(np.pi * frp) ** 0.7
    fib = fbm(P * np.array([2.0, 2.0, 2.0]), 0.0006, 2, seed)
    tone = fbm(P, 0.03, 3, seed + 1)
    base = mix(col(0.18, 0.155, 0.11), col(0.30, 0.255, 0.18), S(tone, 0.3, 0.7))
    base = base * (0.65 + 0.45 * ply)[:, None] * (0.9 + 0.2 * fib)[:, None]
    h = 0.35 * ply + 0.1 * fib
    return base, h, 0.87 - 0.05 * ply


# ===================================================== modern styles (MD pairs)
# Canvas sneakers: cotton duck, vulcanised rubber, flat cotton lace. Ankle boots: polished
# aniline calf, welt and stacked sole, waxed round lace. Albedo targets (linear): off-white duck
# 0.59-0.66, white foxing rubber ~0.58, gum rubber (0.30, 0.17, 0.07), cognac calf ~(0.19, 0.075, 0.025)
# burnished to ~(0.08, 0.03, 0.012), dark edge dressing ~0.05.

def stitch_row(dist, along, offset, pitch, half_w=0.00042, duty=0.68):
    """A row of lock stitches ``offset`` from a seam line: (thread mask, needle-hole pits)."""
    ph = along / pitch
    fr = ph - np.floor(ph)
    dash = S(fr, 0.0, 0.07) * (1 - S(fr, duty - 0.07, duty))
    across = np.exp(-((dist - offset) / half_w) ** 2)
    pit = np.exp(-((dist - offset) / (0.8 * half_w)) ** 2) * np.exp(-((fr - 0.5 * (1 + duty)) / 0.05) ** 2)
    return dash * across, pit


def _u_seam(F):
    """Distance from, and position along, the U round the lacing slit (eyestays + its bottom)."""
    dx = np.maximum(F["dslit"], 0.0)
    dy = np.maximum(F["sfr"] - F["slit_len"], 0.0)
    d = np.sqrt(dx * dx + dy * dy)
    along = np.where(dy > 0, F["slit_len"] + np.arctan2(dy, np.maximum(dx, 1e-5)) * np.maximum(d, 0.002), F["sfr"])
    return d, along


def _holes_dist(P, holes):
    hd = np.full(len(P), 1.0)
    for hp in holes:
        hd = np.minimum(hd, np.linalg.norm(P - hp, axis=1))
    return hd


def _sneaker_zfox(F):
    y = F["pos"][:, 1]
    return 0.0095 + 0.0015 * S(y, F["heel_y"] - 0.05, F["heel_y"])


def _cap(F):
    P = F["pos"]
    return np.sqrt(((P[:, 0] - F["toe_x"]) / F["cap_ax"]) ** 2 + ((P[:, 1] - F["tip_y"]) / F["cap_ay"]) ** 2)


def white_rubber(F, seed):
    P = F["pos"]
    tone = fbm(P, 0.05, 3, seed)
    alb = mix(col(0.585, 0.570, 0.528), col(0.615, 0.600, 0.560), S(tone, 0.3, 0.7))
    stip = fbm(P, 0.00035, 2, seed + 1)
    return alb * (0.985 + 0.03 * stip)[:, None], 0.025 * stip, 0.52 + 0.06 * stip


def canvas(F, seed):
    """Off-white 12 oz cotton duck: a tight plain weave in the flattened panel (Param UV, cm), soft
    flex wrinkles across the vamp, lock-stitched eyestays, collar, heel and quarter seams, a rubber
    toe cap glued over the front, a navy rubber heel label, punched eyelet holes, a little dust."""
    P = F["pos"]
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    h_w, warp_top, tone, slub = T.weave(F["pu"] * 100, F["pv"] * 100, 13.0, 11.5, seed)
    mott = fbm(P, 0.03, 3, seed + 1)
    # broken in: washed-out tone drift, a warmer cast where it has been handled and worn
    drift = fbm(P, 0.045, 3, seed + 9)
    alb = col(0.645, 0.626, 0.584) * (0.95 + 0.10 * (mott - 0.5))[:, None] * (0.94 + 0.06 * tone)[:, None]
    alb = mix(alb, alb * col(0.95, 0.94, 0.90), S(drift, 0.45, 0.75))
    alb = alb * (0.80 + 0.20 * h_w)[:, None]
    h = 0.075 * h_w + 0.02 * slub
    rough = 0.84 + 0.08 * (1 - h_w)
    # flex wrinkles across the vamp and short ones at the throat
    ball = np.exp(-((y - F["ball_y"] - 0.008) / 0.022) ** 2) * S(z, 0.02, 0.035)
    wr = S(ridge(P * np.array([0.25, 1.0, 0.7]), 0.011, 2, seed + 2), 0.55, 0.97) ** 2 * ball
    h = h - 0.10 * wr
    alb = alb * (1 - 0.025 * wr)[:, None]
    # seams and stitching (thread a touch warmer than the duck)
    thread = col(0.60, 0.575, 0.52)
    sts, pits, grooves = np.zeros(len(P)), np.zeros(len(P)), np.zeros(len(P))
    zf = _sneaker_zfox(F)
    above = S(z, zf + 0.0005, zf + 0.002)
    for off in (0.0042,):
        s1, p1 = stitch_row(F["vtop"], F["dback"], off, 0.0029)
        sts += s1; pits += p1
    du, ua = _u_seam(F)
    for off in (0.0022, 0.0046):
        s1, p1 = stitch_row(du, ua, off, 0.0029)
        on = (F["tongue"] < 0.5) & (F["sfr"] > -0.012)
        sts += s1 * on; pits += p1 * on
    db = F["dback"]
    heel_back = above * S(y, F["heel_y"] - 0.07, F["heel_y"] - 0.05)
    lab = (1 - S(db, 0.0105, 0.0115)) * S(z, zf + 0.0035, zf + 0.0045) * (1 - S(z, zf + 0.0165, zf + 0.0175)) * heel_back
    grooves += np.exp(-(db / 0.00055) ** 2) * heel_back * (1 - lab)
    for off in (0.0026,):
        s1, p1 = stitch_row(db, z, off, 0.0029)
        sts += s1 * heel_back * (1 - lab); pits += p1 * heel_back * (1 - lab)
    # quarter over vamp at the side of the foot, slanting back as it rises
    yq = F["ball_y"] + 0.050 + 0.45 * (z - 0.012)
    dq = (y - yq) * 0.91
    side = (F["dslit"] > 0.010) & (z > zf)
    edge = np.exp(-(dq / 0.0005) ** 2) * side
    grooves += edge
    h = h + 0.18 * S(dq, -0.0003, 0.0003) * side
    for off in (0.0016, 0.0036):
        s1, p1 = stitch_row(dq, z + 0.3 * y, off, 0.0029)
        sts += s1 * side; pits += p1 * side
    sts = np.clip(sts, 0, 1)
    h = h + 0.13 * sts - 0.10 * pits - 0.12 * grooves
    alb = mix(alb, thread * (0.92 + 0.1 * fbm(P, 0.0008, 2, seed + 3))[:, None], S(sts, 0.3, 0.7))
    alb = alb * (1 - 0.3 * pits - 0.18 * np.clip(grooves, 0, 1))[:, None]
    # rubber toe cap and heel label
    e = _cap(F)
    cap = 1 - S(e, 0.988, 1.0)
    ra, rh, rr = white_rubber(F, seed + 4)
    alb = mix(alb, ra, cap)
    h = h * (1 - cap) + (rh + 0.22) * cap - 0.10 * np.exp(-((e - 1.0) / 0.006) ** 2)
    rough = rough * (1 - cap) + rr * cap
    rim = lab * (1 - (1 - S(db, 0.0088, 0.0095)) * S(z, zf + 0.0052, zf + 0.006) * (1 - S(z, zf + 0.0148, zf + 0.0156)))
    alb = mix(alb, col(0.024, 0.034, 0.078) * (0.95 + 0.1 * fbm(P, 0.002, 2, seed + 5))[:, None], lab)
    h = h + 0.25 * lab + 0.08 * rim
    rough = rough * (1 - lab) + (0.5 - 0.08 * rim) * lab
    # eyelet holes punched through the eyestay
    hd = _holes_dist(P, F["holes"])
    hole = 1 - S(hd, 0.0017, 0.0021)
    alb = alb * (1 - 0.93 * hole)[:, None]
    h = h - 0.6 * hole
    # dust along the foxing line and on the toe, a faint grey smudge or two
    dust = (1 - S(z - zf, 0.0, 0.016)) * S(fbm(P, 0.010, 3, seed + 6), 0.38, 0.68) * (1 - cap)
    smudge = S(fbm(P, 0.03, 3, seed + 7), 0.62, 0.8)
    # grime ground into the weave (dark in the valleys, the thread tops cleaner)
    valley = (1 - h_w) * S(fbm(P, 0.02, 3, seed + 10), 0.35, 0.7)
    toe_g = S(-(y - (F["toe_y"] + 0.06)), 0.0, 0.05)
    alb = mix(alb, col(0.40, 0.375, 0.335), 0.45 * dust + 0.18 * smudge + 0.10 * valley * (0.4 + toe_g))
    alb = mix(alb, col(0.37, 0.35, 0.315), 0.22 * toe_g * S(fbm(P, 0.008, 3, seed + 11), 0.42, 0.7) * (1 - cap))
    # the rubber toe cap: scuffed grey at the front, a scrape or two
    capd = cap * S(fbm(P * np.array([1.0, 0.5, 1.0]), 0.004, 3, seed + 8), 0.55, 0.8) * toe_g
    alb = mix(alb, col(0.44, 0.425, 0.395), 0.3 * capd)
    rough = rough + 0.04 * dust + 0.05 * capd
    return alb, h, rough


def canvas_lining(F, seed):
    """Cotton drill lining (a 2/1 twill) and a grey-beige sockliner worn darker at heel and ball."""
    P = F["pos"]
    u, v = F["pu"] * 100, F["pv"] * 100
    tw = u * 16 + v * 16
    rib = 0.5 + 0.5 * np.cos(2 * np.pi * tw / 1.5)
    alb = col(0.56, 0.548, 0.515) * (0.9 + 0.1 * rib)[:, None] * (0.96 + 0.06 * fbm(P, 0.02, 3, seed))[:, None]
    h = 0.05 * rib
    z, y = P[:, 2], P[:, 1]
    floor = 1 - S(z, 0.0008, 0.0025)
    wear = np.exp(-((y - F["heel_y"] + 0.035) / 0.03) ** 2) + np.exp(-((y - F["ball_y"]) / 0.03) ** 2)
    sock = col(0.47, 0.445, 0.405) * (1 - 0.15 * np.clip(wear, 0, 1))[:, None] * (0.95 + 0.08 * fbm(P, 0.003, 2, seed + 1))[:, None]
    alb = mix(alb, sock, floor)
    return alb, h * (1 - floor) + 0.04 * fbm(P, 0.0012, 2, seed + 2) * floor, 0.88 + 0.04 * floor


def canvas_rim(F, seed):
    """Collar binding: the duck folded over the top edge, a little greyed by handling."""
    h_w, _, tone, _ = T.weave(F["pu"] * 100, F["pv"] * 100, 15.0, 13.0, seed)
    alb = col(0.60, 0.582, 0.545) * (0.85 + 0.15 * h_w)[:, None] * (0.95 + 0.05 * tone)[:, None]
    return alb, 0.05 * h_w, 0.86 + 0.04 * (1 - h_w)


def vulc_rubber(F, seed):
    """Vulcanised unit: gum outsole with a diamond tread and a smooth border; white foxing with a
    gum edge band, a red line and a navy pinstripe; grime low on the wall, scuffs at toe and heel."""
    P = F["pos"]
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    kind, zr, frac = F["kind"], F["zrel"], F["frac"]
    alb, h, rough = white_rubber(F, seed)
    gum_t = fbm(P, 0.02, 3, seed + 1)
    gum = mix(col(0.28, 0.150, 0.058), col(0.34, 0.19, 0.075), S(gum_t, 0.3, 0.7))
    wall = (kind > 0.5) & (kind < 1.5)
    bot = kind < 0.5
    band = wall & (zr < 0.0036)
    alb = np.where(band[:, None], gum, alb)
    h = h - 0.16 * np.exp(-((zr - 0.0036) / 0.00028) ** 2) * wall
    navy = wall * S(zr, 0.0112, 0.01135) * (1 - S(zr, 0.0132, 0.01335))
    red = wall * S(zr, 0.0092, 0.00935) * (1 - S(zr, 0.0099, 0.01005))
    alb = mix(alb, col(0.022, 0.032, 0.080), navy)
    alb = mix(alb, col(0.30, 0.030, 0.026), red)
    rough = np.where(band, 0.48, rough)
    # mould parting line and fine knurl on the foxing above the stripes
    h = h - 0.05 * np.exp(-((zr - 0.0060) / 0.0002) ** 2) * wall
    # outsole: diamond tread under a 5 mm smooth border, dust packed in the grooves, worn flat
    a = (x + y) / 0.0042
    b = (x - y) / 0.0042
    dd = np.minimum(np.abs(a - np.floor(a) - 0.5), np.abs(b - np.floor(b) - 0.5))
    groove = (1 - S(dd, 0.06, 0.13)) * (1 - S(frac, 0.84, 0.88))
    wearz = np.clip(np.exp(-((y - F["ball_y"]) / 0.035) ** 2) + np.exp(-((y - F["heel_y"] + 0.03) / 0.03) ** 2), 0, 1)
    gb = gum * (1 - 0.35 * groove)[:, None]
    gb = mix(gb, col(0.33, 0.29, 0.23), (0.55 * groove + 0.2 * wearz * S(fbm(P, 0.004, 2, seed + 2), 0.4, 0.7))[:, None])
    alb = np.where(bot[:, None], gb, alb)
    h = np.where(bot, -0.5 * groove * (1 - 0.5 * wearz) + 0.04 * fbm(P, 0.0006, 2, seed + 3), h)
    rough = np.where(bot, 0.62 + 0.15 * groove + 0.1 * wearz, rough)
    # grime low on the wall, scuffs at the toe and heel
    grime = wall * (1 - S(zr, 0.002, 0.014)) * S(fbm(P, 0.01, 3, seed + 4), 0.28, 0.65)
    alb = mix(alb, col(0.29, 0.26, 0.215), 0.55 * grime)
    # the foxing yellows a touch with wear, greyer toward the toe where it meets the ground
    toe_w = wall * S(-(y - (F["toe_y"] + 0.04)), 0.0, 0.04)
    alb = mix(alb, col(0.45, 0.43, 0.39), 0.35 * toe_w * S(fbm(P, 0.007, 2, seed + 7), 0.35, 0.7) * (1 - S(zr, 0.004, 0.014)))
    alb = alb * (1 - 0.04 * S(fbm(P, 0.04, 2, seed + 8), 0.4, 0.8))[:, None] * col(1.0, 0.99, 0.965)
    tip = S(-(y - F["toe_y"] - 0.01), 0.0, 0.02) + S(y - F["heel_y"], -0.01, 0.006)
    scuff = S(fbm(P * np.array([1.0, 0.4, 1.0]), 0.004, 3, seed + 5), 0.66, 0.82) * np.clip(tip, 0, 1) * wall * (1 - 0.8 * (navy + red))
    alb = mix(alb, col(0.42, 0.405, 0.375), 0.5 * scuff)
    rough = rough + 0.12 * scuff + 0.06 * grime
    return alb, h, rough


def flat_lace(F, seed):
    """7 mm flat cotton lace (tubular braid, chevrons), clear-sealed aglets at the ends."""
    P = F["pos"]
    u, v = F["pu"], F["pv"]
    per = np.maximum(F["per"], 1e-4)
    ang = u / per * 2 * np.pi
    ph = v / 0.00065 + 1.2 * np.abs(np.sin(ang))
    rib = np.sin(np.pi * (ph - np.floor(ph))) ** 0.8
    fib = fbm(P, 0.0004, 2, seed)
    alb = col(0.625, 0.612, 0.578) * (0.90 + 0.10 * rib)[:, None] * (0.95 + 0.07 * fib)[:, None]
    edge = np.abs(np.cos(ang)) ** 8
    alb = alb * (1 - 0.10 * edge)[:, None]
    h = 0.05 * rib + 0.03 * fib
    rough = 0.86 - 0.04 * rib
    ag = S(F["aglet"], 0.3, 0.6)
    alb = mix(alb, col(0.50, 0.49, 0.46), ag)
    h = h * (1 - ag) + 0.05 * np.sin(v / 0.0012) * ag
    rough = rough * (1 - ag) + 0.24 * ag
    return alb, h, rough


def nickel(F, seed):
    """Rolled eyelets (no metallic channel in this material set: a bright grey, polished dielectric
    with tarnish in the roll)."""
    P = F["pos"]
    tarn = S(fbm(P, 0.0015, 2, seed), 0.45, 0.75)
    u = F["pu"] / np.maximum(F["per"], 1e-4)
    roll = np.abs(np.cos(u * 2 * np.pi)) ** 4
    alb = mix(col(0.42, 0.42, 0.43), col(0.22, 0.215, 0.21), 0.5 * tarn + 0.3 * roll)
    return alb, 0.02 * tarn, 0.24 + 0.12 * tarn


def calf(F, seed):
    """Cognac aniline calf, polished: tonal mottling, very fine pores, burnished darker toe and
    heel, creases across the vamp and the front of the ankle, lapped quarters, backstay and heel
    counter double-stitched in cream, eyestays stitched round the lacing, punched eyelet holes."""
    P = F["pos"]
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    tone = fbm(P, 0.05, 4, seed)
    mott = fbm(P, 0.009, 3, seed + 1)
    alb = mix(col(0.150, 0.058, 0.019), col(0.215, 0.088, 0.030), S(tone, 0.3, 0.72))
    alb = alb * (0.90 + 0.2 * mott)[:, None]
    f1, _, _, _ = cells(P, 0.00026, seed + 2)
    pore = 1 - S(f1, 0.0, 0.2)
    peel = fbm(P, 0.0011, 2, seed + 3)
    sheen = fbm(P, 0.006, 3, seed + 9)
    h = -0.014 * pore + 0.035 * (peel - 0.5) + 0.05 * (sheen - 0.5)
    rough = 0.40 + 0.06 * pore + 0.10 * (sheen - 0.5) + 0.04 * (mott - 0.5)
    # burnish: antique darkening at the toe, the heel and high on the shaft back
    toe = S(-(y - (F["toe_y"] + 0.055)), 0.0, 0.05)
    heel = S(y - (F["heel_y"] - 0.035), 0.0, 0.03) * (1 - S(z, 0.05, 0.10))
    burn = np.clip(toe + 0.7 * heel, 0, 1) * (0.75 + 0.25 * fbm(P, 0.02, 2, seed + 4))
    alb = mix(alb, col(0.068, 0.026, 0.011), 0.7 * burn)
    rough = rough - 0.07 * burn
    # creases: a few soft, broad flex lines across the vamp and the front of the ankle
    ball = np.exp(-((y - F["ball_y"] - 0.006) / 0.016) ** 2) * S(z, 0.025, 0.04)
    wv = (y - F["ball_y"]) / 0.0075 + 1.8 * fbm(P * np.array([0.3, 1, 1]), 0.02, 2, seed + 5)
    cr = (0.5 + 0.5 * np.cos(2 * np.pi * wv)) ** 6 * ball
    ank = np.exp(-((z - 0.074) / 0.012) ** 2) * S(F["dback"], 0.05, 0.08)
    wa = z / 0.0065 + 1.5 * fbm(P * np.array([1, 1, 0.3]), 0.02, 2, seed + 6)
    cr2 = (0.5 + 0.5 * np.cos(2 * np.pi * wa)) ** 6 * ank
    h = h - 0.08 * cr - 0.06 * cr2
    alb = alb * (1 - 0.06 * cr - 0.05 * cr2)[:, None]
    rough = rough + 0.06 * cr + 0.05 * cr2
    thread = col(0.30, 0.175, 0.085)
    sts, pits, grooves = np.zeros(len(P)), np.zeros(len(P)), np.zeros(len(P))
    not_tongue = F["tongue"] < 0.5
    # topline (two rows)
    for off in (0.0026, 0.0045):
        s1, p1 = stitch_row(F["vtop"], F["dback"], off, 0.0026, half_w=0.00028)
        sts += s1 * not_tongue; pits += p1 * not_tongue
    # eyestays round the lacing slit
    du, ua = _u_seam(F)
    for off in (0.0022, 0.0040):
        s1, p1 = stitch_row(du, ua, off, 0.0026, half_w=0.00028)
        on = not_tongue & (F["sfr"] > -0.02)
        sts += s1 * on; pits += p1 * on
    edge_slit = np.exp(-(F["dslit"] / 0.0005) ** 2) * (F["sfr"] < F["slit_len"])
    # quarters lapped over the vamp: a line from the foot of the lacing down and back to the welt
    A = F["slit_end"]
    B = np.array([A[0], F["ball_y"] + 0.040, 0.004])
    tq = np.array([0.0, B[1] - A[1], B[2] - A[2]]); tq /= np.linalg.norm(tq)
    nq = np.array([0.0, tq[2], -tq[1]])
    if nq[1] < 0:
        nq = -nq                                         # +: behind the line (the quarter)
    rel = P - A
    dq = rel @ nq
    aq = rel @ tq
    sideq = (F["dslit"] > 0.004) & (aq > -0.01)
    lap = S(dq, -0.0003, 0.0003) * sideq
    h = h + 0.22 * lap
    grooves += np.exp(-(dq / 0.0005) ** 2) * sideq
    for off in (0.0017, 0.0035):
        s1, p1 = stitch_row(dq, aq, off, 0.0026, half_w=0.00028)
        sts += s1 * sideq; pits += p1 * sideq
    # backstay strip and heel counter
    db = F["dback"]
    zc = 0.052 * S(y, F["heel_y"] - 0.10, F["heel_y"] - 0.035) + 0.010
    dc = z - zc
    rear = y > F["heel_y"] - 0.105
    bs = (z > zc - 0.002) & (y > F["heel_y"] - 0.06)
    grooves += np.exp(-(db / 0.0005) ** 2) * bs + np.exp(-((db - 0.0082) / 0.0005) ** 2) * bs
    h = h + 0.2 * (1 - S(db, 0.0079, 0.0085)) * bs
    for off in (0.0062,):
        s1, p1 = stitch_row(db, z, off, 0.0026, half_w=0.00028)
        sts += s1 * bs; pits += p1 * bs
    grooves += np.exp(-(dc / 0.0005) ** 2) * rear
    h = h + 0.2 * (1 - S(dc, -0.0003, 0.0003)) * rear
    for off in (-0.0017, -0.0035):
        s1, p1 = stitch_row(dc, y + db, off, 0.0026, half_w=0.00028)
        sts += s1 * rear; pits += p1 * rear
    sts = np.clip(sts, 0, 1)
    h = h + 0.08 * sts - 0.08 * pits - 0.10 * np.clip(grooves, 0, 1)
    alb = mix(alb, thread * (0.9 + 0.15 * fbm(P, 0.0008, 2, seed + 7))[:, None], 0.85 * S(sts, 0.3, 0.7))
    alb = alb * (1 - 0.35 * pits - 0.3 * np.clip(grooves + edge_slit, 0, 1))[:, None]
    rough = rough + 0.12 * sts
    # seams and edges take the burnish too
    alb = mix(alb, alb * 0.72, 0.5 * np.clip(grooves * 3, 0, 1) * S(fbm(P, 0.004, 2, seed + 10), 0.3, 0.7))
    # tongue a shade darker and unstitched
    tg = F["tongue"]
    alb = mix(alb, alb * 0.82, tg)
    # eyelet holes
    hd = _holes_dist(P, F["holes"])
    hole = 1 - S(hd, 0.0012, 0.0015)
    alb = alb * (1 - 0.93 * hole)[:, None]
    h = h - 0.5 * hole
    # a little dust where the upper meets the welt
    dust = (1 - S(z, 0.0, 0.012)) * S(fbm(P, 0.012, 3, seed + 8), 0.5, 0.75)
    alb = mix(alb, col(0.20, 0.15, 0.11), 0.3 * dust)
    rough = rough + 0.08 * dust
    return alb, h, rough


def calf_lining(F, seed):
    """Natural vegetable-tanned lining and a pale leather insole darkened where she stands."""
    P = F["pos"]
    z, y = P[:, 2], P[:, 1]
    m = fbm(P, 0.02, 3, seed)
    alb = mix(col(0.30, 0.205, 0.125), col(0.36, 0.25, 0.155), S(m, 0.3, 0.7)) * (0.95 + 0.08 * fbm(P, 0.0008, 2, seed + 1))[:, None]
    floor = 1 - S(z, 0.0008, 0.0025)
    wear = np.clip(np.exp(-((y - F["heel_y"] + 0.035) / 0.03) ** 2) + np.exp(-((y - F["ball_y"]) / 0.03) ** 2), 0, 1)
    alb = alb * (1 - 0.25 * wear * floor)[:, None]
    return alb, 0.03 * fbm(P, 0.0012, 2, seed + 2), 0.55 + 0.1 * floor


def calf_rim(F, seed):
    P = F["pos"]
    m = fbm(P, 0.004, 2, seed)
    return col(0.050, 0.026, 0.014) * (0.85 + 0.3 * m)[:, None], 0.02 * m, 0.34 + 0.08 * m


def welt_sole(F, seed):
    """Welt, sole edge and stacked heel, dressed dark brown and burnished; tan welt stitching
    with dot-wheel marks; a natural leather sole worn at the ball and heel; rubber top-piece."""
    P = F["pos"]
    x, y, z = P[:, 0], P[:, 1], P[:, 2]
    kind, zr, frac = F["kind"], F["zrel"], F["frac"]
    wall = (kind > 0.5) & (kind < 1.5)
    top = kind >= 1.5
    bot = kind < 0.5
    heel = y > F["breast_y"]
    m = fbm(P, 0.008, 3, seed)
    edge_col = mix(col(0.042, 0.023, 0.013), col(0.065, 0.036, 0.020), S(m, 0.3, 0.7))
    alb = edge_col.copy()
    h = 0.02 * fbm(P, 0.0006, 2, seed + 1)
    rough = 0.33 + 0.08 * (m - 0.5)
    # welt / outsole junction and the stacked lifts of the heel
    y0, y1, rise = F["lift_p"]                     # the welt rises with the toe spring
    jz = F["z_welt_bot"] + rise * S(-(y - y0), 0.0, y0 - y1) ** 1.3
    h = h - 0.25 * np.exp(-((z - jz) / 0.0003) ** 2) * wall
    lift = (z - F["z_bot"]) / 0.0019
    li = np.floor(lift)
    lfr = lift - li
    ltone = T.hash1(li, seed + 2)
    stack = wall & heel & (z < jz)
    stack_col = mix(col(0.085, 0.048, 0.025), col(0.13, 0.078, 0.042), ltone[:, None])
    stack_col = stack_col * (0.9 + 0.2 * fbm(P * np.array([1, 1, 4.0]), 0.004, 2, seed + 3))[:, None]
    line = np.exp(-((lfr - 0.0) / 0.06) ** 2) + np.exp(-((lfr - 1.0) / 0.06) ** 2)
    alb = np.where(stack[:, None], stack_col * (1 - 0.5 * line)[:, None], alb)
    h = h - 0.08 * line * stack
    rough = np.where(stack, 0.45, rough)
    tp = wall & heel & (zr < 0.0028)
    alb = np.where(tp[:, None], col(0.030, 0.028, 0.026), alb)
    rough = np.where(tp, 0.72, rough)
    # welt top: stitch row with dot-wheel impressions between the stitches
    across = np.clip((kind - 2.0) / 0.9, 0, 1)
    s1, p1 = stitch_row(across * 0.003, F["perim"], 0.0014, 0.0030, half_w=0.00032, duty=0.62)
    s1 = s1 * top
    alb = mix(alb, col(0.20, 0.12, 0.06), 0.8 * S(s1, 0.3, 0.7))
    h = h + 0.10 * s1 - 0.08 * p1 * top
    alb = alb * (1 - 0.3 * p1 * top)[:, None]
    rough = rough + 0.12 * s1
    # sole bottom
    sole = mix(col(0.22, 0.14, 0.08), col(0.28, 0.185, 0.11), S(fbm(P, 0.02, 3, seed + 4), 0.3, 0.7))
    wearz = np.clip(np.exp(-((y - F["ball_y"]) / 0.04) ** 2) + np.exp(-((y - F["toe_y"] - 0.02) / 0.03) ** 2), 0, 1)
    scr = S(ridge(P * np.array([1.0, 0.35, 1.0]), 0.003, 2, seed + 5), 0.9, 1.0) * wearz
    sole = mix(sole, col(0.12, 0.085, 0.06), (0.35 * wearz * S(fbm(P, 0.005, 2, seed + 6), 0.35, 0.7))[:, None])
    sole = mix(sole, col(0.33, 0.25, 0.17), (0.5 * scr)[:, None])
    rim_band = S(frac, 0.90, 0.94)
    sole = mix(sole, edge_col, rim_band[:, None])
    chan = np.exp(-((frac - 0.875) / 0.006) ** 2)
    sole = sole * (1 - 0.25 * chan)[:, None]
    rubber = col(0.032, 0.030, 0.028) * (0.9 + 0.2 * fbm(P, 0.003, 2, seed + 7))[:, None]
    hb = bot & heel
    sole = np.where(heel[:, None], mix(rubber, edge_col, S(frac, 0.95, 0.99)[:, None]), sole)
    alb = np.where(bot[:, None], sole, alb)
    h = np.where(bot, 0.03 * fbm(P, 0.0008, 2, seed + 8) - 0.08 * chan - 0.04 * scr, h)
    rough = np.where(bot, np.where(heel, 0.75, 0.62 - 0.1 * wearz), rough)
    h = np.where(hb, h + 0.04 * fbm(P, 0.0005, 2, seed + 9), h)
    # scuffed toe edge
    sc = wall * S(-(y - F["toe_y"]), -0.005, 0.01) * S(ridge(P, 0.002, 2, seed + 10), 0.9, 1.0)
    alb = mix(alb, col(0.13, 0.085, 0.05), 0.5 * sc)
    return alb, h, rough


def round_lace(F, seed):
    """2.6 mm waxed cotton lace, dark brown, with its two plies and a waxy sheen; dark aglets."""
    P = F["pos"]
    u, v = F["pu"], F["pv"]
    per = np.maximum(F["per"], 1e-4)
    ph = v / 0.0021 + 2 * u / per
    ply = np.sin(np.pi * (ph - np.floor(ph))) ** 0.7
    alb = col(0.052, 0.031, 0.018) * (0.75 + 0.35 * ply)[:, None] * (0.92 + 0.12 * fbm(P, 0.0005, 2, seed))[:, None]
    h = 0.12 * ply
    rough = 0.48 - 0.08 * ply
    ag = S(F["aglet"], 0.3, 0.6)
    alb = mix(alb, col(0.036, 0.030, 0.026), ag)
    return alb, h * (1 - ag), rough * (1 - ag) + 0.22 * ag


def brass(F, seed):
    """Small antique-brass eyelets (dielectric stand-in: dark gold, glossy, tarnished in the roll)."""
    P = F["pos"]
    tarn = S(fbm(P, 0.0012, 2, seed), 0.4, 0.75)
    u = F["pu"] / np.maximum(F["per"], 1e-4)
    roll = np.abs(np.cos(u * 2 * np.pi)) ** 4
    alb = mix(col(0.30, 0.205, 0.085), col(0.11, 0.075, 0.035), 0.55 * tarn + 0.3 * roll)
    return alb, 0.02 * tarn, 0.26 + 0.12 * tarn


KINDS = {
    PR.SUEDE: suede, PR.FUR_LINING: lambda F, s: wool(F, s, clean=0.6), PR.CUFF: cuff, PR.THONG: thong,
    PR.TURN_OUTER: turn_outer, PR.TURN_INNER: flesh_side, PR.TURN_RIM: turn_rim,
    PR.LACE: lambda F, s: thong(F, s, color=(0.14, 0.08, 0.042)), PR.SANDAL_SOLE: plant_sole, PR.CORD: cord,
    MD.SNK_CANVAS: canvas, MD.SNK_LINING: canvas_lining, MD.SNK_RIM: canvas_rim, MD.SNK_RUBBER: vulc_rubber,
    MD.SNK_LACE: flat_lace, MD.SNK_EYELET: nickel,
    MD.BOOT_CALF: calf, MD.BOOT_LINING: calf_lining, MD.BOOT_RIM: calf_rim, MD.BOOT_SOLE: welt_sole,
    MD.BOOT_LACE: round_lace, MD.BOOT_EYELET: brass,
}
DENSITY = {PR.FUR_LINING: 0.3, PR.TURN_INNER: 0.4, PR.TURN_RIM: 1.0, PR.CUFF: 1.0, PR.SUEDE: 1.0,
           PR.THONG: 1.0, PR.LACE: 1.8, PR.CORD: 1.4, PR.SANDAL_SOLE: 1.0, PR.TURN_OUTER: 1.0,
           MD.SNK_CANVAS: 1.35, MD.SNK_LINING: 0.3, MD.SNK_RIM: 1.35, MD.SNK_RUBBER: 0.75, MD.SNK_LACE: 1.2,
           MD.SNK_EYELET: 1.0, MD.BOOT_CALF: 1.3, MD.BOOT_LINING: 0.3, MD.BOOT_RIM: 1.3, MD.BOOT_SOLE: 0.75,
           MD.BOOT_LACE: 1.2, MD.BOOT_EYELET: 1.0}
# brightest linear albedo a part may reach (white canvas, rubber and lace are allowed past 0.6)
ALBEDO_MAX = {MD.SNK_CANVAS: 0.7, MD.SNK_LINING: 0.7, MD.SNK_RIM: 0.7, MD.SNK_RUBBER: 0.7, MD.SNK_LACE: 0.7}


# ------------------------------------------------------------------ plumbing

def _loops(me, layer):
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers[layer].data.foreach_get("uv", uv)
    return uv.reshape(-1, 2)


UNWRAP_PARTS = (PR.FUR_LINING, PR.SUEDE, PR.CUFF, PR.TURN_OUTER, PR.TURN_INNER, PR.TURN_RIM,
                MD.SNK_CANVAS, MD.SNK_LINING, MD.SNK_RIM, MD.BOOT_CALF, MD.BOOT_LINING, MD.BOOT_RIM)


def select_faces(obj, mask):
    """Select exactly the faces in ``mask`` (and their verts/edges) for edit-mode UV operators."""
    me = obj.data
    mask = np.asarray(mask, bool)
    lv = np.empty(len(me.loops), np.int64); me.loops.foreach_get("vertex_index", lv)
    ls = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_start", ls)
    lt = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_total", lt)
    vsel = np.zeros(len(me.vertices), bool)
    vsel[lv[np.repeat(mask, lt)]] = True
    ev = np.empty(len(me.edges) * 2, np.int64); me.edges.foreach_get("vertices", ev); ev = ev.reshape(-1, 2)
    me.vertices.foreach_set("select", vsel.tolist())
    me.edges.foreach_set("select", vsel[ev].all(1).tolist())
    me.polygons.foreach_set("select", mask.tolist())
    bpy.context.scene.tool_settings.mesh_select_mode = (False, False, True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def unwrap_shells(obj):
    """Angle-based unwrap of the left foot's shell parts (lining, upper, cuff, rim), cut at part
    boundaries and along the back/sole centre line (dback = 0), each part scaled to meters."""
    import bmesh
    me = obj.data
    part = np.empty(len(me.polygons), np.int32)
    me.attributes["part"].data.foreach_get("value", part)
    dback = np.empty(len(me.vertices), np.float32)
    me.attributes["dback"].data.foreach_get("value", dback)
    co = np.empty(len(me.vertices) * 3); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3)
    bm = bmesh.new()
    bm.from_mesh(me)
    for e in bm.edges:
        fs = e.link_faces
        a, b = e.verts
        seam = len(fs) == 2 and part[fs[0].index] != part[fs[1].index]
        if not seam and any(part[f.index] in UNWRAP_PARTS for f in fs):
            seam = dback[a.index] < 1e-6 and dback[b.index] < 1e-6
        e.seam = bool(seam)
    bm.to_mesh(me)
    bm.free()
    cen = np.empty(len(me.polygons) * 3); me.polygons.foreach_get("center", cen); cen = cen.reshape(-1, 3)
    sel = np.isin(part, UNWRAP_PARTS) & (cen[:, 0] > 0)
    select_faces(obj, sel)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.uv.unwrap(method="ANGLE_BASED", margin=0.002)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = _loops(me, "UVMap")
    ls = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_start", ls)
    lt = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_total", lt)
    for p in UNWRAP_PARTS:
        faces = np.where(sel & (part == p))[0]
        if not len(faces):
            continue
        a3 = sum(me.polygons[int(f)].area for f in faces)
        idx = np.concatenate([np.arange(ls[f], ls[f] + lt[f]) for f in faces])
        a2 = 0.0
        for f in faces:
            q = uv[ls[f]:ls[f] + lt[f]]
            a2 += 0.5 * abs(np.dot(q[:, 0], np.roll(q[:, 1], -1)) - np.dot(q[:, 1], np.roll(q[:, 0], -1)))
        c0 = uv[idx].mean(0)
        uv[idx] = (uv[idx] - c0) * math.sqrt(a3 / max(a2, 1e-12))
    me.uv_layers["UVMap"].data.foreach_set("uv", uv.ravel())


def prepare_uvs(obj, log=print):
    """Keep the meter-scale parameter UVs in 'Param', weight island density by part, pack."""
    me = obj.data
    if "dback" in me.attributes:
        unwrap_shells(obj)
    param = me.uv_layers.new(name="Param")
    uv = _loops(me, "UVMap")
    param.data.foreach_set("uv", uv.ravel())
    part = np.empty(len(me.polygons), np.int32)
    me.attributes["part"].data.foreach_get("value", part)
    ls = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_start", ls)
    lt = np.empty(len(me.polygons), np.int64); me.polygons.foreach_get("loop_total", lt)
    loop_part = np.repeat(part, lt)
    w = np.array([DENSITY.get(int(p), 1.0) for p in loop_part])
    uv = uv * w[:, None]
    me.uv_layers["UVMap"].data.foreach_set("uv", uv.ravel())
    me.uv_layers.active = me.uv_layers["UVMap"]
    # pack the left foot's islands only; the right foot (appended mirror, same face order,
    # reversed corners) then copies them
    nf = len(me.polygons)
    half = nf // 2
    cen = np.empty(nf * 3); me.polygons.foreach_get("center", cen); cen = cen.reshape(-1, 3)
    assert np.allclose(cen[:half] * np.array([-1, 1, 1]), cen[half:], atol=1e-4), "right foot is not the mirror"
    sel = np.zeros(nf, bool); sel[:half] = True
    select_faces(obj, sel)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.context.scene.tool_settings.use_uv_select_sync = False
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin=0.003, shape_method="CONCAVE")
    bpy.ops.object.mode_set(mode="OBJECT")
    uvp = _loops(me, "UVMap")
    for i in range(half):
        a, n = ls[i], lt[i]
        uvp[ls[i + half]:ls[i + half] + n] = uvp[a:a + n][::-1]
    me.uv_layers["UVMap"].data.foreach_set("uv", uvp.ravel())
    # density per part: UV area / surface area
    dens = {}
    for p in np.unique(part):
        fa, ua = 0.0, 0.0
        for poly in me.polygons:
            if part[poly.index] != p:
                continue
            fa += poly.area
            q = uvp[poly.loop_start:poly.loop_start + poly.loop_total]
            ua += 0.5 * abs(np.dot(q[:, 0], np.roll(q[:, 1], -1)) - np.dot(q[:, 1], np.roll(q[:, 0], -1)))
        dens[int(p)] = math.sqrt(ua / max(fa, 1e-12))          # UV units per meter
    return dens


def rasterize_fields(obj, R):
    me = obj.data
    me.calc_loop_triangles()
    nt = len(me.loop_triangles)
    tl = np.empty(nt * 3, np.int64); me.loop_triangles.foreach_get("loops", tl); tl = tl.reshape(-1, 3)
    tp = np.empty(nt, np.int64); me.loop_triangles.foreach_get("polygon_index", tp)
    lv = np.empty(len(me.loops), np.int64); me.loops.foreach_get("vertex_index", lv)
    uv = _loops(me, "UVMap")
    pu = _loops(me, "Param")
    co = np.empty(len(me.vertices) * 3); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3)
    part = np.empty(len(me.polygons), np.int32); me.attributes["part"].data.foreach_get("value", part)
    names = [a.name for a in me.attributes if a.domain == "POINT" and a.data_type == "FLOAT"
             and not a.name.startswith(".")]
    attrs = {}
    for n in names:
        a = np.empty(len(me.vertices), np.float32)
        me.attributes[n].data.foreach_get("value", a)
        attrs[n] = a
    # perimeter of the tube around each loop's row (for thong/cord patterns): max Param u span per poly island
    chans = [("part", None), ("pu", 0), ("pv", 1), ("x", 0), ("y", 1), ("z", 2)] + [(n, None) for n in names]
    # both feet share UVs (the right is the left's mirror): rasterize the left foot only
    left = co[lv[tl]][..., 0].mean(1) > 0
    tl, tp, nt = tl[left], tp[left], int(left.sum())
    vals = np.zeros((nt, 3, len(chans)), np.float32)
    vals[:, :, 0] = part[tp][:, None]
    vals[:, :, 1] = pu[tl][..., 0]
    vals[:, :, 2] = pu[tl][..., 1]
    vals[:, :, 3:6] = co[lv[tl]]
    for k, n in enumerate(names):
        vals[:, :, 6 + k] = attrs[n][lv[tl]]
    img, cov = T.rasterize(uv[tl], vals, R)
    return img, cov, [c[0] for c in chans]


def bake_ao(obj, R, samples=64):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    from outfit.render import use_gpu
    use_gpu()
    sc.cycles.samples = samples
    img = bpy.data.images.new("_ao", R, R, alpha=False, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    mat = bpy.data.materials.new("_ao_bake")
    if mat.node_tree is None:
        mat.use_nodes = True
    node = mat.node_tree.nodes.new("ShaderNodeTexImage")
    node.image = img
    mat.node_tree.nodes.active = node
    saved = list(obj.data.materials)
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    obj.data.uv_layers.active = obj.data.uv_layers["UVMap"]
    for o in bpy.context.scene.objects:
        o.hide_render = o is not obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    sc.render.bake.margin = 8
    sc.world = sc.world or bpy.data.worlds.new("w")
    bpy.ops.object.bake(type="AO", use_clear=True, margin=8)
    a = np.array(img.pixels[:], np.float32).reshape(R, R, 4)[..., 0]
    for o in bpy.context.scene.objects:
        o.hide_render = False
    obj.data.materials.clear()
    for m in saved:
        obj.data.materials.append(m)
    bpy.data.images.remove(img)
    bpy.data.materials.remove(mat)
    return a


def synthesize(obj, built, out_dir, stem, R=2048, seed=0, extra=None, log=print):
    dens = prepare_uvs(obj, log)
    img, cov, names = rasterize_fields(obj, R)
    img = T.dilate(img, cov, 4)
    covd = cov.copy()
    part = np.rint(img[..., 0]).astype(int)
    work = out_dir.parent / "_work"
    work.mkdir(exist_ok=True)
    T.write_png(work / "uv_parts.png", np.where(cov, 0.2 + 0.8 * (part % 7) / 6.0, 0.0))
    px_per_m = {p: d * R for p, d in dens.items()}
    log(f"  {stem}: coverage {cov.mean():.1%}, px/cm " + ", ".join(f"{p}:{v / 100:.1f}" for p, v in px_per_m.items()))
    albedo = np.zeros((R, R, 3), np.float32)
    height = np.zeros((R, R), np.float32)
    rough = np.full((R, R), 0.85, np.float32)
    texel_mm = np.full((R, R), 1000.0 / max(px_per_m.values()), np.float32)
    ch = {n: i for i, n in enumerate(names)}
    live = cov | (img[..., 0] > 0.5)
    for p, fn in KINDS.items():
        m = live & (part == p)
        if not m.any():
            continue
        sel = img[m]
        F = {"pos": sel[:, 3:6].astype(np.float64), "pu": sel[:, 1].astype(np.float64),
             "pv": sel[:, 2].astype(np.float64)}
        for n in names[6:]:
            F[n] = sel[:, ch[n]].astype(np.float64)
        F.update(extra or {})
        F.setdefault("per", np.full(len(sel), 0.016))
        if "per" in ch:
            F["per"] = sel[:, ch["per"]].astype(np.float64)
        a, h, r = fn(F, seed + p)
        albedo[m] = np.clip(a, 0, ALBEDO_MAX.get(p, 0.6))
        height[m] = h
        rough[m] = np.clip(r, 0.3, 0.98)
        texel_mm[m] = 1000.0 / px_per_m[p]
    albedo = T.dilate(albedo, live, 24)
    height = T.dilate(height, live, 24)
    rough = T.dilate(rough, live, 24)
    nrm = T.height_to_normal(height, 1.0)
    # rescale gradient by per-part texel size
    g = (nrm[..., :2] - 0.5) * 2
    nz = (nrm[..., 2] - 0.5) * 2
    g = g / np.maximum(nz[..., None], 1e-4) / texel_mm[..., None]
    n3 = np.concatenate([g, np.ones((R, R, 1), np.float32)], -1)
    n3 /= np.linalg.norm(n3, axis=-1, keepdims=True)
    nrm = n3 * 0.5 + 0.5
    cav = T.box_blur(height, 3) - height
    cav2 = T.box_blur(height, 12) - height
    ao = np.clip(1 - 0.5 * np.clip(cav, 0, None) - 0.25 * np.clip(cav2, 0, None), 0.45, 1.0)
    geo_ao = bake_ao(obj, R)
    geo_ao = T.dilate(geo_ao, live, 16) if geo_ao.shape == ao.shape else np.ones_like(ao)
    ao = np.clip(ao * (0.25 + 0.75 * geo_ao), 0.2, 1.0)
    paths = {}
    for kind, im in (("basecolor", T.to_srgb(albedo)), ("normal", nrm), ("roughness", rough), ("ao", ao)):
        p = out_dir / f"T_{stem}_{kind}.png"
        T.write_png(p, im)
        paths[kind] = p
    stats = dict(resolution=R, uv_coverage=float(cov.mean()),
                 px_per_cm={str(k): round(v / 100, 1) for k, v in px_per_m.items()},
                 albedo_mean=[float(x) for x in albedo[covd].mean(0)], albedo_max=float(albedo[covd].max()),
                 roughness_mean=float(rough[covd].mean()))
    return paths, stats
