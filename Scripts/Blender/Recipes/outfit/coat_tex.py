"""Sheepskin coat: shell-aware UV layout and the suede / fleece / horn texture set.

The coat is one skinned mesh with one material. Its outer shell is the buffed flesh side of the
skin (suede), its inner shell and the turned-out collar, cuffs and front edges are the fleece,
the rims show the cut edge of the skin. After Solidify the inner shell is packed at half texel
density (it is only seen at the openings) next to the outer shell and rims.
"""
import math

import bmesh
import bpy
import numpy as np

from . import textures as T
from .detail import STRIP


def shell_of_faces(me, is_outer):
    """0 = inner (fitted, fleece tips), 1 = outer (suede), 2 = rim."""
    out = np.empty(len(me.polygons), np.int64)
    for p in me.polygons:
        o = is_outer[np.array(p.vertices)]
        out[p.index] = 1 if o.all() else (0 if not o.any() else 2)
    return out


def unwrap_coat(obj, is_outer, inner_scale=0.5, rim_scale=0.8, extra_seams=()):
    me = obj.data
    shell = shell_of_faces(me, is_outer)
    panel = np.empty(len(me.polygons), np.int32)
    me.attributes["panel"].data.foreach_get("value", panel)
    key = panel.astype(np.int64) * 3 + shell
    bm = bmesh.new()
    bm.from_mesh(me)
    for e in bm.edges:
        fs = e.link_faces
        a, b = sorted(v.index for v in e.verts)
        e.seam = bool(len(fs) != 2 or key[fs[0].index] != key[fs[1].index] or (a, b) in extra_seams)
    bm.to_mesh(me)
    bm.free()
    if not me.uv_layers:
        me.uv_layers.new(name="UVMap")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.unwrap(method="ANGLE_BASED", margin=0.003)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    co = np.array([v.co for v in me.vertices])
    for k in np.unique(key):
        faces = np.where(key == k)[0]
        a3 = a2 = 0.0
        loops = []
        for fi in faces:
            p = me.polygons[fi]
            ls = list(range(p.loop_start, p.loop_start + p.loop_total))
            loops += ls
            P = co[[me.loops[l].vertex_index for l in ls]]
            U = uv[ls]
            for t in range(1, len(ls) - 1):
                a3 += np.linalg.norm(np.cross(P[t] - P[0], P[t + 1] - P[0])) / 2
                a2 += abs(np.cross(U[t] - U[0], U[t + 1] - U[0])) / 2
        loops = np.array(loops)
        fac = {0: inner_scale, 1: 1.0, 2: rim_scale}[int(k % 3)]
        sc = math.sqrt(a3 / max(a2, 1e-12)) * fac
        c0 = uv[loops].mean(0)
        uv[loops] = (uv[loops] - c0) * sc
    me.uv_layers.active.data.foreach_set("uv", uv.ravel())
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin=0.004)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2) * (1 - STRIP) + np.array([0.0, STRIP])
    me.uv_layers.active.data.foreach_set("uv", uv.ravel())
    # texel density of the outer shell (UV units per meter)
    a3 = a2 = 0.0
    for p in me.polygons:
        if shell[p.index] != 1:
            continue
        ls = list(range(p.loop_start, p.loop_start + p.loop_total))
        P = co[[me.loops[l].vertex_index for l in ls]]
        U = uv[ls]
        for t in range(1, len(ls) - 1):
            a3 += np.linalg.norm(np.cross(P[t] - P[0], P[t + 1] - P[0])) / 2
            a2 += abs(np.cross(U[t] - U[0], U[t + 1] - U[0])) / 2
    return math.sqrt(a2 / a3), shell


# ------------------------------------------------------------------ worley

def worley3(P, cell, seed):
    """F1, F2 distances (in cells) and the nearest feature's id for 3D points (..., 3)."""
    q = P / cell
    base = np.floor(q)
    f1 = np.full(q.shape[:-1], 9.0)
    f2 = np.full(q.shape[:-1], 9.0)
    fid = np.zeros(q.shape[:-1])
    fvec = np.zeros(q.shape)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                c = base + np.array([dx, dy, dz])
                h = c[..., 0] * 127.1 + c[..., 1] * 311.7 + c[..., 2] * 74.7
                fp = c + np.stack([T.hash1(h, seed), T.hash1(h, seed + 1), T.hash1(h, seed + 2)], -1)
                d = np.linalg.norm(fp - q, axis=-1)
                closer = d < f1
                f2 = np.where(closer, f1, np.minimum(f2, d))
                fid = np.where(closer, T.hash1(h, seed + 3), fid)
                fvec = np.where(closer[..., None], q - fp, fvec)
                f1 = np.where(closer, d, f1)
    return f1, f2, fid, fvec


def fleece_bumps(P, seed, cell=0.013):
    """Soft wool clumps (0..1) for displacing the turned-out fleece at the trims."""
    f1, f2, fid, _ = worley3(P, cell, seed)
    dome = 1 - T.smoothstep(f1, 0.1, 0.9)
    soft = T.fbm3(P * 45.0, 2, seed + 1)
    return np.clip(0.6 * dome * (0.8 + 0.4 * fid) + 0.4 * soft, 0, 1)


# ---------------------------------------------------------------- compose

SUEDE = np.array([0.195, 0.138, 0.082])
FLEECE_TIP = np.array([0.580, 0.520, 0.430])
FLEECE_ROOT = np.array([0.330, 0.280, 0.210])
HORN = (np.array([0.58, 0.52, 0.42]), np.array([0.045, 0.032, 0.024]))
THONG = np.array([0.115, 0.072, 0.042])
SINEW = np.array([0.52, 0.47, 0.38])


def compose_coat(obj, vfields, trim, shellc, face_shell, uvpm, out_dir, name, R=4096, seed=0, log=print):
    """vfields: name -> (dist m, along m) per vertex; trim: 0..1 per vertex (fleece turned out);
    shellc: 0 inner .. 1 outer per vertex; face_shell: per polygon."""
    me = obj.data
    me.calc_loop_triangles()
    nt = len(me.loop_triangles)
    loops = np.empty(nt * 3, np.int64)
    me.loop_triangles.foreach_get("loops", loops)
    poly = np.empty(nt, np.int64)
    me.loop_triangles.foreach_get("polygon_index", poly)
    lv = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get("vertex_index", lv)
    uv = np.empty(len(me.loops) * 2)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    loops = loops.reshape(-1, 3)
    uvt, vt = uv[loops], lv[loops]
    names = sorted(vfields)
    co = np.array([v.co for v in me.vertices])
    cols = []
    for n in names:
        cols += [vfields[n][0] * 100.0, vfields[n][1] * 100.0]
    cols += [trim, shellc, co[:, 0], co[:, 1], co[:, 2]]
    PV = np.stack(cols, 1).astype(np.float32)
    vals = PV[vt]
    fs = face_shell[poly].astype(np.float32)
    vals = np.concatenate([vals, np.repeat(fs[:, None, None], 3, 1)], 2)
    R2 = R // 2
    F2, cov2 = T.rasterize(uvt, vals, R2)
    F2 = T.dilate(F2, cov2, 16)
    cov = T.upsample2(cov2.astype(np.float32)) > 0.5
    ch = {n: 2 * i for i, n in enumerate(names)}
    k0 = 2 * len(names)
    i_trim, i_sc, i_xyz, i_shell = k0, k0 + 1, k0 + 2, k0 + 5
    px_per_cm = uvpm * R / 100.0
    texel_mm = 10.0 / px_per_cm
    log(f"  {name}: rasterized {nt} tris, coverage {cov.mean():.2%}, {px_per_cm:.1f} px/cm (suede)")
    albedo = np.zeros((R, R, 3), np.float32)
    height = np.zeros((R, R), np.float32)
    rough = np.zeros((R, R), np.float32)
    # wool clumps are low-frequency: evaluate the cellular field at half resolution
    xyz2 = F2[..., i_xyz:i_xyz + 3].astype(np.float64)
    clump_lo = np.zeros((R2, R2), np.float32)
    tuft_lo = np.zeros((R2, R2), np.float32)
    cid_lo = np.zeros((R2, R2), np.float32)
    for r0 in range(0, R2, 256):
        f1, _, fid, _ = worley3(xyz2[r0:r0 + 256], 0.011, seed + 10)
        clump_lo[r0:r0 + 256] = 1 - T.smoothstep(f1, 0.15, 0.85)
        cid_lo[r0:r0 + 256] = fid
        f1, f2, _, _ = worley3(xyz2[r0:r0 + 256], 0.0042, seed + 15)
        tuft_lo[r0:r0 + 256] = (1 - T.smoothstep(f1, 0.0, 0.8)) ** 0.6 * T.smoothstep(f2 - f1, 0.0, 0.25)
    del xyz2
    strip_rows = int(STRIP * R)
    chunk = 256
    for y0 in range(0, R, chunk):
        y1 = min(R, y0 + chunk)
        a0, a1 = max(0, y0 // 2 - 2), min(R2, y1 // 2 + 2)
        sub = T.upsample2(F2[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        xyz = sub[..., i_xyz:i_xyz + 3].astype(np.float64)
        shell = np.rint(sub[..., i_shell])
        trim_w = np.clip(sub[..., i_trim], 0, 1)
        sc = np.clip(sub[..., i_sc], 0, 1)
        # ---- suede (flesh side): fine nap, smoke-tan mottling, a few soft creases, burnished wear
        nap = T.fbm3(xyz * 900.0, 2, seed + 1)
        mott = T.fbm3(xyz * 7.0, 4, seed + 2)
        mott2 = T.fbm3(xyz * 26.0, 3, seed + 3)
        smoke = T.smoothstep(T.fbm3(xyz * 3.5 + 1.0, 3, seed + 8), 0.45, 0.75)
        s_col = SUEDE * (0.80 + 0.38 * mott[..., None]) * (0.90 + 0.20 * mott2[..., None]) * (0.90 + 0.18 * nap[..., None])
        s_col = s_col * (1 - 0.18 * smoke[..., None] * np.array([0.6, 0.8, 1.0]))
        scuff = T.smoothstep(T.fbm3(xyz * np.array([90.0, 90.0, 30.0]) + 5.0, 2, seed + 9), 0.66, 0.72)
        s_col = s_col * (1 - 0.10 * scuff[..., None])
        s_h = 0.08 * nap + 0.08 * mott2 - 0.05 * scuff
        s_r = 0.88 + 0.06 * nap - 0.05 * scuff
        # creases only where the skin bends: inside the elbows and across the small of the back
        bend = np.zeros_like(nap)
        for sx in (1.0, -1.0):
            c = np.array([0.32 * sx, -0.01, 1.20])
            bend = np.maximum(bend, 1 - T.smoothstep(np.linalg.norm(xyz - c, axis=-1), 0.03, 0.09))
        bend = np.maximum(bend, T.smoothstep(xyz[..., 1], 0.03, 0.08) * np.exp(-((xyz[..., 2] - 1.12) / 0.05) ** 2))
        cr = np.abs(np.sin((xyz[..., 2] * 90.0 + 6.0 * T.fbm3(xyz * 20.0, 2, seed + 4))))
        crease = bend * (1 - T.smoothstep(cr, 0.0, 0.25))
        s_col = s_col * (1 - 0.14 * crease[..., None])
        s_h -= 0.25 * crease
        # one healed scratch across the skin, very faint
        scar = np.clip(1 - np.abs(T.fbm3(xyz * np.array([40.0, 40.0, 6.0]) + 7.0, 2, seed + 5) - 0.5) * 60, 0, 1)
        scar = scar * T.smoothstep(T.fbm3(xyz * 3.0 - 2.0, 2, seed + 6), 0.70, 0.74)
        s_col = s_col * (1 - 0.12 * scar[..., None])
        s_h -= 0.08 * scar
        # hand grime / burnish near the front edges, cuffs and hem
        edge = np.full(xyz.shape[:-1], 99.0)
        for nme in ("opening", "cuff", "hem", "collartop"):
            if nme in ch:
                edge = np.minimum(edge, sub[..., ch[nme]])
        burn = (1 - T.smoothstep(edge, 1.0, 7.0)) * (0.5 + 0.7 * mott2)
        elbow = np.zeros_like(nap)
        for sx in (1.0, -1.0):
            c = np.array([0.325 * sx, 0.06, 1.19])
            elbow = np.maximum(elbow, 1 - T.smoothstep(np.linalg.norm(xyz - c, axis=-1), 0.02, 0.07))
        burn = np.clip(burn + 0.8 * elbow, 0, 1)
        s_col = s_col * (1 - 0.26 * burn[..., None])
        s_r = s_r - 0.12 * burn
        # seams: butted and whip-stitched with sinew, slanted stitches every ~5 mm
        stitch = np.zeros_like(nap)
        for nme in ("side", "shoulder", "armhole", "cuffseam", "collarseam"):
            if nme not in ch:
                continue
            d, a = sub[..., ch[nme]], sub[..., ch[nme] + 1]
            across = 1 - T.smoothstep(d, 0.16, 0.24)
            ph = (a + 0.55 * d) / 0.5 + 0.2 * T.vnoise1(a * 1.3, seed + 7)
            f = ph - np.floor(ph)
            st = across * (1 - T.smoothstep(np.abs(f - 0.5), 0.10, 0.18))
            stitch = np.maximum(stitch, st)
            s_h += 0.25 * (1 - T.smoothstep(d, 0.0, 0.08)) - 0.12 * across
        s_col = s_col * (1 - 0.85 * stitch[..., None]) + SINEW * (0.9 + 0.2 * nap[..., None]) * 0.85 * stitch[..., None]
        s_h += 0.28 * stitch
        # ---- fleece: shearling in tight curly tufts, gathered into soft locks
        clump = T.upsample2(clump_lo[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        tuft = T.upsample2(tuft_lo[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        cid = T.upsample2(cid_lo[a0:a1])[(y0 - 2 * a0):(y0 - 2 * a0) + (y1 - y0)]
        boucle = np.abs(2 * T.fbm3(xyz * 520.0 + 3.0, 3, seed + 14) - 1)          # rounded curls, sharp creases
        fib = T.fbm3(xyz * np.array([1500.0, 1500.0, 500.0]), 2, seed + 11)
        f_h = 0.28 * clump + 0.42 * tuft + 0.22 * boucle + 0.08 * fib
        tip = np.clip(-0.15 + 1.2 * f_h, 0, 1) ** 0.8
        f_col = FLEECE_ROOT * (1 - tip[..., None]) + FLEECE_TIP * tip[..., None]
        f_col = f_col * (0.94 + 0.12 * cid[..., None]) * (0.95 + 0.10 * fib[..., None])
        fine = fib
        yell = T.smoothstep(T.fbm3(xyz * 6.0 + 4.0, 3, seed + 12), 0.5, 0.75)
        f_col = f_col * (1 - 0.10 * yell[..., None] * np.array([0.0, 0.3, 1.0]))
        f_col = f_col * (1 - 0.25 * burn[..., None] * trim_w[..., None])
        f_r = 0.82 + 0.10 * (1 - f_h) + 0.03 * fine
        f_h = f_h * 1.6
        # ---- choose per texel: outer = suede unless a turned-out trim, inner = fleece, rim = cut edge
        is_fleece = np.where(shell == 1, T.smoothstep(trim_w, 0.25, 0.6), 1.0)
        is_fleece = np.where(shell == 2, np.where(sc > 0.9, 0.0, 1.0), is_fleece)
        col = s_col * (1 - is_fleece[..., None]) + f_col * is_fleece[..., None]
        H = s_h * (1 - is_fleece) + f_h * is_fleece
        rg = s_r * (1 - is_fleece) + f_r * is_fleece
        if y0 < strip_rows:
            rr = min(y1, strip_rows) - y0
            u = (np.arange(R)[None, :] + 0.5) / R
            v = (np.arange(y0, y0 + rr)[:, None] + 0.5) / strip_rows
            u = np.broadcast_to(u, (rr, R))
            v = np.broadcast_to(v, (rr, R))
            horn = u < 0.335
            # horn: near-black cores with honey-coloured translucent streaks, paler worn tips
            k = np.clip(np.mod(u - 0.02, 0.105) / 0.09, 0, 1)
            tipw = np.clip(np.abs(k - 0.5) * 2, 0, 1) ** 3
            streak = T.vnoise1(v * 45.0 + u * 5.0, seed + 20)
            fleck = T.vnoise1(v * 140.0 + u * 31.0, seed + 23)
            hc = HORN[1] * (0.7 + 0.6 * fleck[..., None]) + np.array([0.30, 0.19, 0.09]) * (streak[..., None] ** 3) * 0.6
            hc = hc * (1 - tipw[..., None]) + HORN[0] * 0.55 * tipw[..., None]
            thong = THONG * (0.85 + 0.3 * T.vnoise1(u * 300 + v * 17, seed + 21)[..., None])
            col[:rr] = np.where(horn[..., None], hc, thong)
            H[:rr] = np.where(horn, 0.06 * streak, 0.15 * T.vnoise1(u * 500, seed + 22))
            rg[:rr] = np.where(horn, 0.38 + 0.12 * streak, 0.78)
        albedo[y0:y1] = np.clip(col, 0, 0.74)
        height[y0:y1] = H
        rough[y0:y1] = np.clip(rg, 0.35, 0.97)
    nrm = T.height_to_normal(height, texel_mm, strength=1.4)
    cav = T.box_blur(height, max(2, int(px_per_cm * 0.15))) - height
    cav2 = T.box_blur(height, max(4, int(px_per_cm * 0.8))) - height
    ao = np.clip(1 - 1.4 * np.clip(cav, 0, None) - 0.8 * np.clip(cav2, 0, None), 0.45, 1.0)
    paths = {}
    for kind, img in (("basecolor", T.to_srgb(albedo)), ("normal", nrm), ("roughness", rough), ("ao", ao)):
        p = out_dir / f"T_{name}_{kind}.png"
        T.write_png(p, img)
        paths[kind] = p
    stats = dict(albedo_max=float(albedo.max()), albedo_mean=[float(x) for x in albedo[cov].mean(0)],
                 rough_mean=float(rough[cov].mean()), px_per_cm=px_per_cm, resolution=R)
    return paths, stats
