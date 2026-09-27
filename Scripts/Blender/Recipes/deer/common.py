"""Geometry helpers shared by the deer modules."""
import math

import bpy
import numpy as np
from mathutils import Matrix

import homestead_sdf as sdf

ZUP = np.array([0.0, 0.0, 1.0])


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v)


def catmull_rom(points, samples=400):
    """Dense samples along a centripetal Catmull-Rom spline through ``points``."""
    pts = [np.asarray(p, dtype=np.float64) for p in points]
    pts = [2 * pts[0] - pts[1]] + pts + [2 * pts[-1] - pts[-2]]
    out = []
    per = max(2, samples // (len(pts) - 3))
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = pts[i - 1:i + 3]
        t0 = 0.0
        t1 = t0 + np.linalg.norm(p1 - p0) ** 0.5
        t2 = t1 + np.linalg.norm(p2 - p1) ** 0.5
        t3 = t2 + np.linalg.norm(p3 - p2) ** 0.5
        for t in np.linspace(t1, t2, per, endpoint=i == len(pts) - 3):
            a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1
            a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2
            a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3
            b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2
            b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3
            out.append((t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2)
    return np.array(out)


class Curve:
    """Arclength-parametrised polyline (with extrapolation past both ends)."""

    def __init__(self, points):
        self.p = np.asarray(points, dtype=np.float64)
        seg = np.linalg.norm(np.diff(self.p, axis=0), axis=1)
        self.s = np.concatenate([[0.0], np.cumsum(seg)])
        self.length = float(self.s[-1])

    def at(self, s):
        if s <= 0:
            return self.p[0] + self.tangent(0.0) * s
        if s >= self.length:
            return self.p[-1] + self.tangent(self.length) * (s - self.length)
        i = int(np.searchsorted(self.s, s) - 1)
        f = (s - self.s[i]) / max(self.s[i + 1] - self.s[i], 1e-12)
        return self.p[i] * (1 - f) + self.p[i + 1] * f

    def tangent(self, s):
        i = int(np.clip(np.searchsorted(self.s, s) - 1, 0, len(self.p) - 2))
        return unit(self.p[i + 1] - self.p[i])

    def closest(self, q):
        """(arclength, distance, point) of the closest curve point to ``q`` in XY."""
        a, b = self.p[:-1, :2], self.p[1:, :2]
        ab = b - a
        t = np.clip(np.einsum("ij,ij->i", q[:2] - a, ab) / np.maximum(np.einsum("ij,ij->i", ab, ab), 1e-12), 0, 1)
        c = a + ab * t[:, None]
        d = np.linalg.norm(c - q[:2], axis=1)
        i = int(np.argmin(d))
        return self.s[i] + t[i] * np.linalg.norm(ab[i]), d[i], c[i]


def matrix(rotation, translation):
    m = Matrix.Identity(4)
    for r in range(3):
        for c in range(3):
            m[r][c] = float(rotation[r][c])
        m[r][3] = float(translation[r])
    return m


def place(obj, rotation, translation, tag=True):
    """Store the authored (part-local) coordinates as ``pcoord`` and move the mesh into
    the world with ``rotation`` (columns = local axes) and ``translation``."""
    import homestead_kit as kit
    if tag:
        kit.tag_coords(obj.data)
    obj.data.transform(matrix(rotation, translation))
    obj.data.update()
    return obj


def sdf_part(name, node, voxel, tris, material, rotation=np.eye(3), translation=(0, 0, 0)):
    obj = sdf.mesh(name, node, voxel, target_tris=tris, material=material)
    return place(obj, rotation, translation)


def sweep(kit, name, points, half_w, half_h, width_hint, material, sides=8, power=0.7):
    """Loft a flattened (super-elliptic) section along ``points``: ``half_w(t)`` across
    ``width_hint`` (projected perpendicular to the path), ``half_h(t)`` across the other
    axis; t is the 0..1 arclength fraction. pcoord = (across w, across h, arclength)."""
    pts = [np.asarray(p, dtype=np.float64) for p in points]
    lengths = [0.0]
    for a, b in zip(pts, pts[1:]):
        lengths.append(lengths[-1] + np.linalg.norm(b - a))
    total = lengths[-1]
    rows, coords = [], []
    for i, p in enumerate(pts):
        t = lengths[i] / total
        tangent = unit(pts[min(i + 1, len(pts) - 1)] - pts[max(i - 1, 0)])
        hint = np.asarray(width_hint(t) if callable(width_hint) else width_hint, dtype=np.float64)
        w = unit(hint - tangent * tangent.dot(hint))
        h = np.cross(tangent, w)
        ring, pco = [], []
        for j in range(sides):
            a = 2 * math.pi * j / sides
            ca, sa = math.cos(a), math.sin(a)
            cx = math.copysign(abs(ca) ** power, ca) * half_w(t)
            cy = math.copysign(abs(sa) ** power, sa) * half_h(t)
            ring.append(tuple(p + w * cx + h * cy))
            pco.append((cx, cy, lengths[i]))
        rows.append(ring)
        coords.append(pco)
    return kit.loft(name, rows, material=material, coords=coords, cap_start=True, cap_end=True)


def copy_object(obj, name):
    new = obj.copy()
    new.data = obj.data.copy()
    new.name = new.data.name = name
    bpy.context.scene.collection.objects.link(new)
    return new


def vertices(obj):
    co = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get("co", co)
    return co.reshape(-1, 3)


def heights(objects, lo, cell, shape):
    """Raster of the highest vertex z per XY cell (-inf where empty)."""
    grid = np.full(shape, -np.inf)
    for obj in objects:
        p = vertices(obj)
        ij = np.floor((p[:, :2] - lo) / cell).astype(int)
        ok = (ij[:, 0] >= 0) & (ij[:, 1] >= 0) & (ij[:, 0] < shape[0]) & (ij[:, 1] < shape[1])
        np.maximum.at(grid, (ij[ok, 0], ij[ok, 1]), p[ok, 2])
    return grid


def dilate_max(grid, radius):
    """Max filter over a (2r+1)^2 window."""
    out = grid.copy()
    for di in range(-radius, radius + 1):
        for dj in range(-radius, radius + 1):
            shifted = np.full_like(grid, -np.inf)
            si = slice(max(di, 0), grid.shape[0] + min(di, 0))
            sj = slice(max(dj, 0), grid.shape[1] + min(dj, 0))
            ti = slice(max(-di, 0), grid.shape[0] + min(-di, 0))
            tj = slice(max(-dj, 0), grid.shape[1] + min(-dj, 0))
            shifted[ti, tj] = grid[si, sj]
            out = np.maximum(out, shifted)
    return out


def shift(objects, delta):
    m = Matrix.Translation(tuple(float(c) for c in delta))
    for obj in objects:
        obj.data.transform(m)
        obj.data.update()


def settle(objects, clearance=0.001):
    """Drop (or lift) ``objects`` together so their lowest point sits at ``clearance``."""
    low = min(vertices(o)[:, 2].min() for o in objects)
    shift(objects, (0.0, 0.0, clearance - low))
    return clearance - low


def lay_flat(objects, at, heading, clearance=0.0008, flip=False):
    """Lay ``objects`` down on their broadest face (principal axes: longest along the
    heading, thinnest vertical), centred at ``at`` (XY) and resting on the ground."""
    v = np.concatenate([vertices(o) for o in objects])
    c = v.mean(axis=0)
    _, _, vt = np.linalg.svd(v - c, full_matrices=False)
    if np.linalg.det(vt) < 0:
        vt[2] *= -1
    if flip:
        vt[1] *= -1
        vt[2] *= -1
    a = math.radians(heading)
    rz = np.array([[math.cos(a), -math.sin(a), 0], [math.sin(a), math.cos(a), 0], [0, 0, 1]])
    rot = rz @ vt
    m = matrix(rot, np.asarray(at, dtype=np.float64) - rot @ c)
    for obj in objects:
        obj.data.transform(m)
        obj.data.update()
    return settle(objects, clearance)

def lay_down(objects, clearance=0.0008):
    """Lay ``objects`` flat where they are, keeping the XY heading of their long axis."""
    v = np.concatenate([vertices(o) for o in objects])
    xy = v[:, :2] - v[:, :2].mean(axis=0)
    _, _, vt = np.linalg.svd(xy, full_matrices=False)
    heading = math.degrees(math.atan2(vt[0, 1], vt[0, 0]))
    return lay_flat(objects, (*v[:, :2].mean(axis=0), 0.0), heading, clearance)