"""Signed-distance modelling for organic props (numpy SDFs polygonised by OpenVDB).

Blender bundles the ``openvdb`` Python module, so a prop can be sculpted as a tree of
signed-distance primitives with smooth booleans (bones, skulls, knotty roots) and meshed
in seconds, without a voxel remesh of intersecting parts:

    import homestead_sdf as sdf
    shape = sdf.smooth_union([sdf.ellipsoid((0, 0, 0), (0.05, 0.03, 0.03)),
                              sdf.round_cone((0, 0, 0), (0.1, 0, 0), 0.02, 0.01)], k=0.006)
    shape = sdf.subtract(shape, sdf.sphere((0.02, 0.03, 0), 0.012), k=0.002)
    obj = sdf.mesh("Bone", shape, voxel=0.0008, target_tris=3000)

Every node carries a bounding box and each primitive is evaluated only near its own box.
Distances are clipped to a narrow band (a few voxels plus the largest smoothing radius),
which is all the polygoniser needs. Coordinates are meters; rotations are 3x3 matrices
whose columns are the primitive's local axes in the parent frame.
"""
import math

import numpy as np


class Node:
    def __init__(self, fn, lo, hi, k=0.0):
        self.fn = fn
        self.lo = np.asarray(lo, dtype=np.float64)
        self.hi = np.asarray(hi, dtype=np.float64)
        self.k = k                    # largest smoothing radius below this node

    def __call__(self, p, band):
        return self.fn(p, band)


def _as(v):
    return np.asarray(v, dtype=np.float64)


def _primitive(dist, lo, hi):
    lo, hi = _as(lo), _as(hi)

    def fn(p, band):
        out = np.full(len(p), band, dtype=np.float32)
        if np.any(p.max(axis=0) < lo - band) or np.any(p.min(axis=0) > hi + band):
            return out
        inside = np.all((p >= lo - band) & (p <= hi + band), axis=1)
        if inside.any():
            out[inside] = np.clip(dist(p[inside]), -band, band)
        return out
    return Node(fn, lo, hi)


def _frame_bounds(centre, half, rotation):
    """Axis-aligned bounds of a box of half extents ``half`` in a rotated frame."""
    centre, half = _as(centre), _as(half)
    if rotation is None:
        return centre - half, centre + half
    extent = np.abs(_as(rotation)) @ half
    return centre - extent, centre + extent


def _local(p, centre, rotation):
    q = p - _as(centre)
    return q if rotation is None else q @ _as(rotation)


def rotation(axis_x=None, axis_y=None, axis_z=None):
    """Orthonormal frame from one or two given axes (the first given wins exactly)."""
    axes = [None if a is None else _as(a) / np.linalg.norm(a) for a in (axis_x, axis_y, axis_z)]
    given = [i for i, a in enumerate(axes) if a is not None]
    if len(given) == 1:
        a = axes[given[0]]
        helper = np.array([0.0, 0.0, 1.0]) if abs(a[2]) < 0.9 else np.array([1.0, 0.0, 0.0])
        axes[(given[0] + 1) % 3] = np.cross(helper, a) if given[0] != 1 else np.cross(a, helper)
        given.append((given[0] + 1) % 3)
    i, j = given[:2]
    axes[j] = axes[j] - axes[i] * axes[i].dot(axes[j])
    axes[j] /= np.linalg.norm(axes[j])
    k = 3 - i - j
    axes[k] = np.cross(axes[(k + 1) % 3], axes[(k + 2) % 3])
    return np.stack(axes, axis=1)


def axis_angle(axis, angle):
    axis = _as(axis) / np.linalg.norm(axis)
    x, y, z = axis
    c, s, t = math.cos(angle), math.sin(angle), 1 - math.cos(angle)
    return np.array([[t * x * x + c, t * x * y - s * z, t * x * z + s * y],
                     [t * x * y + s * z, t * y * y + c, t * y * z - s * x],
                     [t * x * z - s * y, t * y * z + s * x, t * z * z + c]])


# ---------------------------------------------------------------- primitives

def sphere(centre, radius):
    centre = _as(centre)
    return _primitive(lambda p: np.linalg.norm(p - centre, axis=1) - radius, centre - radius, centre + radius)


def ellipsoid(centre, radii, rotation=None):
    """Ellipsoid (the usual k0 * (k0 - 1) / k1 distance bound)."""
    radii = _as(radii)

    def dist(p):
        q = _local(p, centre, rotation)
        k0 = np.linalg.norm(q / radii, axis=1)
        k1 = np.maximum(np.linalg.norm(q / (radii * radii), axis=1), 1e-9)
        return k0 * (k0 - 1.0) / k1
    lo, hi = _frame_bounds(centre, radii, rotation)
    return _primitive(dist, lo, hi)


def round_cone(a, b, ra, rb=None):
    """Capsule from ``a`` to ``b`` whose radius goes from ``ra`` to ``rb``."""
    a, b = _as(a), _as(b)
    rb = ra if rb is None else rb
    ba = b - a
    l2 = float(ba.dot(ba))
    rr = ra - rb
    a2 = l2 - rr * rr
    il2 = 1.0 / max(l2, 1e-12)

    def dist(p):
        pa = p - a
        y = pa @ ba
        z = y - l2
        xv = pa * l2 - np.outer(y, ba)
        x2 = np.einsum("ij,ij->i", xv, xv)
        y2 = y * y * l2
        z2 = z * z * l2
        k = np.sign(rr) * rr * rr * x2
        d = (np.sqrt(np.maximum(x2 * a2 * il2, 0.0)) + y * rr) * il2 - ra
        d = np.where(np.sign(z) * a2 * z2 > k, np.sqrt(x2 + z2) * il2 - rb, d)
        d = np.where(np.sign(y) * a2 * y2 < k, np.sqrt(x2 + y2) * il2 - ra, d)
        return d
    r = max(ra, rb)
    return _primitive(dist, np.minimum(a, b) - r, np.maximum(a, b) + r)


def tube(points, radii, k=0.0):
    """Round cones along a polyline (``radii`` per point); ``k`` smooths the joints."""
    parts = [round_cone(points[i], points[i + 1], radii[i], radii[i + 1]) for i in range(len(points) - 1)]
    return smooth_union(parts, k) if k else union(parts)


def box(centre, half, rotation=None, rounding=0.0):
    half = _as(half)

    def dist(p):
        q = np.abs(_local(p, centre, rotation)) - (half - rounding)
        outside = np.linalg.norm(np.maximum(q, 0.0), axis=1)
        return outside + np.minimum(q.max(axis=1), 0.0) - rounding
    lo, hi = _frame_bounds(centre, half, rotation)
    return _primitive(dist, lo, hi)


def torus(centre, major, minor, rotation=None):
    """Ring in the local XY plane (axis local Z)."""
    def dist(p):
        q = _local(p, centre, rotation)
        ring = np.hypot(q[:, 0], q[:, 1]) - major
        return np.hypot(ring, q[:, 2]) - minor
    lo, hi = _frame_bounds(centre, (major + minor, major + minor, minor), rotation)
    return _primitive(dist, lo, hi)


def halfspace(point, normal, lo, hi):
    """Everything behind the plane (``normal`` points out), limited to bounds lo..hi."""
    point, normal = _as(point), _as(normal) / np.linalg.norm(normal)
    return _primitive(lambda p: (p - point) @ normal, lo, hi)


def prism(points2d, half_thickness, rotation=None, centre=(0, 0, 0), rounding=0.0):
    """Convex polygon (local XY, counter-clockwise) extruded +-``half_thickness`` along
    local Z, edges rounded by ``rounding``."""
    pts = [np.asarray(q, dtype=np.float64) for q in points2d]
    edges = []
    for i, a in enumerate(pts):
        b = pts[(i + 1) % len(pts)]
        e = b - a
        n = np.array([e[1], -e[0]]) / np.linalg.norm(e)
        edges.append((a, n))

    def dist(p):
        q = _local(p, centre, rotation)
        d2 = np.max(np.stack([(q[:, :2] - a) @ n for a, n in edges]), axis=0) + rounding
        dz = np.abs(q[:, 2]) - half_thickness + rounding
        w = np.stack([d2, dz], axis=1)
        return np.linalg.norm(np.maximum(w, 0.0), axis=1) + np.minimum(w.max(axis=1), 0.0) - rounding
    arr = np.array(pts)
    half = np.array([*(arr.max(0) - arr.min(0)) / 2, half_thickness])
    mid = np.array([*(arr.max(0) + arr.min(0)) / 2, 0.0])
    c = _as(centre) + (mid if rotation is None else _as(rotation) @ mid)
    lo, hi = _frame_bounds(c, half, rotation)
    return _primitive(dist, lo, hi)


# ------------------------------------------------------------------ booleans

def union(nodes):
    nodes = list(nodes)

    def fn(p, band):
        return np.min(np.stack([n(p, band) for n in nodes]), axis=0)
    return Node(fn, np.min([n.lo for n in nodes], 0), np.max([n.hi for n in nodes], 0),
                max(n.k for n in nodes))


def _smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1.0 - h)


def smooth_union(nodes, k):
    nodes = list(nodes)
    if not k:
        return union(nodes)

    def fn(p, band):
        d = nodes[0](p, band)
        for n in nodes[1:]:
            d = _smin(d, n(p, band), k)
        return d
    return Node(fn, np.min([n.lo for n in nodes], 0) - k, np.max([n.hi for n in nodes], 0) + k,
                max([k] + [n.k for n in nodes]))


def subtract(a, cutters, k=0.0):
    """``a`` minus each cutter (smoothly with radius ``k``)."""
    cutters = cutters if isinstance(cutters, (list, tuple)) else [cutters]

    def fn(p, band):
        d = a(p, band)
        for c in cutters:
            e = c(p, band)
            if k:
                h = np.clip(0.5 - 0.5 * (d + e) / k, 0.0, 1.0)
                d = d + (-e - d) * h + k * h * (1.0 - h)
            else:
                d = np.maximum(d, -e)
        return d
    return Node(fn, a.lo, a.hi, max([k, a.k] + [c.k for c in cutters]))


def intersect(a, b, k=0.0):
    def fn(p, band):
        d, e = a(p, band), b(p, band)
        if k:
            h = np.clip(0.5 - 0.5 * (e - d) / k, 0.0, 1.0)
            return e + (d - e) * h + k * h * (1.0 - h)
        return np.maximum(d, e)
    return Node(fn, np.maximum(a.lo, b.lo), np.minimum(a.hi, b.hi), max(k, a.k, b.k))


def offset(a, amount):
    return Node(lambda p, band: a(p, band) - amount, a.lo - max(amount, 0), a.hi + max(amount, 0), a.k)


def displaced(a, fn, amplitude):
    """Add ``fn(p) -> meters`` (|value| <= ``amplitude``) to the surface of ``a``."""
    def f(p, band):
        d = a(p, band)
        near = np.abs(d) < band
        if near.any():
            d = d.copy()
            d[near] -= fn(p[near])
        return d
    return Node(f, a.lo - amplitude, a.hi + amplitude, a.k)


def transformed(a, rotation, translation):
    """Place node ``a`` (authored in its own frame) at ``translation`` with ``rotation``."""
    rotation, translation = _as(rotation), _as(translation)
    corners = np.array([[x, y, z] for x in (a.lo[0], a.hi[0]) for y in (a.lo[1], a.hi[1])
                        for z in (a.lo[2], a.hi[2])]) @ rotation.T + translation
    return Node(lambda p, band: a((p - translation) @ rotation, band), corners.min(0), corners.max(0), a.k)


# --------------------------------------------------------------------- noise

def _hash(ix, iy, iz, seed):
    n = (ix * 73856093) ^ (iy * 19349663) ^ (iz * 83492791) ^ (seed * 2654435761 & 0x7FFFFFFF)
    n = (n ^ (n >> 13)) * 1274126177
    return ((n ^ (n >> 16)) & 0xFFFF).astype(np.float64) / 32767.5 - 1.0


def noise(p, seed=0):
    """Smooth value noise in [-1, 1] at unit feature size (vectorised)."""
    with np.errstate(over="ignore"):
        cell = np.floor(p).astype(np.int64)
        f = p - cell
        u = f * f * (3.0 - 2.0 * f)
        out = np.zeros(len(p))
        for dx in (0, 1):
            wx = u[:, 0] if dx else 1 - u[:, 0]
            for dy in (0, 1):
                wy = u[:, 1] if dy else 1 - u[:, 1]
                for dz in (0, 1):
                    wz = u[:, 2] if dz else 1 - u[:, 2]
                    out += wx * wy * wz * _hash(cell[:, 0] + dx, cell[:, 1] + dy, cell[:, 2] + dz, seed)
    return out


def fbm(p, octaves=3, seed=0):
    out, amp, freq, total = np.zeros(len(p)), 1.0, 1.0, 0.0
    for i in range(octaves):
        out += amp * noise(p * freq, seed + 17 * i)
        total += amp
        amp *= 0.5
        freq *= 2.03
    return out / total


# ----------------------------------------------------------------- meshing

def _dense(node, lo, n, voxel, band, mask=None, brick=20):
    """Node values on the regular grid lo + voxel * index. With ``mask``, only masked
    voxels are evaluated, brick by brick, so primitives far from a brick cost nothing."""
    values = np.full(tuple(n), band, dtype=np.float32)
    if mask is not None:
        ijk = np.argwhere(mask)
        if not len(ijk):
            return values
        key = ((ijk[:, 0] // brick) * ((n[1] // brick + 1) * (n[2] // brick + 1))
               + (ijk[:, 1] // brick) * (n[2] // brick + 1) + ijk[:, 2] // brick)
        order = np.argsort(key, kind="stable")
        ijk, key = ijk[order], key[order]
        starts = np.flatnonzero(np.r_[True, key[1:] != key[:-1]])
        ends = np.r_[starts[1:], len(key)]
        for a, b in zip(starts, ends):
            block = ijk[a:b]
            values[block[:, 0], block[:, 1], block[:, 2]] = node(lo + voxel * block, band)
        return values
    ys = lo[1] + voxel * np.arange(n[1])
    zs = lo[2] + voxel * np.arange(n[2])
    yz = np.stack(np.meshgrid(ys, zs, indexing="ij"), axis=-1).reshape(-1, 2)
    slab = max(1, int(2_000_000 // len(yz)))
    for i0 in range(0, n[0], slab):
        i1 = min(n[0], i0 + slab)
        xs = lo[0] + voxel * np.arange(i0, i1)
        p = np.concatenate([np.repeat(xs, len(yz))[:, None], np.tile(yz, (len(xs), 1))], axis=1)
        values[i0:i1] = node(p, band).reshape(i1 - i0, n[1], n[2])
    return values


def evaluate(node, voxel, coarse=4):
    """Clipped distance grid over the node's bounds: (values, origin, clip). A grid
    ``coarse`` times coarser finds the surface first, so only voxels within a few voxels
    of it are evaluated at full resolution (the rest keep the coarse sign). Primitives
    are clipped at ``clip`` (>= 1.5x the largest smoothing radius) so smooth blends stay
    right, but only a 3-voxel band is refined."""
    clip = float(max(3.0 * voxel, 1.5 * node.k))
    lo = node.lo - 4 * voxel
    hi = node.hi + 4 * voxel
    n = np.maximum(np.ceil((hi - lo) / voxel).astype(int) + 1, 2)
    if coarse <= 1 or n.min() < 4 * coarse:
        return _dense(node, lo, n, voxel, clip), lo, clip
    cv = voxel * coarse
    nc = np.ceil((n - 1) / coarse).astype(int) + 1
    rough = _dense(node, lo, nc, cv, clip + 1.2 * cv)
    near = np.abs(rough) < 3.0 * voxel + 1.2 * cv
    idx = [np.minimum(np.rint(np.arange(n[a]) / coarse).astype(int), nc[a] - 1) for a in range(3)]
    mask = near[np.ix_(*idx)]
    values = _dense(node, lo, n, voxel, clip, mask)
    sign = np.sign(rough[np.ix_(*idx)])
    values = np.where(mask, values, sign * clip).astype(np.float32)
    return values, lo, clip

def polygonise(node, voxel):
    """(vertices (N, 3), faces list) of the zero level set of ``node``."""
    import openvdb
    values, lo, band = evaluate(node, voxel)
    grid = openvdb.FloatGrid(band)
    grid.copyFromArray(values)
    grid.transform = openvdb.createLinearTransform(voxelSize=voxel)
    grid.gridClass = openvdb.GridClass.LEVEL_SET
    points, triangles, quads = grid.convertToPolygons(isovalue=0.0, adaptivity=0.0)
    points = np.asarray(points, dtype=np.float64) + lo
    faces = [tuple(int(i) for i in q) for q in np.asarray(quads).reshape(-1, 4)]
    faces += [tuple(int(i) for i in t) for t in np.asarray(triangles).reshape(-1, 3)]
    return points, faces


def mesh(name, node, voxel, target_tris=None, material=None):
    """Mesh ``node`` into a new Blender object (outward normals, optionally decimated to
    about ``target_tris`` triangles). Vertex positions are in the node's own frame."""
    import bmesh
    import bpy
    points, faces = polygonise(node, voxel)
    if not faces:
        raise RuntimeError(f"SDF {name} produced no surface (voxel {voxel})")
    data = bpy.data.meshes.new(name)
    data.from_pydata([tuple(p) for p in points], [], faces)
    data.validate()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=voxel * 0.05)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    tris = sum(len(p.vertices) - 2 for p in data.polygons)
    if target_tris and tris > target_tris:
        mod = obj.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = target_tris / tris
        mod.use_collapse_triangulate = True
        depsgraph = bpy.context.evaluated_depsgraph_get()
        reduced = bpy.data.meshes.new_from_object(obj.evaluated_get(depsgraph))
        obj.modifiers.clear()
        obj.data = reduced
        bpy.data.meshes.remove(data)
        data = reduced
        data.name = name
    if not data.uv_layers:
        data.uv_layers.new(name="UVMap")
    if material is not None:
        data.materials.append(material)
    return obj
