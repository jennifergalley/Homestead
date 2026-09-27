"""Layered fitting: garments that must clear the body *and* the garments worn under them.

A garment layer is an open surface (the outer shell of a solidified garment). The signed distance
to it is only meaningful where the query point's foot of perpendicular lands inside the shell,
not past its edge, so ``LayerSurface.signed`` also returns a validity mask. ``relax_layers`` is
the membrane solve of ``garments.relax`` generalised to several such constraints, per-vertex
hem axes (sleeve cuffs keep their position along the arm) and a drape ("hang") allowance.
"""
import math

import numpy as np

from .body import Surface, mesh_arrays
from .garments import edges_of


class ClosedSurface:
    """A closed surface (the body): every foot of perpendicular is valid."""

    def __init__(self, surf):
        self.surf = surf

    def signed(self, pts):
        s, loc, nrm = self.surf.signed(pts)
        return s, loc, nrm, np.ones(len(pts), bool)


class LayerSurface:
    """Outward-facing faces of a worn garment."""

    def __init__(self, V, N, T, name=""):
        self.surf = Surface(V, N, T)
        self.name = name

    def signed(self, pts):
        loc, nrm, _ = self.surf.nearest(pts)
        diff = pts - loc
        dist = np.linalg.norm(diff, axis=1)
        s = (diff * nrm).sum(1)
        valid = (dist < 0.0008) | (np.abs(s) > 0.8 * dist)
        return s, loc, nrm, valid


def outer_shell(obj, body_surf, name=None):
    """Faces of a solidified garment that face away from the body (outer shell and rims)."""
    V, N, T = mesh_arrays(obj)
    c = V[T].mean(1)
    fn = np.cross(V[T[:, 1]] - V[T[:, 0]], V[T[:, 2]] - V[T[:, 0]])
    fn /= np.maximum(np.linalg.norm(fn, axis=1, keepdims=True), 1e-12)
    _, _, bn = body_surf.signed(c)
    keep = (fn * bn).sum(1) > 0.2
    return LayerSurface(V, N, T[keep], name or obj.name)


# ------------------------------------------------------------------- drape

class RadialProfile:
    """Torso radius r(theta, z) about the torso centre line, for drape allowances."""

    def __init__(self, body, z0, z1, dz=0.005, dth=5.0):
        self.body = body
        self.zs = np.arange(z0, z1 + 1e-9, dz)
        self.ths = np.arange(-180.0, 180.0, dth)
        self.dth, self.dz = dth, dz
        self.yc = np.array([body.center_y(z) for z in self.zs])
        R = np.zeros((len(self.zs), len(self.ths)))
        for i, z in enumerate(self.zs):
            for j, t in enumerate(self.ths):
                loc, _ = body.cyl_hit(t, z, yc=self.yc[i])
                if loc is not None:
                    tr = math.radians(t)
                    if loc[0] * math.sin(tr) - (loc[1] - self.yc[i]) * math.cos(tr) <= 0:
                        loc = None      # passed through an armpit and hit the far flank
                R[i, j] = math.hypot(loc[0], loc[1] - self.yc[i]) if loc is not None else np.nan
        # fill misses from neighbours around the ring
        for i in range(len(self.zs)):
            row = R[i]
            if np.isnan(row).all():
                R[i] = R[i - 1] if i else 0.1
                continue
            idx = np.where(~np.isnan(row))[0]
            R[i] = np.interp(np.arange(len(row)), idx, row[idx], period=len(row))
        self.R = R

    def lookup(self, V, table):
        zi = np.clip((V[:, 2] - self.zs[0]) / self.dz, 0, len(self.zs) - 1.001)
        yc = np.interp(V[:, 2], self.zs, self.yc)
        th = np.degrees(np.arctan2(V[:, 0], -(V[:, 1] - yc)))
        tj = ((th + 180.0) / self.dth) % len(self.ths)
        i0 = zi.astype(int)
        j0 = tj.astype(int) % len(self.ths)
        j1 = (j0 + 1) % len(self.ths)
        a, b = zi - i0, tj - np.floor(tj)
        top = table[i0, j0] * (1 - b) + table[i0, j1] * b
        bot = table[np.minimum(i0 + 1, len(self.zs) - 1), j0] * (1 - b) + \
            table[np.minimum(i0 + 1, len(self.zs) - 1), j1] * b
        return top * (1 - a) + bot * a

    def hang(self, V, k, z_from):
        """Cloth hangs from the widest point above it: k * (envelope radius - radius)."""
        d = self.hang_table(z_from)
        out = k * self.lookup(V, d)
        return np.where(V[:, 2] < z_from, out, 0.0)

    def hang_table(self, z_from):
        top = np.searchsorted(self.zs, z_from)
        env = self.R.copy()
        for i in range(min(top, len(self.zs) - 1) - 1, -1, -1):
            env[i] = np.maximum(env[i], env[i + 1])
        # smooth the envelope around the ring so side seams don't kink
        for _ in range(3):
            env = 0.25 * np.roll(env, 1, 1) + 0.5 * env + 0.25 * np.roll(env, -1, 1)
        return np.maximum(env - self.R, 0.0)


class Drape:
    """Radial floor for loose tops: below ``z_from`` the cloth may not come closer to the torso
    axis than body radius + k * (hang allowance) + base, measured horizontally, so it falls
    straight from the bust and shoulder blades instead of following the waist in."""

    def __init__(self, prof, k, z_from, base, side_relief=0.75):
        self.prof, self.z_from = prof, z_from
        d = prof.hang_table(z_from)
        # under the arms the cloth stays closer to the ribs, or the arms would swing into it
        th = np.abs(prof.ths)[None, :]
        side = np.exp(-((th - 90.0) / 24.0) ** 2)
        t = np.clip((prof.zs[:, None] - (z_from - 0.18)) / 0.14, 0, 1)
        upper = t * t * (3 - 2 * t)
        keff = k * (1 - side_relief * side * upper)
        self.rmin = prof.R + keff * d + base

    def signed(self, pts):
        pr = self.prof
        yc = np.interp(pts[:, 2], pr.zs, pr.yc)
        rel = np.stack([pts[:, 0], pts[:, 1] - yc], 1)
        r = np.linalg.norm(rel, axis=1)
        u = rel / np.maximum(r, 1e-9)[:, None]
        rmin = pr.lookup(pts, self.rmin)
        nrm = np.stack([u[:, 0], u[:, 1], np.zeros(len(pts))], 1)
        loc = np.stack([u[:, 0] * rmin, yc + u[:, 1] * rmin, pts[:, 2]], 1)
        valid = (pts[:, 2] < self.z_from) & (pts[:, 2] > pr.zs[0])
        return r - rmin, loc, nrm, valid


# ------------------------------------------------------------------- relax

def relax_layers(V, F, fixed, constraints, hem_axis=None, iters=160, pull=0.02, smooth=0.5,
                 log=None, pinned=None):
    """Membrane solve against several constraints.

    ``constraints``: list of (surface, off_min[n], mask[n] or None). Surfaces have
    ``signed(pts) -> s, loc, nrm, valid``. Every vertex keeps ``s >= off_min`` against each
    constraint where valid; free vertices are gently pulled toward the tightest one.
    ``hem_axis``: (n, 3) unit axes; fixed vertices may not move along their axis (zero rows
    fall back to the original rule: hems keep their height). ``pinned`` vertices never move."""
    V = V.copy()
    n = len(V)
    E = edges_of(F)
    deg = np.bincount(E.ravel(), minlength=n).astype(np.float64)
    free = ~fixed
    if pinned is not None:
        free &= ~pinned
    cons = []
    for surf, off, mask in constraints:
        off = np.broadcast_to(np.asarray(off, np.float64), (n,)).copy()
        cons.append((surf, off, np.ones(n, bool) if mask is None else mask))
    ax = None if hem_axis is None else np.asarray(hem_axis, np.float64)
    for it in range(iters):
        before = V.copy()
        avg = np.zeros_like(V)
        np.add.at(avg, E[:, 0], V[E[:, 1]])
        np.add.at(avg, E[:, 1], V[E[:, 0]])
        avg /= np.maximum(deg, 1)[:, None]
        V[free] += smooth * (avg[free] - V[free])
        results = []
        best = np.full(n, np.inf)
        best_n = np.zeros_like(V)
        for surf, off, mask in cons:
            s = np.full(n, np.inf); loc = np.zeros_like(V); nrm = np.zeros_like(V); valid = np.zeros(n, bool)
            idx = np.where(mask)[0]
            if len(idx):
                s[idx], loc[idx], nrm[idx], valid[idx] = surf.signed(V[idx])
            results.append((s, loc, nrm, valid, off))
            sl = np.where(valid, s - off, np.inf)
            better = sl < best
            best[better] = sl[better]
            best_n[better] = nrm[better]
        far = free & np.isfinite(best) & (best > 0.0008)
        V[far] -= (pull * (best[far] - 0.0008))[:, None] * best_n[far]
        n_inside = 0
        for s, loc, nrm, valid, off in results:
            s2 = ((V - loc) * nrm).sum(1)
            inside = valid & (s2 < off)
            if pinned is not None:
                inside &= ~pinned
            n_inside += int(inside.sum())
            V[inside] = loc[inside] + nrm[inside] * off[inside][:, None]
        # fixed (hem) vertices: no motion along their hem axis
        m = fixed.copy() if pinned is None else fixed & ~pinned
        if m.any():
            d = V[m] - before[m]
            if ax is not None:
                a = ax[m]
                has = np.linalg.norm(a, axis=1) > 0.5
            else:
                has = np.zeros(m.sum(), bool)
                a = np.zeros_like(d)
            up = np.array([0.0, 0.0, 1.0])
            a = np.where(has[:, None], a, up)
            d = d - (d * a).sum(1)[:, None] * a
            V[m] = before[m] + d
        if log and (it % 40 == 0 or it == iters - 1):
            log(f"  relax {it}: constraint hits {n_inside}")
    gaps = []
    for surf, off, mask in cons:
        s = np.full(n, np.inf)
        idx = np.where(mask)[0]
        if len(idx):
            ss, _, _, valid = surf.signed(V[idx])
            s[idx] = np.where(valid, ss, np.inf)
        gaps.append(s)
    return V, gaps


def collide_layers(V, constraints, rounds=2):
    V = V.copy()
    for _ in range(rounds):
        for surf, off, mask in constraints:
            off = np.broadcast_to(np.asarray(off, np.float64), (len(V),))
            idx = np.where(mask)[0] if mask is not None else np.arange(len(V))
            s, loc, nrm, valid = surf.signed(V[idx])
            bad = valid & (s < off[idx])
            V[idx[bad]] = loc[bad] + nrm[bad] * off[idx[bad]][:, None]
    return V
