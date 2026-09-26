"""Numpy texture synthesis for homespun cloth: plain weave with slub yarns, hand-sewn seams,
rolled hems, gathers, creases and faint stains, rasterized into each garment's UV layout.

All maps are computed directly (no Blender bake): per-texel garment attributes (rest
position, distances to seams/hems) are rasterized from vertex data, the weave is evaluated
in flat pattern space (UV == cloth, warp along +V), and the tangent-space normal map is the
gradient of the composed height field (OpenGL convention, +Y up).
"""
import struct
import zlib

import numpy as np


# --------------------------------------------------------------------------- io

def write_png(path, img):
    """8-bit PNG from a float array in [0,1], shape (H, W) or (H, W, 3); row 0 = bottom (UV v=0)."""
    a = np.clip(np.asarray(img) * 255.0 + 0.5, 0, 255).astype(np.uint8)[::-1]
    if a.ndim == 2:
        color, ch = 0, 1
    else:
        color, ch = 2, a.shape[2]
    h, w = a.shape[:2]
    raw = np.concatenate([np.zeros((h, 1), np.uint8), a.reshape(h, w * ch)], axis=1).tobytes()

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, color, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def to_srgb(lin):
    lin = np.clip(lin, 0, 1)
    return np.where(lin <= 0.0031308, lin * 12.92, 1.055 * np.power(lin, 1 / 2.4) - 0.055)


# -------------------------------------------------------------------- raster

def rasterize(uv, vals, R):
    """uv: (m,3,2) in [0,1]; vals: (m,3,k). Returns (R,R,k) float32 and a coverage mask."""
    k = vals.shape[2]
    out = np.zeros((R, R, k), np.float32)
    cov = np.zeros((R, R), bool)
    P = uv * R
    for t in range(len(P)):
        a, b, c = P[t]
        x0 = max(int(np.floor(min(a[0], b[0], c[0]))) - 1, 0)
        x1 = min(int(np.ceil(max(a[0], b[0], c[0]))) + 1, R)
        y0 = max(int(np.floor(min(a[1], b[1], c[1]))) - 1, 0)
        y1 = min(int(np.ceil(max(a[1], b[1], c[1]))) + 1, R)
        if x1 <= x0 or y1 <= y0:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
        v0, v1 = b - a, c - a
        den = v0[0] * v1[1] - v1[0] * v0[1]
        if abs(den) < 1e-12:
            continue
        px, py = xs - a[0], ys - a[1]
        wb = (px * v1[1] - v1[0] * py) / den
        wc = (v0[0] * py - px * v0[1]) / den
        wa = 1 - wb - wc
        m = (wa >= -1e-3) & (wb >= -1e-3) & (wc >= -1e-3)
        if not m.any():
            continue
        val = wa[..., None] * vals[t, 0] + wb[..., None] * vals[t, 1] + wc[..., None] * vals[t, 2]
        sub = out[y0:y1, x0:x1]
        sub[m] = val[m]
        cov[y0:y1, x0:x1] |= m
    return out, cov


def dilate(img, cov, iters=16):
    img = img.copy()
    cov = cov.copy()
    for _ in range(iters):
        acc = np.zeros_like(img)
        cnt = np.zeros(cov.shape, np.float32)
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            acc += np.roll(np.roll(img * cov[..., None] if img.ndim == 3 else img * cov, dy, 0), dx, 1)
            cnt += np.roll(np.roll(cov, dy, 0), dx, 1)
        fill = (~cov) & (cnt > 0)
        if img.ndim == 3:
            img[fill] = acc[fill] / cnt[fill][:, None]
        else:
            img[fill] = acc[fill] / cnt[fill]
        cov = cov | fill
    return img


def upsample2(a):
    """Bilinear 2x upsample (H,W[,k]) -> (2H,2W[,k])."""
    def up(x, axis):
        n = x.shape[axis]
        xm = np.take(x, np.clip(np.arange(n) - 1, 0, n - 1), axis=axis)
        xp = np.take(x, np.clip(np.arange(n) + 1, 0, n - 1), axis=axis)
        lo, hi = 0.75 * x + 0.25 * xm, 0.75 * x + 0.25 * xp
        st = np.stack([lo, hi], axis=axis + 1)
        shape = list(x.shape)
        shape[axis] = 2 * n
        return st.reshape(shape)
    return up(up(a, 0), 1)


def box_blur(a, r):
    """Separable box blur with radius r (pixels), edges clamped."""
    def blur1(x, axis):
        n = x.shape[axis]
        pad = [(0, 0)] * x.ndim
        pad[axis] = (r + 1, r)
        c = np.cumsum(np.pad(x, pad, mode="edge"), axis=axis)
        hi = np.take(c, np.arange(2 * r + 1, n + 2 * r + 1), axis=axis)
        lo = np.take(c, np.arange(0, n), axis=axis)
        return (hi - lo) / (2 * r + 1)
    return blur1(blur1(a, 0), 1)


# --------------------------------------------------------------------- noise

def hash1(n, seed=0):
    n = np.asarray(n, np.float64)
    return np.modf(np.sin(n * 12.9898 + seed * 78.233) * 43758.5453)[0] % 1.0


def vnoise1(x, seed=0, period=None):
    i = np.floor(x)
    f = x - i
    i0, i1 = i, i + 1
    if period:
        i0, i1 = np.mod(i0, period), np.mod(i1, period)
    u = f * f * (3 - 2 * f)
    return hash1(i0, seed) * (1 - u) + hash1(i1, seed) * u


def vnoise3(p, seed=0):
    """Vectorized 3D value noise in [0,1]; p: (...,3)."""
    i = np.floor(p)
    f = p - i
    u = f * f * (3 - 2 * f)

    def h(dx, dy, dz):
        return hash1((i[..., 0] + dx) * 127.1 + (i[..., 1] + dy) * 311.7 + (i[..., 2] + dz) * 74.7, seed)
    x00 = h(0, 0, 0) * (1 - u[..., 0]) + h(1, 0, 0) * u[..., 0]
    x10 = h(0, 1, 0) * (1 - u[..., 0]) + h(1, 1, 0) * u[..., 0]
    x01 = h(0, 0, 1) * (1 - u[..., 0]) + h(1, 0, 1) * u[..., 0]
    x11 = h(0, 1, 1) * (1 - u[..., 0]) + h(1, 1, 1) * u[..., 0]
    y0 = x00 * (1 - u[..., 1]) + x10 * u[..., 1]
    y1 = x01 * (1 - u[..., 1]) + x11 * u[..., 1]
    return y0 * (1 - u[..., 2]) + y1 * u[..., 2]


def fbm3(p, octaves=4, seed=0):
    tot, amp, norm = 0.0, 0.5, 0.0
    for o in range(octaves):
        tot = tot + amp * vnoise3(p * (2.0 ** o), seed + o)
        norm += amp
        amp *= 0.5
    return tot / norm


# --------------------------------------------------------------------- weave

def weave(s_cm, t_cm, warp_per_cm=9.0, weft_per_cm=8.0, seed=0, period=None):
    """Plain (tabby) weave of hand-spun yarn. s runs across the cloth (weft direction), t along
    the warp. Returns height in [0,1], warp-on-top mask, per-thread tone and slub mask.
    ``period`` = (warp_count, weft_count) threads per tile makes it tileable."""
    ws, fs = s_cm * warp_per_cm, t_cm * weft_per_cm
    i = np.floor(ws); fu = ws - i - 0.5
    j = np.floor(fs); fv = fs - j - 0.5
    pw = pf = None
    if period:
        pw, pf = period
        i, j = np.mod(i, pw), np.mod(j, pf)
    # slubs: occasional thick soft spots along each thread, plus per-thread thickness
    slw = np.clip(vnoise1(fs * 0.22 + hash1(i, seed) * 91.0, seed + 3, pf and pf * 0.22) - 0.62, 0, 1) * 2.6
    slf = np.clip(vnoise1(ws * 0.22 + hash1(j, seed + 5) * 91.0, seed + 7, pw and pw * 0.22) - 0.62, 0, 1) * 2.6
    thw = 0.84 + 0.3 * hash1(i, seed + 11) + 0.45 * slw
    thf = 0.84 + 0.3 * hash1(j, seed + 13) + 0.45 * slf
    # hand-spun wander: threads are not perfectly straight
    fu = fu + 0.08 * (vnoise1(fs * 0.5 + i * 3.1, seed + 17, pf and pf * 0.5) - 0.5)
    fv = fv + 0.08 * (vnoise1(ws * 0.5 + j * 3.1, seed + 19, pw and pw * 0.5) - 0.5)
    prof_w = np.sqrt(np.clip(1 - (fu / np.minimum(0.34 * thw, 0.49)) ** 2, 0, 1))
    prof_f = np.sqrt(np.clip(1 - (fv / np.minimum(0.34 * thf, 0.49)) ** 2, 0, 1))
    zw = 0.5 + 0.5 * np.cos(np.pi * (fs - 0.5) + np.pi * i)
    zf = 0.5 - 0.5 * np.cos(np.pi * (ws - 0.5) + np.pi * j)
    hw = prof_w * (0.35 + 0.65 * zw) * (0.85 + 0.15 * thw)
    hf = prof_f * (0.35 + 0.65 * zf) * (0.85 + 0.15 * thf)
    warp_top = hw >= hf
    h = np.maximum(hw, hf)
    tone = np.where(warp_top, 0.97 + 0.07 * hash1(i, seed + 23), 0.93 + 0.07 * hash1(j, seed + 29))
    slub = np.where(warp_top, slw, slf)
    return h, warp_top, tone, slub


def height_to_normal(h_mm, texel_mm, strength=1.0):
    gx = (np.roll(h_mm, -1, 1) - np.roll(h_mm, 1, 1)) / (2 * texel_mm) * strength
    gy = (np.roll(h_mm, -1, 0) - np.roll(h_mm, 1, 0)) / (2 * texel_mm) * strength
    n = np.stack([-gx, -gy, np.ones_like(gx)], -1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5


def smoothstep(x, e0, e1):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)
