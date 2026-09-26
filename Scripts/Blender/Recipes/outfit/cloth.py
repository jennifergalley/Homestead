"""Compose the baked cloth maps (basecolor, normal, roughness, AO) for one garment."""
import math

import numpy as np

from . import textures as T
from .detail import STRIP

STYLE = {
    "SKM_PrimitiveTankTop": dict(
        base=(0.655, 0.618, 0.540), thread=(0.50, 0.47, 0.40), warp=10.0, weft=9.0,
        seams={"hem": ("roll", 0.9, [0.62]), "bind": ("roll", 0.65, [0.48]),
               "side": ("felled", 0.8, [0.15, 0.65]), "shoulder": ("felled", 0.8, [0.15, 0.65])}),
    "SKM_PrimitiveShorts": dict(
        base=(0.500, 0.432, 0.335), thread=(0.40, 0.345, 0.27), warp=9.0, weft=8.0,
        seams={"waist": ("casing", 2.7, [0.35, 2.5]), "leghem": ("roll", 1.15, [0.95]),
               "side": ("felled", 0.85, [0.15, 0.7]), "inseam": ("felled", 0.85, [0.15, 0.7]),
               "crotch": ("felled", 0.85, [0.15, 0.7])}),
}


def _tri_data(me):
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    loops = np.empty(n * 3, np.int64)
    me.loop_triangles.foreach_get("loops", loops)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get("vertex_index", lv)
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    loops = loops.reshape(-1, 3)
    return uv[loops], lv[loops]


def _stitches(d, a, inset, seed, length=0.32, gap=0.26, width=0.034):
    """Hand running stitch: irregular dashes along a line ``inset`` cm from an edge."""
    jitter = 0.05 * (T.vnoise1(a * 0.8, seed) - 0.5)
    across = np.exp(-((d - inset - jitter) / width) ** 2)
    per = length + gap
    ph = (a + 0.12 * T.vnoise1(a * 2.1, seed + 1)) / per
    k = np.floor(ph)
    own = length / per * (0.85 + 0.3 * T.hash1(k, seed + 2))
    f = ph - k
    dash = T.smoothstep(f, 0.0, 0.06) * (1 - T.smoothstep(f, own - 0.06, own))
    return across * dash


def compose(obj, fields, uvpm, out_dir, name, R=4096, seed=0, log=print):
    st = STYLE[obj.name]
    me = obj.data
    uvt, vt = _tri_data(me)
    names = sorted(fields)
    per_vert = []
    for n in names:
        per_vert += [fields[n][0] * 100.0, fields[n][1] * 100.0]     # cm
    co = np.array([v.co for v in me.vertices])
    per_vert += [co[:, 0], co[:, 1], co[:, 2]]
    PV = np.stack(per_vert, 1).astype(np.float32)
    R2 = R // 2
    F2, cov2 = T.rasterize(uvt, PV[vt], R2)
    F2 = T.dilate(F2, cov2, 24)
    cov = T.upsample2(cov2.astype(np.float32)) > 0.5
    log(f"  {name}: rasterized {len(uvt)} tris, coverage {cov.mean():.2%}, {uvpm * 100 * R / 100:.1f} px/cm")
    ch = {n: 2 * i for i, n in enumerate(names)}
    ixyz = 2 * len(names)
    # macro fields from rest position (1024 -> 4096)
    xyz4 = F2[::2, ::2, ixyz:ixyz + 3].astype(np.float64)
    mottle = T.fbm3(xyz4 * 14.0, 4, seed + 1)
    blotch = T.fbm3(xyz4 * 7.0 + 3.1, 4, seed + 2)
    grass = T.fbm3(xyz4 * 11.0 - 5.3, 4, seed + 3)
    crink = np.abs(T.fbm3(xyz4 * 19.0 + 1.7, 3, seed + 4) - 0.5) * 2
    crink2 = np.abs(T.fbm3(xyz4 * 55.0 - 2.9, 2, seed + 5) - 0.5) * 2
    macro = np.stack([mottle, blotch, grass, crink, crink2], -1).astype(np.float32)
    macro = T.upsample2(T.upsample2(macro))

    px_per_cm = uvpm * R / 100.0
    texel_mm = 10.0 / px_per_cm
    base = np.array(st["base"])
    thread_col = np.array(st["thread"])
    albedo = np.zeros((R, R, 3), np.float32)
    height = np.zeros((R, R), np.float32)
    rough = np.zeros((R, R), np.float32)
    strip_rows = int(STRIP * R)
    chunk = 512
    for y0 in range(0, R, chunk):
        y1 = min(R, y0 + chunk)
        ys = np.arange(y0, y1)[:, None] + 0.5
        xs = np.arange(R)[None, :] + 0.5
        s_cm = np.broadcast_to(xs / px_per_cm, (y1 - y0, R))
        t_cm = np.broadcast_to(ys / px_per_cm, (y1 - y0, R))
        # upsample the 2048 field rows needed for this chunk
        a0, a1 = max(0, y0 // 2 - 2), min(R2, y1 // 2 + 2)
        sub = T.upsample2(F2[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        m = macro[y0:y1]
        h_w, warp_top, tone, slub = T.weave(s_cm, t_cm, st["warp"], st["weft"], seed)
        H = 0.13 * h_w                                    # mm
        col = base[None, None, :] * (tone * (0.80 + 0.20 * h_w) * (1 + 0.08 * slub))[..., None]
        rgh = 0.915 - 0.05 * h_w + 0.02 * slub
        stitch_mask = np.zeros_like(H)
        grime_edge = np.zeros_like(H)
        for fname, (style, w, insets) in st["seams"].items():
            if fname not in ch:
                continue
            d = sub[..., ch[fname]]
            a = sub[..., ch[fname] + 1]
            if style == "roll":
                inside = np.clip(1 - ((d - w / 2) / (w / 2)) ** 2, 0, 1)
                H += 0.55 * np.sqrt(inside) * (d < w)
                pucker = T.vnoise1(a * 1.3, seed + 31) - 0.5
                H += 0.16 * pucker * (1 - T.smoothstep(d, w * 0.6, w + 0.5))
            elif style == "felled":
                band = T.smoothstep(d, 0.0, 0.08) * (1 - T.smoothstep(d, w - 0.1, w + 0.05))
                H += 0.32 * band - 0.18 * (1 - T.smoothstep(d, 0.0, 0.07))
                H += 0.12 * (T.vnoise1(a * 0.9, seed + 33) - 0.5) * (1 - T.smoothstep(d, w, w + 0.8))
            elif style == "casing":
                band = 1 - T.smoothstep(d, w - 0.1, w + 0.05)
                fold = np.sqrt(np.clip(1 - (d / 0.45) ** 2, 0, 1))
                H += 0.30 * band + 0.35 * fold
                # drawstring gathers: irregular soft puckers, strongest in the casing, fading below it
                warp_a = a + 1.2 * T.vnoise1(a * 0.35, seed + 36)
                g1 = T.vnoise1(warp_a * 0.75, seed + 35) - 0.5
                g2 = T.vnoise1(warp_a * 1.9 + d * 0.4, seed + 37) - 0.5
                fade = 1 - T.smoothstep(d, w * 0.5, w + 2.2)
                H += (0.55 * g1 + 0.22 * g2) * fade
            for k, inset in enumerate(insets):
                sm = _stitches(d, a, inset, seed + 40 + 7 * k + sum(map(ord, fname)) % 97)
                stitch_mask = np.maximum(stitch_mask, sm)
                H -= 0.10 * np.exp(-((d - inset) / 0.09) ** 2)    # thread pulls a small groove
            grime_edge = np.maximum(grime_edge, 1 - T.smoothstep(d, 0.0, 0.5 if style != "casing" else 0.3))
            if fname == "leghem":
                fray = (1 - T.smoothstep(d, 0.0, 0.18)) * (T.vnoise1(a * 6.0, seed + 51) > 0.45)
                H += 0.12 * fray
                col = col * (1 + 0.10 * fray[..., None])
        H += 0.30 * stitch_mask
        # creases and crinkles of linen, gentle
        H += 0.34 * (1 - m[..., 3]) ** 6 + 0.14 * (1 - m[..., 4]) ** 5
        col = col * (1 - stitch_mask[..., None]) + thread_col[None, None] * (0.9 + 0.1 * h_w[..., None]) * stitch_mask[..., None]
        # ageing: mottled bleaching, greyed/yellowed edges, faint earth and grass
        col = col * (0.93 + 0.14 * m[..., 0:1])
        xyz = sub[..., ixyz:ixyz + 3]
        dirt_col = np.array([0.30, 0.245, 0.175])
        grass_col = np.array([0.20, 0.235, 0.12])
        if "Tank" in obj.name:
            yellow = 0.0
            if "bind" in ch:
                yellow = (1 - T.smoothstep(sub[..., ch["bind"]], 0.0, 2.5)) * 0.55
            col = col * (1 - 0.14 * np.asarray(yellow)[..., None] * np.array([0.0, 0.25, 1.0]))
            dirt = 0.15 * grime_edge + 0.14 * T.smoothstep(m[..., 1], 0.62, 0.78) * T.smoothstep(-xyz[..., 2], -1.26, -1.20)
            gr = 0.0
        else:
            low = 1 - T.smoothstep(xyz[..., 2], 0.86, 0.95)
            seat = T.smoothstep(xyz[..., 1], 0.02, 0.06) * T.smoothstep(xyz[..., 2], 0.82, 0.90)
            hands = np.exp(-((np.abs(xyz[..., 0]) - 0.16) / 0.035) ** 2) * np.exp(-((xyz[..., 2] - 0.96) / 0.05) ** 2) \
                * T.smoothstep(-xyz[..., 1], 0.0, 0.05)
            dirt = 0.16 * grime_edge + 0.20 * T.smoothstep(m[..., 1], 0.58, 0.75) * np.maximum(low, seat) + 0.22 * hands * m[..., 1]
            gr = 0.24 * T.smoothstep(m[..., 2], 0.64, 0.78) * np.maximum(low * T.smoothstep(-xyz[..., 1], 0.0, 0.05), seat)
        dirt = np.clip(dirt, 0, 0.45)[..., None]
        col = col * (1 - dirt) + dirt_col * dirt * (0.8 + 0.4 * h_w[..., None])
        gr = np.clip(np.asarray(gr), 0, 0.3)
        if np.ndim(gr):
            gr = gr[..., None]
            col = col * (1 - gr) + grass_col * gr
        rgh = rgh + 0.03 * dirt[..., 0]
        # accessory strip (drawstring cord / loose threads): 3-ply twisted cord
        if y0 < strip_rows:
            rr = min(y1, strip_rows) - y0
            u_cm = s_cm[:rr] * 0 + (xs / R) * 30.0
            v_l = (ys[:rr] / strip_rows)
            ply = 0.5 + 0.5 * np.sin(2 * np.pi * (u_cm / 0.42 - v_l * 3.0))
            fib = T.vnoise1(u_cm * 40 + v_l * 13, seed + 61)
            H[:rr] = 0.35 * ply ** 0.7 + 0.05 * fib
            col[:rr] = 0.78 * thread_col[None, None] * (0.72 + 0.28 * ply[..., None]) * (0.95 + 0.1 * fib[..., None])
            rgh[:rr] = 0.9
        albedo[y0:y1] = np.clip(col, 0, 0.74)
        height[y0:y1] = H
        rough[y0:y1] = np.clip(rgh, 0.80, 0.97)
    nrm = T.height_to_normal(height, texel_mm)
    cav = T.box_blur(height, max(2, int(px_per_cm * 0.12))) - height
    cav2 = T.box_blur(height, max(4, int(px_per_cm * 0.6))) - height
    ao = np.clip(1 - 1.6 * np.clip(cav, 0, None) - 0.6 * np.clip(cav2, 0, None), 0.55, 1.0)
    paths = {}
    for kind, img in (("basecolor", T.to_srgb(albedo)), ("normal", nrm), ("roughness", rough), ("ao", ao)):
        p = out_dir / f"T_{name}_{kind}.png"
        T.write_png(p, img)
        paths[kind] = p
    stats = dict(albedo_max=float(albedo.max()), albedo_mean=[float(x) for x in albedo[cov].mean(0)],
                 rough_mean=float(rough[cov].mean()), px_per_cm=px_per_cm, resolution=R)
    return paths, stats


def detail_weave(out_dir, seed=0, R=1024, tile_cm=4.0):
    """Tileable homespun plain-weave detail (4 cm tile) for close-up detail normals in Unreal."""
    y, x = np.mgrid[0:R, 0:R] + 0.5
    s = x / R * tile_cm
    t = y / R * tile_cm
    h, warp_top, tone, slub = T.weave(s, t, 9.0, 8.0, seed, period=(int(9 * tile_cm), int(8 * tile_cm)))
    H = 0.13 * h
    nrm = T.height_to_normal(H, tile_cm * 10.0 / R)
    rough = np.clip(0.915 - 0.05 * h + 0.02 * slub, 0.8, 0.97)
    shade = np.clip(tone * (0.80 + 0.20 * h) * (1 + 0.05 * slub), 0, 1)
    paths = {}
    for kind, img in (("normal", nrm), ("roughness", rough), ("tone", shade)):
        p = out_dir / f"T_HomespunWeave_Detail_{kind}.png"
        T.write_png(p, img)
        paths[kind] = p
    return paths
