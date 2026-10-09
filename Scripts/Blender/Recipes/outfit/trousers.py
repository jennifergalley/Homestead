"""Homespun wool trousers: the shorts' hip section (same drawstring waist height, so the cord belt
still sits on the waistband) continued down each leg to the ankle bone, cast from the leg's bone
line (hip -> knee -> ankle) so the legs taper and stay close enough for boots below the knee."""
import math

import numpy as np

from .garments import Garment
from .tops import resample


def trousers_params(body):
    lm = body.lm
    ankle_z = float(body.bones["foot_l"][0][2])
    return dict(name="SKM_WoolTrousers", kind="trousers", pitch=0.0112,
                waist_front_z=lm["navel"][2] - 0.078, waist_back_rise=0.036,
                junction_z=lm["perineum"][2] + 0.045, knee_z=float(body.bones["calf_l"][0][2]),
                hem_z=ankle_z - 0.002, offset=0.0035)


def leg_axis(body, side, z):
    hip = body.bones[f"thigh_{side}"][0]
    knee = body.bones[f"calf_{side}"][0]
    ankle = body.bones[f"foot_{side}"][0]
    a, b = (hip, knee) if z >= knee[2] else (knee, ankle)
    t = (z - a[2]) / (b[2] - a[2])
    return a + (b - a) * t


def build_trousers(body, p):
    g = Garment(p["name"])
    g.extra["params"] = p
    pitch = p["pitch"]
    off = p["offset"]
    zj = p["junction_z"]
    hs = body.torso

    def z_w(t):
        b = (1 - math.cos(math.radians(t))) / 2
        return p["waist_front_z"] + p["waist_back_rise"] * b ** 1.3

    zr = zj + 0.03
    q1 = resample(body, zr, 0, 90, pitch, surf=hs, min_n=6)
    q2 = resample(body, zr, 90, 180, pitch, surf=hs, min_n=6)
    left = q1[:-1] + q2
    thetas = sorted(set(round(-180.0 if t >= 180 - 1e-9 else t, 9) for t in left + [-t for t in left[1:-1]]))
    N = len(thetas)
    j_cf = int(np.argmin(np.abs(np.array(thetas))))
    j_cb = 0
    j_l = int(np.argmin(np.abs(np.array(thetas) - 90)))
    j_r = int(np.argmin(np.abs(np.array(thetas) + 90)))
    tops = [z_w(t) for t in thetas]
    Rh = int(math.ceil((max(tops) - zj) / pitch))
    hip = np.zeros((Rh + 1, N), np.int64)
    for j, t in enumerate(thetas):
        prev = None
        for k in range(Rh + 1):
            z = zj + (tops[j] - zj) * k / Rh
            loc, nrm = body.cyl_hit(t, z, surf=hs)
            pt = loc + nrm * off if loc is not None else (prev if prev is not None else np.array([0, 0, z]))
            prev = pt
            hip[k, j] = g.add(pt)
    for k in range(Rh):
        for j in range(N):
            jn = (j + 1) % N
            front = abs(thetas[j]) < 90 and abs(thetas[jn]) <= 90 and thetas[j] >= -90
            panel = (0 if front else 2) + (0 if thetas[j] >= 0 else 1)
            g.quad(hip[k, j], hip[k, jn], hip[k + 1, jn], hip[k + 1, j], panel)
    Pb, Pf = g.V[hip[0, j_cb]], g.V[hip[0, j_cf]]
    zp = body.lm["perineum"][2] - off - 0.002
    us = np.linspace(0, 1, 200)
    path = np.array([[0.0, Pb[1] + (Pf[1] - Pb[1]) * u, zj - (zj - zp) * math.sin(math.pi * u) ** 0.55] for u in us])
    arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(path, axis=0), axis=1))])
    K = max(10, int(round(arc[-1] / pitch)))
    K += K % 2
    bridge = [g.add([np.interp(s, arc, path[:, i]) for i in range(3)]) for s in np.linspace(0, arc[-1], K + 1)[1:-1]]
    g.seam([hip[0, j_cb]] + bridge + [hip[0, j_cf]])
    g.feature("crotch", list(hip[::-1, j_cb]) + bridge + list(hip[:, j_cf]))
    g.seam(list(hip[:, j_cb]))
    g.seam(list(hip[:, j_cf]))
    g.extra["crotch_vertex"] = bridge[len(bridge) // 2]
    legs = {}
    for side, sgn, surf in (("l", 1, body.leg_l), ("r", -1, body.leg_r)):
        if side == "l":
            hip_part = [hip[0, j] for j in range(j_cf, N)] + [hip[0, j_cb]]
            side_idx = j_l - j_cf
        else:
            hip_part = [hip[0, j] for j in range(j_cf, -1, -1)]
            side_idx = j_cf - j_r
        ring0 = hip_part + bridge
        inseam_idx = len(hip_part) + len(bridge) // 2
        Q = len(ring0)
        zs = np.array([g.V[i][2] for i in ring0])
        T = int(math.ceil((zj - p["hem_z"]) / pitch)) + 1
        rows = [ring0]
        dirs = []
        for i in ring0:
            ax = leg_axis(body, side, g.V[i][2])
            d = g.V[i][:2] - ax[:2]
            dirs.append(d / max(np.linalg.norm(d), 1e-9))
        knee_row = None
        for r in range(1, T + 1):
            row = []
            for q, i in enumerate(ring0):
                d = dirs[q]
                z = zs[q] + (p["hem_z"] - zs[q]) * (r / T) ** 1.0
                ax = leg_axis(body, side, z)
                dd = np.array([d[0], d[1], 0.0])
                loc, nrm = surf.ray(np.array([ax[0], ax[1], z]) + dd * 0.3, -dd, dist=0.3)
                prev = g.V[rows[-1][q]]
                row.append(g.add(loc + nrm * off if loc is not None else np.array([prev[0], prev[1], z])))
            rows.append(row)
            if knee_row is None and g.V[row[side_idx]][2] < p["knee_z"]:
                knee_row = r
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
        legs[side] = dict(rows=rows, side_col=side_col, T=T, knee_row=knee_row)
    for side, j in (("l", j_l), ("r", j_r)):
        col = list(hip[:, j]) + legs[side]["side_col"][1:]
        g.seam(list(hip[:, j]))
        g.feature("side", col[::-1])
    g.extra.update(hip=hip, thetas=thetas, legs=legs, Rh=Rh, j_cf=j_cf, j_cb=j_cb)
    return g


def trouser_labels(g, V):
    zj = g.extra["params"]["junction_z"]
    lab = np.zeros(len(V), np.int64)
    lab[(V[:, 2] < zj - 0.004) & (V[:, 0] > 0.004)] = 1
    lab[(V[:, 2] < zj - 0.004) & (V[:, 0] < -0.004)] = 2
    return lab


def trouser_offsets(g, V, body):
    """Close at the waist and hips (the belt and pouch sit where the shorts are), a little ease
    over the thigh and knee, tapering to boot-tight below the knee."""
    p = g.extra["params"]
    per = np.array(body.lm["perineum"])
    d = np.linalg.norm(V - per, axis=1)
    off = p["offset"] + p.get("crotch_ease", 0.0060) * np.exp(-(d / 0.05) ** 2)
    z = V[:, 2]
    zj, zk = p["junction_z"], p["knee_z"]
    thigh = np.clip((zj - z) / 0.08, 0, 1)
    off += p.get("thigh_ease", 0.0010) * thigh                # 4.5 mm over the thigh by default
    knee = np.exp(-((z - zk) / 0.07) ** 2)
    off += 0.0012 * knee
    below = np.clip((zk - 0.06 - z) / 0.10, 0, 1)
    off -= 0.0006 * below                                     # ~4 mm on the calf and ankle
    return off


class LegDrape:
    """Radial floor about each leg's bone line: over the thigh the wool falls from the hip's
    widest girth instead of hugging the leg, fading out toward the knee (boot-close below)."""

    def __init__(self, body, p, side, k=0.5, surf=None):
        self.body, self.side, self.p = body, side, p
        z_top, z_knee = p["junction_z"] - 0.01, p["knee_z"]
        self.zs = np.arange(z_knee - 0.02, z_top + 1e-9, 0.005)
        self.phis = np.radians(np.arange(0, 360, 10.0))
        surf = surf or (body.leg_l if side == "l" else body.leg_r)
        R = np.zeros((len(self.zs), len(self.phis)))
        for i, z in enumerate(self.zs):
            ax = leg_axis(body, side, z)
            for j, ph in enumerate(self.phis):
                d = np.array([math.cos(ph), math.sin(ph), 0.0])
                loc, _ = surf.ray(np.array([ax[0], ax[1], z]) + d * 0.3, -d, dist=0.3)
                R[i, j] = np.hypot(loc[0] - ax[0], loc[1] - ax[1]) if loc is not None else np.nan
            row = R[i]
            ok = ~np.isnan(row)
            R[i] = np.interp(np.arange(len(row)), np.where(ok)[0], row[ok], period=len(row)) if ok.any() else 0.07
        env = R.copy()
        for i in range(len(self.zs) - 2, -1, -1):
            env[i] = np.maximum(env[i], env[i + 1])
        for _ in range(3):
            env = 0.25 * np.roll(env, 1, 1) + 0.5 * env + 0.25 * np.roll(env, -1, 1)
        fade = np.clip((self.zs - z_knee - 0.02) / 0.14, 0, 1)[:, None]
        self.rmin = R + k * fade * (env - R) + p["offset"]

    def signed(self, pts):
        n = len(pts)
        zi = np.clip((pts[:, 2] - self.zs[0]) / 0.005, 0, len(self.zs) - 1.001)
        axs = np.array([leg_axis(self.body, self.side, z) for z in pts[:, 2]])
        rel = pts[:, :2] - axs[:, :2]
        r = np.linalg.norm(rel, axis=1)
        u = rel / np.maximum(r, 1e-9)[:, None]
        ph = np.degrees(np.arctan2(u[:, 1], u[:, 0])) % 360
        tj = ph / 10.0
        i0 = zi.astype(int)
        j0 = tj.astype(int) % len(self.phis)
        j1 = (j0 + 1) % len(self.phis)
        a, b = zi - i0, tj - np.floor(tj)
        i1 = np.minimum(i0 + 1, len(self.zs) - 1)
        tab = self.rmin
        rmin = ((tab[i0, j0] * (1 - b) + tab[i0, j1] * b) * (1 - a) + (tab[i1, j0] * (1 - b) + tab[i1, j1] * b) * a)
        nrm = np.stack([u[:, 0], u[:, 1], np.zeros(n)], 1)
        loc = np.stack([axs[:, 0] + u[:, 0] * rmin, axs[:, 1] + u[:, 1] * rmin, pts[:, 2]], 1)
        valid = (pts[:, 2] > self.zs[0]) & (pts[:, 2] < self.zs[-1])
        return r - rmin, loc, nrm, valid


class Plane:
    """Half-space x * sign >= 0: keeps each trouser leg on its own side where the thighs touch."""

    def __init__(self, sign, margin=0.0):
        self.n = np.array([sign, 0.0, 0.0])
        self.margin = margin

    def signed(self, pts):
        s = pts @ self.n - self.margin
        loc = pts - s[:, None] * self.n
        return s, loc, np.broadcast_to(self.n, pts.shape).copy(), np.ones(len(pts), bool)
