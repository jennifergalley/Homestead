"""Seam/hem feature fields, low-frequency sculpted wrinkles and UV layout for the garments."""
import math

import bpy
import numpy as np
from mathutils import Vector, noise
from mathutils.kdtree import KDTree

STRIP = 0.055   # bottom band of UV space reserved for the drawstring cord and loose threads


def classify_loops(g, V, loops):
    """Name each boundary loop: hem/bind for the top, waist/leghem for the shorts."""
    named = []
    if "Tank" in g.name:
        hem_v = int(g.extra["grid"][0, 0])
        for lp in loops:
            named.append(("hem" if hem_v in lp else "bind", lp + lp[:1]))
    else:
        top = max(loops, key=lambda lp: V[lp, 2].mean())
        for lp in loops:
            named.append(("waist" if lp is top else "leghem", lp + lp[:1]))
    return named


def feature_fields(g, V, loops):
    """Per-vertex distance (m) to, and arc position (m) along, each seam/edge type."""
    polys = {}
    for name, lp in classify_loops(g, V, loops):
        polys.setdefault(name, []).append(lp)
    for name, lst in g.features.items():
        polys.setdefault(name, []).extend(lst)
    fields = {}
    for name, lst in polys.items():
        pts, arcs = [], []
        for poly in lst:
            P = V[poly]
            seg = np.linalg.norm(np.diff(P, axis=0), axis=1)
            arc = np.concatenate([[0], np.cumsum(seg)])
            n = max(2, int(arc[-1] / 0.001))
            s = np.linspace(0, arc[-1], n)
            pts.append(np.stack([np.interp(s, arc, P[:, i]) for i in range(3)], 1))
            arcs.append(s)
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


def _smooth(x, e0, e1):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def _noise1(a, seed):
    return np.array([noise.noise(Vector((float(v) * 1.0, seed * 7.31, seed * 1.7))) for v in a])


def wrinkles(g, V, fields, body, seed):
    """Low-frequency sculpted folds (meters, along the skin normal) that read in silhouette."""
    h = np.zeros(len(V))
    if "Tank" in g.name:
        d, a = fields["hem"]
        fall = (1 - _smooth(d, 0.0, 0.035))
        h += 0.0012 * fall * _noise1(a * 26, seed) + 0.0005 * fall * _noise1(a * 70, seed + 5)
        apex = np.array(body.lm["apex"])
        for sx in (1, -1):
            ap = apex * np.array([sx, 1, 1])
            side = np.array([sx * 0.135, ap[1] + 0.07, g.extra["params"]["ua_z"] - 0.01])
            dirv = side - ap
            L = np.linalg.norm(dirv); dirv /= L
            rel = V - ap
            t = rel @ dirv / L
            perp = rel - np.outer(rel @ dirv, dirv)
            q = np.linalg.norm(perp, axis=1) * np.sign(perp[:, 2] + 1e-9)
            mask = _smooth(t, 0.15, 0.35) * (1 - _smooth(t, 0.8, 1.0)) * (1 - _smooth(np.abs(q), 0.02, 0.045))
            h += 0.0012 * mask * np.sin(2 * np.pi * q / 0.021)
        back = _smooth(V[:, 1] - body.center_y(1.2), 0.0, 0.03)
        h += 0.0008 * back * (1 - _smooth(d, 0.01, 0.06)) * np.sin(2 * np.pi * (V[:, 2] - 1.2) / 0.024)
    else:
        d, a = fields["waist"]
        gather = (1 - _smooth(d, 0.028, 0.055))
        h += gather * (0.0019 * _noise1(a * 38, seed) + 0.0009 * _noise1(a * 95, seed + 1))
        cv = V[g.extra["crotch_vertex"]]
        rel = V - cv
        r = np.hypot(rel[:, 0], rel[:, 2])
        psi = np.arctan2(rel[:, 2], np.abs(rel[:, 0]))
        front = _smooth(-(V[:, 1] - cv[1]), 0.0, 0.02)
        whisk = _smooth(r, 0.025, 0.045) * (1 - _smooth(r, 0.08, 0.13)) * front * (rel[:, 2] > -0.01)
        h += 0.0013 * whisk * np.sin(psi * 9.0 + 0.6)
        backm = _smooth(V[:, 1] - cv[1], 0.02, 0.05)
        zj = g.extra["params"]["junction_z"]
        band = _smooth(V[:, 2], zj - 0.035, zj - 0.015) * (1 - _smooth(V[:, 2], zj + 0.01, zj + 0.04))
        h += 0.0011 * backm * band * np.sin(2 * np.pi * (V[:, 2] + 0.25 * (np.abs(V[:, 0]) - 0.09) ** 2 * 10) / 0.02)
        dl, al = fields["leghem"]
        h += 0.0012 * (1 - _smooth(dl, 0.0, 0.035)) * _noise1(al * 24, seed + 2)
    return h


def collide(V, surf, off_min):
    s, loc, nrm = surf.signed(V)
    m = s < off_min
    V = V.copy()
    V[m] = loc[m] + nrm[m] * np.broadcast_to(off_min, (len(V),))[m][:, None]
    return V, surf.signed(V)[0]


# ------------------------------------------------------------------------- UVs

def _loop_uvs(me):
    uv = np.empty(len(me.loops) * 2, np.float64)
    me.uv_layers.active.data.foreach_get("uv", uv)
    return uv.reshape(-1, 2)


def unwrap(obj):
    """Angle-based unwrap split at the sewing seams, each pattern piece rotated so the warp
    (straight grain, pointing up the body) runs along +V, uniform texel density, packed above
    the reserved accessory strip."""
    me = obj.data
    if not me.uv_layers:
        me.uv_layers.new(name="UVMap")
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.unwrap(method="ANGLE_BASED", margin=0.004)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = _loop_uvs(me)
    co = np.array([v.co for v in me.vertices])
    panel = np.empty(len(me.polygons), np.int32)
    me.attributes["panel"].data.foreach_get("value", panel)
    loop_face = np.empty(len(me.loops), np.int64)
    for p in me.polygons:
        loop_face[p.loop_start:p.loop_start + p.loop_total] = p.index
    loop_vert = np.empty(len(me.loops), np.int64)
    me.loops.foreach_get("vertex_index", loop_vert)
    for pid in np.unique(panel):
        fl = np.where(panel[loop_face] == pid)[0]
        # gradient of height (z) in UV space, accumulated over the piece's faces
        gsum = np.zeros(2)
        area3, area2 = 0.0, 0.0
        for p in me.polygons:
            if panel[p.index] != pid:
                continue
            ls = list(range(p.loop_start, p.loop_start + p.loop_total))
            for k in range(1, len(ls) - 1):
                a, b, c = ls[0], ls[k], ls[k + 1]
                u0, u1, u2 = uv[a], uv[b], uv[c]
                z0, z1, z2 = co[loop_vert[[a, b, c]], 2]
                M = np.array([u1 - u0, u2 - u0])
                det = np.linalg.det(M)
                if abs(det) < 1e-14:
                    continue
                grad = np.linalg.solve(M, np.array([z1 - z0, z2 - z0]))
                w = abs(det)
                gsum += grad * w
                area2 += abs(det) / 2
                P = co[loop_vert[[a, b, c]]]
                area3 += np.linalg.norm(np.cross(P[1] - P[0], P[2] - P[0])) / 2
        ang = math.atan2(gsum[1], gsum[0])
        rot = math.pi / 2 - ang
        R = np.array([[math.cos(rot), -math.sin(rot)], [math.sin(rot), math.cos(rot)]])
        c0 = uv[fl].mean(0)
        scale = math.sqrt(area3 / max(area2, 1e-12))   # -> UV units == meters
        uv[fl] = (uv[fl] - c0) @ R.T * scale
    me.uv_layers.active.data.foreach_set("uv", uv.ravel())
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=False, scale=True, margin=0.006)
    bpy.ops.object.mode_set(mode="OBJECT")
    uv = _loop_uvs(me)
    uv = uv * (1 - STRIP) + np.array([0.0, STRIP])
    me.uv_layers.active.data.foreach_set("uv", uv.ravel())
    # texel density: UV units per meter
    tri_area3 = sum(p.area for p in me.polygons)
    uva = 0.0
    for p in me.polygons:
        ls = uv[p.loop_start:p.loop_start + p.loop_total]
        x, y = ls[:, 0], ls[:, 1]
        uva += 0.5 * abs(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1)))
    return math.sqrt(uva / tri_area3)
