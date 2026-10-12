"""The shallow level's air volume as a signed-distance field (tunnel frame, metres; see ``layout``).

Each gallery, drift and alcove is a straight run with a flat floor, near-vertical walls and a
rounded back (roof), swept along its edge with rounded ends; runs meet with a smooth blend so the
junctions open out the way hand-cut and blasted levels do. The ladder chamber is a larger room
under the shaft, and the sump is a kerbed pit at the alcove's end. The floor is one shared plane
(z = 0) so it stays flat across every junction. ``air()`` is negative in the air; the rock surface
is its zero set, meshed with ``homestead_sdf.mesh`` and turned inside out (normals into the air).
"""
import math

import numpy as np

import homestead_sdf as sdf
from mine import layout as L

JUNCTION_BLEND = 0.9         # smooth-union radius where runs meet (m)


def _profile(q, z, half, wall_h, height):
    """Distance-like value of a run's cross-section: |q| lateral, z up (floor not included)."""
    rise = height - wall_h
    wall = q - half
    e = np.sqrt((q / half) ** 2 + ((z - wall_h) / rise) ** 2)
    arch = (e - 1.0) * min(half, rise)
    return np.where(z > wall_h, arch, wall)


def run(a, b, half, wall_h, height, end_a=True, end_b=True):
    """A run from a to b (x, y). Its ends are rounded (radius ``half``) so blind drifts end in a
    curved face; ``end_*`` False squares an end off (unused)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = b - a
    length = float(np.hypot(*d))
    u = d / length
    lo = np.array([min(a[0], b[0]) - half, min(a[1], b[1]) - half, -1.0])
    hi = np.array([max(a[0], b[0]) + half, max(a[1], b[1]) + half, height + 0.5])

    def dist(p):
        rel = p[:, :2] - a
        t = np.clip(rel @ u, 0.0, length)
        q = np.hypot(rel[:, 0] - u[0] * t, rel[:, 1] - u[1] * t)
        return _profile(q, p[:, 2], half, wall_h, height)
    return sdf._primitive(dist, lo, hi)


def chamber():
    cx, cy = L.CHAMBER["centre"]
    hx, hy = L.CHAMBER["half"]
    height = L.CHAMBER["height"]
    wall_h = height - 1.3
    corner = 1.2
    lo = np.array([cx - hx, cy - hy, -1.0])
    hi = np.array([cx + hx, cy + hy, height + 0.5])

    def dist(p):
        x, y, z = p[:, 0] - cx, p[:, 1] - cy, p[:, 2]
        qx, qy = np.abs(x) - (hx - corner), np.abs(y) - (hy - corner)
        box = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - corner
        # A shallow dome: the back rises from the wall head to the full height at the middle.
        r = np.sqrt((x / hx) ** 2 + (y / hy) ** 2)
        roof = z - (wall_h + (height - wall_h) * np.sqrt(np.clip(1.0 - r ** 2, 0.0, 1.0)))
        return np.maximum(box, roof)
    return sdf._primitive(dist, lo, hi)


def shaft():
    sx, sy = L.SHAFT["centre"]
    h = L.SHAFT["half"]
    z0, z1 = L.CHAMBER["height"] - 1.0, L.CHAMBER["height"] + L.SHAFT["height"]
    return sdf.box((sx, sy, (z0 + z1) / 2), (h, h, (z1 - z0) / 2), rounding=0.05)


def runs():
    nodes = []
    for f, t, kind in L.EDGES:
        if kind == "drift":
            dims = (L.DRIFT_HALF, L.DRIFT_WALL_H, L.DRIFT_H)
        elif kind == "alcove":
            dims = (1.1, 1.9, 2.8)
        else:
            dims = (L.GALLERY_HALF, L.GALLERY_WALL_H, L.GALLERY_H)
        nodes.append(run(L.POINTS[f], L.POINTS[t], *dims))
    return nodes


def sump():
    cx, cy = L.SUMP["centre"]
    hx, hy = L.SUMP["half"]
    return sdf.box((cx, cy, -L.SUMP["depth"] / 2 + 0.2), (hx, hy, L.SUMP["depth"] / 2 + 0.2), rounding=0.12)


# Jenny (08-tunnels): "the tunnel arches up above the timber braces and leaves them supporting nothing. The
# tunnel's ceiling should probably be flat against the timber brace." At each set the roof comes down onto the
# lagging over its cap (SET_HOLD m either side of it), then rises back to the gallery's arch over SET_EASE m.
SET_ROOF_ABOVE_WALL_H = 0.20   # the lagging's top over the run's wall height (mine_kit.timber_set: cap at wall_h - 0.05)
SET_HOLD, SET_EASE = 0.45, 1.4


def set_roofs():
    """[(point, along unit, lateral half, roof z)] for every timber set."""
    out = []
    for edge, t in L.TIMBER_SETS:
        a, b = np.asarray(L.POINTS[edge[0]], float), np.asarray(L.POINTS[edge[1]], float)
        u = (b - a) / np.linalg.norm(b - a)
        drift = edge != ("L", "A")
        half = (L.DRIFT_HALF if drift else L.GALLERY_HALF) + 0.4
        wall_h = L.DRIFT_WALL_H if drift else L.GALLERY_WALL_H
        out.append((a + (b - a) * t, u, half, wall_h + SET_ROOF_ABOVE_WALL_H))
    return out


def air(extra=None, blend=JUNCTION_BLEND):
    """The whole level's air. ``extra`` adds more upper-volume nodes (e.g. node recesses)."""
    upper = sdf.smooth_union(runs() + [chamber(), shaft()] + list(extra or []), blend)
    pit = sump()
    roofs = set_roofs()

    def fn(p, band):
        floor = -p[:, 2]
        d = np.maximum(floor, upper(p, band))
        for c, u, half, roof in roofs:
            rel = p[:, :2] - c
            s = np.abs(rel @ u)
            q = np.abs(rel[:, 0] * -u[1] + rel[:, 1] * u[0])
            near = (q < half) & (s < SET_HOLD + SET_EASE)
            if near.any():
                ease = np.clip((s[near] - SET_HOLD) / SET_EASE, 0.0, 1.0)
                lid = p[near, 2] - (roof + 1.6 * ease * ease * (3 - 2 * ease))
                d[near] = np.maximum(d[near], lid)
        return np.minimum(d, pit(p, band)).astype(np.float32)
    lo = np.minimum(upper.lo, pit.lo)
    hi = np.maximum(upper.hi, pit.hi)
    lo[2] = -L.SUMP["depth"] - 0.5
    return sdf.Node(fn, lo, hi, upper.k)


def turn_inside_out(obj):
    """Point the meshed rock's normals into the air (the level is seen from inside)."""
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    return obj
