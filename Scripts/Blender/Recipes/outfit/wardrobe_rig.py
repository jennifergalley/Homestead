"""Skinning sources, test poses, layered poke metrics and sewn fittings for the wardrobe garments."""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector

from . import rig as R
from .body import Surface, mesh_arrays
from .detail import STRIP


# ------------------------------------------------------------------ weights

def region_source(full_src, keep_face):
    """Copy of the weight source keeping only faces where ``keep_face(centroid, face)`` is true."""
    dup = full_src.copy()
    dup.data = full_src.data.copy()
    bpy.context.scene.collection.objects.link(dup)
    me = dup.data
    bm = bmesh.new()
    bm.from_mesh(me)
    dl = bm.verts.layers.deform.active
    names = {vg.index: vg.name for vg in dup.vertex_groups}
    kill = []
    for f in bm.faces:
        c = f.calc_center_median()
        if not keep_face(c, f, dl, names):
            kill.append(f)
    bmesh.ops.delete(bm, geom=kill, context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bm.to_mesh(me)
    bm.free()
    dup.hide_render = True
    return dup


ARM_PREFIX = ("upperarm", "lowerarm", "hand", "thumb", "index", "middle", "ring", "pinky")
LEG_PREFIX = ("thigh", "calf", "foot", "ball", "bigtoe", "indextoe", "middletoe", "ringtoe", "littletoe")


def _share(f, dl, names, prefixes):
    tot = arm = 0.0
    for v in f.verts:
        for gi, w in v[dl].items():
            tot += w
            if names.get(gi, "").startswith(prefixes):
                arm += w
    return arm / max(tot, 1e-9)


def limb_sources(full_src):
    out = {}
    for side, sgn in (("l", 1), ("r", -1)):
        out[("arm", side)] = region_source(
            full_src, lambda c, f, dl, n, s=sgn: c.x * s > 0 and _share(f, dl, n, ARM_PREFIX) > 0.5)
        out[("leg", side)] = region_source(
            full_src, lambda c, f, dl, n, s=sgn: c.x * s > 0.0 and _share(f, dl, n, LEG_PREFIX) > 0.5)
    return out


def transfer_to_array(obj, src, names_all):
    """Nearest-face interpolated weights from ``src`` onto a temporary copy of ``obj``."""
    tmp = obj.copy()
    tmp.data = obj.data.copy()
    bpy.context.scene.collection.objects.link(tmp)
    for vg in list(tmp.vertex_groups):
        tmp.vertex_groups.remove(vg)
    R.transfer_weights(tmp, src)
    names, W = R.read_weights(tmp)
    out = np.zeros((len(obj.data.vertices), len(names_all)))
    col = {n: i for i, n in enumerate(names_all)}
    for j, n in enumerate(names):
        if n in col:
            out[:, col[n]] = W[:, j]
    me = tmp.data
    bpy.data.objects.remove(tmp)
    bpy.data.meshes.remove(me)
    return out


def skin(obj, full_src, limb, blend, gap, F_edges, iters=3):
    """Weights from the whole body, with limb vertices taking theirs from that limb alone
    (so a sleeve can't pick up the flank it hangs next to): ``blend`` maps a source key to a
    per-vertex 0..1 weight."""
    names_all = [vg.name for vg in full_src.vertex_groups]
    W = transfer_to_array(obj, full_src, names_all)
    for key, w in blend.items():
        if not np.any(w > 0):
            continue
        Wl = transfer_to_array(obj, limb[key], names_all)
        ok = Wl.sum(1) > 0.5
        w = np.where(ok, w, 0.0)
        W = W * (1 - w[:, None]) + Wl * w[:, None]
    alpha = np.clip((gap - 0.006) / 0.02, 0.05, 0.5)
    W = R.clean_weights(W, alpha, F_edges, iters=iters)
    for vg in list(obj.vertex_groups):
        obj.vertex_groups.remove(vg)
    R.write_weights(obj, names_all, W)
    return W


# ---------------------------------------------------------------- thickness

def smooth_normals(V, F, iters=4):
    N = np.zeros_like(V)
    for k in range(4):
        a, b, c = F[:, k], F[:, (k + 1) % 4], F[:, (k + 3) % 4]
        np.add.at(N, a, np.cross(V[b] - V[a], V[c] - V[a]))
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
    E = set()
    for f in F:
        for a, b in zip(f, np.roll(f, -1)):
            E.add((min(a, b), max(a, b)))
    E = np.array(sorted(E))
    deg = np.bincount(E.ravel(), minlength=len(V)).astype(np.float64)
    for _ in range(iters):
        avg = np.zeros_like(N)
        np.add.at(avg, E[:, 0], N[E[:, 1]])
        np.add.at(avg, E[:, 1], N[E[:, 0]])
        N = 0.5 * N + 0.5 * avg / np.maximum(deg, 1)[:, None]
        N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
    return N


def make_shell(obj, thickness):
    """Give a single-layer garment real thickness: the fitted surface becomes the inside (faces
    toward the body), a copy pushed out along smoothed normals by ``thickness`` (per vertex, m)
    the outside, closed by rim quads round every open edge. Unlike the Solidify modifier this
    never spikes at the corner where a slit ends, and the vertex order is known: inner shell
    0..n-1, outer shell n..2n-1. Weights, UVs and the face 'panel' attribute carry over."""
    me = obj.data
    n = len(me.vertices)
    V = np.array([v.co for v in me.vertices])
    F = np.array([list(p.vertices) for p in me.polygons], np.int64)
    assert F.shape[1] == 4
    N = smooth_normals(V, F)
    t = np.broadcast_to(np.asarray(thickness, np.float64), (n,))
    Vo = V + N * t[:, None]
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    bm.faces.ensure_lookup_table()
    uvl = bm.loops.layers.uv.active
    dl = bm.verts.layers.deform.active
    pl = bm.faces.layers.int.get("panel")
    inner = list(bm.verts)
    outer = []
    for i, v in enumerate(inner):
        nv = bm.verts.new(Vo[i])
        if dl is not None:
            for gi, w in v[dl].items():
                nv[dl][gi] = w
        outer.append(nv)
    bm.verts.ensure_lookup_table()
    orig_faces = list(bm.faces)
    boundary = []
    for f in orig_faces:
        vs = [v.index for v in f.verts]
        uvs = [lp[uvl].uv.copy() for lp in f.loops] if uvl else None
        nf = bm.faces.new([outer[i] for i in vs])
        if uvl:
            for lp, uv in zip(nf.loops, uvs):
                lp[uvl].uv = uv
        if pl is not None:
            nf[pl] = f[pl]
        nf.smooth = True
        for k in range(4):
            a, b = f.verts[k], f.verts[(k + 1) % 4]
            e = bm.edges.get((a, b))
            if e is not None and len(e.link_faces) == 1:
                uvab = (f.loops[k][uvl].uv.copy(), f.loops[(k + 1) % 4][uvl].uv.copy()) if uvl else None
                boundary.append((a.index, b.index, f[pl] if pl is not None else 0, uvab))
    for f in orig_faces:
        f.normal_flip()
    for a, b, pan, uvab in boundary:
        rf = bm.faces.new((inner[a], inner[b], outer[b], outer[a]))
        rf.smooth = True
        if pl is not None:
            rf[pl] = pan
        if uvl:
            ua, ub = uvab
            for lp, uv in zip(rf.loops, (ua, ub, ub, ua)):
                lp[uvl].uv = uv
    bm.normal_update()
    bm.to_mesh(me)
    bm.free()
    me.update()
    return N, Vo


# ------------------------------------------------------------------- poses

X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))
SPINE = ["spine_01", "spine_02", "spine_03", "spine_04", "spine_05"]

POSES = {
    # mid-stride, left leg forward, arms swinging opposite and hanging closer than the A pose
    "walk": [("pelvis", Z, 4), ("thigh_l", X, -26), ("calf_l", X, 10), ("foot_l", X, 8),
             ("thigh_r", X, 14), ("calf_r", X, 26), ("foot_r", X, -18)]
    + [(s, Z, -1.5) for s in SPINE]
    + [("upperarm_l", Y, 22), ("upperarm_r", Y, -22), ("upperarm_l", X, 20), ("upperarm_r", X, -22),
       ("lowerarm_l", X, -8), ("lowerarm_r", X, -24)],
    "kneel_left_forward": R.POSES["kneel_left_forward"],
    "squat": R.POSES["squat"],
    "arms_overhead": R.POSES["arms_overhead"],
    # top of the back-swing with an axe over her right shoulder: feet apart, knees soft, torso
    # wound up to the right, both arms raised forward and across, elbows bent
    "felling_swing": [("thigh_l", Y, -9), ("thigh_r", Y, 9), ("thigh_l", X, -6), ("thigh_r", X, -6),
                      ("calf_l", X, 12), ("calf_r", X, 12), ("pelvis", Z, -12)]
    + [(s, Z, -6) for s in SPINE] + [(s, X, -2) for s in SPINE]
    + [("clavicle_l", Y, -14), ("clavicle_r", Y, 14),
       ("upperarm_l", Y, 25), ("upperarm_r", Y, -25),
       ("upperarm_l", X, -140), ("upperarm_r", X, -150),
       ("upperarm_l", Z, -30), ("upperarm_r", Z, -30),
       ("lowerarm_l", X, -45), ("lowerarm_r", X, -55)],
}


def apply_pose(arm, name):
    R.reset_pose(arm)
    for bone, axis, deg in POSES[name]:
        R.rotate_bone_world(arm, bone, axis, deg)


def posed_surface(arm, body_obj, face_V, face_N, face_T):
    V, N, T = mesh_arrays(body_obj)
    hb = arm.pose.bones["head"]
    M = np.array(arm.matrix_world @ hb.matrix @ hb.bone.matrix_local.inverted())
    fV = face_V @ M[:3, :3].T + M[:3, 3]
    fN = face_N @ M[:3, :3].T
    return Surface(np.vstack([V, fV]), np.vstack([N, fN]), np.vstack([T, face_T + len(V)]))


def layer_poke(outer_obj, inner_obj, body_surf, mask=None, loft=None, idx=None):
    """How far the (posed) inner garment pushes into the outer one: outer-garment vertices
    (optionally only its fitted inner shell, ``idx``) inside the inner garment's outer shell.
    With ``loft`` (aligned with ``idx``), also counts where the inner garment would show through
    the outer garment's whole thickness."""
    from .layers import outer_shell
    lay = outer_shell(inner_obj, body_surf)
    V, _, _ = mesh_arrays(outer_obj)
    idx = np.arange(len(V)) if idx is None else np.asarray(idx)
    keep = np.ones(len(idx), bool) if mask is None else mask[idx]
    lft = None if loft is None else np.asarray(loft)[keep]
    idx = idx[keep]
    s, _, _, valid = lay.signed(V[idx])
    inside = valid & (s < -0.0008)
    depth = np.where(inside, -s, 0.0)
    out = dict(checked=int(len(idx)), inside=int(inside.sum()), inside_pct=float(100 * inside.mean()) if len(idx) else 0.0,
               max_depth_mm=float(depth.max() * 1000) if len(idx) else 0.0)
    if lft is not None:
        through = inside & (depth > lft)
        out["through_outer"] = int(through.sum())
        out["through_outer_pct"] = float(100 * through.mean()) if len(idx) else 0.0
    return out


# ---------------------------------------------------------------- fittings

def on_surface(surf, p, lift):
    _, loc, n = surf.signed(np.asarray(p, float)[None])
    return loc[0] + n[0] * lift, n[0]


def eyelets(g, V, surf):
    """Centres of the worked eyelets up both edges of the long shirt's neck slit (4 pairs)."""
    le, ri = g.extra["slit"]
    P_l, P_r = V[le], V[ri]
    z0, z1 = P_l[:, 2].min(), P_l[:, 2].max()
    zs = np.linspace(z0 + 0.016, z1 - 0.016, 4)
    eye = {"l": [], "r": []}
    for side, P, sgn in (("l", P_l, 1.0), ("r", P_r, -1.0)):
        for z in zs:
            k = int(np.argmin(np.abs(P[:, 2] - z)))
            q, _ = on_surface(surf, P[k] + np.array([sgn * 0.0065, 0.0, 0.0]), 0.0)
            eye[side].append(q)
    return eye


def lacing(garment, eye, uv_x=(0.02, 0.98)):
    """Linen cord laced criss-cross through the eyelets, the two ends left hanging from the top.
    Between eyelets the cord rides a couple of millimetres proud of the cloth; at each eyelet it
    dips into the hole."""
    surf_V, surf_N, surf_T = mesh_arrays(garment, evaluated=False)
    gs = Surface(surf_V, surf_N, surf_T)
    E = np.array(eye["l"] + eye["r"])
    out = np.array([0.0, -1.0, 0.0])
    _, _, nrm = gs.signed(E)
    n_front = nrm[(nrm @ out) > 0].mean(0) if ((nrm @ out) > 0).any() else out
    n_front /= np.linalg.norm(n_front)
    parts = []
    down = np.array([0.0, 0.0, -1.0])
    for k, (a, b) in enumerate((("l", "r"), ("r", "l"))):
        pts = []
        seq = [eye[a][0], eye[b][1], eye[a][2], eye[b][3]]
        for i in range(len(seq) - 1):
            for t in np.linspace(0, 1, 10, endpoint=False):
                q = seq[i] * (1 - t) + seq[i + 1] * t
                pts.append(q + n_front * (0.0004 + (0.0024 + 0.0008 * k) * math.sin(math.pi * t)))
        top = seq[-1]
        hang = 0.065 + 0.012 * k
        for t in np.linspace(0, 1, 14)[1:]:
            q = top + down * hang * t + np.array([(0.004 if k else -0.004) * t, 0, 0])
            p_, pn = on_surface(gs, q, 0.0)
            if pn @ n_front < 0:
                pn = -pn
            pts.append(p_ + pn * (0.0022 + 0.0015 * t))
        parts.append(R.tube_mesh(f"_lace{k}", [seq[0] - n_front * 0.0015] + pts,
                                 lambda s: 0.0014 * (1.35 if s > 0.975 else 1.0), 8,
                                 (uv_x[0] + 0.48 * k, uv_x[0] + 0.48 * k + 0.44, 0.0, STRIP * 0.62)))
    return parts


def toggles(coat, opening, V, n_toggles=3, z_list=(1.115, 1.205, 1.295)):
    """Horn toggles sewn to her left front with a leather thong, and leather loops from the right
    front that pass round them. UVs land in the coat's accessory strip."""
    surf_V, surf_N, surf_T = mesh_arrays(coat, evaluated=False)
    gs = Surface(surf_V, surf_N, surf_T)
    le, ri = opening
    parts = []
    for k, z in enumerate(z_list):
        kl = int(np.argmin(np.abs(V[le, 2] - z)))
        kr = int(np.argmin(np.abs(V[ri, 2] - z)))
        edge_l, edge_r = V[le[kl]], V[ri[kr]]
        c, n = on_surface(gs, edge_l + np.array([0.042, -0.02, 0.0]), 0.0)
        tang = np.array([1.0, 0.0, 0.0]) - n * n[0]
        tang /= np.linalg.norm(tang)
        L = 0.044
        rad = 0.0056
        pts = []
        for t in np.linspace(-0.5, 0.5, 13):
            bow = 0.003 * (1 - (2 * t) ** 2)
            pts.append(c + n * (rad + 0.0022 + bow) + tang * L * t)
        u0 = 0.02 + 0.105 * k
        parts.append(R.tube_mesh(f"_toggle{k}", pts, lambda s: rad * (0.62 + 0.38 * math.sin(math.pi * s) ** 0.5), 10,
                                 (u0, u0 + 0.09, 0.0, STRIP * 0.45)))
        # lashing that holds the toggle on: a short thong wrapped round its middle
        wrap = [c + n * (0.001 + rad * (1.0 + 0.9 * math.cos(a))) + np.array([0, 0, rad * 1.25 * math.sin(a)])
                for a in np.linspace(-0.3, 2 * math.pi + 0.3, 16)]
        parts.append(R.tube_mesh(f"_lash{k}", wrap, 0.0016, 6, (0.34 + 0.1 * k, 0.43 + 0.1 * k, STRIP * 0.5, STRIP * 0.95)))
        # the loop: from the right front, across the opening, round the toggle's shank
        a_pt, an = on_surface(gs, edge_r + np.array([-0.030, -0.02, 0.0]), 0.0)
        path = []
        for t in np.linspace(0, 1, 18):
            q = a_pt * (1 - t) + (c - tang * 0.006) * t
            q = q + np.array([0, 0, 0.0045 * math.sin(math.pi * t) * 0.3])
            p_, pn = on_surface(gs, q, 0.0030 + 0.0045 * math.sin(math.pi * t))
            path.append(p_ + np.array([0, 0, 0.0035]))
        back = []
        for t in np.linspace(1, 0, 18):
            q = a_pt * (1 - t) + (c - tang * 0.006) * t
            p_, pn = on_surface(gs, q, 0.0030 + 0.0045 * math.sin(math.pi * t))
            back.append(p_ - np.array([0, 0, 0.0035]))
        turn = [c + n * (rad + 0.0022) + np.array([0, 0, 0.0035 * math.cos(a)]) + (n * 0.0 - tang * 0.0) +
                n * (rad * 0.9 * math.sin(a)) for a in np.linspace(0, math.pi, 7)]
        loop = path + turn[1:-1][::-1] + back
        parts.append(R.tube_mesh(f"_loop{k}", loop, 0.0017, 6, (0.64 + 0.11 * k, 0.74 + 0.11 * k, STRIP * 0.5, STRIP * 0.95)))
    return parts
