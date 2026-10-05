"""Clerk outfit geometry helpers: apron panel, tapes, neckerchief, rolled sleeve bands, buttons,
boots shell and the pencil prop. Plain numpy + bpy mesh construction; used by clerk_outfit.py."""
import math

import bmesh
import bpy
import numpy as np

from outfit import layers as Ly


def unit(v):
    v = np.asarray(v, float)
    return v / max(np.linalg.norm(v), 1e-12)


def link(obj):
    bpy.context.scene.collection.objects.link(obj)
    return obj


def mesh_object(name, V, F, uv=None, panel=None, smooth=True):
    """uv: per-loop list aligned with F (list of lists of (u, v)) or None."""
    me = bpy.data.meshes.new(name)
    me.from_pydata(np.asarray(V).tolist(), [], [list(map(int, f)) for f in F])
    me.update()
    if smooth:
        me.shade_smooth()
    if uv is not None:
        lay = me.uv_layers.new(name="UVMap")
        k = 0
        for poly in me.polygons:
            for li in poly.loop_indices:
                lay.data[li].uv = uv[poly.index][li - poly.loop_start]
            k += 1
    if panel is not None:
        a = me.attributes.new("panel", "INT", "FACE")
        a.data.foreach_set("value", np.asarray(panel, np.int32))
    return link(bpy.data.objects.new(name, me))


def smooth1d(a, iters, closed=False, axis=0):
    a = np.array(a, float)
    for _ in range(iters):
        if closed:
            a = 0.5 * a + 0.25 * (np.roll(a, 1, axis) + np.roll(a, -1, axis))
        else:
            b = a.copy()
            sl = [slice(None)] * a.ndim
            s0, s1, s2 = list(sl), list(sl), list(sl)
            s0[axis], s1[axis], s2[axis] = slice(1, -1), slice(0, -2), slice(2, None)
            b[tuple(s0)] = 0.5 * a[tuple(s0)] + 0.25 * (a[tuple(s1)] + a[tuple(s2)])
            a = b
    return a


# ----------------------------------------------------------------- envelopes

def front_envelope(cloud, xs, zs, rx=0.015, rz=0.015):
    """Minimum y (most forward) of the point cloud near each (x, z); NaN where empty."""
    out = np.full((len(zs), len(xs)), np.nan)
    for i, z in enumerate(zs):
        band = cloud[np.abs(cloud[:, 2] - z) < rz]
        if not len(band):
            continue
        for j, x in enumerate(xs):
            m = np.abs(band[:, 0] - x) < rx
            if m.any():
                out[i, j] = band[m, 1].min()
    return out


def radial_ring(cloud, center_xy, z, thetas, dz=0.012, dth=6.0, max_r=None):
    """Max radius of the cloud about a vertical axis at height z for each theta (deg, 0 = front
    -y, +90 = +x). Gaps are interpolated around the ring. max_r drops far points (shoulders)."""
    band = cloud[np.abs(cloud[:, 2] - z) < dz]
    rel = band[:, :2] - np.asarray(center_xy)[None]
    ang = np.degrees(np.arctan2(rel[:, 0], -rel[:, 1]))
    rad = np.linalg.norm(rel, axis=1)
    if max_r is not None:
        near = rad < max_r
        ang, rad = ang[near], rad[near]
    r = np.full(len(thetas), np.nan)
    for k, t in enumerate(thetas):
        d = np.abs((ang - t + 180) % 360 - 180)
        m = d < dth
        if m.any():
            r[k] = rad[m].max()
    ok = ~np.isnan(r)
    if ok.sum() >= 2 and not ok.all():
        tt = np.asarray(thetas, float)
        r[~ok] = np.interp(tt[~ok], tt[ok], r[ok], period=360)
    return r


def ring_point(center_xy, z, theta, r):
    t = math.radians(theta)
    return np.array([center_xy[0] + r * math.sin(t), center_xy[1] - r * math.cos(t), z])


# --------------------------------------------------------------------- apron

def apron_halfwidth(z):
    return float(np.interp(z, [0.33, 1.06, 1.12, 1.40], [0.200, 0.170, 0.125, 0.125]))


def build_apron(cloud, cons, z_top, z_hem, ncols=27, pitch=0.012, gap=0.010, log=print):
    """Bib apron hanging like stiff canvas: rows top -> hem, columns across. Returns V, F, grid,
    panel list, (rows, cols)."""
    nrows = int(round((z_top - z_hem) / pitch)) + 1
    zs = np.linspace(z_top, z_hem, nrows)
    us = np.linspace(-1, 1, ncols)
    X = np.array([[u * apron_halfwidth(z) for u in us] for z in zs])
    Y = np.zeros_like(X)
    for i, z in enumerate(zs):
        env = front_envelope(cloud, X[i], [z])[0]
        Y[i] = env
    # fill holes from neighbours (outside the silhouette) with the row's front-most value
    for i in range(nrows):
        row = Y[i]
        if np.isnan(row).all():
            Y[i] = Y[i - 1]
            continue
        row[np.isnan(row)] = np.nanmin(row)
    # canvas hangs straight down from the most forward point above it
    Y = np.minimum.accumulate(Y, axis=0)
    # stiff across: no dipping between hips / legs
    Yn = Y.copy()
    Yn[:, 1:-1] = np.minimum(np.minimum(Y[:, :-2], Y[:, 2:]), Y[:, 1:-1])
    Y = smooth1d(Yn, 6, axis=1)
    Y = smooth1d(Y, 4, axis=0)
    # slight forward flare towards the hem, and edges wrap a touch towards the sides
    flare = np.clip((0.95 - zs) / 0.6, 0, 1)[:, None]
    Y = Y - gap - 0.012 * flare * (1 - 0.5 * np.abs(us)[None]) + 0.010 * (us[None] ** 4) * (zs[:, None] < 1.1)
    V = np.stack([X, Y, np.repeat(zs[:, None], ncols, 1)], -1).reshape(-1, 3)
    V = Ly.collide_layers(V, cons, rounds=3)
    grid = np.arange(nrows * ncols).reshape(nrows, ncols)
    F = []
    for i in range(nrows - 1):
        for j in range(ncols - 1):
            F.append((grid[i, j], grid[i, j + 1], grid[i + 1, j + 1], grid[i + 1, j]))
    F = np.array(F, np.int64)
    log(f"apron grid {nrows} x {ncols}")
    return V, F, grid, zs, us


def pocket_patch(V_apron, grid, zs, us, x0, x1, z0, z1, lift=0.0035, cols=10, rows=10):
    """Patch pocket sitting on the apron front: sample the apron surface (bilinear in the grid)."""
    nr, nc = grid.shape
    P = V_apron[grid]  # nr, nc, 3
    out = []
    for z in np.linspace(z1, z0, rows):
        i = np.clip(np.searchsorted(-zs, -z) - 1, 0, nr - 2)
        t = (zs[i] - z) / (zs[i] - zs[i + 1])
        rowP = P[i] * (1 - t) + P[i + 1] * t
        for x in np.linspace(x0, x1, cols):
            j = np.clip(np.searchsorted(rowP[:, 0], x) - 1, 0, nc - 2)
            s = (x - rowP[j, 0]) / max(rowP[j + 1, 0] - rowP[j, 0], 1e-9)
            p = rowP[j] * (1 - s) + rowP[j + 1] * s
            out.append(p + np.array([0, -lift, 0]))
    Vp = np.array(out)
    g = np.arange(rows * cols).reshape(rows, cols)
    Fp = [(g[i, j], g[i, j + 1], g[i + 1, j + 1], g[i + 1, j]) for i in range(rows - 1) for j in range(cols - 1)]
    return Vp, np.array(Fp, np.int64), g


# ---------------------------------------------------------------- tapes/tubes

def resample(P, step):
    P = np.asarray(P, float)
    d = np.r_[0, np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))]
    n = max(int(d[-1] / step), 2) + 1
    s = np.linspace(0, d[-1], n)
    return np.stack([np.interp(s, d, P[:, k]) for k in range(3)], 1)


def tape(name, P, normals, width, thick, uv_rect, taper=None):
    """Flat closed tape swept along P, its width lying in the surface (normal = normals)."""
    P = np.asarray(P, float)
    n = len(P)
    T = np.gradient(P, axis=0)
    T /= np.maximum(np.linalg.norm(T, axis=1, keepdims=True), 1e-12)
    Nn = normals - (normals * T).sum(1, keepdims=True) * T
    Nn /= np.maximum(np.linalg.norm(Nn, axis=1, keepdims=True), 1e-12)
    Bn = np.cross(T, Nn)
    w = np.full(n, width) if taper is None else np.asarray(taper) * width
    corners = [(-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5)]
    V = []
    for i in range(n):
        for a, b in corners:
            V.append(P[i] + Bn[i] * a * w[i] + Nn[i] * b * thick)
    F, uv = [], []
    u0, u1, v0, v1 = uv_rect
    for i in range(n - 1):
        for k in range(4):
            k2 = (k + 1) % 4
            F.append((4 * i + k, 4 * i + k2, 4 * (i + 1) + k2, 4 * (i + 1) + k))
            ua, ub = u0 + (u1 - u0) * i / (n - 1), u0 + (u1 - u0) * (i + 1) / (n - 1)
            va, vb = v0 + (v1 - v0) * k / 4, v0 + (v1 - v0) * (k + 1) / 4
            uv.append([(ua, va), (ua, vb), (ub, vb), (ub, va)])
    for i, rev in ((0, True), (n - 1, False)):
        f = [4 * i + k for k in range(4)]
        F.append(tuple(f[::-1]) if rev else tuple(f))
        uv.append([(u0, v0), (u0, v1), (u0 + 0.001, v1), (u0 + 0.001, v0)])
    return mesh_object(name, V, F, uv=uv, smooth=True)


def push_out(P, cons, rounds=3, smooth=4):
    P = np.asarray(P, float)
    for _ in range(rounds):
        P = Ly.collide_layers(P, cons, rounds=2)
        Q = P.copy()
        for _ in range(smooth):
            Q[1:-1] = 0.5 * Q[1:-1] + 0.25 * (Q[:-2] + Q[2:])
        P = Q
    return Ly.collide_layers(P, cons, rounds=2)


# ---------------------------------------------------------------- neckerchief

def neck_axis(body, z):
    a = np.asarray(body.bones["neck_01"][0], float)
    b = np.asarray(body.bones["head"][0], float)
    t = (z - a[2]) / (b[2] - a[2])
    p = a + (b - a) * t
    return p[:2]


def neck_radius(body, z, theta, fallback=0.066):
    """Radius of the skin (body + face) about the neck axis at height z, direction theta (deg,
    0 = front): the first sample, marching out from the axis, that is outside the skin. (Ray casts
    from inside the neck miss on this body, so this uses the signed distance.)"""
    z = np.atleast_1d(np.asarray(z, float))
    th = np.radians(np.atleast_1d(np.asarray(theta, float)))
    rs = np.arange(0.03, 0.16, 0.0015)
    C = np.array([neck_axis(body, float(zz)) for zz in z])
    d = np.stack([np.sin(th), -np.cos(th)], 1)
    P = np.empty((len(z), len(rs), 3))
    P[:, :, :2] = C[:, None, :] + d[:, None, :] * rs[None, :, None]
    P[:, :, 2] = z[:, None]
    s = body.all.signed(P.reshape(-1, 3))[0].reshape(len(z), len(rs))
    out = s > 0
    first = np.where(out.any(1), out.argmax(1), -1)
    r = np.where(first >= 0, rs[np.maximum(first, 0)], fallback)
    return r if r.size > 1 else float(r[0])


def neck_base(body, thetas, z0=1.42, z1=1.58, top=1.56, tol=0.012):
    """Lowest height per theta at which the skin is within ``tol`` of the neck radius at ``top``:
    where the neck column meets the chest / trapezius (low at the throat, high at the sides)."""
    thetas = np.asarray(thetas, float)
    zs = np.arange(z0, z1, 0.002)
    rt = neck_radius(body, np.full(len(thetas), top), thetas)
    ZZ, TT = np.meshgrid(zs, thetas)
    rr = neck_radius(body, ZZ.ravel(), TT.ravel()).reshape(ZZ.shape)
    ok = rr < rt[:, None] + tol
    return np.where(ok.any(1), zs[ok.argmax(1)], top)


# lower edge of the neckerchief band about the neck (deg from the front -> height, m), measured on
# the clerk body: the neck column meets the chest at the throat (jugular ~1.451) but the trapezius
# rides up to ~1.51 at the sides.
NK_BASE_TABLE = ((0, 1.452), (30, 1.468), (60, 1.505), (90, 1.526), (120, 1.526), (150, 1.512), (180, 1.502))


def neckerchief_ring(cloud, body, lift, height, cols=48, rows=5, pad=0.003, skin=0.006, reach=0.03, z_front=None):
    """A band round the neck: its lower edge follows ``NK_BASE_TABLE`` + ``lift`` (front clamped to
    ``z_front``); each vertex starts at the upper-neck radius + ``skin`` and marches outward until
    it clears the skin by ``skin`` and the local cloud (the shirt collar) by ``pad``. Only the radius
    is solved, so the rows keep their heights."""
    th = np.linspace(-180, 180, cols, endpoint=False)
    tz = np.array(NK_BASE_TABLE, float)
    base = np.interp(np.abs(th), tz[:, 0], tz[:, 1]) + lift
    if z_front is not None:
        w = np.maximum(np.cos(np.radians(th)), 0) ** 2
        base = base * (1 - w) + np.maximum(base, z_front) * w
    Z = base[None, :] + height * (np.arange(rows) / (rows - 1))[:, None]
    r_top = smooth1d(neck_radius(body, np.full(cols, 1.535), th), 3, closed=True)
    steps = np.arange(0.0, 0.08, 0.0015)
    R = np.zeros((rows, cols))
    for r in range(rows):
        P = np.empty((cols, len(steps), 3))
        for k, t in enumerate(th):
            c = neck_axis(body, Z[r, k])
            d = np.array([math.sin(math.radians(t)), -math.cos(math.radians(t))])
            rr = r_top[k] + steps
            P[k, :, :2] = c[None, :] + d[None, :] * rr[:, None]
            P[k, :, 2] = Z[r, k]
        s = body.all.signed(P.reshape(-1, 3))[0].reshape(cols, len(steps))
        ok = s > skin
        first = np.where(ok.any(1), ok.argmax(1), len(steps) - 1)
        for k, t in enumerate(th):
            rb = r_top[k] + steps[first[k]]
            c = neck_axis(body, Z[r, k])
            rc = radial_ring(cloud, c, Z[r, k], [t], dz=0.007, dth=8.0, max_r=rb + reach)[0]
            R[r, k] = min(max(rb, (rc + pad) if np.isfinite(rc) else 0.0), rb + 0.012)
    # smooth round the neck, and keep the band from pinching in above its lower edge
    for r in range(rows):
        R[r] = np.maximum(smooth1d(R[r], 3, closed=True), R[r] - 0.002)
    for r in range(1, rows):
        R[r] = np.maximum(R[r], R[r - 1] - 0.006)
    V = np.array([ring_point(neck_axis(body, Z[r, k]), Z[r, k], th[k], R[r, k])
                  for r in range(rows) for k in range(cols)])
    G = np.arange(rows * cols).reshape(rows, cols)
    F = []
    for r in range(rows - 1):
        for c in range(cols):
            c2 = (c + 1) % cols
            F.append((G[r, c], G[r, c2], G[r + 1, c2], G[r + 1, c]))
    return V, np.array(F, np.int64), G, th

def ellipsoid(name, center, radii, uv_rect, nu=12, nv=8):
    V, F, uv = [], [], []
    u0, u1, v0, v1 = uv_rect
    for j in range(nv + 1):
        ph = math.pi * j / nv - math.pi / 2
        for i in range(nu):
            a = 2 * math.pi * i / nu
            V.append(center + np.array([math.cos(ph) * math.cos(a) * radii[0], math.cos(ph) * math.sin(a) * radii[1],
                                        math.sin(ph) * radii[2]]))
    for j in range(nv):
        for i in range(nu):
            i2 = (i + 1) % nu
            F.append((j * nu + i, j * nu + i2, (j + 1) * nu + i2, (j + 1) * nu + i))
            uv.append([(u0 + (u1 - u0) * i / nu, v0 + (v1 - v0) * j / nv), (u0 + (u1 - u0) * (i + 1) / nu, v0 + (v1 - v0) * j / nv),
                       (u0 + (u1 - u0) * (i + 1) / nu, v0 + (v1 - v0) * (j + 1) / nv), (u0 + (u1 - u0) * i / nu, v0 + (v1 - v0) * (j + 1) / nv)])
    obj = mesh_object(name, V, F, uv=uv)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bm.to_mesh(obj.data)
    bm.free()
    return obj


# ------------------------------------------------------------ rolled sleeves

def sleeve_radius(Vs, C, e1, e2, t, nbins=24):
    rel = Vs - C
    rel = rel - np.outer(rel @ t, t)
    ang = np.arctan2(rel @ e2, rel @ e1)
    rad = np.linalg.norm(rel, axis=1)
    bins = np.linspace(-math.pi, math.pi, nbins, endpoint=False)
    r = np.full(nbins, np.nan)
    for k, b in enumerate(bins):
        d = np.abs((ang - b + math.pi) % (2 * math.pi) - math.pi)
        m = d < 2 * math.pi / nbins
        if m.any():
            r[k] = rad[m].max()
    ok = ~np.isnan(r)
    r[~ok] = np.interp(bins[~ok], bins[ok], r[ok], period=2 * math.pi)
    return bins, smooth1d(r, 2, closed=True)


def torus(name, C, t, e1, e2, bins, rad, tube, uv_rect, seed=0, nmin=8):
    rng = np.random.default_rng(seed)
    nb = len(bins)
    noise = smooth1d(rng.normal(0, 0.0012, nb), 2, closed=True)
    tw = tube * (1 + smooth1d(rng.normal(0, 0.10, nb), 2, closed=True))
    V, F, uv = [], [], []
    u0, u1, v0, v1 = uv_rect
    for k in range(nb):
        a = bins[k]
        radial = math.cos(a) * e1 + math.sin(a) * e2
        Rr = rad[k] + 0.8 * tw[k] + noise[k]
        for m in range(nmin):
            psi = 2 * math.pi * m / nmin
            V.append(C + radial * (Rr + tw[k] * math.cos(psi)) + t * (tw[k] * math.sin(psi)))
    for k in range(nb):
        k2 = (k + 1) % nb
        for m in range(nmin):
            m2 = (m + 1) % nmin
            F.append((k * nmin + m, k2 * nmin + m, k2 * nmin + m2, k * nmin + m2))
            ua, ub = u0 + (u1 - u0) * k / nb, u0 + (u1 - u0) * (k + 1) / nb
            va, vb = v0 + (v1 - v0) * m / nmin, v0 + (v1 - v0) * (m + 1) / nmin
            uv.append([(ua, va), (ub, va), (ub, vb), (ua, vb)])
    return mesh_object(name, V, F, uv=uv)


def arm_frame_at(ax, s):
    """Point and orthonormal frame on the arm line (works along the forearm too)."""
    C = ax.point(s)
    t = ax.tangent(s)
    e1 = ax.e1 - (ax.e1 @ t) * t
    e1 /= np.linalg.norm(e1)
    return C, t, e1, np.cross(t, e1)


# -------------------------------------------------------------------- buttons

def button(name, center, normal, radius, height, uv_rect, sides=12):
    n = unit(normal)
    a = unit(np.cross(n, [0, 0, 1]))
    if np.linalg.norm(a) < 0.5:
        a = unit(np.cross(n, [1, 0, 0]))
    b = np.cross(n, a)
    V, F, uv = [], [], []
    u0, u1, v0, v1 = uv_rect
    rings = [(0.0, 1.0), (height * 0.75, 0.92), (height, 0.55)]
    for h, rf in rings:
        for k in range(sides):
            ang = 2 * math.pi * k / sides
            V.append(center + n * (h - 0.0008) + radius * rf * (math.cos(ang) * a + math.sin(ang) * b))
    for r in range(len(rings) - 1):
        for k in range(sides):
            k2 = (k + 1) % sides
            F.append((r * sides + k, r * sides + k2, (r + 1) * sides + k2, (r + 1) * sides + k))
            uv.append([(u0, v0), (u1, v0), (u1, v1), (u0, v1)])
    top = len(V)
    V.append(center + n * (height - 0.0004))
    for k in range(sides):
        k2 = (k + 1) % sides
        F.append((2 * sides + k, 2 * sides + k2, top))
        uv.append([(u0, v0), (u1, v0), ((u0 + u1) / 2, v1)])
    bot = len(V)
    V.append(center - n * 0.0008)
    for k in range(sides):
        k2 = (k + 1) % sides
        F.append((k2, k, bot))
        uv.append([(u0, v0), (u1, v0), ((u0 + u1) / 2, v1)])
    return mesh_object(name, V, F, uv=uv)


# ---------------------------------------------------------------------- boots

def boots_mesh(name, body_obj, z_top=0.15, push=0.0045, smooth_iters=6):
    """Copy the body's feet (faces entirely below z_top), pushed out and smoothed into a shoe
    last; returned without thickness."""
    me = body_obj.data
    Vb = np.array([v.co for v in me.vertices])
    Nb = np.array([v.normal for v in me.vertices])
    keep = [p for p in me.polygons if all(Vb[i][2] < z_top for i in p.vertices)]
    used = sorted({i for p in keep for i in p.vertices})
    remap = {v: k for k, v in enumerate(used)}
    V = Vb[used].copy()
    N = Nb[used]
    F = [tuple(remap[i] for i in p.vertices) for p in keep]
    V = V + N * push
    # merge split seam verts so smoothing can't tear the shell open
    bm = bmesh.new()
    vs = [bm.verts.new(p) for p in V]
    for f in F:
        try:
            bm.faces.new([vs[i] for i in f])
        except ValueError:
            pass
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0008)
    bm.verts.ensure_lookup_table()
    V = np.array([v.co for v in bm.verts])
    F = [tuple(v.index for v in f.verts) for f in bm.faces]
    boundary = np.array([any(len(e.link_faces) < 2 for e in v.link_edges) for v in bm.verts])
    bm.free()
    # adjacency smoothing (rounds off the toes into a boot last); the open top stays pinned
    nbr = [set() for _ in V]
    for f in F:
        for k in range(len(f)):
            a, b = f[k], f[(k + 1) % len(f)]
            nbr[a].add(b)
            nbr[b].add(a)
    nbr = [np.array(sorted(s)) if s else np.array([k]) for k, s in enumerate(nbr)]
    for _ in range(smooth_iters):
        L = np.array([V[n].mean(0) for n in nbr]) - V
        L[boundary] = 0.0
        V = V + 0.5 * L
    obj = mesh_object(name, V, F)
    return obj, np.asarray(used)


# --------------------------------------------------------------------- pencil

def pencil_mesh(name, length=0.17, across=0.0095, thick=0.0065, cone=0.035, lead=0.010):
    """Flat-hexagon carpenter's pencil, long axis +X, pivot at the middle. The +X end is
    knife-sharpened: wood cone then graphite tip. UV u runs along the pencil (0 = back end)."""
    half = length / 2
    hx = [(-0.5, 0.0), (-0.32, 0.5), (0.32, 0.5), (0.5, 0.0), (0.32, -0.5), (-0.32, -0.5)]
    sec = [(across * a, thick * b) for a, b in hx]
    x_c = half - cone            # start of the cone
    x_l = half - lead            # wood/graphite boundary
    stations = [(-half, 1.0), (x_c, 1.0), (x_c + 0.4 * (x_l - x_c), 0.62), (x_l, 0.28), (half - 0.003, 0.08)]
    V, F, uv = [], [], []
    n = len(sec)
    rng = np.random.default_rng(7)
    for xs, sc in stations:
        for k, (a, b) in enumerate(sec):
            jit = 1.0 + (rng.normal(0, 0.05) if 0.1 < sc < 1.0 else 0.0)  # knife facets
            V.append((xs, a * sc * jit, b * sc * jit))
    tip = len(V)
    V.append((half, 0.0, 0.0))
    back = len(V)
    V.append((-half, 0.0, 0.0))

    def u_of(x):
        return (x + half) / length

    for s in range(len(stations) - 1):
        for k in range(n):
            k2 = (k + 1) % n
            F.append((s * n + k, s * n + k2, (s + 1) * n + k2, (s + 1) * n + k))
            ua, ub = u_of(stations[s][0]), u_of(stations[s + 1][0])
            uv.append([(ua, k / n), (ua, (k + 1) / n), (ub, (k + 1) / n), (ub, k / n)])
    last = (len(stations) - 1) * n
    for k in range(n):
        k2 = (k + 1) % n
        F.append((last + k, last + k2, tip))
        uv.append([(u_of(stations[-1][0]), k / n), (u_of(stations[-1][0]), (k + 1) / n), (1.0, (k + 0.5) / n)])
        F.append((k2, k, back))
        uv.append([(0.012, (k + 1) / n), (0.012, k / n), (0.0, (k + 0.5) / n)])
    obj = mesh_object(name, V, F, uv=uv, smooth=False)
    return obj, dict(cone_u=u_of(x_c), lead_u=u_of(x_l))
