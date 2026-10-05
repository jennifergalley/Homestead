"""Texture synthesis for the wardrobe: hand-woven linen (tabby) and fulled wool (2/2 twill) garments,
and the sheepskin coat (suede flesh side, fleece, horn and leather fittings).

Same approach as ``cloth.py``: per-texel garment attributes (rest position, distances to seams,
edges and features) are rasterized from vertex data into the garment's own UV layout, the weave
is evaluated in flat pattern space, and the tangent-space normal map is the gradient of the
composed height (OpenGL, +Y up). Cloth maps are computed at twice the output size and box-filtered
down, so thread-scale detail anti-aliases instead of shimmering.
"""
import numpy as np

from . import textures as T
from .cloth import _stitches, _tri_data
from .detail import STRIP

LINEN_THREAD = (0.54, 0.51, 0.44)

STYLES = {
    "SKM_LinenTee": dict(
        fabric="linen", base=(0.200, 0.245, 0.290), thread=LINEN_THREAD, warp=11.0, weft=10.0, streak=0.035,
        fade=0.16, seams={"hem": ("roll", 1.0, [0.72]), "cuff": ("roll", 0.9, [0.62]), "neck": ("bind", 0.8, [0.2, 0.62]),
                          "side": ("felled", 0.8, [0.15, 0.65]), "shoulder": ("felled", 0.8, [0.15, 0.65]),
                          "armhole": ("felled", 0.8, [0.15, 0.65])},
        grime=dict(neck=0.16, cuff=0.10, hem=0.10), yellow=0.0),
    "SKM_LinenLongShirt": dict(
        fabric="linen", base=(0.585, 0.548, 0.470), thread=(0.53, 0.50, 0.43), warp=14.0, weft=13.0, streak=0.02,
        fade=0.05, seams={"hem": ("roll", 0.9, [0.62]), "cuff": ("roll", 0.8, [0.55]),
                          "cuffseam": ("gather", 0.5, [0.25]), "neck": ("bind", 0.9, [0.2, 0.7]),
                          "side": ("felled", 0.8, [0.15, 0.65]), "shoulder": ("felled", 0.8, [0.15, 0.65]),
                          "armhole": ("felled", 0.8, [0.15, 0.65])},
        grime=dict(neck=0.18, cuff=0.16, hem=0.08), yellow=0.5),
    "SKM_WoolTrousers": dict(
        fabric="wool", base=(0.086, 0.054, 0.034), thread=(0.155, 0.120, 0.088), warp=7.5, weft=7.0, streak=0.03,
        fade=0.08, heather=0.22, seams={"waist": ("casing", 3.0, [0.35, 2.8]), "leghem": ("turn", 2.4, [1.8]),
                                        "side": ("felled", 0.9, [0.15, 0.75]), "inseam": ("felled", 0.9, [0.15, 0.75]),
                                        "crotch": ("felled", 0.9, [0.15, 0.75])},
        grime=dict(hem=0.26), yellow=0.0),
}


def twill(s_cm, t_cm, warp_per_cm, weft_per_cm, seed=0, period=None):
    """2/2 twill of hand-spun woollen yarn: warp floats over two picks, stepping one thread per
    pick, which makes the diagonal rib. Returns height [0,1], warp-on-top, tone, slub."""
    ws, fs = s_cm * warp_per_cm, t_cm * weft_per_cm
    i = np.floor(ws); fu = ws - i - 0.5
    j = np.floor(fs); fv = fs - j - 0.5
    pw = pf = None
    if period:
        pw, pf = period
        i, j = np.mod(i, pw), np.mod(j, pf)
    slw = np.clip(T.vnoise1(fs * 0.2 + T.hash1(i, seed) * 91.0, seed + 3, pf and pf * 0.2) - 0.6, 0, 1) * 2.4
    slf = np.clip(T.vnoise1(ws * 0.2 + T.hash1(j, seed + 5) * 91.0, seed + 7, pw and pw * 0.2) - 0.6, 0, 1) * 2.4
    thw = 0.9 + 0.3 * T.hash1(i, seed + 11) + 0.4 * slw
    thf = 0.9 + 0.3 * T.hash1(j, seed + 13) + 0.4 * slf
    fu = fu + 0.10 * (T.vnoise1(fs * 0.5 + i * 3.1, seed + 17, pf and pf * 0.5) - 0.5)
    fv = fv + 0.10 * (T.vnoise1(ws * 0.5 + j * 3.1, seed + 19, pw and pw * 0.5) - 0.5)
    over = np.mod(i + j, 4) < 2
    prof_w = np.sqrt(np.clip(1 - (fu / np.minimum(0.44 * thw, 0.5)) ** 2, 0, 1))
    prof_f = np.sqrt(np.clip(1 - (fv / np.minimum(0.44 * thf, 0.5)) ** 2, 0, 1))
    # floats sag between their tie points
    phase = np.mod(i + j, 4)
    lift_w = np.where(over, 0.8 + 0.2 * np.cos(np.pi * (fv + np.where(phase == 0, 0.5, -0.5))), 0.35)
    lift_f = np.where(~over, 0.8 + 0.2 * np.cos(np.pi * (fu + np.where(phase == 2, 0.5, -0.5))), 0.35)
    hw = prof_w * lift_w
    hf = prof_f * lift_f
    warp_top = hw >= hf
    h = np.maximum(hw, hf)
    tone = np.where(warp_top, 0.96 + 0.09 * T.hash1(i, seed + 23), 0.92 + 0.09 * T.hash1(j, seed + 29))
    slub = np.where(warp_top, slw, slf)
    return h, warp_top, tone, slub


def _seam_height(style, d, a, w, seed, H, col):
    if style == "roll":
        inside = np.clip(1 - ((d - w / 2) / (w / 2)) ** 2, 0, 1)
        H += 0.55 * np.sqrt(inside) * (d < w)
        H += 0.16 * (T.vnoise1(a * 1.3, seed + 31) - 0.5) * (1 - T.smoothstep(d, w * 0.6, w + 0.5))
    elif style == "turn":
        band = 1 - T.smoothstep(d, w - 0.12, w + 0.05)
        fold = np.sqrt(np.clip(1 - (d / 0.5) ** 2, 0, 1))
        H += 0.30 * band + 0.30 * fold
        H += 0.14 * (T.vnoise1(a * 0.8, seed + 32) - 0.5) * (1 - T.smoothstep(d, w, w + 1.0))
    elif style == "bind":
        band = 1 - T.smoothstep(d, w - 0.08, w + 0.04)
        fold = np.sqrt(np.clip(1 - (d / 0.35) ** 2, 0, 1))
        H += 0.34 * band + 0.25 * fold
        H -= 0.12 * np.exp(-((d - w) / 0.06) ** 2)
    elif style == "felled":
        band = T.smoothstep(d, 0.0, 0.08) * (1 - T.smoothstep(d, w - 0.1, w + 0.05))
        H += 0.32 * band - 0.18 * (1 - T.smoothstep(d, 0.0, 0.07))
        H += 0.12 * (T.vnoise1(a * 0.9, seed + 33) - 0.5) * (1 - T.smoothstep(d, w, w + 0.8))
    elif style == "casing":
        band = 1 - T.smoothstep(d, w - 0.1, w + 0.05)
        fold = np.sqrt(np.clip(1 - (d / 0.45) ** 2, 0, 1))
        H += 0.30 * band + 0.35 * fold
        warp_a = a + 1.2 * T.vnoise1(a * 0.35, seed + 36)
        g1 = T.vnoise1(warp_a * 0.75, seed + 35) - 0.5
        g2 = T.vnoise1(warp_a * 1.9 + d * 0.4, seed + 37) - 0.5
        H += (0.55 * g1 + 0.22 * g2) * (1 - T.smoothstep(d, w * 0.5, w + 2.2))
    elif style == "gather":
        # tiny stroked gathers either side of a gathering seam
        pl = np.sin(2 * np.pi * (a / 0.22 + 0.4 * T.vnoise1(a * 0.8, seed + 38)))
        H += 0.30 * pl * (1 - T.smoothstep(d, 0.0, 1.6)) + 0.2 * (1 - T.smoothstep(d, 0.0, w))
    return H


def compose_cloth(obj, fields, uvpm, out_dir, name, R_out=2048, supersample=2, seed=0, log=print,
                  eyelets=None, patch=None):
    st = STYLES[obj.name]
    R = R_out * supersample
    me = obj.data
    uvt, vt = _tri_data(me)
    names = sorted(fields)
    per_vert = []
    for n in names:
        per_vert += [fields[n][0] * 100.0, fields[n][1] * 100.0]
    co = np.array([v.co for v in me.vertices])
    per_vert += [co[:, 0], co[:, 1], co[:, 2]]
    PV = np.stack(per_vert, 1).astype(np.float32)
    R2 = R // 2
    F2, cov2 = T.rasterize(uvt, PV[vt], R2)
    F2 = T.dilate(F2, cov2, 24)
    cov = T.upsample2(cov2.astype(np.float32)) > 0.5
    log(f"  {name}: rasterized {len(uvt)} tris, coverage {cov.mean():.2%}, {uvpm * R / 100:.1f} px/cm (internal)")
    ch = {n: 2 * i for i, n in enumerate(names)}
    ixyz = 2 * len(names)
    xyz4 = F2[::2, ::2, ixyz:ixyz + 3].astype(np.float64)
    macro = np.stack([T.fbm3(xyz4 * 14.0, 4, seed + 1), T.fbm3(xyz4 * 7.0 + 3.1, 4, seed + 2),
                      T.fbm3(xyz4 * 11.0 - 5.3, 4, seed + 3), np.abs(T.fbm3(xyz4 * 19.0 + 1.7, 3, seed + 4) - 0.5) * 2,
                      np.abs(T.fbm3(xyz4 * 55.0 - 2.9, 2, seed + 5) - 0.5) * 2], -1).astype(np.float32)
    macro = T.upsample2(T.upsample2(macro))
    px_per_cm = uvpm * R / 100.0
    texel_mm = 10.0 / px_per_cm
    base = np.array(st["base"])
    thread_col = np.array(st["thread"])
    wool = st["fabric"] == "wool"
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
        a0, a1 = max(0, y0 // 2 - 2), min(R2, y1 // 2 + 2)
        sub = T.upsample2(F2[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        m = macro[y0:y1]
        xyz = sub[..., ixyz:ixyz + 3]
        if wool:
            h_w, warp_top, tone, slub = twill(s_cm, t_cm, st["warp"], st["weft"], seed)
            fuzz = T.vnoise1(s_cm * 38.0 + T.vnoise1(t_cm * 9.0, seed + 71) * 3.0, seed + 72) * 0.6 + \
                T.vnoise1(t_cm * 41.0 + s_cm * 7.0, seed + 73) * 0.4
            H = 0.10 * h_w + 0.07 * fuzz                   # fulled: the nap softens the weave
            fibre = T.hash1(np.floor(s_cm * 60) * 13.1 + np.floor(t_cm * 60) * 7.7, seed + 74)
            heather = 1 + st["heather"] * (np.where(fibre > 0.93, 0.9, 0.0) + np.where(fibre < 0.05, -0.6, 0.0))
            col = base[None, None, :] * (tone * (0.84 + 0.16 * h_w) * (0.92 + 0.16 * fuzz) * heather)[..., None]
            rgh = 0.93 - 0.03 * h_w + 0.02 * fuzz
        else:
            h_w, warp_top, tone, slub = T.weave(s_cm, t_cm, st["warp"], st["weft"], seed)
            H = 0.12 * h_w
            col = base[None, None, :] * (tone * (0.80 + 0.20 * h_w) * (1 + 0.08 * slub))[..., None]
            rgh = 0.915 - 0.05 * h_w + 0.02 * slub
        # uneven dye take-up along the warp threads (hand-dyed yarn), and sun-faded patches
        ithread = np.floor(s_cm * st["warp"])
        streak = 1 + st["streak"] * (T.hash1(ithread, seed + 81) - 0.5) * 2 * (0.6 + 0.4 * T.vnoise1(t_cm * 0.3 + ithread * 1.7, seed + 82))
        col = col * streak[..., None]
        fade = st["fade"] * T.smoothstep(m[..., 1], 0.45, 0.8)
        col = col * (1 - fade[..., None]) + np.array([0.62, 0.60, 0.55]) * base.mean() / 0.6 * fade[..., None] * 1.0
        stitch_mask = np.zeros_like(H)
        grime = np.zeros_like(H)
        for fname, (style, w, insets) in st["seams"].items():
            if fname not in ch:
                continue
            d = sub[..., ch[fname]]
            a = sub[..., ch[fname] + 1]
            H = _seam_height(style, d, a, w, seed, H, col)
            for k, inset in enumerate(insets):
                sm = _stitches(d, a, inset, seed + 40 + 7 * k + sum(map(ord, fname)) % 97)
                stitch_mask = np.maximum(stitch_mask, sm)
                H -= 0.10 * np.exp(-((d - inset) / 0.09) ** 2)
            if fname in st["grime"]:
                g_w = st["grime"][fname]
                grime = np.maximum(grime, g_w * (1 - T.smoothstep(d, 0.0, 2.5 if fname != "hem" else 6.0))
                                   * (0.6 + 0.8 * m[..., 1]))
            if fname in ("leghem", "hem", "cuff") and style in ("roll", "turn"):
                fray = (1 - T.smoothstep(d, 0.0, 0.16)) * (T.vnoise1(a * 6.0, seed + 51) > 0.5)
                H += 0.10 * fray
        # eyelets worked with buttonhole stitch round a pierced hole (long shirt slit)
        if eyelets is not None and len(eyelets):
            dmin = np.full(H.shape, 1e3)
            for c in eyelets:
                dmin = np.minimum(dmin, np.linalg.norm(xyz - c[None, None, :], axis=-1) * 100.0)
            ring = np.exp(-((dmin - 0.20) / 0.08) ** 2)
            hole = 1 - T.smoothstep(dmin, 0.10, 0.15)
            stitch_mask = np.maximum(stitch_mask, ring * 0.95)
            H += 0.45 * ring - 0.5 * hole
            col = col * (1 - 0.75 * hole[..., None])
        # a mended knee: a patch of greyer wool, raw edges turned under and running-stitched
        if patch is not None:
            c, half, nrm_y = patch
            du = np.abs(xyz[..., 0] - c[0]) / half[0]
            dv = np.abs(xyz[..., 2] - c[2]) / half[1]
            front = T.smoothstep(-(xyz[..., 1] - c[1]), -0.02, 0.0)
            wob = 0.04 * (T.vnoise1((xyz[..., 0] + xyz[..., 2]) * 80.0, seed + 91) - 0.5)
            inside = (np.maximum(du, dv) < 1.0 + wob) * front
            edge_d = (1.0 - np.maximum(du, dv)) * min(half) * 100.0      # cm in from the patch edge
            pcol = np.array([0.125, 0.105, 0.085])
            col = col * (1 - inside[..., None]) + pcol * (col / base * 1.0).mean(-1, keepdims=True) * inside[..., None]
            H += 0.22 * inside + 0.18 * inside * (1 - T.smoothstep(edge_d, 0.0, 0.25))
            ps = _stitches(edge_d, (xyz[..., 0] + xyz[..., 2]) * 100.0, 0.35, seed + 92) * inside
            stitch_mask = np.maximum(stitch_mask, ps)
        H += 0.30 * stitch_mask
        H += (0.34 if not wool else 0.18) * (1 - m[..., 3]) ** 6 + 0.12 * (1 - m[..., 4]) ** 5
        col = col * (1 - stitch_mask[..., None]) + thread_col[None, None] * (0.9 + 0.1 * h_w[..., None]) * stitch_mask[..., None]
        col = col * (0.93 + 0.14 * m[..., 0:1])
        dirt_col = np.array([0.24, 0.19, 0.13]) if not wool else np.array([0.20, 0.155, 0.10])
        dirt = grime
        if wool and st.get("wool_wear", True):
            knees = np.zeros_like(H)
            for sx in (1.0, -1.0):
                kc = np.array([0.105 * sx, -0.06, 0.50])
                dk = np.linalg.norm((xyz - kc[None, None]) * np.array([1.0, 0.6, 1.0]), axis=-1)
                knees = np.maximum(knees, (1 - T.smoothstep(dk, 0.03, 0.09)) * T.smoothstep(-(xyz[..., 1] + 0.02), 0.0, 0.03))
            seat = T.smoothstep(xyz[..., 1], 0.03, 0.07) * T.smoothstep(xyz[..., 2], 0.80, 0.88) * (1 - T.smoothstep(xyz[..., 2], 0.95, 1.0))
            mud = 1 - T.smoothstep(xyz[..., 2], 0.09, 0.30)
            dirt = dirt + 0.30 * knees * (0.5 + m[..., 1]) + 0.12 * seat * m[..., 1] + 0.30 * mud * T.smoothstep(m[..., 2], 0.35, 0.7)
        elif not wool:
            if st["yellow"]:
                # sweat-yellowed underarms and collar on unbleached linen
                ua = np.zeros_like(H)
                for sx in (1.0, -1.0):
                    c = np.array([0.135 * sx, 0.0, 1.30])
                    ua = np.maximum(ua, 1 - T.smoothstep(np.linalg.norm(xyz - c[None, None], axis=-1), 0.02, 0.07))
                col = col * (1 - st["yellow"] * 0.18 * ua[..., None] * np.array([0.0, 0.25, 1.0]))
            dust = (1 - T.smoothstep(xyz[..., 2], 1.02, 1.12)) * T.smoothstep(-xyz[..., 1], 0.0, 0.05)
            dirt = dirt + st.get("dust", 0.08) * dust * m[..., 1]
        dirt = np.clip(dirt, 0, 0.45)[..., None]
        col = col * (1 - dirt) + dirt_col * dirt * (0.8 + 0.4 * h_w[..., None])
        rgh = rgh + 0.03 * dirt[..., 0]
        if y0 < strip_rows:
            rr = min(y1, strip_rows) - y0
            u_cm = (xs / R) * 30.0 + 0 * s_cm[:rr]
            v_l = ys[:rr] / strip_rows
            ply = 0.5 + 0.5 * np.sin(2 * np.pi * (u_cm / 0.42 - v_l * 3.0))
            fib = T.vnoise1(u_cm * 40 + v_l * 13, seed + 61)
            H[:rr] = 0.35 * ply ** 0.7 + 0.05 * fib
            cord = thread_col if not wool else np.array([0.36, 0.31, 0.24])
            col[:rr] = 0.8 * cord[None, None] * (0.72 + 0.28 * ply[..., None]) * (0.95 + 0.1 * fib[..., None])
            rgh[:rr] = 0.9
        albedo[y0:y1] = np.clip(col, 0, 0.74)
        height[y0:y1] = H
        rough[y0:y1] = np.clip(rgh, 0.80, 0.97)
    nrm = T.height_to_normal(height, texel_mm)
    cav = T.box_blur(height, max(2, int(px_per_cm * 0.12))) - height
    cav2 = T.box_blur(height, max(4, int(px_per_cm * 0.6))) - height
    ao = np.clip(1 - 1.6 * np.clip(cav, 0, None) - 0.6 * np.clip(cav2, 0, None), 0.55, 1.0)
    albedo, rough, ao = down(albedo, supersample), down(rough, supersample), down(ao, supersample)
    nrm = down(nrm * 2 - 1, supersample)
    nrm = nrm / np.maximum(np.linalg.norm(nrm, axis=-1, keepdims=True), 1e-6) * 0.5 + 0.5
    cov_o = down(cov.astype(np.float32), supersample) > 0.5
    paths = {}
    for kind, img in (("basecolor", T.to_srgb(albedo)), ("normal", nrm), ("roughness", rough), ("ao", ao)):
        pth = out_dir / f"T_{name}_{kind}.png"
        T.write_png(pth, img)
        paths[kind] = pth
    stats = dict(albedo_max=float(albedo.max()), albedo_mean=[float(x) for x in albedo[cov_o].mean(0)],
                 rough_mean=float(rough[cov_o].mean()), px_per_cm=px_per_cm / supersample, resolution=R_out)
    return paths, stats


def down(a, f):
    if f == 1:
        return a
    h, w = a.shape[:2]
    return a.reshape(h // f, f, w // f, f, *a.shape[2:]).mean((1, 3))


def wool_detail(out_dir, seed=0, R=1024, tile_cm=4.0):
    """Tileable fulled-wool twill detail (4 cm tile) for close-up normals on the trousers."""
    y, x = np.mgrid[0:R, 0:R] + 0.5
    s = x / R * tile_cm
    t = y / R * tile_cm
    h, _, tone, _ = twill(s, t, 7.5, 7.0, seed, period=(int(7.5 * tile_cm), int(7 * tile_cm)))
    fuzz = T.vnoise1(s * 38.0, seed + 72, 38 * tile_cm) * 0.5 + T.vnoise1(t * 41.0, seed + 73, 41 * tile_cm) * 0.5
    H = 0.10 * h + 0.07 * fuzz
    nrm = T.height_to_normal(H, tile_cm * 10.0 / R)
    rough = np.clip(0.93 - 0.03 * h + 0.02 * fuzz, 0.8, 0.97)
    shade = np.clip(tone * (0.84 + 0.16 * h) * (0.92 + 0.16 * fuzz), 0, 1)
    paths = {}
    for kind, img in (("normal", nrm), ("roughness", rough), ("tone", shade)):
        p = out_dir / f"T_HomespunWool_Detail_{kind}.png"
        T.write_png(p, img)
        paths[kind] = p
    return paths
