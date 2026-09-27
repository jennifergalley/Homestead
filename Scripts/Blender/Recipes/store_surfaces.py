
"""Seamless 2 m tiling PBR surface textures for the general store shell.

Real-object research (written before modeling):
- Cornish 1850s village shops and cottages around St Agnes commonly use local granite
  rubble: grey-buff, roughly squared stones laid in rough courses with recessed lime
  mortar. Blocks vary in size and colour; joints are hand-laid rather than machined.
- Delabole slate roofs use dark blue-grey rectangular slates in diminishing courses,
  with about 18 cm exposed per course in the mid-roof. Rust stains and lichen gather
  on some faces and edges; the relief is mostly the overlapped slate lips.
- Limewashed interiors cover uneven stone/rubble with warm off-white limewash. The wash
  is patchy and rubbed, not paint-white, with soft relief from the stonework underneath.
- Deal/pine floors use 18-22 cm boards running along one axis, staggered butt joints,
  black gaps, nail pairs and traffic-polished lanes.

This recipe procedurally writes seamless 2048² PNG maps for 2.0 m x 2.0 m world-aligned
use. It also creates 2 m preview planes so New-Prop exports and renders the surfaces.
Everything is original generated data: no third-party texture sources.
"""
import math
import random
import sys
from pathlib import Path

import bpy
import numpy as np

NAME = "StoreSurfaces"
DESCRIPTION = "Seamless 2 m PBR texture tiles for the store shell: granite wall, slate roof, limewash and floorboards."
COLLISION = "none"
TRIANGLE_BUDGET = 4000
PROVENANCE = "Original project-authored procedural texture maps; no third-party asset or texture."
BEAUTY = {"meshes": {
    "SM_Store_WallGranite": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
    "SM_Store_SlateRoof": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
    "SM_Store_Limewash": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
    "SM_Store_Floorboards": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
    "SM_Store_WallAshlar": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
    "SM_Store_Render": {"pose": (0, 0, 0), "focus": (0, 0, 0), "detail_distance": 1.2},
}}
NOTES = {"tile_m": 2.0, "maps": "basecolor, normal (OpenGL), roughness, AO at 2048²; seamless on U and V"}

SIZE = 2048
TILE_M = 2.0
ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "Assets" / "Props" / NAME
TEX = OUT / "Textures"


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def grid():
    p = (np.arange(SIZE, dtype=np.float32) + 0.5) / SIZE * TILE_M
    return np.meshgrid(p, p, indexing="xy")


def periodic_noise(x, y, cells=16, seed=0, octaves=1, persistence=0.5):
    out = np.zeros_like(x, dtype=np.float32)
    amp = 1.0
    norm = 0.0
    rng = np.random.default_rng(seed)
    for o in range(octaves):
        c = int(cells * (2 ** o))
        vals = rng.random((c, c), dtype=np.float32)
        gx = (x / TILE_M * c) % c
        gy = (y / TILE_M * c) % c
        ix = np.floor(gx).astype(np.int32)
        iy = np.floor(gy).astype(np.int32)
        fx = gx - ix
        fy = gy - iy
        sx = fx * fx * (3 - 2 * fx)
        sy = fy * fy * (3 - 2 * fy)
        v00 = vals[iy % c, ix % c]
        v10 = vals[iy % c, (ix + 1) % c]
        v01 = vals[(iy + 1) % c, ix % c]
        v11 = vals[(iy + 1) % c, (ix + 1) % c]
        v0 = v00 * (1 - sx) + v10 * sx
        v1 = v01 * (1 - sx) + v11 * sx
        out += (v0 * (1 - sy) + v1 * sy) * amp
        norm += amp
        amp *= persistence
    return out / max(norm, 1e-6)


def save_png(path, rgb, colorspace="sRGB"):
    path.parent.mkdir(parents=True, exist_ok=True)
    arr = np.asarray(rgb, dtype=np.float32)
    if arr.ndim == 2:
        arr = np.repeat(arr[..., None], 3, axis=2)
    alpha = np.ones((arr.shape[0], arr.shape[1], 1), dtype=np.float32)
    rgba = np.clip(np.concatenate([arr, alpha], axis=2), 0, 1)
    img = bpy.data.images.new(path.stem, width=arr.shape[1], height=arr.shape[0], alpha=True, float_buffer=False)
    img.colorspace_settings.name = colorspace
    img.pixels.foreach_set(rgba.reshape(-1))
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def normal_from_height(height, strength=4.0):
    # Periodic central differences. Height values are in metres of relief.
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5
    px = TILE_M / SIZE
    nx = -dx / px * strength
    ny = -dy / px * strength
    nz = np.ones_like(height)
    inv = 1.0 / np.sqrt(nx * nx + ny * ny + nz * nz)
    n = np.stack((0.5 + 0.5 * nx * inv, 0.5 + 0.5 * ny * inv, 0.5 + 0.5 * nz * inv), axis=2)
    return np.clip(n, 0, 1)


def write_set(stem, base, height, rough, ao, normal_strength=2.5):
    base = np.clip(base, 0, 1).astype(np.float32)
    rough = np.clip(rough, 0, 1).astype(np.float32)
    ao = np.clip(ao, 0, 1).astype(np.float32)
    normal = normal_from_height(height.astype(np.float32), normal_strength)
    light = np.array([-0.45, -0.55, 0.78], dtype=np.float32)
    light /= np.linalg.norm(light)
    nvec = normal * 2.0 - 1.0
    shade = np.clip(nvec[..., 0] * light[0] + nvec[..., 1] * light[1] + nvec[..., 2] * light[2], 0.0, 1.0)
    lit = np.clip((0.22 + 0.95 * shade) * ao, 0.0, 1.0)
    save_png(OUT / f"litcheck_T_{stem}_normal_ao.png", np.repeat(lit[..., None], 3, axis=2), "sRGB")
    paths = {
        "basecolor": TEX / f"T_{stem}_basecolor.png",
        "roughness": TEX / f"T_{stem}_roughness.png",
        "normal": TEX / f"T_{stem}_normal.png",
        "ao": TEX / f"T_{stem}_ao.png",
    }
    save_png(paths["basecolor"], base, "sRGB")
    save_png(paths["roughness"], rough, "Non-Color")
    save_png(paths["normal"], normal, "Non-Color")
    save_png(paths["ao"], ao, "Non-Color")
    # Seam check composite: 2x2 repeat of basecolor, saved beside textures for review.
    save_png(OUT / f"tilecheck_T_{stem}_basecolor_2x2.png", np.tile(base, (2, 2, 1)), "sRGB")
    return paths


def course_layout(seed, min_h, max_h):
    rng = random.Random(seed)
    heights = []
    total = 0.0
    while total < TILE_M - min_h:
        h = rng.uniform(min_h, max_h)
        if total + h > TILE_M:
            break
        heights.append(h)
        total += h
    if TILE_M - total < min_h * 0.55 and heights:
        heights[-1] += TILE_M - total
    else:
        heights.append(TILE_M - total)
    scale = TILE_M / sum(heights)
    heights = [h * scale for h in heights]
    bounds = [0.0]
    for h in heights:
        bounds.append(bounds[-1] + h)
    return bounds


def split_widths(seed, min_w, max_w):
    rng = random.Random(seed)
    widths = []
    total = 0.0
    while total < TILE_M - min_w:
        w = rng.uniform(min_w, max_w)
        if total + w > TILE_M:
            break
        widths.append(w)
        total += w
    widths.append(TILE_M - total)
    scale = TILE_M / sum(widths)
    widths = [w * scale for w in widths]
    b = [0.0]
    for w in widths:
        b.append(b[-1] + w)
    return b


def dist_to_bounds(v, bounds):
    d = np.full_like(v, TILE_M, dtype=np.float32)
    for b in bounds[:-1]:
        dd = np.abs(v - b)
        d = np.minimum(d, np.minimum(dd, TILE_M - dd))
    return d


def wall_granite():
    x, y = grid()
    mortar_noise = periodic_noise(x, y, 36, 405, 3, 0.58) - 0.5
    edge_grime = 0.5 + 0.5 * np.cos(2 * math.pi * y / TILE_M)
    base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    base[:] = np.array([0.50, 0.465, 0.385], dtype=np.float32) + mortar_noise[..., None] * 0.045
    base *= (1.0 - 0.055 * edge_grime[..., None])
    height = np.full((SIZE, SIZE), -0.012, dtype=np.float32)
    rough = np.full((SIZE, SIZE), 0.96, dtype=np.float32)
    ao = np.full((SIZE, SIZE), 0.72, dtype=np.float32)
    stone_mask_all = np.zeros((SIZE, SIZE), dtype=bool)

    rows = course_layout(110, 0.145, 0.31)
    # Shared wavy course boundaries, zero at tile edges for vertical seamlessness.
    boundary = []
    for k, b in enumerate(rows):
        if k in (0, len(rows)-1):
            boundary.append(np.full_like(x, b))
        else:
            phase = 0.7 * k
            wig = 0.010 * np.sin(2 * math.pi * (x / TILE_M * (2 + k % 3) + phase))
            wig += 0.005 * np.sin(2 * math.pi * (x / TILE_M * (5 + k % 4) + phase * 0.37))
            boundary.append(b + wig.astype(np.float32))

    crystal_a = periodic_noise(x, y, 180, 1600, 2, 0.50)
    crystal_b = periodic_noise(x, y, 260, 1601, 1, 0.50)
    crystal_c = periodic_noise(x, y, 120, 1602, 2, 0.52)
    feldspar = smoothstep(0.58, 0.82, crystal_a)
    mica = smoothstep(0.80, 0.93, crystal_b)
    quartz = smoothstep(0.45, 0.70, crystal_c) * (1.0 - mica)
    fine_relief = (crystal_a - 0.5) * 0.0018 + (crystal_b - 0.5) * 0.0012
    lichen_g = smoothstep(0.70, 0.86, periodic_noise(x, y, 9, 1700, 4, 0.60))
    lichen_o = smoothstep(0.83, 0.93, periodic_noise(x, y, 18, 1701, 3, 0.58))
    joint_w_global = 0.012 + 0.014 * periodic_noise(x, y, 24, 2600, 2, 0.55)

    for r in range(len(rows)-1):
        y0, y1 = rows[r], rows[r+1]
        widths = split_widths(2200 + r * 37, 0.18, 0.58)
        rng = random.Random(2300 + r)
        bottom = boundary[r]
        top = boundary[r+1]
        for sidx, (x0, x1) in enumerate(zip(widths[:-1], widths[1:])):
            amp_l = rng.uniform(0.006, 0.018)
            amp_r = rng.uniform(0.006, 0.018)
            ph1, ph2 = rng.random(), rng.random()
            left = np.full_like(x, x0)
            right = np.full_like(x, x1)
            if x0 > 0.0001:
                left += amp_l * np.sin(2 * math.pi * (y / TILE_M * rng.choice([2, 3, 4]) + ph1)).astype(np.float32)
                left += (amp_l * 0.45) * np.sin(2 * math.pi * (y / TILE_M * rng.choice([5, 6, 7]) + ph2)).astype(np.float32)
            if x1 < TILE_M - 0.0001:
                right += amp_r * np.sin(2 * math.pi * (y / TILE_M * rng.choice([2, 3, 4]) + ph2)).astype(np.float32)
                right += (amp_r * 0.45) * np.sin(2 * math.pi * (y / TILE_M * rng.choice([5, 6, 7]) + ph1)).astype(np.float32)
            region = (x >= max(0, x0 - 0.06)) & (x < min(TILE_M, x1 + 0.06)) & (y >= y0 - 0.055) & (y < y1 + 0.055)
            dl, dr = x - left, right - x
            db, dt = y - bottom, top - y
            dist = np.minimum(np.minimum(dl, dr), np.minimum(db, dt))
            stone = region & (dist > joint_w_global)
            # Chipped corners / near-polygonal arrises.
            for cx, cy in ((x0, y0), (x0, y1), (x1, y0), (x1, y1)):
                rr = rng.uniform(0.020, 0.050)
                ex = (x - cx) / (rr * rng.uniform(0.9, 1.6))
                ey = (y - cy) / (rr * rng.uniform(0.8, 1.4))
                stone &= ~(ex*ex + ey*ey < 1.0)
            if not np.any(stone):
                continue
            tone = rng.uniform(0.255, 0.335)
            buff = rng.uniform(-0.015, 0.030)
            pink = rng.uniform(0.0, 0.028)
            col = np.array([tone + buff + pink, tone + buff * 0.70, tone - 0.012 + pink * 0.25], dtype=np.float32)
            g = (periodic_noise(x, y, 14, 2800 + r*13 + sidx, 2, 0.58) - 0.5)
            stone_col = col + g[..., None] * 0.030
            stone_col += feldspar[..., None] * np.array([0.055, 0.052, 0.045], dtype=np.float32)
            stone_col += quartz[..., None] * np.array([0.028, 0.032, 0.035], dtype=np.float32)
            stone_col -= mica[..., None] * np.array([0.095, 0.092, 0.085], dtype=np.float32)
            lmask = (lichen_g * rng.uniform(0.0, 0.45)) * stone
            omask = (lichen_o * rng.uniform(0.0, 0.16)) * stone
            stone_col = stone_col * (1 - lmask[..., None]) + np.array([0.30, 0.36, 0.26], dtype=np.float32) * lmask[..., None]
            stone_col = stone_col * (1 - omask[..., None]) + np.array([0.55, 0.26, 0.095], dtype=np.float32) * omask[..., None]
            base[stone] = stone_col[stone]
            proud = 0.006 + 0.015 * smoothstep(0.0, 0.055, dist)
            crown = 0.003 * smoothstep(0.0, 0.10, dist) * (1.0 - smoothstep(0.11, 0.20, dist))
            height[stone] = (proud + crown + fine_relief + lmask * 0.0012)[stone]
            rough[stone] = np.clip((0.84 + mica * 0.09 - quartz * 0.12 + lmask * 0.08)[stone], 0.58, 0.98)
            edge_ao = 1.0 - 0.34 * (1.0 - smoothstep(0.0, 0.045, dist))
            ao[stone] = np.minimum(ao[stone], edge_ao[stone])
            stone_mask_all |= stone

    # Small pinning stones/spalls embedded in the lime mortar joints.
    rng = random.Random(3300)
    mortar = ~stone_mask_all
    for i in range(36):
        cx, cy = rng.random() * TILE_M, rng.random() * TILE_M
        rx, ry = rng.uniform(0.022, 0.075), rng.uniform(0.012, 0.040)
        ang = rng.uniform(0, math.pi)
        ca, sa = math.cos(ang), math.sin(ang)
        xx = ((x - cx + TILE_M * 0.5) % TILE_M) - TILE_M * 0.5
        yy = ((y - cy + TILE_M * 0.5) % TILE_M) - TILE_M * 0.5
        u = (xx * ca + yy * sa) / rx
        v = (-xx * sa + yy * ca) / ry
        spall = (u*u + v*v < 1.0) & mortar
        if np.count_nonzero(spall) < 20:
            continue
        col = np.array([rng.uniform(0.23, 0.33), rng.uniform(0.22, 0.31), rng.uniform(0.20, 0.28)], dtype=np.float32)
        base[spall] = col
        h = 0.004 + 0.006 * (1.0 - np.sqrt(np.clip(u*u + v*v, 0, 1)))
        height[spall] = h[spall]
        rough[spall] = 0.88
        ao[spall] = 0.82

    mortar = ~stone_mask_all
    grit = periodic_noise(x, y, 95, 3400, 2, 0.55)
    base[mortar] += (grit[mortar] - 0.5)[:, None] * 0.030
    height[mortar] += (grit[mortar] - 0.5) * 0.0015
    rough[mortar] = 0.95 + (grit[mortar] - 0.5) * 0.04
    ao[mortar] = np.minimum(ao[mortar], 0.70 + (grit[mortar] - 0.5) * 0.08)
    return write_set("Store_WallGranite", base, height, rough, ao, 3.0)

def slate_roof():
    x, y = grid()
    # Diminishing courses: larger exposed slates at the eaves/bottom of the tile,
    # smaller courses toward the top. Normalised to a 2 m seamless tile.
    hs = [0.235, 0.225, 0.210, 0.195, 0.185, 0.175, 0.165, 0.155, 0.148, 0.142, 0.135]
    scale = TILE_M / sum(hs)
    rows = [0.0]
    for h in hs:
        rows.append(rows[-1] + h * scale)
    base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    height = np.zeros((SIZE, SIZE), dtype=np.float32)
    rough = np.full((SIZE, SIZE), 0.80, dtype=np.float32)
    ao = np.ones((SIZE, SIZE), dtype=np.float32)
    slate_all = np.zeros((SIZE, SIZE), dtype=bool)
    boundary = []
    for k, b in enumerate(rows):
        if k in (0, len(rows)-1):
            boundary.append(np.full_like(x, b))
        else:
            boundary.append(b + (0.0035 * np.sin(2 * math.pi * (x / TILE_M * (4 + k % 3) + k * 0.27))).astype(np.float32))
    grain = periodic_noise(x, y, 70, 700, 3, 0.55) - 0.5
    rust = smoothstep(0.74, 0.88, periodic_noise(x, y, 15, 702, 3, 0.58)) * 0.32
    lichen = smoothstep(0.69, 0.84, periodic_noise(x, y, 8, 703, 4, 0.6)) * 0.24
    for r in range(len(rows)-1):
        rng = random.Random(650 + r)
        y0, y1 = rows[r], rows[r+1]
        bottom, top = boundary[r], boundary[r+1]
        widths = split_widths(600 + r * 23, 0.20, 0.36)
        off = (r * 0.073) % TILE_M
        widths = [((b + off) % TILE_M) for b in widths]
        widths = sorted(set([0.0, TILE_M] + [b for b in widths if 0.02 < b < TILE_M - 0.02]))
        for sidx, (x0, x1) in enumerate(zip(widths[:-1], widths[1:])):
            left = np.full_like(x, x0)
            right = np.full_like(x, x1)
            if x0 > 0.0001:
                left += (0.0035 * np.sin(2 * math.pi * (y / TILE_M * rng.choice([3,4,5]) + rng.random()))).astype(np.float32)
            if x1 < TILE_M - 0.0001:
                right += (0.0035 * np.sin(2 * math.pi * (y / TILE_M * rng.choice([3,4,5]) + rng.random()))).astype(np.float32)
            region = (x >= max(0, x0 - 0.03)) & (x < min(TILE_M, x1 + 0.03)) & (y >= y0 - 0.03) & (y < y1 + 0.03)
            dl, dr = x - left, right - x
            db, dt = y - bottom, top - y
            dist = np.minimum(np.minimum(dl, dr), np.minimum(db, dt))
            slate = region & (dist > 0.0045)
            # Occasional chipped corners.
            for cx, cy in ((x0, y0), (x1, y0), (x0, y1), (x1, y1)):
                if rng.random() < 0.38:
                    rr = rng.uniform(0.012, 0.035)
                    slate &= ~(((x - cx)/(rr*1.25))**2 + ((y - cy)/rr)**2 < 1.0)
            if not np.any(slate):
                continue
            tone = rng.uniform(0.095, 0.155)
            blue = rng.uniform(0.015, 0.040)
            col = np.array([tone * 0.78, tone * 0.88 + blue * 0.30, tone + blue], dtype=np.float32)
            slate_col = col + grain[..., None] * np.array([0.018, 0.022, 0.030], dtype=np.float32)
            slate_col = slate_col * (1 - rust[..., None]) + np.array([0.25, 0.12, 0.055], dtype=np.float32) * rust[..., None]
            slate_col = slate_col * (1 - lichen[..., None]) + np.array([0.19, 0.24, 0.15], dtype=np.float32) * lichen[..., None]
            base[slate] = slate_col[slate]
            t = np.clip((y - y0) / max(y1 - y0, 1e-6), 0, 1)
            lip = 0.007 * smoothstep(0.075, 0.0, t)
            height[slate] = (0.0015 + lip + grain * 0.0014 + lichen * 0.0012)[slate]
            rough[slate] = np.clip((0.74 + lichen * 0.10 + rust * 0.08 - np.maximum(grain, 0) * 0.05)[slate], 0.58, 0.94)
            # dark gap/shadow just below each overlap lip
            shadow = smoothstep(0.050, 0.0, t)
            ao[slate] = np.minimum(ao[slate], (1.0 - shadow * 0.18)[slate])
            slate_all |= slate
    gaps = ~slate_all
    base[gaps] = np.array([0.025, 0.028, 0.032], dtype=np.float32)
    height[gaps] = -0.004
    rough[gaps] = 0.92
    ao[gaps] = 0.55
    return write_set("Store_SlateRoof", base, height, rough, ao, 2.6)

def limewash():
    x, y = grid()
    # Thick interior limewash: no visible block layout in colour; only soft,
    # rounded rubble undulations survive in normal/AO under a patchy warm wash.
    warm = np.array([0.665, 0.630, 0.545], dtype=np.float32)
    base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    broad = periodic_noise(x, y, 4, 910, 4, 0.62) - 0.5
    mid = periodic_noise(x, y, 11, 911, 4, 0.58) - 0.5
    fine = periodic_noise(x, y, 48, 912, 3, 0.55) - 0.5
    blush = periodic_noise(x, y, 7, 913, 3, 0.60) - 0.5
    # Seam-safe lower-wall greying/dirt: strongest at the tile's wrapped lower edge,
    # so top and bottom still match when repeated.
    lower = (0.5 + 0.5 * np.cos(2 * math.pi * y / TILE_M)) ** 1.8
    tone = 1.0 + broad * 0.090 + mid * 0.050 + fine * 0.018
    base[:] = warm * tone[..., None]
    base += np.stack((blush * 0.018, blush * 0.010, -blush * 0.006), axis=2)
    grey = np.array([0.560, 0.545, 0.505], dtype=np.float32)
    base = base * (1 - lower[..., None] * 0.085) + grey * lower[..., None] * 0.085

    # Soft rubble telegraphing through the lime: rounded bulges of mixed sizes, with
    # no straight joints or rectangular blocks.  All distances wrap on the tile torus.
    height = np.zeros((SIZE, SIZE), dtype=np.float32)
    ao = np.ones((SIZE, SIZE), dtype=np.float32)
    rng = random.Random(9810)
    for i in range(42):
        cx, cy = rng.random() * TILE_M, rng.random() * TILE_M
        rx, ry = rng.uniform(0.16, 0.42), rng.uniform(0.12, 0.34)
        ang = rng.uniform(0, math.pi)
        ca, sa = math.cos(ang), math.sin(ang)
        xx = ((x - cx + TILE_M * 0.5) % TILE_M) - TILE_M * 0.5
        yy = ((y - cy + TILE_M * 0.5) % TILE_M) - TILE_M * 0.5
        u = (xx * ca + yy * sa) / rx
        v = (-xx * sa + yy * ca) / ry
        r2 = u*u + v*v
        blob = np.exp(-r2 * rng.uniform(1.1, 2.2))
        amp = rng.uniform(0.0012, 0.0048)
        height += blob.astype(np.float32) * amp
    # Feathered trowel/lime texture and a few shallow hollows; keep strength low.
    plaster = periodic_noise(x, y, 18, 9820, 4, 0.57) - 0.5
    fine_pit = periodic_noise(x, y, 95, 9821, 2, 0.55) - 0.5
    hollows = smoothstep(0.68, 0.86, periodic_noise(x, y, 9, 9822, 4, 0.60))
    height += plaster * 0.0012 + fine_pit * 0.00045 - hollows * 0.0016
    # AO is only a broad, soft shade in concave patches / lower dirty patches, not joint lines.
    ao -= hollows * 0.045
    ao -= lower * 0.025
    rough = np.full((SIZE, SIZE), 0.93, dtype=np.float32) + fine * 0.035 + hollows * 0.025
    return write_set("Store_Limewash", np.clip(base, 0.56, 0.74), height, rough, ao, 0.65)

def wall_ashlar():
    x, y = grid()
    base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    height = np.full((SIZE, SIZE), -0.0045, dtype=np.float32)
    rough = np.full((SIZE, SIZE), 0.86, dtype=np.float32)
    ao = np.full((SIZE, SIZE), 0.82, dtype=np.float32)
    mortar = np.ones((SIZE, SIZE), dtype=bool)
    # Regular dressed courses, normalised to tile exactly.
    hs = [0.31, 0.34, 0.29, 0.33, 0.30, 0.32, 0.28]
    sc = TILE_M / sum(hs)
    rows = [0.0]
    for h in hs:
        rows.append(rows[-1] + h * sc)
    dh = dist_to_bounds(y, rows)
    joint_w = 0.0065 + 0.0015 * (periodic_noise(x, y, 18, 4300, 2, 0.55) - 0.5)
    crystal = periodic_noise(x, y, 190, 4310, 2, 0.50)
    fleck = periodic_noise(x, y, 330, 4311, 1, 0.50)
    tool = 0.5 + 0.5 * np.sin(2 * math.pi * (x * 42.0 + 0.4 * periodic_noise(x, y, 9, 4312, 2)))
    streak = smoothstep(0.62, 0.86, periodic_noise(x * 0.5, y, 7, 4313, 4, 0.62))
    for r in range(len(rows)-1):
        rng = random.Random(4400 + r)
        y0, y1 = rows[r], rows[r+1]
        widths = split_widths(4500 + r * 17, 0.45, 0.90)
        # ashlar is regular but not a brick grid: offset alternate courses and vary lengths
        off = (0.21 * r + 0.07) % TILE_M
        bounds = sorted(set([0.0, TILE_M] + [((b + off) % TILE_M) for b in widths if 0.02 < ((b + off) % TILE_M) < TILE_M - 0.02]))
        for i, (x0, x1) in enumerate(zip(bounds[:-1], bounds[1:])):
            block = (y >= y0) & (y < y1) & (x >= x0) & (x < x1)
            dv = np.minimum(x - x0, x1 - x)
            dist = np.minimum(np.minimum(y - y0, y1 - y), dv)
            stone = block & (dist > joint_w)
            if not np.any(stone):
                continue
            tone = rng.uniform(0.30, 0.39)
            warm = rng.uniform(-0.012, 0.018)
            col = np.array([tone + warm, tone + warm * 0.65, tone - 0.015], dtype=np.float32)
            grain = (crystal - 0.5) * 0.035 + (fleck > 0.82) * 0.035 - (fleck < 0.10) * 0.050
            weather = streak * rng.uniform(0.0, 0.10)
            stone_col = col + grain[..., None]
            stone_col *= (1.0 - weather[..., None])
            base[stone] = stone_col[stone]
            # Fine tooled face with slight dressed arris proud of recessed joints.
            arris = 0.006 + 0.004 * smoothstep(0.0, 0.030, dist)
            height[stone] = (arris + (tool - 0.5) * 0.0009 + (crystal - 0.5) * 0.0012)[stone]
            rough[stone] = np.clip((0.80 + (fleck < 0.12) * 0.10 - (fleck > 0.86) * 0.08 + weather * 0.08)[stone], 0.62, 0.94)
            ao[stone] = np.minimum(1.0 - 0.20 * (1.0 - smoothstep(0.0, 0.018, dist))[stone], 0.98)
            mortar[stone] = False
    mtex = periodic_noise(x, y, 55, 4320, 2, 0.55) - 0.5
    base[mortar] = np.array([0.52, 0.49, 0.42], dtype=np.float32) + mtex[mortar, None] * 0.025
    height[mortar] = -0.006 + mtex[mortar] * 0.0008
    rough[mortar] = 0.95
    ao[mortar] = np.minimum(ao[mortar], 0.70)
    return write_set("Store_WallAshlar", np.clip(base, 0, 1), height, rough, ao, 1.8)


def roughcast_render():
    x, y = grid()
    broad = periodic_noise(x, y, 4, 5300, 4, 0.62) - 0.5
    mid = periodic_noise(x, y, 13, 5301, 4, 0.58) - 0.5
    fine = periodic_noise(x, y, 95, 5302, 2, 0.55) - 0.5
    pebbles = periodic_noise(x, y, 240, 5303, 1, 0.50)
    lower = (0.5 + 0.5 * np.cos(2 * math.pi * y / TILE_M)) ** 1.6
    cream = np.array([0.62, 0.58, 0.50], dtype=np.float32)
    buff = np.array([0.66, 0.60, 0.50], dtype=np.float32)
    base = cream * (1 + broad[..., None] * 0.055) + mid[..., None] * 0.018
    base = base * 0.90 + buff * 0.10
    grey_stain = np.array([0.47, 0.46, 0.41], dtype=np.float32)
    staining = smoothstep(0.60, 0.84, periodic_noise(x * 0.45, y, 8, 5304, 4, 0.60)) * 0.22
    base = base * (1 - staining[..., None]) + grey_stain * staining[..., None]
    base = base * (1 - lower[..., None] * 0.08) + np.array([0.50, 0.49, 0.45], dtype=np.float32) * lower[..., None] * 0.08
    worn = smoothstep(0.74, 0.91, periodic_noise(x, y, 18, 5305, 3, 0.58)) * smoothstep(0.35, 1.0, lower)
    base = base * (1 - worn[..., None] * 0.18) + np.array([0.36, 0.36, 0.34], dtype=np.float32) * worn[..., None] * 0.18
    height = (pebbles - 0.5) * 0.0024 + fine * 0.0014 + broad * 0.0022 - staining * 0.0012
    rough = np.full((SIZE, SIZE), 0.90, dtype=np.float32) + (pebbles - 0.5) * 0.08 + staining * 0.05
    ao = np.ones((SIZE, SIZE), dtype=np.float32) - smoothstep(0.80, 0.96, pebbles) * 0.035 - staining * 0.025 - lower * 0.025
    return write_set("Store_Render", np.clip(base, 0.36, 0.70), height, rough, ao, 1.15)


def floorboards():
    x, y = grid()
    widths = []
    rng = random.Random(1200)
    total = 0.0
    while total < TILE_M - 0.18:
        w = rng.uniform(0.18, 0.22)
        widths.append(w)
        total += w
    widths.append(TILE_M - total)
    scale = TILE_M / sum(widths)
    widths = [w * scale for w in widths]
    xb = [0.0]
    for w in widths:
        xb.append(xb[-1] + w)
    dx = dist_to_bounds(x, xb)
    gaps = dx < 0.004
    base = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    height = np.zeros((SIZE, SIZE), dtype=np.float32)
    rough = np.full((SIZE, SIZE), 0.70, dtype=np.float32)
    ao = np.ones((SIZE, SIZE), dtype=np.float32)
    butt = np.zeros((SIZE, SIZE), dtype=bool)
    nail = np.zeros((SIZE, SIZE), dtype=np.float32)
    for i in range(len(widths)):
        x0, x1 = xb[i], xb[i+1]
        board = (x >= x0) & (x < x1)
        tone = rng.uniform(0.30, 0.44)
        base[board] = (tone, tone * 0.70, tone * 0.40)
        height[board] = 0.004 + rng.uniform(-0.0008, 0.0012)
        # Staggered butt joints on individual boards.
        offset = (i * 0.37 + rng.uniform(0.0, 0.5)) % TILE_M
        segs = [offset % TILE_M]
        yy = segs[0]
        for k in range(3):
            yy = (yy + rng.uniform(0.55, 0.95)) % TILE_M
            segs.append(yy)
        for b in segs:
            d = np.minimum(np.abs(y - b), TILE_M - np.abs(y - b))
            bm = board & (d < 0.004)
            butt |= bm
            # nail pairs near each side of this board at the butt.
            for nx in (x0 + 0.030, x1 - 0.030):
                rr = (np.minimum(np.abs(x - nx), TILE_M - np.abs(x - nx)) / 0.010) ** 2 + (d / 0.010) ** 2
                nail = np.maximum(nail, np.exp(-rr * 2.4) * board)
    grain = periodic_noise(x * 0.35, y, 36, 1210, 5, 0.58) - 0.5
    long = periodic_noise(x * 0.20, y, 8, 1211, 4, 0.58) - 0.5
    traffic = np.exp(-((x - 1.05) / 0.32) ** 2) * (0.6 + 0.4 * periodic_noise(x, y, 4, 1212, 2))
    base += grain[..., None] * np.array([0.070, 0.045, 0.022], dtype=np.float32) + long[..., None] * 0.04
    base = base * (1 - traffic[..., None] * 0.22) + np.array([0.48, 0.34, 0.19], dtype=np.float32) * traffic[..., None] * 0.24
    base[gaps | butt] = np.array([0.035, 0.025, 0.018], dtype=np.float32)
    base = base * (1 - nail[..., None] * 0.75)
    height += grain * 0.0025 + long * 0.002
    height[gaps | butt] = -0.006
    height -= nail * 0.0025
    rough = rough - traffic * 0.18 + np.abs(grain) * 0.08
    rough[gaps | butt] = 0.92
    ao -= (gaps | butt).astype(np.float32) * 0.45
    ao -= nail * 0.25
    return write_set("Store_Floorboards", base, height, rough, ao, 2.6)


def material_for(stem, paths):
    mat = bpy.data.materials.new("M_" + stem)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    def tex(role, colorspace):
        img = bpy.data.images.load(str(paths[role]), check_existing=True)
        img.colorspace_settings.name = colorspace
        node = nodes.new("ShaderNodeTexImage")
        node.image = img
        return node
    base = tex("basecolor", "sRGB")
    rough = tex("roughness", "Non-Color")
    normal = tex("normal", "Non-Color")
    ao = tex("ao", "Non-Color")
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 0.35
    links.new(base.outputs["Color"], [s for s in mix.inputs if s.type == "RGBA"][0])
    links.new(ao.outputs["Color"], [s for s in mix.inputs if s.type == "RGBA"][1])
    links.new(next(s for s in mix.outputs if s.type == "RGBA"), bsdf.inputs["Base Color"])
    links.new(rough.outputs["Color"], bsdf.inputs["Roughness"])
    nmap = nodes.new("ShaderNodeNormalMap")
    nmap.space = "TANGENT"
    links.new(normal.outputs["Color"], nmap.inputs["Color"])
    links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    mat["homestead_procedural"] = True
    return mat


def preview_plane(kit, name, mat, xoff):
    steps = 16
    verts = []
    uvs = []
    for j in range(steps + 1):
        for i in range(steps + 1):
            u, v = i / steps, j / steps
            verts.append((xoff + (u - 0.5) * TILE_M, (v - 0.5) * TILE_M, 0.001 * math.sin(math.pi * u) * math.sin(math.pi * v)))
            uvs.append((u, v))
    faces = [(j*(steps+1)+i, j*(steps+1)+i+1, (j+1)*(steps+1)+i+1, (j+1)*(steps+1)+i) for j in range(steps) for i in range(steps)]
    obj = kit.mesh("SM_" + name, verts, faces, material=mat)
    layer = obj.data.uv_layers["UVMap"].data
    for loop in obj.data.loops:
        layer[loop.index].uv = uvs[loop.vertex_index]
    return kit.finalize(obj, pivot=None, unwrap=False, reshade=False)


def build(kit):
    TEX.mkdir(parents=True, exist_ok=True)
    generators = [
        ("Store_WallGranite", wall_granite),
        ("Store_SlateRoof", slate_roof),
        ("Store_Limewash", limewash),
        ("Store_Floorboards", floorboards),
        ("Store_WallAshlar", wall_ashlar),
        ("Store_Render", roughcast_render),
    ]
    planes = []
    for i, (stem, fn) in enumerate(generators):
        paths = fn()
        mat = material_for(stem, paths)
        planes.append(preview_plane(kit, stem, mat, i * 2.5))
    return planes
