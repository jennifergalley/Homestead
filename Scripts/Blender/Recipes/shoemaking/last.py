"""The shoemaker's last: a smooth parametric envelope of one of the heroine's feet and lower legs.

A real last is the wooden form a shoe is built on. Here it is a family of cross-section planes
swept down the leg and along the foot. Each plane cuts the body mesh, the cut is replaced by its
2D convex hull (bridging the gaps between toes, the arch and the Achilles hollows, as leather
does), and the hull is sampled by rays from a smoothed section centre. That gives
``rho_h(s, alpha)``: the skin envelope in a (length, angle) parametrisation that every layer of a
shoe (lining, upper, sole) is an offset of.

The sweep bends around a pivot line (parallel to X) held well in front of the ankle crease:

* shaft (``s < L_sh``): horizontal planes from the top of the table down to ``z_p``;
* fan (``L_sh <= s < s_f0``): half-planes through the pivot, rotating from horizontal-back to
  vertical-down, so the heel and ankle are cut on radial planes that never cross;
* foot (``s >= s_f0``): vertical planes perpendicular to Y, from the pivot to beyond the toes.

``alpha`` is measured in each plane from ``w`` (back of the leg, back of the heel, sole centre)
toward ``xh`` (lateral: +X for the left foot, -X for the right), so alpha = 0 is the back/sole
seam line, 90 deg the lateral side, 180 deg the front of the shin and the top of the foot,
270 deg the medial side, for both feet. Units are meters, Z up, the character faces -Y.
"""
import math

import numpy as np

TAU = 2.0 * math.pi


# ------------------------------------------------------------------ 2D helpers

def hull2d(P):
    """Convex hull (counter-clockwise, no repeated end point) of 2D points, monotone chain."""
    P = np.unique(np.round(np.asarray(P, float), 7), axis=0)
    if len(P) < 3:
        return P
    P = P[np.lexsort((P[:, 1], P[:, 0]))]

    def half(points):
        out = []
        for p in points:
            while len(out) >= 2:
                o, a = out[-2], out[-1]
                if (a[0] - o[0]) * (p[1] - o[1]) - (a[1] - o[1]) * (p[0] - o[0]) <= 0:
                    out.pop()
                else:
                    break
            out.append(p)
        return out
    lower, upper = half(P), half(P[::-1])
    return np.array(lower[:-1] + upper[:-1])


def poly_area_centroid(H):
    x, y = H[:, 0], H[:, 1]
    xn, yn = np.roll(x, -1), np.roll(y, -1)
    cr = x * yn - xn * y
    a = 0.5 * cr.sum()
    if abs(a) < 1e-12:
        return 0.0, H.mean(0)
    cx = ((x + xn) * cr).sum() / (6 * a)
    cy = ((y + yn) * cr).sum() / (6 * a)
    return abs(a), np.array([cx, cy])


def ray_poly(c, H, ang):
    """Distance from ``c`` (inside the convex polygon ``H``) to its boundary along each angle."""
    d = np.stack([np.cos(ang), np.sin(ang)], 1)                 # (k,2)
    p, q = H, np.roll(H, -1, 0)
    e = q - p                                                  # (m,2)
    pc = p - c
    den = d[:, None, 0] * e[None, :, 1] - d[:, None, 1] * e[None, :, 0]   # cross(d, e)
    with np.errstate(divide="ignore", invalid="ignore"):
        t = (pc[None, :, 0] * e[None, :, 1] - pc[None, :, 1] * e[None, :, 0]) / den
        u = (pc[None, :, 0] * d[:, None, 1] - pc[None, :, 1] * d[:, None, 0]) / den
    ok = (u >= -1e-9) & (u <= 1 + 1e-9) & (t > 0) & np.isfinite(t)
    t = np.where(ok, t, -np.inf)
    r = t.max(1)
    return np.where(np.isfinite(r), r, np.nan)


def smin(a, b, k):
    """Polynomial smooth minimum (rounds the corner where two surfaces meet over ~k)."""
    b = np.minimum(b, a + 4 * k)
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b * (1 - h) + a * h - k * h * (1 - h)


def smoothstep(x, e0, e1):
    t = np.clip((np.asarray(x, float) - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def gauss1d(x, sigma, axis=0, periodic=False):
    """Gaussian smoothing along one axis (edge-clamped or periodic), sigma in samples."""
    if sigma <= 0:
        return x
    r = int(math.ceil(3 * sigma))
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    x = np.moveaxis(np.asarray(x, float), axis, 0)
    if periodic:
        xp = np.concatenate([x[-r:], x, x[:r]], 0)
    else:
        xp = np.concatenate([np.repeat(x[:1], r, 0), x, np.repeat(x[-1:], r, 0)], 0)
    out = np.zeros_like(x)
    for i, kk in enumerate(k):
        out += kk * xp[i:i + len(x)]
    return np.moveaxis(out, 0, axis)


# ------------------------------------------------------------------------ last

class Last:
    """Section table of one foot. ``V``/``T``: body vertices and triangles (bind pose, meters)."""

    def __init__(self, V, T, side, z_top, z_p=0.14, pivot_margin=0.075, R_f=0.10, ds=0.002,
                 n_alpha=192, tip_extra=0.04, log=print):
        self.side = side
        self.sgn = 1.0 if side == "l" else -1.0
        sel = (V[:, 0] * self.sgn > 0.03) & (V[:, 2] < z_top + 0.07)
        tri = T[sel[T].all(1)]
        used = np.unique(tri)
        Vs = V[used]
        self.leg_vertices = used
        self.z_p = z_p
        self.y_p = float(Vs[Vs[:, 2] > z_p, 1].min() - pivot_margin)
        self.z0 = z_top + 0.035
        self.L_sh = self.z0 - z_p
        self.R_f = R_f
        self.s_f0 = self.L_sh + R_f * math.pi / 2
        self.y_toe = float(Vs[:, 1].min())
        self.y_heel = float(Vs[Vs[:, 2] < 0.04, 1].max())
        self.s_toe = self.s_f0 + (self.y_p - self.y_toe)
        self.ds = ds
        self.s = np.arange(0.0, self.s_toe + tip_extra, ds)
        self.alpha = np.arange(n_alpha) * TAU / n_alpha
        self.na = n_alpha
        P3 = V[tri]                                                   # (m,3,3)
        O, w, xh, n = self.frame(self.s)
        hulls, cents, areas = [], [], []
        for i in range(len(self.s)):
            dv = P3 @ n[i] - O[i] @ n[i]                              # (m,3)
            pts = []
            for a, b in ((0, 1), (1, 2), (2, 0)):
                m = dv[:, a] * dv[:, b] < 0
                if m.any():
                    t = dv[m, a] / (dv[m, a] - dv[m, b])
                    pts.append(P3[m, a] + t[:, None] * (P3[m, b] - P3[m, a]))
            if not pts:
                hulls.append(None); cents.append(None); areas.append(0.0)
                continue
            P = np.vstack(pts) - O[i]
            A, B = P @ w[i], P @ xh[i]
            keep = A > -1e-4
            if keep.sum() < 3:
                hulls.append(None); cents.append(None); areas.append(0.0)
                continue
            H = hull2d(np.stack([A[keep], B[keep]], 1))
            area, c = poly_area_centroid(H) if len(H) >= 3 else (0.0, None)
            hulls.append(H if len(H) >= 3 else None); cents.append(c); areas.append(area)
        areas = np.array(areas)
        valid = areas > 1.2e-4
        i_tip = int(np.where(valid)[0].max())
        self.i_tip = i_tip
        self.s_tip = float(self.s[i_tip])
        C = np.array([cents[i] if valid[i] else cents[i_tip] for i in range(len(self.s))])
        C[i_tip:] = C[i_tip]
        Cs = gauss1d(C[:i_tip + 1], 6.0)
        rho = np.zeros((len(self.s), n_alpha))
        for i in range(i_tip + 1):
            r = ray_poly(Cs[i], hulls[i], self.alpha)
            if np.isnan(r).any():
                Cs[i] = C[i]
                r = ray_poly(Cs[i], hulls[i], self.alpha)
            rho[i] = r
        Cs = np.vstack([Cs, np.repeat(Cs[-1:], len(self.s) - i_tip - 1, 0)])
        rho[i_tip + 1:] = rho[i_tip]
        # leather over the toes: per-column concave envelope of rho along s over the forefoot,
        # so the toe box bridges from the ball over the toe tips instead of showing each toe
        i_env = int(np.searchsorted(self.s, self.s_f0 + (self.y_p - (self.y_toe + 0.095))))
        ss = self.s[i_env:i_tip + 1]
        for j in range(n_alpha):
            r = rho[i_env:i_tip + 1, j]
            H = []
            for k in range(len(ss)):
                while len(H) >= 2:
                    a, b_ = H[-2], H[-1]
                    if (ss[b_] - ss[a]) * (r[k] - r[a]) - (r[b_] - r[a]) * (ss[k] - ss[a]) >= 0:
                        H.pop()
                    else:
                        break
                H.append(k)
            rho[i_env:i_tip + 1, j] = np.maximum(r, np.interp(ss, ss[H], r[H]))
        self.i_env = i_env
        self.C = Cs
        self.rho_h = rho
        self.hulls = hulls
        self.areas = areas
        log(f"last {side}: pivot y {self.y_p:.3f} z {z_p:.3f}, shaft {self.L_sh:.3f} m, fan to {self.s_f0:.3f},"
            f" toe tip s {self.s_tip:.3f} (y {self.y_toe:.3f}), {len(self.s)} sections x {n_alpha}")

    # --- frames
    def frame(self, s):
        s = np.asarray(s, float)
        shp = s.shape
        s = s.ravel()
        O = np.zeros((len(s), 3)); w = np.zeros((len(s), 3)); n = np.zeros((len(s), 3))
        xh = np.tile([self.sgn, 0.0, 0.0], (len(s), 1))
        sh = s < self.L_sh
        ft = s >= self.s_f0
        fan = ~sh & ~ft
        O[sh] = np.stack([np.zeros(sh.sum()), np.full(sh.sum(), self.y_p), self.z0 - s[sh]], 1)
        w[sh] = (0, 1, 0); n[sh] = (0, 0, -1)
        phi = (s[fan] - self.L_sh) / self.R_f
        O[fan] = (0, self.y_p, self.z_p)
        w[fan] = np.stack([np.zeros_like(phi), np.cos(phi), -np.sin(phi)], 1)
        n[fan] = np.stack([np.zeros_like(phi), -np.sin(phi), -np.cos(phi)], 1)
        O[ft] = np.stack([np.zeros(ft.sum()), self.y_p - (s[ft] - self.s_f0), np.full(ft.sum(), self.z_p)], 1)
        w[ft] = (0, 0, -1); n[ft] = (0, -1, 0)
        r = lambda a: a.reshape(shp + (3,))
        return r(O), r(w), r(xh), r(n)

    # --- table interpolation
    def interp(self, table, s, alpha):
        """Bilinear lookup of a (Ns, Na) table at arbitrary (s, alpha); alpha is periodic."""
        fs = np.clip(np.asarray(s, float) / self.ds, 0, len(self.s) - 1.000001)
        i0 = np.floor(fs).astype(int); ts = fs - i0
        fa = np.mod(np.asarray(alpha, float), TAU) / TAU * self.na
        j0 = np.floor(fa).astype(int) % self.na; ta = fa - np.floor(fa)
        j1 = (j0 + 1) % self.na
        a = table[i0, j0] * (1 - ta) + table[i0, j1] * ta
        b = table[i0 + 1, j0] * (1 - ta) + table[i0 + 1, j1] * ta
        return a * (1 - ts) + b * ts

    def center(self, s):
        fs = np.clip(np.asarray(s, float) / self.ds, 0, len(self.s) - 1.000001)
        i0 = np.floor(fs).astype(int); t = (fs - i0)[..., None]
        return self.C[i0] * (1 - t) + self.C[i0 + 1] * t

    def point(self, s, alpha, rho):
        s, alpha, rho = np.broadcast_arrays(np.asarray(s, float), np.asarray(alpha, float), np.asarray(rho, float))
        O, w, xh, _ = self.frame(s)
        c = self.center(s)
        A = c[..., 0] + rho * np.cos(alpha)
        B = c[..., 1] + rho * np.sin(alpha)
        return O + A[..., None] * w + B[..., None] * xh

    def direction(self, s, alpha):
        s, alpha = np.broadcast_arrays(np.asarray(s, float), np.asarray(alpha, float))
        _, w, xh, _ = self.frame(s)
        return np.cos(alpha)[..., None] * w + np.sin(alpha)[..., None] * xh

    def hull_points(self):
        S, A = np.meshgrid(self.s, self.alpha, indexing="ij")
        return self.point(S, A, self.rho_h)

    def floor_rho(self, z_floor):
        S, A = np.meshgrid(self.s, self.alpha, indexing="ij")
        c3 = self.point(S, A, 0.0)
        d = self.direction(S, A)
        with np.errstate(divide="ignore"):
            r = np.where(d[..., 2] < -1e-6, (c3[..., 2] - z_floor) / np.maximum(-d[..., 2], 1e-6), np.inf)
        return np.where(r > 0, r, np.inf)

    # --- layers
    def layer(self, off, floor_z, tip_gap, round_k=0.004, min_off=None, smooth=8, dome_len=0.022,
              extra_smooth=None):
        """One shoe layer: skin envelope + ``off`` (Ns, Na), a flat floor at ``floor_z`` with a
        rounded edge, a domed toe ending ``tip_gap`` beyond the toe tip, and Laplacian smoothing
        that never goes below ``rho_h + min_off``. ``extra_smooth=(s_from, iters)`` adds a
        rubber-sheet pass over the forefoot so the toe box bridges the toes. Returns (rho, s_end)."""
        rf = self.floor_rho(floor_z)
        rho = smin(self.rho_h + off, rf, round_k)
        i_a = int(round((self.s_tip - dome_len) / self.ds))
        s_a = self.s[i_a]
        s_end = max(self.s_tip, self.s_toe) + tip_gap
        for i in range(i_a + 1, len(self.s)):
            u = (self.s[i] - s_a) / (s_end - s_a)
            dome = rho[i_a] * math.sqrt(max(0.0, 1 - u * u))
            rho[i] = np.maximum(rho[i], dome) if i <= self.i_tip else dome
        rho[i_a:] = smin(rho[i_a:], rf[i_a:], round_k)
        lo = np.minimum(self.rho_h + (off if min_off is None else min_off), rf)
        lo[self.i_tip + 1:] = 0
        live = self.s <= s_end
        passes = [(live, smooth)]
        if extra_smooth:
            passes.append((live & (self.s >= extra_smooth[0]), extra_smooth[1]))
        for mask, iters in passes:
            for _ in range(iters):
                avg = 0.25 * (np.roll(rho, 1, 1) + np.roll(rho, -1, 1))
                up = np.vstack([rho[:1], rho[:-1]]); dn = np.vstack([rho[1:], rho[-1:]])
                avg = avg + 0.25 * (up + dn)
                rho = np.where(mask[:, None], rho + 0.6 * (avg - rho), rho)
                rho = np.minimum(np.maximum(rho, lo), rf)
        self._rf = rf
        return np.maximum(rho, 0.0), s_end

    def enforce(self, rho, surf, target, floor_z, iters=5, s_end=None):
        """Push the layer outward until every table point is at least ``target`` (Ns, Na) from the
        skin (normal distance, not the radial offset), except on the flat floor; then smooth
        outward-only so the corrections leave no dents."""
        rf = self.floor_rho(floor_z)
        S, A = np.meshgrid(self.s, self.alpha, indexing="ij")
        live = (self.s <= self.s_tip)[:, None] & np.ones_like(rho, bool)
        rho = rho.copy()
        worst = 0.0
        for it in range(iters):
            P = self.point(S[live], A[live], rho[live])
            d, _, _ = surf.signed(P)
            on_floor = P[:, 2] < floor_z + 0.0025
            tgt = target(P) if callable(target) else target[live]
            deficit = np.where(on_floor, 0.0, np.clip(tgt - d, 0, None))
            worst = float(deficit.max())
            if worst < 2e-4:
                break
            r = rho[live]
            r += 1.15 * deficit
            rho[live] = r
            rho = np.minimum(rho, rf)
        for _ in range(3):
            avg = 0.25 * (np.roll(rho, 1, 1) + np.roll(rho, -1, 1))
            up = np.vstack([rho[:1], rho[:-1]]); dn = np.vstack([rho[1:], rho[-1:]])
            avg = avg + 0.25 * (up + dn)
            rho = np.where(live, np.maximum(rho, avg), rho)
            rho = np.minimum(rho, rf)
        return rho, worst

    def normal(self, rho, s, alpha, eps=0.0015):
        """Outward unit normal of a layer surface at (s, alpha), by central differences."""
        s = np.asarray(s, float); alpha = np.asarray(alpha, float)
        ea = eps / np.maximum(self.interp(rho, s, alpha), 0.01)
        f = lambda ss, aa: self.point(ss, aa, self.interp(rho, ss, aa))
        du = f(s, alpha + ea) - f(s, alpha - ea)
        dv = f(s + eps, alpha) - f(s - eps, alpha)
        n = np.cross(du, dv)
        n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-12)
        out = self.direction(s, alpha)
        return np.where((n * out).sum(-1, keepdims=True) < 0, -n, n)

    def s_at_height(self, rho, alpha, z, s_lo=0.0, s_hi=None):
        """For each alpha, the s where the layer surface crosses height z (z decreases with s)."""
        s_hi = self.s_tip if s_hi is None else s_hi
        a = np.asarray(alpha, float)
        lo = np.full(a.shape, s_lo); hi = np.full(a.shape, s_hi)
        z = np.broadcast_to(np.asarray(z, float), a.shape)
        for _ in range(40):
            mid = 0.5 * (lo + hi)
            pz = self.point(mid, a, self.interp(rho, mid, a))[..., 2]
            above = pz > z
            lo = np.where(above, mid, lo); hi = np.where(above, hi, mid)
        return 0.5 * (lo + hi)

    def surface(self, rho, s_start, s_end, alphas, n_rows, power=1.0):
        """Grid of points (n_rows+1, n_cols, 3): each column alpha_j runs from s_start[j] to s_end
        (where the domed toe closes to a point)."""
        s_start = np.broadcast_to(np.asarray(s_start, float), alphas.shape)
        t = np.linspace(0, 1, n_rows + 1) ** power
        S = s_start[None, :] + (np.asarray(s_end, float) - s_start[None, :]) * t[:, None]
        A = np.broadcast_to(alphas[None, :], S.shape)
        R = self.interp(rho, S, A)
        R[-1] = 0.0
        return self.point(S, A, R), S, A
