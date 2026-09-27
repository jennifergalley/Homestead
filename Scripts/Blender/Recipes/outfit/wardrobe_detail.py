"""Edge/seam fields and sculpted folds for the wardrobe garments (tee, long shirt, trousers, coat)."""
import math

import numpy as np
from mathutils import Vector, noise
from mathutils.kdtree import KDTree

from .garments import boundary_loops


def _smooth(x, e0, e1):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def _n1(a, seed, scale=1.0):
    return np.array([noise.noise(Vector((float(v) * scale, seed * 7.31, seed * 1.7))) for v in a])


def _n3(P, seed, scale=1.0):
    return np.array([noise.noise(Vector((p[0] * scale + seed * 3.1, p[1] * scale - seed * 1.3, p[2] * scale)))
                     for p in P])


# ------------------------------------------------------------------ edges

def edge_polylines(g, V, F):
    """Name the open edges. Tops: hem, cuff, neck (tee/long shirt, slit included) or, for the
    coat's single outline loop, hem + opening + collartop runs. Trousers: waist, leghem."""
    loops = boundary_loops(F, len(V))
    out = {}
    kind = g.extra["params"]["kind"]
    if kind == "trousers":
        top = max(loops, key=lambda lp: V[lp, 2].mean())
        for lp in loops:
            out.setdefault("waist" if lp is top else "leghem", []).append(lp + lp[:1])
        return out, loops
    cat = {}
    grid = g.extra["grid"]
    for v in grid[0]:
        cat[int(v)] = "hem"
    for sl in g.extra["sleeves"].values():
        for v in sl["rows"][-1]:
            cat[int(v)] = "cuff"
    if "collar" in g.extra:
        for v in g.extra["collar"]["rows"][-1]:
            cat[int(v)] = "collartop"
    else:
        for v in g.extra["neck_loop"]:
            cat[int(v)] = "neck"
    if "opening" in g.extra:
        le, ri = g.extra["opening"]
        for v in list(le) + list(ri):
            cat[int(v)] = "opening"
    for lp in loops:
        labels = [cat.get(v, "neck") for v in lp]
        if len(set(labels)) == 1:
            out.setdefault(labels[0], []).append(lp + lp[:1])
            continue
        start = next(i for i in range(len(lp)) if labels[i] != labels[i - 1])
        lp = lp[start:] + lp[:start]
        labels = labels[start:] + labels[:start]
        run = [lp[0]]
        for i in range(1, len(lp) + 1):
            v = lp[i % len(lp)]
            lab = labels[i % len(lp)]
            if i == len(lp) or lab != labels[i - 1]:
                run.append(v)
                out.setdefault(labels[i - 1], []).append(run)
                run = [v]
            else:
                run.append(v)
    return out, loops


def fields_for(V, polys):
    """Per-vertex distance (m) to, and arc position (m) along, each named set of polylines."""
    fields = {}
    for name, lst in polys.items():
        pts, arcs = [], []
        for poly in lst:
            if len(poly) < 2:
                continue
            P = V[poly]
            seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
            arc = np.concatenate([[0], np.cumsum(seg)])
            n = max(2, int(arc[-1] / 0.001))
            s = np.linspace(0, arc[-1], n)
            pts.append(np.stack([np.interp(s, arc, P[:, i]) for i in range(3)], 1))
            arcs.append(s)
        if not pts:
            continue
        pts, arcs = np.vstack(pts), np.concatenate(arcs)
        kd = KDTree(len(pts))
        for i, p in enumerate(pts):
            kd.insert(Vector(p), i)
        kd.balance()
        dist = np.empty(len(V)); along = np.empty(len(V))
        for i, v in enumerate(V):
            _, j, d = kd.find(Vector(v))
            dist[i], along[i] = d, arcs[j]
        fields[name] = (dist, along)
    return fields


# --------------------------------------------------------------- wrinkles

def wrinkles(g, V, fields, body, seed):
    kind = g.extra["params"]["kind"]
    if kind == "trousers":
        return _trouser_folds(g, V, fields, body, seed)
    return _top_folds(g, V, fields, body, seed)


def _top_folds(g, V, fields, body, seed):
    p = g.extra["params"]
    kind = p["kind"]
    reg = g.extra["region"]
    ax = g.extra["axial"]
    h = np.zeros(len(V))
    torso = reg == 0
    amp = {"tee": 0.0024, "longshirt": 0.0036, "coat": 0.0022}[kind]
    # vertical drape folds falling from the bust and shoulder blades
    yc = np.array([body.center_y(z) for z in np.clip(V[:, 2], 0.9, 1.6)])
    th = np.arctan2(V[:, 0], -(V[:, 1] - yc))
    apex_z = body.lm["apex"][2]
    fall = _smooth(apex_z - 0.02 - V[:, 2], 0.0, 0.18) * torso
    warp = 0.35 * _n3(V, seed + 11, 9.0)
    folds = _n1(th * (7.5 if kind != "coat" else 4.5) + warp, seed + 3) * 0.8 + 0.2 * _n1(th * 17 + warp, seed + 4)
    h += amp * fall * folds
    if "hem" in fields:
        d, a = fields["hem"]
        near = 1 - _smooth(d, 0.0, 0.05)
        h += 0.0009 * near * _n1(a * 16, seed + 5)
    # sleeves
    sl = (reg == 1) | (reg == 2)
    if sl.any():
        for side, r in (("l", 1), ("r", 2)):
            m = reg == r
            if not m.any():
                continue
            S = g.extra["sleeves"][side]
            arm = S["axis"]
            P = V[m]
            s = ax[m]
            ang = np.array([arm.coords(q)[1] for q in P])
            if kind == "tee":
                # loose diagonal drag folds from the armpit across the sleeve underside
                under = _smooth(np.cos(ang - math.pi), -0.2, 0.8)
                h[m] += 0.0016 * under * np.sin(ang * 3.0 + s * 55.0 + _n1(s * 20, seed + 7))
            else:
                L1 = arm.L1
                s_end = S["s_end"]
                # loose sleeve body: long soft folds along the arm, crowding into the cuff
                body_f = _smooth(s, 0.06, 0.12) * (1 - _smooth(s, s_end - (p.get("cuff") or 0.03) - 0.01, s_end - (p.get("cuff") or 0.03)))
                twist = ang * (5.0 if kind == "longshirt" else 3.0) + s * 9.0
                h[m] += (0.0030 if kind == "longshirt" else 0.0020) * body_f * _n1(twist + 0.6 * _n3(P, seed + 12, 7.0), seed + 8)
                # elbow: rings of compression folds on the inner elbow
                elbow = np.exp(-((s - L1) / 0.045) ** 2)
                h[m] += (0.0022 if kind == "longshirt" else 0.0026) * elbow * np.sin((s - L1) / 0.012 * math.pi + _n1(ang * 2, seed + 9))
                if kind == "longshirt" and p.get("cuff"):
                    # gathers into the cuff band: tight radial pleats that open up the sleeve
                    c0 = s_end - p["cuff"]
                    g_f = _smooth(s, c0 - 0.07, c0 - 0.005) * (1 - _smooth(s, c0 - 0.002, c0 + 0.004))
                    h[m] += 0.0032 * g_f * np.sin(ang * 13.0 + 1.3 * _n1(ang * 3, seed + 10))
            if "cuff" in fields:
                d, a = fields["cuff"]
                near = (1 - _smooth(d, 0.0, 0.03)) * m
                h += 0.0007 * near * _n1(a * 30, seed + 13)
    # the laced slit neck of the long shirt is gathered a little into its facing
    if kind == "longshirt" and "neck" in fields:
        d, a = fields["neck"]
        near = 1 - _smooth(d, 0.004, 0.05)
        h += 0.0012 * near * np.sin(a * 2 * math.pi / 0.018 + _n1(a * 20, seed + 14))
    if kind == "coat":
        # stiff leather: broad low bulges, soft folds at the small of the back and waist sides
        h += 0.0018 * _n3(V, seed + 15, 11.0) * torso
        back = _smooth(V[:, 1] - yc, 0.02, 0.07) * torso
        h += 0.0020 * back * _smooth(V[:, 2], 1.06, 1.12) * (1 - _smooth(V[:, 2], 1.16, 1.22)) * \
            np.sin((V[:, 2] - 1.1) / 0.022 * math.pi + _n1(th * 3, seed + 16))
    return h


def _trouser_folds(g, V, fields, body, seed):
    p = g.extra["params"]
    h = np.zeros(len(V))
    d, a = fields["waist"]
    gather = 1 - _smooth(d, 0.028, 0.06)
    h += gather * (0.0021 * _n1(a * 36, seed) + 0.0010 * _n1(a * 90, seed + 1))
    cv = V[g.extra["crotch_vertex"]]
    rel = V - cv
    r = np.hypot(rel[:, 0], rel[:, 2])
    psi = np.arctan2(rel[:, 2], np.abs(rel[:, 0]))
    front = _smooth(-(V[:, 1] - cv[1]), 0.0, 0.02)
    whisk = _smooth(r, 0.025, 0.045) * (1 - _smooth(r, 0.09, 0.15)) * front * (rel[:, 2] > -0.02)
    h += 0.0015 * whisk * np.sin(psi * 8.0 + 0.6)
    z = V[:, 2]
    zk = p["knee_z"]
    lab_front = np.zeros(len(V))
    for side in ("l", "r"):
        knee = body.bones[f"calf_{side}"][0]
        m = (V[:, 0] > 0) if side == "l" else (V[:, 0] < 0)
        lab_front[m] = _smooth(-(V[m, 1] - knee[1]), -0.01, 0.03)
    # knee: soft horizontal folds where the wool has bagged over the kneecap and bunches behind
    kz = np.exp(-((z - zk - 0.01) / 0.06) ** 2)
    h += 0.0020 * kz * np.sin((z - zk) / 0.016 * math.pi + 1.8 * _n3(V, seed + 2, 16.0))
    # the back of the knee creases more tightly
    back = 1 - lab_front
    h += 0.0012 * back * np.exp(-((z - zk + 0.01) / 0.035) ** 2) * np.sin((z - zk) / 0.009 * math.pi + _n1(V[:, 0] * 60, seed + 3))
    # ankle stacking above the hem (boots push the cloth up)
    dl, al = fields["leghem"]
    stack = (1 - _smooth(dl, 0.03, 0.14))
    h += 0.0015 * stack * np.sin(dl / 0.019 * 2 * math.pi + 2.0 * _n1(al * 18, seed + 4))
    h += 0.0008 * (1 - _smooth(dl, 0.0, 0.03)) * _n1(al * 30, seed + 5)
    # a few long drag lines down the thigh from the hip
    thigh = _smooth(p["junction_z"] - z, 0.02, 0.1) * (1 - _smooth(p["junction_z"] - z, 0.22, 0.34))
    h += 0.0010 * thigh * _n3(V * np.array([1.0, 1.0, 0.25]), seed + 6, 40.0)
    return h
