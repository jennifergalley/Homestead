"""Mesh assembly for footwear: parametric grids, swept thongs and cords, surface-hugging paths,
UVs in meters, per-vertex attributes for texture synthesis, and Blender mesh creation."""
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector


def _norm(v):
    return v / np.maximum(np.linalg.norm(v, axis=-1, keepdims=True), 1e-12)


def grid_uv(P, cyclic=True, center_col=None, closed_rows=False):
    """Arc-length UVs (meters) for a grid (R, C, 3): U around each row (centred on
    ``center_col``), V down each column. Cyclic grids get C+1 UV columns (seam duplicate)."""
    Pc = np.concatenate([P, P[:, :1]], 1) if cyclic else P
    du = np.linalg.norm(np.diff(Pc, axis=1), axis=2)
    U = np.concatenate([np.zeros((len(P), 1)), np.cumsum(du, 1)], 1)
    if center_col is not None:
        U = U - U[:, center_col:center_col + 1]
    else:
        U = U - 0.5 * U[:, -1:]
    dv = np.linalg.norm(np.diff(Pc, axis=0), axis=2)
    V = np.concatenate([np.zeros((1, Pc.shape[1])), np.cumsum(dv, 0)], 0)
    return np.stack([U, V], -1)


class MeshBuilder:
    """Accumulates vertices, faces (with per-corner UVs and a part id) and per-vertex float
    attributes; ``build`` turns them into one Blender mesh object."""

    def __init__(self):
        self.V = []
        self.n = 0
        self.faces, self.fuv, self.fpart = [], [], []
        self.attrs = {}

    def _add_verts(self, P, attrs):
        P = np.asarray(P, float).reshape(-1, 3)
        idx = np.arange(self.n, self.n + len(P))
        self.V.append(P)
        for k in set(self.attrs) | set(attrs or {}):
            if k not in self.attrs:
                self.attrs[k] = [np.zeros(self.n)]
            val = (attrs or {}).get(k)
            self.attrs[k].append(np.zeros(len(P)) if val is None else np.asarray(val, float).reshape(-1))
        self.n += len(P)
        return idx

    def add_grid(self, P, uv, part, cyclic=True, collapse_first=False, collapse_last=False, flip=False,
                 attrs=None):
        """P: (R, C, 3); uv: (R, C+1, 2) if cyclic else (R, C, 2). A collapsed first/last row is
        stored as one vertex (a pole) and closed with triangles."""
        R, C = P.shape[:2]
        flat = {k: np.asarray(v, float).reshape(R, C) for k, v in (attrs or {}).items()}
        idx = np.empty((R, C), np.int64)
        rows = list(range(R))
        for i in rows:
            pole = (collapse_first and i == 0) or (collapse_last and i == R - 1)
            if pole:
                idx[i] = self._add_verts(P[i].mean(0)[None], {k: v[i].mean()[None] for k, v in flat.items()})[0]
            else:
                idx[i] = self._add_verts(P[i], {k: v[i] for k, v in flat.items()})
        ncol = C if cyclic else C - 1
        for i in range(R - 1):
            for j in range(ncol):
                jn = (j + 1) % C
                a, b, c, d = idx[i, j], idx[i, jn], idx[i + 1, jn], idx[i + 1, j]
                ua, ub, uc, ud = uv[i, j], uv[i, j + 1], uv[i + 1, j + 1], uv[i + 1, j]
                if collapse_first and i == 0:
                    f, fu = [a, c, d], [ua, uc, ud]
                elif collapse_last and i == R - 2:
                    f, fu = [a, b, c], [ua, ub, uc]
                else:
                    f, fu = [a, b, c, d], [ua, ub, uc, ud]
                if flip:
                    f, fu = f[::-1], fu[::-1]
                self.faces.append(f)
                self.fuv.append(fu)
                self.fpart.append(part)
        return idx

    def vertices(self):
        return np.vstack(self.V)

    def mirror_x(self):
        """Append a mirror image (x -> -x) of everything built so far: the right foot from the
        left. Winding is reversed so normals stay outward; UVs are shared, so both feet use the
        same texels (mirrored UVs are fine for tangent-space normals)."""
        V = self.vertices()
        n = len(V)
        self.V.append(V * np.array([-1.0, 1.0, 1.0]))
        for k in self.attrs:
            self.attrs[k].append(np.concatenate(self.attrs[k]))
        nf = len(self.faces)
        for i in range(nf):
            self.faces.append([v + n for v in self.faces[i][::-1]])
            self.fuv.append(self.fuv[i][::-1])
            self.fpart.append(self.fpart[i])
        self.n += n
        return n

    def attr(self, name):
        return np.concatenate(self.attrs[name])

    def build(self, name, weld=2e-5):
        V = self.vertices()
        me = bpy.data.meshes.new(name)
        me.from_pydata(V.tolist(), [], [list(map(int, f)) for f in self.faces])
        me.update()
        uvl = me.uv_layers.new(name="UVMap")
        flat = np.array([u for fu in self.fuv for u in fu], float)
        uvl.data.foreach_set("uv", flat.ravel())
        part = me.attributes.new("part", "INT", "FACE")
        part.data.foreach_set("value", np.array(self.fpart, np.int32))
        for k in self.attrs:
            a = me.attributes.new(k, "FLOAT", "POINT")
            a.data.foreach_set("value", self.attr(k).astype(np.float32))
        obj = bpy.data.objects.new(name, me)
        bpy.context.scene.collection.objects.link(obj)
        if weld:
            bm = bmesh.new()
            bm.from_mesh(me)
            bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=weld)
            bm.to_mesh(me)
            bm.free()
        me.shade_smooth()
        return obj


# ------------------------------------------------------------------- sweeps

def ellipse_profile(k, width, thick):
    a = np.arange(k) * 2 * math.pi / k
    return np.stack([0.5 * width * np.cos(a), 0.5 * thick * np.sin(a)], 1)


def sweep(path, up, profile, scale=None):
    """Sweep a closed 2D profile (k,2) = (across, along ``up``) along ``path`` (n,3). ``up`` gives
    the profile's second axis per sample (a surface normal for strips lying on a surface).
    ``scale`` (n,) or (n,2) tapers the profile. Returns the ring grid (n, k, 3)."""
    path = np.asarray(path, float)
    t = np.gradient(path, axis=0)
    t = _norm(t)
    u = np.asarray(up, float)
    u = _norm(u - (u * t).sum(1, keepdims=True) * t)
    b = np.cross(t, u)
    sc = np.ones((len(path), 2)) if scale is None else np.asarray(scale, float).reshape(len(path), -1) * np.ones((1, 2))
    return (path[:, None, :] + (profile[None, :, 0:1] * sc[:, None, 0:1]) * b[:, None, :]
            + (profile[None, :, 1:2] * sc[:, None, 1:2]) * u[:, None, :])


def add_tube(mb, path, up, profile, part, scale=None, caps=True, attrs=None, uv_scale=1.0, flip=True,
             chunk=0.12):
    """Swept tube with pole caps; UVs: U around (profile arc, meters), V along the path.
    Long tubes are split into ~``chunk`` m UV islands (rings shared and welded) so they pack well.
    (The sweep's ring order winds inward, so faces are flipped by default.)"""
    L = arclen(np.asarray(path))
    if chunk and L > 1.5 * chunk and not isinstance(up, tuple):
        rings_full = sweep(path, up, profile, scale)
        n = int(round(L / chunk))
        cuts = np.linspace(0, len(path) - 1, n + 1).astype(int)
        out = None
        for k in range(n):
            a, b = cuts[k], cuts[k + 1] + 1
            sub = {kk: (np.asarray(v)[a:b] if np.asarray(v).size == len(path) else v) for kk, v in (attrs or {}).items()}
            out = add_tube(mb, path[a:b], ("rings", rings_full[a:b]), profile, part,
                           caps=(caps and k == 0, caps and k == n - 1), attrs=sub, uv_scale=uv_scale, flip=flip,
                           chunk=0)
        return out
    cap0, cap1 = caps if isinstance(caps, tuple) else (caps, caps)
    rings = up[1] if isinstance(up, tuple) else sweep(path, up, profile, scale)
    parts = [rings]
    if cap0:
        parts.insert(0, path[:1, None, :].repeat(rings.shape[1], 1))
    if cap1:
        parts.append(path[-1:, None, :].repeat(rings.shape[1], 1))
    rings = np.concatenate(parts, 0)
    uv = grid_uv(rings, cyclic=True) * uv_scale
    at = {"per": np.full(rings.shape[:2], arclen(np.vstack([profile, profile[:1]]) @ np.eye(2, 3)))}
    for k, v in (attrs or {}).items():
        v = np.asarray(v, float)
        if v.size == 1:
            v = np.full(len(path), v.item())
        if v.ndim == 1:
            v = np.concatenate(([v[:1]] if cap0 else []) + [v] + ([v[-1:]] if cap1 else []))
            v = v[:, None].repeat(rings.shape[1], 1)
        at[k] = v
    return mb.add_grid(rings, uv, part, cyclic=True, collapse_first=cap0, collapse_last=cap1, attrs=at,
                       flip=flip), rings


def catmull(points, n):
    """Centripetal-ish Catmull-Rom through points, resampled to n points by arc length."""
    P = np.asarray(points, float)
    P = np.vstack([2 * P[0] - P[1], P, 2 * P[-1] - P[-2]])
    out = []
    for i in range(1, len(P) - 2):
        p0, p1, p2, p3 = P[i - 1], P[i], P[i + 1], P[i + 2]
        for t in np.linspace(0, 1, 24, endpoint=False):
            t2, t3 = t * t, t * t * t
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2
                              + (-p0 + 3 * p1 - 3 * p2 + p3) * t3))
    out.append(P[-2])
    return resample(np.array(out), n)


def resample(P, n):
    d = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])
    s = np.linspace(0, d[-1], n)
    return np.stack([np.interp(s, d, P[:, i]) for i in range(3)], 1)


def arclen(P):
    return float(np.linalg.norm(np.diff(P, axis=0), axis=1).sum())


def hug(surface, waypoints, offset, step=0.003, iters=4, smooth=3, lift=None, taut=12):
    """A path through ``waypoints`` resting ``offset`` (scalar or fn(t)) outside ``surface``
    (outfit.body.Surface), like a cord pulled taut: it is smoothed (tension) and pushed out
    wherever it sinks below the offset, so it bridges hollows instead of following every dip.
    The end points stay put. Returns (points, normals)."""
    P = catmull(waypoints, max(8, int(arclen(np.asarray(waypoints)) / step) + 1))
    t = np.linspace(0, 1, len(P))
    off = offset(t) if callable(offset) else np.full(len(P), float(offset))
    for _ in range(iters):
        _, loc, nrm = surface.signed(P)
        P = loc + nrm * off[:, None]
        for _ in range(smooth):
            P[1:-1] = P[1:-1] + 0.5 * (0.5 * (P[:-2] + P[2:]) - P[1:-1])
    for _ in range(taut):
        P[1:-1] = P[1:-1] + 0.5 * (0.5 * (P[:-2] + P[2:]) - P[1:-1])
        d, loc, nrm = surface.signed(P)
        push = np.clip(off - d, 0, None)
        P = P + nrm * push[:, None]
    _, _, nrm = surface.signed(P)
    if lift is not None:
        P = P + nrm * lift(t)[:, None]
    return P, nrm


def surface_from(obj_or_arrays):
    from outfit.body import Surface, mesh_arrays
    if isinstance(obj_or_arrays, tuple):
        return Surface(*obj_or_arrays)
    return Surface(*mesh_arrays(obj_or_arrays, evaluated=False))


def pack_uvs(obj, margin=0.004):
    """Pack the (meter-scaled, density-weighted) islands into 0-1, keeping relative scale."""
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin=margin, shape_method="CONCAVE")
    bpy.ops.object.mode_set(mode="OBJECT")
