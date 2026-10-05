"""Procedural numpy texture maps for the clerk's boots and pencil, plus small edits to the cloth
maps ``wardrobe_tex.compose_cloth`` writes (fabric copied into the accessory strip for rolled
sleeves / the neckerchief knot, horn-button colour patch). All original, generated here."""
from pathlib import Path

import bpy
import numpy as np

from outfit import textures as T


def _srgb(c):
    c = np.clip(np.asarray(c, np.float64), 0, 1)
    return np.where(c <= 0.0031308, 12.92 * c, 1.055 * np.power(c, 1 / 2.4) - 0.055)


def _save_rgb(path, rgb):
    """rgb: float array (H, W, 3 or 1) already in file encoding (sRGB for colour), row 0 = top."""
    h, w = rgb.shape[:2]
    if rgb.ndim == 2:
        rgb = rgb[..., None]
    if rgb.shape[2] == 1:
        rgb = np.repeat(rgb, 3, 2)
    img = bpy.data.images.new(Path(path).stem, w, h, alpha=False)
    px = np.ones((h, w, 4), np.float32)
    px[..., :3] = np.clip(rgb[::-1], 0, 1)
    img.pixels.foreach_set(px.ravel())
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def _normal(height, texel_mm):
    gy, gx = np.gradient(height)
    s = 1.0 / max(texel_mm, 1e-3)
    n = np.stack([-gx * s, gy * s, np.ones_like(height)], -1)  # OpenGL (+Y up); row 0 = top
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5


def _noise2(u, v, freq, seed, octaves=4):
    out = np.zeros_like(u)
    amp, tot = 1.0, 0.0
    for o in range(octaves):
        pts = np.stack([u * freq * 2 ** o, v * freq * 2 ** o, np.full_like(u, 0.37 * o + seed % 7)], -1)
        out += amp * T.fbm3(pts, 1, seed + 17 * o)
        tot += amp
        amp *= 0.5
    return out / tot


def write_maps(out_dir, short, albedo_lin, height, rough, ao, texel_mm):
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = {k: out_dir / f"T_{short}_{k}.png" for k in ("basecolor", "normal", "roughness", "ao")}
    _save_rgb(paths["basecolor"], _srgb(albedo_lin))
    _save_rgb(paths["normal"], _normal(height, texel_mm))
    _save_rgb(paths["roughness"], np.clip(rough, 0, 1))
    _save_rgb(paths["ao"], np.clip(ao, 0, 1))
    return paths


def leather_maps(out_dir, short, R=1024, seed=0, base=(0.040, 0.026, 0.017)):
    """Dark waxed leather: pebbled grain, a few soft creases and scuffs, darker welt band at the
    bottom of the UV space is not assumed (smart-projected UVs), so everything is isotropic."""
    y, x = np.mgrid[0:R, 0:R] / R
    grain = _noise2(x, y, 90.0, seed, 3)
    cells = np.abs(_noise2(x + 3.1, y - 1.7, 40.0, seed + 5, 2) - 0.5) * 2
    crease = np.abs(_noise2(x * 0.6, y * 2.4, 6.0, seed + 9, 3) - 0.5)
    crease = np.clip(1 - crease / 0.04, 0, 1)
    scuff = np.clip((_noise2(x, y, 5.0, seed + 13, 4) - 0.62) * 4, 0, 1)
    height = 0.35 * grain - 0.25 * cells ** 3 - 0.5 * crease
    tone = 0.85 + 0.25 * _noise2(x, y, 3.0, seed + 21, 3)
    albedo = np.array(base)[None, None] * tone[..., None]
    albedo = albedo * (1 - 0.35 * scuff[..., None]) + np.array([0.11, 0.085, 0.065]) * 0.35 * scuff[..., None]
    rough = 0.48 + 0.12 * grain + 0.25 * scuff + 0.1 * crease
    ao = 1 - 0.35 * crease - 0.08 * cells
    return write_maps(out_dir, short, albedo, height, rough, ao, texel_mm=300.0 / R)


def pencil_maps(out_dir, short, cone_u, lead_u, R=1024, seed=0):
    """UV u runs along the pencil (0 = flat back end, 1 = graphite tip), v round the facets.
    Natural cedar with long grain lines on the shaft; knife-cut bare wood on the cone with
    facet ridges; graphite at the point. The end cap uses u < 0.02."""
    y, x = np.mgrid[0:R, 0:R] / R  # x = u, y = v (row 0 = top -> v = 1 - y)
    v = 1 - y
    u = x
    grain = np.sin(2 * np.pi * (v * 34 + 0.6 * _noise2(u * 0.3, v, 3.0, seed, 3))) * 0.5 + 0.5
    fine = _noise2(u * 0.2, v * 6, 40.0, seed + 3, 2)
    cedar = np.array([0.42, 0.24, 0.12])
    albedo = cedar[None, None] * (0.82 + 0.16 * grain[..., None] + 0.1 * fine[..., None])
    height = 0.15 * grain + 0.1 * fine
    rough = 0.62 + 0.08 * fine
    # knife-cut cone: paler fresh wood, facets as bands in v
    cone = np.clip((u - cone_u) / 0.01, 0, 1) * (u < lead_u)
    facet = np.abs(np.sin(np.pi * v * 7 + 0.4 * _noise2(u, v, 4.0, seed + 7, 2)))
    fresh = np.array([0.62, 0.43, 0.26])
    albedo = albedo * (1 - cone[..., None]) + fresh * (0.85 + 0.15 * facet[..., None]) * cone[..., None]
    height = height * (1 - cone) + cone * (0.3 * facet)
    rough = rough * (1 - cone) + cone * 0.78
    lead = (u >= lead_u).astype(np.float64)
    gcol = np.array([0.045, 0.045, 0.05])
    albedo = albedo * (1 - lead[..., None]) + gcol * (0.9 + 0.2 * fine[..., None]) * lead[..., None]
    rough = rough * (1 - lead) + 0.38 * lead
    # end cap (flat back): end grain rings
    cap = (u < 0.02).astype(np.float64)
    rings = 0.5 + 0.5 * np.sin(2 * np.pi * np.hypot((u - 0.01) * 50, (v - 0.5) * 3) * 4)
    albedo = albedo * (1 - cap[..., None]) + cedar * (0.7 + 0.25 * rings[..., None]) * cap[..., None]
    ao = np.ones_like(u)
    return write_maps(out_dir, short, albedo, height, rough, ao, texel_mm=170.0 / R)


# ------------------------------------------------------------ cloth map edits

def _load(path):
    img = bpy.data.images.load(str(path), check_existing=False)
    w, h = img.size
    px = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(px)
    return img, px.reshape(h, w, 4)


def _store(img, px):
    img.pixels.foreach_set(px.ravel())
    img.save()
    bpy.data.images.remove(img)


def fabric_into_rect(paths, src_uv, rect, block=(160, 48)):
    """Tile a block of the garment's own fabric (taken around ``src_uv``, inside a big panel)
    into the UV rectangle ``rect`` (u0, u1, v0, v1) of the reserved accessory strip, so small
    added parts (rolled sleeve turns, the knot) read as the same cloth."""
    for key, path in paths.items():
        img, px = _load(path)
        h, w = px.shape[:2]
        bw, bh = min(block[0], w // 4), min(block[1], h // 8)
        cx, cy = int(src_uv[0] * w), int(src_uv[1] * h)
        x0 = int(np.clip(cx - bw // 2, 0, w - bw))
        y0 = int(np.clip(cy - bh // 2, 0, h - bh))
        tile = px[y0:y0 + bh, x0:x0 + bw].copy()
        u0, u1, v0, v1 = (int(rect[0] * w), int(rect[1] * w), int(rect[2] * h), int(rect[3] * h))
        ry, rx = v1 - v0, u1 - u0
        reps = (ry // bh + 2, rx // bw + 2, 1)
        big = np.tile(tile, reps)
        px[v0:v1, u0:u1] = big[:ry, :rx]
        _store(img, px)


def paint_rect(paths, rect, color_lin, rough, seed=0):
    """Fill ``rect`` (UV) with a smooth mottled material (horn buttons)."""
    for key, path in paths.items():
        img, px = _load(path)
        h, w = px.shape[:2]
        u0, u1, v0, v1 = (int(rect[0] * w), int(rect[1] * w), int(rect[2] * h), int(rect[3] * h))
        yy, xx = np.mgrid[v0:v1, u0:u1]
        mott = _noise2(xx / w, yy / h, 60.0, seed, 3)
        if key == "basecolor":
            c = np.array(color_lin)[None, None] * (0.75 + 0.5 * mott[..., None])
            px[v0:v1, u0:u1, :3] = _srgb(c)
        elif key == "normal":
            px[v0:v1, u0:u1, :3] = (0.5, 0.5, 1.0)
        elif key == "roughness":
            px[v0:v1, u0:u1, :3] = (rough + 0.08 * mott)[..., None]
        elif key == "ao":
            px[v0:v1, u0:u1, :3] = 1.0
        _store(img, px)
