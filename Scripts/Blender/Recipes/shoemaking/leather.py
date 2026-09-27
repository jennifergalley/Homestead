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


KINDS = {
    PR.SUEDE: suede, PR.FUR_LINING: lambda F, s: wool(F, s, clean=0.6), PR.CUFF: cuff, PR.THONG: thong,
    PR.TURN_OUTER: turn_outer, PR.TURN_INNER: flesh_side, PR.TURN_RIM: turn_rim,
    PR.LACE: lambda F, s: thong(F, s, color=(0.14, 0.08, 0.042)), PR.SANDAL_SOLE: plant_sole, PR.CORD: cord,
}
DENSITY = {PR.FUR_LINING: 0.3, PR.TURN_INNER: 0.4, PR.TURN_RIM: 1.0, PR.CUFF: 1.0, PR.SUEDE: 1.0,
           PR.THONG: 1.0, PR.LACE: 1.8, PR.CORD: 1.4, PR.SANDAL_SOLE: 1.0, PR.TURN_OUTER: 1.0}


# ------------------------------------------------------------------ plumbing

def _loops(me, layer):
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers[layer].data.foreach_get("uv", uv)
    return uv.reshape(-1, 2)


UNWRAP_PARTS = (PR.FUR_LINING, PR.SUEDE, PR.CUFF, PR.TURN_OUTER, PR.TURN_INNER, PR.TURN_RIM)


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
        albedo[m] = np.clip(a, 0, 0.6)
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
