"""Procedural garment patterns fitted to the body: a cropped tank top and low-rise shorts.

Each garment starts as a structured quad grid whose rows and columns follow the sewing
pattern (hem, side seams, straps, crotch), cast onto the body from outside so fabric
bridges hollows (cleavage, navel, gluteal cleft) like a taut woven cloth. ``relax`` then
runs a small membrane solve: Laplacian tension, a gentle pull toward the skin and a hard
minimum offset, with the designed hems held in place.
"""
import math

import numpy as np

PITCH = 0.0085  # target quad size (m)


class Garment:
    def __init__(self, name):
        self.name = name
        self.V = []            # positions
        self.F = []            # quads (vertex index tuples)
        self.face_panel = []   # panel id per face
        self.uv_seams = set()  # undirected edges (a, b) that split UV islands
        self.features = {}     # name -> list of ordered vertex polylines (for texture detail)
        self.extra = {}

    def add(self, p):
        self.V.append(np.asarray(p, dtype=np.float64))
        return len(self.V) - 1

    def quad(self, a, b, c, d, panel):
        self.F.append((a, b, c, d))
        self.face_panel.append(panel)

    def seam(self, poly):
        for a, b in zip(poly[:-1], poly[1:]):
            self.uv_seams.add((min(a, b), max(a, b)))

    def feature(self, name, poly):
        self.features.setdefault(name, []).append(list(poly))

    def arrays(self):
        return np.array(self.V), np.array(self.F, dtype=np.int64)


def _resample_angles(body, z, a0, a1, surf=None, yc=None, min_n=1, density=1.0):
    """Angles between a0 and a1 (deg) spaced ~PITCH apart along the surface at height z."""
    ts = np.linspace(a0, a1, 64)
    pts = []
    for t in ts:
        loc, _ = body.cyl_hit(t, z, surf=surf, yc=yc)
        pts.append(loc if loc is not None else (pts[-1] if pts else np.zeros(3)))
    pts = np.array(pts)
    seg = np.linalg.norm(np.diff(pts, axis=0), axis=1)
    arc = np.concatenate([[0], np.cumsum(seg)])
    n = max(min_n, int(round(density * arc[-1] / PITCH)))
    return list(np.interp(np.linspace(0, arc[-1], n + 1), arc, ts))


def _theta_for_x(body, x_target, z, back):
    lo, hi = (105.0, 180.0) if back else (0.0, 75.0)
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        loc, _ = body.cyl_hit(mid, z)
        x = loc[0] if loc is not None else 0.0
        if (x < x_target) != back:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def _hit_or(body, theta, z, off, fallback, surf=None, yc=None):
    loc, nrm = body.cyl_hit(theta, z, surf=surf, yc=yc)
    if loc is None:
        return fallback
    return loc + nrm * off


# ----------------------------------------------------------------------- tank top

def tank_top_params(body):
    lm = body.lm
    p = {}
    p["hem_z"] = max(lm["underbust_z"] - 0.052, lm["navel"][2] + 0.075)
    p["ua_z"] = lm["armpit_z"] - 0.022
    p["fs_z"] = max(lm["armpit_z"] + 0.045, lm["apex"][2] + 0.075)
    p["bs_z"] = p["fs_z"] - 0.005
    p["scoop_z"] = max(lm["apex"][2] + 0.04, lm["jugular_z"] - 0.125)
    p["bscoop_z"] = p["bs_z"] - 0.045
    p["strap_x"] = lm["neck_x"] + 0.47 * (lm["shoulder_x"] - lm["neck_x"])
    p["strap_w"] = 0.017
    p["offset"] = 0.0032
    return p


def build_tank_top(body, p=None):
    p = p or tank_top_params(body)
    g = Garment("SKM_PrimitiveTankTop")
    g.extra["params"] = p
    off = p["offset"]
    xi, xo = p["strap_x"] - p["strap_w"] / 2, p["strap_x"] + p["strap_w"] / 2
    zf, zb = p["fs_z"], p["bs_z"]
    th_si, th_so = _theta_for_x(body, xi, zf, False), _theta_for_x(body, xo, zf, False)
    phb_si = 180 - _theta_for_x(body, xi, zb, True)
    phb_so = 180 - _theta_for_x(body, xo, zb, True)
    zr = p["ua_z"] - 0.02
    fl = (_resample_angles(body, zr, 0, th_si, min_n=4)[:-1]
          + list(np.linspace(th_si, th_so, 5))[:-1]
          + _resample_angles(body, zr, th_so, 90, min_n=3, density=1.8))
    bl = (_resample_angles(body, zr, 90, 180 - phb_so, min_n=3, density=1.8)[:-1]
          + list(np.linspace(180 - phb_so, 180 - phb_si, 5))[:-1]
          + _resample_angles(body, zr, 180 - phb_si, 180, min_n=4))
    left = fl[:-1] + bl            # 0 .. 180
    thetas = sorted(set(round(-180.0 if t >= 180 - 1e-9 else t, 9) for t in left + [-t for t in left[1:-1]]))
    C = len(thetas)

    def z_top(t):
        a = abs(t)
        if a <= 90:
            if a <= th_si:
                return p["scoop_z"] + (zf - p["scoop_z"]) * (a / th_si) ** 2.4
            if a <= th_so + 1e-6:
                return zf
            u = (90 - a) / (90 - th_so)
            return p["ua_z"] + (zf - p["ua_z"]) * u ** 1.8
        ph = 180 - a
        if ph <= phb_si:
            return p["bscoop_z"] + (zb - p["bscoop_z"]) * (ph / phb_si) ** 2.4
        if ph <= phb_so + 1e-6:
            return zb
        u = (90 - ph) / (90 - phb_so)
        return p["ua_z"] + (zb - p["ua_z"]) * u ** 1.8

    tops = [z_top(t) for t in thetas]
    R = int(math.ceil((max(tops) - p["hem_z"]) / PITCH))
    grid = np.zeros((R + 1, C), np.int64)
    for j, t in enumerate(thetas):
        prev = None
        for k in range(R + 1):
            z = p["hem_z"] + (tops[j] - p["hem_z"]) * k / R
            pt = _hit_or(body, t, z, off, prev if prev is not None else np.array([0, 0, z]))
            prev = pt
            grid[k, j] = g.add(pt)
    for k in range(R):
        for j in range(C):
            jn = (j + 1) % C
            g.quad(grid[k, j], grid[k, jn], grid[k + 1, jn], grid[k + 1, j],
                   0 if (-90 <= thetas[j] < 90) else 1)
    # straps: ribbons from the front strap columns over the shoulder crest to the back
    for side in (1, -1):
        fcols = [j for j, t in enumerate(thetas) if 0 <= side * t <= 90 and th_si - 1e-6 <= abs(t) <= th_so + 1e-6]
        bcols = [j for j, t in enumerate(thetas) if abs(t) > 90 and side * t > 0
                 and phb_si - 1e-6 <= 180 - abs(t) <= phb_so + 1e-6]
        fcols.sort(key=lambda j: abs(thetas[j]))             # inner -> outer
        bcols.sort(key=lambda j: -abs(thetas[j]))            # inner (near 180) -> outer
        assert len(fcols) == len(bcols) == 5, (fcols, bcols)
        front = [grid[R, j] for j in fcols]
        back = [grid[R, j] for j in bcols]
        paths = []
        for fa, ba in zip(front, back):
            P0, P2 = g.V[fa], g.V[ba]
            crest = body.shoulder_crest(0.5 * (P0[0] + P2[0]))
            _, cn, _ = body.all.nearest(crest[None])
            crest = crest + cn[0] * off
            Q = 2 * crest - 0.5 * (P0 + P2)
            paths.append((P0, Q, P2, crest))
        length = np.mean([np.linalg.norm(a[3] - a[0]) + np.linalg.norm(a[2] - a[3]) for a in paths])
        L = max(6, int(round(length / PITCH)))
        L += L % 2
        rib = [front]
        for l in range(1, L):
            u = l / L
            row = []
            for (P0, Q, P2, crest) in paths:
                B = (1 - u) ** 2 * P0 + 2 * u * (1 - u) * Q + u * u * P2
                Cc = np.array([crest[0] * 0.55, crest[1], crest[2] - 0.13])
                loc, nrm = body.torso.ray(Cc, B - Cc)
                row.append(g.add(loc + nrm * off if loc is not None else B))
            rib.append(row)
        rib.append(back)
        for l in range(L):
            for c in range(4):
                g.quad(rib[l][c], rib[l][c + 1], rib[l + 1][c + 1], rib[l + 1][c], 0 if l < L // 2 else 1)
        g.seam(rib[L // 2])
        g.feature("shoulder", rib[L // 2])
        g.extra.setdefault("strap_rows", []).append(rib)
    for t_side in (90.0, -90.0):
        j = int(np.argmin([abs(t - t_side) for t in thetas]))
        col = list(grid[:, j])
        g.seam(col)
        g.feature("side", col)
    g.extra.update(grid=grid, thetas=thetas, R=R)
    return g


# ------------------------------------------------------------------------- shorts

def shorts_params(body):
    lm = body.lm
    p = {}
    p["waist_front_z"] = lm["navel"][2] - 0.078
    p["waist_back_rise"] = 0.036
    p["junction_z"] = lm["perineum"][2] + 0.045
    p["inseam_z"] = lm["perineum"][2] - 0.045
    p["outseam_lift"] = 0.022
    p["offset"] = 0.0032
    return p


def build_shorts(body, p=None):
    p = p or shorts_params(body)
    g = Garment("SKM_PrimitiveShorts")
    g.extra["params"] = p
    off = p["offset"]
    zj = p["junction_z"]
    hs = body.torso

    def z_w(t):
        b = (1 - math.cos(math.radians(t))) / 2
        return p["waist_front_z"] + p["waist_back_rise"] * b ** 1.3

    zr = zj + 0.03
    q1 = _resample_angles(body, zr, 0, 90, surf=hs, min_n=6)
    q2 = _resample_angles(body, zr, 90, 180, surf=hs, min_n=6)
    left = q1[:-1] + q2                      # 0 .. 180 inclusive
    thetas = sorted(set(round(-180.0 if t >= 180 - 1e-9 else t, 9) for t in left + [-t for t in left[1:-1]]))
    N = len(thetas)
    j_cf = int(np.argmin(np.abs(np.array(thetas))))
    j_cb = 0
    j_l = int(np.argmin(np.abs(np.array(thetas) - 90)))
    j_r = int(np.argmin(np.abs(np.array(thetas) + 90)))
    tops = [z_w(t) for t in thetas]
    Rh = int(math.ceil((max(tops) - zj) / PITCH))
    hip = np.zeros((Rh + 1, N), np.int64)
    for j, t in enumerate(thetas):
        prev = None
        for k in range(Rh + 1):
            z = zj + (tops[j] - zj) * k / Rh
            pt = _hit_or(body, t, z, off, prev if prev is not None else np.array([0, 0, z]), surf=hs)
            prev = pt
            hip[k, j] = g.add(pt)
    for k in range(Rh):
        for j in range(N):
            jn = (j + 1) % N
            front = abs(thetas[j]) < 90 and abs(thetas[jn]) <= 90 and thetas[j] >= -90
            left_side = thetas[j] >= 0
            panel = (0 if front else 2) + (0 if left_side else 1)
            g.quad(hip[k, j], hip[k, jn], hip[k + 1, jn], hip[k + 1, j], panel)
    # crotch bridge from the back center under the body to the front center
    Pb, Pf = g.V[hip[0, j_cb]], g.V[hip[0, j_cf]]
    zp = body.lm["perineum"][2] - off - 0.002
    us = np.linspace(0, 1, 200)
    path = np.array([[0.0, Pb[1] + (Pf[1] - Pb[1]) * u,
                      zj - (zj - zp) * math.sin(math.pi * u) ** 0.55] for u in us])
    arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(path, axis=0), axis=1))])
    K = max(10, int(round(arc[-1] / PITCH)))
    K += K % 2
    bridge = []
    for s in np.linspace(0, arc[-1], K + 1)[1:-1]:
        bridge.append(g.add([np.interp(s, arc, path[:, i]) for i in range(3)]))
    crotch_line = [hip[0, j_cb]] + bridge + [hip[0, j_cf]]
    g.seam(crotch_line)
    g.feature("crotch", list(hip[::-1, j_cb]) + bridge + list(hip[:, j_cf]))
    g.seam(list(hip[:, j_cb]))
    g.seam(list(hip[:, j_cf]))
    crotch_pt = bridge[len(bridge) // 2]
    g.extra["crotch_vertex"] = crotch_pt
    # legs
    legs = {}
    for side, sgn, surf in (("l", 1, body.leg_l), ("r", -1, body.leg_r)):
        if side == "l":
            hip_part = [hip[0, j] for j in range(j_cf, N)] + [hip[0, j_cb]]      # 0 -> 180
            side_idx = j_l - j_cf
        else:
            hip_part = [hip[0, j] for j in range(j_cf, -1, -1)]                # 0 -> -180
            side_idx = j_cf - j_r
        # bridge is ordered back -> front, so the ring continues from the back center to the front
        ring0 = hip_part + bridge
        inseam_idx = len(hip_part) + len(bridge) // 2
        Q = len(ring0)
        zs = np.array([g.V[i][2] for i in ring0])
        T = int(math.ceil((zj - (p["inseam_z"] + p["outseam_lift"])) / PITCH)) + 1
        rows = [ring0]
        dirs = []
        for i in ring0:
            ax = body.thigh_axis(side, g.V[i][2])
            d = g.V[i][:2] - ax[:2]
            dirs.append(d / max(np.linalg.norm(d), 1e-9))
        for r in range(1, T + 1):
            row = []
            for q, i in enumerate(ring0):
                d = dirs[q]
                lat = max(0.0, d[0] * sgn)
                zh = p["inseam_z"] + p["outseam_lift"] * lat ** 1.4
                z = zs[q] + (zh - zs[q]) * r / T
                ax = body.thigh_axis(side, z)
                dd = np.array([d[0], d[1], 0.0])
                origin = np.array([ax[0], ax[1], z]) + dd * 0.35
                loc, nrm = surf.ray(origin, -dd)
                prev = g.V[rows[-1][q]]
                row.append(g.add(loc + nrm * off if loc is not None else np.array([prev[0], prev[1], z])))
            rows.append(row)
        for r in range(T):
            for q in range(Q):
                qn = (q + 1) % Q
                back = side_idx <= q < inseam_idx
                panel = (2 if back else 0) + (0 if side == "l" else 1)
                g.quad(rows[r][q], rows[r][qn], rows[r + 1][qn], rows[r + 1][q], panel)
        side_col = [rows[r][side_idx] for r in range(T + 1)]
        ins_col = [rows[r][inseam_idx] for r in range(T + 1)]
        g.seam(side_col)
        g.seam(ins_col)
        g.feature("inseam", ins_col)
        legs[side] = dict(rows=rows, side_col=side_col)
    for side, j in (("l", j_l), ("r", j_r)):
        col = list(hip[:, j]) + legs[side]["side_col"][1:]
        g.seam(list(hip[:, j]))
        g.feature("side", col[::-1])
    g.extra.update(hip=hip, thetas=thetas, legs=legs, Rh=Rh, j_cf=j_cf, j_cb=j_cb)
    return g


# ------------------------------------------------------------------------ relax

def boundary_loops(F, nv):
    """Ordered boundary vertex loops of a quad mesh."""
    count = {}
    for f in F:
        for a, b in zip(f, list(f[1:]) + [f[0]]):
            key = (min(a, b), max(a, b))
            count[key] = count.get(key, 0) + 1
    nbr = {}
    for (a, b), c in count.items():
        if c == 1:
            nbr.setdefault(a, []).append(b)
            nbr.setdefault(b, []).append(a)
    loops, seen = [], set()
    for start in nbr:
        if start in seen:
            continue
        loop, prev, cur = [start], None, start
        seen.add(start)
        while True:
            nxt = [n for n in nbr[cur] if n != prev]
            if not nxt or nxt[0] == start:
                break
            prev, cur = cur, nxt[0]
            if cur in seen:
                break
            seen.add(cur)
            loop.append(cur)
        loops.append(loop)
    return loops


def edges_of(F):
    e = set()
    for f in F:
        for a, b in zip(f, list(f[1:]) + [f[0]]):
            e.add((min(a, b), max(a, b)))
    return np.array(sorted(e), dtype=np.int64)


def surface_labels(g, V, body):
    """Collision surface per vertex: each shorts leg only sees its own thigh (her thighs touch
    near the crotch, so the other leg would otherwise push fabric the wrong way)."""
    if "Tank" in g.name:
        return [body.all], np.zeros(len(V), np.int64)
    zj = g.extra["params"]["junction_z"]
    lab = np.zeros(len(V), np.int64)
    lab[(V[:, 2] < zj - 0.004) & (V[:, 0] > 0.004)] = 1
    lab[(V[:, 2] < zj - 0.004) & (V[:, 0] < -0.004)] = 2
    return [body.torso, body.leg_l, body.leg_r], lab


def offsets(g, V, body):
    """Per-vertex minimum skin clearance (m) used by ``relax``."""
    if "Tank" in g.name:
        return np.full(len(V), 0.0030)
    p = g.extra["params"]
    per = np.array(body.lm["perineum"])
    d = np.linalg.norm(V - per, axis=1)
    off = 0.0035 + 0.0060 * np.exp(-(d / 0.05) ** 2)
    leg = np.clip((p["junction_z"] - V[:, 2]) / (p["junction_z"] - p["inseam_z"]), 0, 1)
    medial = np.clip((np.abs(V[:, 0]) - 0.01) / 0.05, 0, 1)
    off += 0.0075 * leg ** 1.5 * medial
    return off


class MultiSurface:
    def __init__(self, surfs, labels):
        self.surfs, self.labels = surfs, labels

    def signed(self, pts):
        s = np.empty(len(pts)); loc = np.empty_like(pts); nrm = np.empty_like(pts)
        for i, surf in enumerate(self.surfs):
            m = self.labels == i
            if m.any():
                s[m], loc[m], nrm[m] = surf.signed(pts[m])
        return s, loc, nrm


def relax(V, F, fixed, surf, off_min, iters=160, pull=0.02, smooth=0.5, log=None):
    """Membrane solve: Laplacian tension (bridges hollows, spreads quads evenly), a weak pull
    back toward the skin, and a hard per-vertex minimum offset. Fixed (hem) vertices only move
    along the skin normal."""
    V = V.copy()
    off_min = np.broadcast_to(np.asarray(off_min, dtype=np.float64), (len(V),)).copy()
    off_target = off_min + 0.0008
    E = edges_of(F)
    deg = np.bincount(E.ravel(), minlength=len(V)).astype(np.float64)
    free = ~fixed
    for it in range(iters):
        before = V.copy()
        avg = np.zeros_like(V)
        np.add.at(avg, E[:, 0], V[E[:, 1]])
        np.add.at(avg, E[:, 1], V[E[:, 0]])
        avg /= np.maximum(deg, 1)[:, None]
        V[free] += smooth * (avg[free] - V[free])
        s, loc, nrm = surf.signed(V)
        far = s > off_target
        V[far] -= (pull * (s[far] - off_target[far]))[:, None] * nrm[far]
        s = ((V - loc) * nrm).sum(1)
        inside = s < off_min
        V[inside] = loc[inside] + nrm[inside] * off_min[inside][:, None]
        # hems keep their designed height: push them out horizontally where the skin allows
        m = fixed & (np.abs(nrm[:, 2]) < 0.6)
        if m.any():
            d = V[m] - before[m]
            nh = nrm[m].copy()
            nh[:, 2] = 0
            amount = (d * nrm[m]).sum(1) / np.maximum((nh * nrm[m]).sum(1), 0.3)
            V[m] = before[m] + nh * amount[:, None]
        if log and (it % 20 == 0 or it == iters - 1):
            log(f"  relax {it}: inside {inside.sum()} max gap {1000 * s.max():.1f} mm")
    s, _, _ = surf.signed(V)
    return V, s
