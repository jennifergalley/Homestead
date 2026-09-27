"""Sleeved tops fitted to the body: a linen T-tunic shirt, a loose laced-neck long shirt and the
shell of the sheepskin coat.

Same idea as the tank top in ``garments.py``: a structured quad grid cast onto the torso from
its centre line (hem -> neckline / armhole), wide yoke ribbons over the shoulder crests, then a
sleeve tube grown from each armhole loop along the arm's bone axis (shoulder -> elbow -> wrist),
and for the coat a stand collar grown from the neckline. Openings (the shirt's neck slit, the
coat's front) are cut after the membrane solve so both edges keep meeting.
"""
import math

import numpy as np

from .garments import Garment, boundary_loops

UP = np.array([0.0, 0.0, 1.0])


def _unit(v):
    v = np.asarray(v, np.float64)
    return v / max(np.linalg.norm(v), 1e-12)


def cyl_hit(body, theta_deg, z, surf=None):
    """``Body.cyl_hit`` that ignores hits on the far side (the torso surface has holes where the
    arms were cut away, so a ray can pass through an armpit and hit the other flank)."""
    loc, nrm = body.cyl_hit(theta_deg, z, surf=surf)
    if loc is None:
        return None, None
    t = math.radians(theta_deg)
    d = np.array([math.sin(t), -math.cos(t), 0.0])
    yc = body.center_y(z)
    if (loc - np.array([0.0, yc, z])) @ d <= 0.0:
        return None, None
    return loc, nrm


def resample(body, z, a0, a1, pitch, surf=None, min_n=1, density=1.0):
    ts = np.linspace(a0, a1, 64)
    pts = []
    for t in ts:
        loc, _ = cyl_hit(body, t, z, surf=surf)
        pts.append(loc if loc is not None else (pts[-1] if pts else np.zeros(3)))
    pts = np.array(pts)
    arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
    n = max(min_n, int(round(density * arc[-1] / pitch)))
    return list(np.interp(np.linspace(0, arc[-1], n + 1), arc, ts))


def theta_for_x(body, x_target, z, back):
    lo, hi = (100.0, 180.0) if back else (0.0, 80.0)
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        loc, _ = cyl_hit(body, mid, z)
        x = loc[0] if loc is not None else 1.0
        if (x < x_target) != back:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def crest(body, x):
    """Top of the shoulder at x (from just above it, so the head is never hit), midway between
    the front and back of the shoulder, on the full body (the deltoid belongs to the arm)."""
    lm = body.lm
    z0 = lm["armpit_z"] + 0.06
    front, _ = body.all.ray((x, -0.6, z0), (0, 1, 0))
    back, _ = body.all.ray((x, 0.6, z0), (0, -1, 0))
    y = 0.5 * (front[1] + back[1]) if front is not None and back is not None else body.center_y(z0)
    loc, _ = body.all.ray((x, y, lm["jugular_z"] + 0.075), (0, 0, -1))
    return loc


class Belt:
    """The cord belt's centreline (Assets/Props/CordBelt/cord_belt_contour.json)."""

    def __init__(self, data, radius=0.004):
        self.pivot = np.array(data["pivot"])
        self.P = np.array(data["points"]) + self.pivot
        self.radius = radius

    def z(self, theta_deg):
        phi = math.degrees(math.atan2(-math.cos(math.radians(theta_deg)), math.sin(math.radians(theta_deg))))
        f = (phi % 360.0) / 360.0 * len(self.P)
        i = int(f) % len(self.P)
        w = f - int(f)
        return float(self.P[i, 2] * (1 - w) + self.P[(i + 1) % len(self.P), 2] * w)


def arm_surface(body, side):
    key = f"_arm_{side}"
    if not hasattr(body, key):
        tri_arm = body.arm_share[body.T].mean(1)
        cx = body.V[body.T][:, :, 0].mean(1)
        m = (tri_arm > 0.5) & ((cx > 0) if side == "l" else (cx < 0))
        setattr(body, key, body.all.subset(m))
    return getattr(body, key)


class ArmAxis:
    """Shoulder -> elbow -> wrist bone line with a smooth tangent and a lateral-up frame."""

    def __init__(self, body, side):
        self.S = body.bones[f"upperarm_{side}"][0]
        self.E = body.bones[f"lowerarm_{side}"][0]
        self.W = body.bones[f"hand_{side}"][0]
        self.u1, self.u2 = _unit(self.E - self.S), _unit(self.W - self.E)
        self.L1, self.L2 = np.linalg.norm(self.E - self.S), np.linalg.norm(self.W - self.E)
        sgn = 1.0 if side == "l" else -1.0
        self.e1 = _unit(np.cross(self.u1, [0, 1, 0]) * sgn)

    def point(self, s):
        return self.S + self.u1 * s if s <= self.L1 else self.E + self.u2 * (s - self.L1)

    def tangent(self, s, blend=0.05):
        w = np.clip((s - (self.L1 - blend)) / (2 * blend), 0, 1)
        w = w * w * (3 - 2 * w)
        return _unit(self.u1 * (1 - w) + self.u2 * w)

    def frame(self, s):
        t = self.tangent(s)
        e1 = _unit(self.e1 - (self.e1 @ t) * t)
        return t, e1, np.cross(t, e1)

    def coords(self, p):
        rel = p - self.S
        a = float(rel @ self.u1)
        _, e1, e2 = self.frame(min(a, self.L1 - 0.06))
        perp = rel - a * self.u1
        return a, math.atan2(perp @ e2, perp @ e1), float(np.linalg.norm(perp))


# ----------------------------------------------------------------------- params

def top_params(body, kind, belt):
    lm = body.lm
    jz, ap = lm["jugular_z"], lm["armpit_z"]
    p = dict(kind=kind, pitch=0.0095, hem_gap=0.008)
    if kind == "tee":
        p.update(name="SKM_LinenTee", ua_z=ap - 0.030, x_in=0.066, x_out=0.142,
                 zc=jz - 0.032, z_in=jz - 0.010, z_out=ap + 0.068,
                 zcb=jz + 0.026, z_inb=jz + 0.030, z_outb=ap + 0.064,
                 sleeve_end=0.140, base=0.0035, hang=0.45, sleeve_ease=0.0065, cuff=None,
                 slit=None, collar=None, opening=False)
    elif kind == "longshirt":
        p.update(name="SKM_LinenLongShirt", ua_z=ap - 0.036, x_in=0.064, x_out=0.142,
                 zc=jz - 0.026, z_in=jz - 0.008, z_out=ap + 0.068,
                 zcb=jz + 0.028, z_inb=jz + 0.032, z_outb=ap + 0.064,
                 sleeve_end=None, base=0.0045, hang=0.72, sleeve_ease=0.011, cuff=0.034,
                 cuff_ease=0.0035, slit=0.11, slit_open=0.012, collar=None, opening=False)
    elif kind == "coat":
        p.update(name="SKM_FurCoat", pitch=0.0105, ua_z=ap - 0.050, x_in=0.072, x_out=0.143, hem_gap=0.016,
                 zc=jz - 0.020, z_in=jz - 0.004, z_out=ap + 0.070,
                 zcb=jz + 0.030, z_inb=jz + 0.034, z_outb=ap + 0.066,
                 sleeve_end=None, base=0.010, hang=0.80, sleeve_ease=0.012, cuff=0.055,
                 cuff_ease=0.012, slit=None, collar=dict(rows=3, dz=0.010, lean=0.006, off=0.011),
                 opening=True, open_gap=0.003, facing=0.032)
    else:
        raise ValueError(kind)
    p["hem"] = {int(t): belt.z(t) + belt.radius + p["hem_gap"] for t in range(-180, 181)}
    return p


def hem_z(p, t):
    return float(np.interp(t, np.arange(-180, 181), [p["hem"][i] for i in range(-180, 181)]))


# ------------------------------------------------------------------------ build

def build_top(body, p):
    g = Garment(p["name"])
    g.extra["params"] = p
    pitch = p["pitch"]
    zf_in, zf_out = p["z_in"], p["z_out"]
    zb_in, zb_out = p["z_inb"], p["z_outb"]
    th_si = theta_for_x(body, p["x_in"], zf_in, False)
    th_so = theta_for_x(body, p["x_out"], zf_out, False)
    phb_si = 180 - theta_for_x(body, p["x_in"], zb_in, True)
    phb_so = 180 - theta_for_x(body, p["x_out"], zb_out, True)
    width = p["x_out"] - p["x_in"]
    ny = max(3, int(round(width / pitch))) + 1
    zr = p["ua_z"] - 0.02
    fl = (resample(body, zr, 0, th_si, pitch, min_n=4)[:-1]
          + list(np.linspace(th_si, th_so, ny))[:-1]
          + resample(body, zr, th_so, 90, pitch, min_n=3, density=1.6))
    bl = (resample(body, zr, 90, 180 - phb_so, pitch, min_n=3, density=1.6)[:-1]
          + list(np.linspace(180 - phb_so, 180 - phb_si, ny))[:-1]
          + resample(body, zr, 180 - phb_si, 180, pitch, min_n=4))
    left = fl[:-1] + bl
    thetas = sorted(set(round(-180.0 if t >= 180 - 1e-9 else t, 9) for t in left + [-t for t in left[1:-1]]))
    C = len(thetas)

    def z_top(t):
        a = abs(t)
        if a <= 90:
            if a <= th_si:
                return p["zc"] + (zf_in - p["zc"]) * (a / th_si) ** 2
            if a <= th_so + 1e-6:
                return zf_in + (zf_out - zf_in) * (a - th_si) / (th_so - th_si)
            u = (90 - a) / (90 - th_so)
            return p["ua_z"] + (zf_out - p["ua_z"]) * u ** 1.8
        ph = 180 - a
        if ph <= phb_si:
            return p["zcb"] + (zb_in - p["zcb"]) * (ph / phb_si) ** 2
        if ph <= phb_so + 1e-6:
            return zb_in + (zb_out - zb_in) * (ph - phb_si) / (phb_so - phb_si)
        u = (90 - ph) / (90 - phb_so)
        return p["ua_z"] + (zb_out - p["ua_z"]) * u ** 1.8

    tops = [z_top(t) for t in thetas]
    bots = [hem_z(p, t) for t in thetas]
    R = int(math.ceil(max(tt - bb for tt, bb in zip(tops, bots)) / pitch))
    grid = np.zeros((R + 1, C), np.int64)
    for j, t in enumerate(thetas):
        prev = None
        for k in range(R + 1):
            z = bots[j] + (tops[j] - bots[j]) * k / R
            loc, nrm = cyl_hit(body, t, z)
            pt = loc + nrm * p["base"] if loc is not None else (prev if prev is not None else np.array([0, 0, z]))
            prev = pt
            grid[k, j] = g.add(pt)
    for k in range(R):
        for j in range(C):
            jn = (j + 1) % C
            g.quad(grid[k, j], grid[k, jn], grid[k + 1, jn], grid[k + 1, j], 0 if (-90 <= thetas[j] < 90) else 1)
    # yoke ribbons over the shoulders
    for side in (1, -1):
        fcols = [j for j, t in enumerate(thetas) if 0 <= side * t <= 90 and th_si - 1e-6 <= abs(t) <= th_so + 1e-6]
        bcols = [j for j, t in enumerate(thetas) if abs(t) > 90 and side * t > 0
                 and phb_si - 1e-6 <= 180 - abs(t) <= phb_so + 1e-6]
        fcols.sort(key=lambda j: abs(thetas[j]))
        bcols.sort(key=lambda j: -abs(thetas[j]))
        assert len(fcols) == len(bcols) == ny, (len(fcols), len(bcols), ny)
        front = [grid[R, j] for j in fcols]
        back = [grid[R, j] for j in bcols]
        paths = []
        for fa, ba in zip(front, back):
            P0, P2 = g.V[fa], g.V[ba]
            crest_pt = crest(body, 0.5 * (P0[0] + P2[0]))
            _, cn, _ = body.all.nearest(crest_pt[None])
            crest_pt = crest_pt + cn[0] * p["base"]
            Q = 2 * crest_pt - 0.5 * (P0 + P2)
            paths.append((P0, Q, P2, crest_pt))
        length = np.mean([np.linalg.norm(a[3] - a[0]) + np.linalg.norm(a[2] - a[3]) for a in paths])
        L = max(6, int(round(length / pitch)))
        L += L % 2
        rib = [front]
        for l in range(1, L):
            u = l / L
            row = []
            for (P0, Q, P2, cp) in paths:
                B = (1 - u) ** 2 * P0 + 2 * u * (1 - u) * Q + u * u * P2
                Cc = np.array([cp[0] * 0.55, cp[1], cp[2] - 0.13])
                loc, nrm = body.all.ray(Cc, B - Cc)
                row.append(g.add(loc + nrm * p["base"] if loc is not None else B))
            rib.append(row)
        rib.append(back)
        for l in range(L):
            for c in range(ny - 1):
                g.quad(rib[l][c], rib[l][c + 1], rib[l + 1][c + 1], rib[l + 1][c], 0 if l < L // 2 else 1)
        g.seam(rib[L // 2])
        g.feature("shoulder", rib[L // 2])
    side_cols = {}
    for t_side, sname in ((90.0, "l"), (-90.0, "r")):
        j = int(np.argmin([abs(t - t_side) for t in thetas]))
        col = list(grid[:, j])
        side_cols[sname] = col
    j_cf = int(np.argmin(np.abs(np.array(thetas))))
    g.extra.update(grid=grid, thetas=thetas, R=R, j_cf=j_cf, side_cols=side_cols)
    n_torso = len(g.V)
    # sleeves
    V, F = g.arrays()
    loops = boundary_loops(F, len(V))
    hem_v, neck_v = int(grid[0, 0]), int(grid[R, j_cf])
    arm_loops = [lp for lp in loops if hem_v not in lp and neck_v not in lp]
    neck_loop = next(lp for lp in loops if neck_v in lp)
    assert len(arm_loops) == 2, [len(l) for l in loops]
    region = {}
    axial = {}
    hem_axis = {}
    sleeves = {}
    for lp in arm_loops:
        side = "l" if V[lp, 0].mean() > 0 else "r"
        sleeves[side] = attach_sleeve(g, body, lp, side, p, region, axial, hem_axis)
    for side in ("l", "r"):
        col = side_cols[side]
        und = sleeves[side]["under"]
        g.seam(col)
        g.feature("side", col[::-1] + und[1:])
    g.extra["sleeves"] = sleeves
    g.extra["neck_loop"] = neck_loop
    round_loop(g, body, neck_loop, p["base"], p)
    if p.get("collar"):
        g.extra["collar"] = attach_collar(g, body, neck_loop, p, region)
    n = len(g.V)
    reg = np.zeros(n, np.int64)
    ax = np.zeros(n)
    ha = np.zeros((n, 3))
    for i, r in region.items():
        reg[i] = r
    for i, a in axial.items():
        ax[i] = a
    for i, h in hem_axis.items():
        ha[i] = h
    g.extra.update(region=reg, axial=ax, hem_axis=ha, n_torso=n_torso)
    return g


def attach_sleeve(g, body, loop, side, p, region, axial, hem_axis):
    arm = ArmAxis(body, side)
    surf = arm_surface(body, side)
    pitch = p["pitch"]
    P = np.array([g.V[i] for i in loop])
    co = [arm.coords(q) for q in P]
    a0 = np.array([c[0] for c in co])
    phi = np.array([c[1] for c in co])
    rad = np.array([c[2] for c in co])
    # walk the loop so phi increases (consistent quad winding around the arm)
    if np.unwrap(phi)[-1] < np.unwrap(phi)[0]:
        loop, a0, phi, rad = loop[::-1], a0[::-1], phi[::-1], rad[::-1]
    s_end = p["sleeve_end"] if p["sleeve_end"] else arm.L1 + arm.L2 - 0.004
    if p["kind"] == "coat":
        s_end = arm.L1 + arm.L2 - 0.012
    Q = len(loop)
    T = int(math.ceil((s_end - a0.mean()) / pitch))
    rows = [list(loop)]
    rad = rad.copy()
    for r in range(1, T + 1):
        row = []
        for q in range(Q):
            s = a0[q] + (s_end - a0[q]) * r / T
            t, e1, e2 = arm.frame(s)
            d = math.cos(phi[q]) * e1 + math.sin(phi[q]) * e2
            c = arm.point(s)
            loc, nrm = surf.ray(c + d * 0.2, -d, dist=0.2)
            if loc is not None and np.linalg.norm(loc - c) < 0.12:
                pt = loc + nrm * p["sleeve_ease"]
                rad[q] = np.linalg.norm(loc - c)
            else:
                pt = c + d * (rad[q] + p["sleeve_ease"])
            vi = g.add(pt)
            region[vi] = 1 if side == "l" else 2
            axial[vi] = s
            if r == T:
                hem_axis[vi] = t
            row.append(vi)
        rows.append(row)
    for vi in loop:
        region.setdefault(vi, 0)
    cuff_r = None
    if p.get("cuff"):
        cuff_r = max(1, int(round(T - p["cuff"] / pitch)))
    base_panel = 2 if side == "l" else 3
    for r in range(T):
        for q in range(Q):
            qn = (q + 1) % Q
            panel = base_panel + (2 if cuff_r is not None and r >= cuff_r else 0)
            g.quad(rows[r][q], rows[r][qn], rows[r + 1][qn], rows[r + 1][q], panel)
    # underarm seam: the column hanging lowest below the arm (largest axial start = armpit)
    qu = int(np.argmax(a0))
    under = [rows[r][qu] for r in range(T + 1)]
    g.seam(under)
    g.seam(list(loop) + [loop[0]])
    g.feature("armhole", list(loop) + [loop[0]])
    if cuff_r is not None:
        g.seam(rows[cuff_r] + [rows[cuff_r][0]])
        g.feature("cuffseam", rows[cuff_r] + [rows[cuff_r][0]])
    return dict(rows=rows, under=under, T=T, s_end=s_end, cuff_row=cuff_r, axis=arm)


def round_loop(g, body, loop, off, p, rows=6):
    """Snap the neckline to a smooth crew curve round the neck base: low at the front (``zc``),
    at the neck-shoulder point on the sides, a little lower again at the back (``zcb``). The
    correction fades over a few rows into the panel so the edge quads stay square."""
    loop = list(loop)
    P = np.array([g.V[i] for i in loop])
    yc0 = body.center_y(1.45)
    psi = np.unwrap(np.arctan2(P[:, 0], -(P[:, 1] - yc0)))
    # even out the spacing along the loop (the yoke ribbons crowd the sides)
    n = len(psi)
    uni = psi[0] + (psi[-1] - psi[0]) * np.arange(n) / (n - 1) if abs(psi[-1] - psi[0]) < 1.9 * math.pi else None
    if uni is None:
        start = psi[0]
        uni = start + np.sign(psi[1] - psi[0]) * 2 * math.pi * np.arange(n) / n
    psi = 0.8 * psi + 0.2 * uni
    zs = crest(body, p["x_in"])[2] - 0.004
    cf = np.maximum(np.cos(psi), 0) ** 1.4
    cb = np.maximum(-np.cos(psi), 0) ** 1.4
    zt = zs - (zs - p["zc"]) * cf - (zs - p["zcb"]) * cb
    delta = {}
    Q = P.copy()
    for k, (a, z) in enumerate(zip(psi, zt)):
        yc = body.center_y(z)
        d = np.array([math.sin(a), -math.cos(a), 0.0])
        loc, nrm = body.all.ray(np.array([0.0, yc, z]) + d * 0.3, -d, dist=0.3)
        if loc is not None:
            Q[k] = loc + nrm * off
    for _ in range(6):
        Q = Q + 0.5 * (0.5 * (np.roll(Q, 1, 0) + np.roll(Q, -1, 0)) - Q)
        _, loc, nrm = body.all.signed(Q)
        Q = loc + nrm * off
    for i, q in zip(loop, Q):
        delta[i] = q - g.V[i]
        g.V[i] = q
    V, F = g.arrays()
    nbr = {}
    for f in F:
        for a, b in zip(f, list(f[1:]) + [f[0]]):
            nbr.setdefault(a, set()).add(b)
            nbr.setdefault(b, set()).add(a)
    ring, seen = set(loop), set(loop)
    for r in range(1, rows + 1):
        nxt = set()
        for i in ring:
            nxt |= nbr.get(i, set())
        nxt -= seen
        w = (1.0 - r / (rows + 1)) ** 1.5
        for j in nxt:
            ds = [delta[k] for k in nbr[j] if k in delta]
            if ds:
                delta[j] = w * np.mean(ds, axis=0)
                g.V[j] = g.V[j] + delta[j]
        seen |= nxt
        ring = nxt


def attach_collar(g, body, loop, p, region):
    cp = p["collar"]
    P = np.array([g.V[i] for i in loop])
    rows = [list(loop)]
    prev = P.copy()
    for k in range(1, cp["rows"] + 1):
        row = []
        for q, v in enumerate(P):
            yc0 = body.center_y(v[2])
            front = max(0.0, -(v[1] - yc0) / max(math.hypot(v[0], v[1] - yc0), 1e-6))
            z = v[2] + cp["dz"] * k * (1.0 - 0.6 * front ** 1.2)
            yc = body.center_y(z)
            c = np.array([0.0, yc, z])
            d = np.array([v[0], v[1] - yc, 0.0])
            d = d / max(np.linalg.norm(d), 1e-9)
            loc, nrm = body.all.ray(c + d * 0.3, -d, dist=0.3)
            r_prev = math.hypot(prev[q, 0], prev[q, 1] - yc)
            r = r_prev + cp["lean"]
            if loc is not None:
                r = max(r, math.hypot(loc[0], loc[1] - yc) + cp["off"])
            pt = c + d * r
            vi = g.add(pt)
            region[vi] = 3
            row.append(vi)
        prev = np.array([g.V[i] for i in row])
        rows.append(row)
    Q = len(loop)
    for k in range(cp["rows"]):
        for q in range(Q):
            qn = (q + 1) % Q
            g.quad(rows[k][q], rows[k][qn], rows[k + 1][qn], rows[k + 1][q], 6)
    g.seam(list(loop) + [loop[0]])
    g.feature("collarseam", list(loop) + [loop[0]])
    return dict(rows=rows)


# ----------------------------------------------------------------- after the fit

def front_column(g):
    """Centre-front vertices from hem to neckline (plus the collar), bottom to top."""
    grid, j = g.extra["grid"], g.extra["j_cf"]
    col = list(grid[:, j])
    if "collar" in g.extra:
        top = col[-1]
        rows = g.extra["collar"]["rows"]
        q = rows[0].index(top)
        col += [rows[k][q] for k in range(1, len(rows))]
    return col


def split_front(g, V, F, z_from):
    """Cut the centre front above ``z_from`` (the shirt's neck slit / the coat's opening): the
    faces on her right (x < 0) get their own copy of the cut vertices. Returns the new arrays
    and (left_edge, right_edge) vertex lists, bottom to top."""
    col = [v for v in front_column(g) if V[v, 2] >= z_from - 1e-9]
    if not g.extra["params"].get("opening"):
        col = col[1:]           # the slit's bottom vertex stays shared
    dup = {}
    Vl = list(V)
    for v in col:
        dup[v] = len(Vl)
        Vl.append(V[v].copy())
    F = F.copy()
    cent = V[F].mean(1)
    for fi in np.where(cent[:, 0] < 0)[0]:
        for k in range(4):
            if F[fi, k] in dup:
                F[fi, k] = dup[F[fi, k]]
    left = col
    right = [dup[v] for v in col]
    return np.array(Vl), F, left, right


def spread_opening(V, left, right, width_fn):
    V = V.copy()
    for a, b in zip(left, right):
        w = width_fn(V[a])
        V[a, 0] += w
        V[b, 0] -= w
    return V
