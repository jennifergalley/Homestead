"""Forest-floor litter on and around the remains: fallen oak leaves (small leathery
canyon live oak, 3-6 cm, and deeply lobed California black oak, 10-15 cm) dropped on
whatever lies under them, and moss cushions where bones touch the damp ground.

Leaves are thin closed shells (so they read from both sides) with pcoord =
(u across, v base->tip, per-leaf seed) for ``materials.leaf``."""
import math

import bpy
import numpy as np

import homestead_sdf as sdf
from deer import common as C


def leaf_mesh(kit, name, kind, length, seed, material, curl=0.25):
    rng = np.random.default_rng(seed)
    lobes = kind == "black"
    rows, cols = (13, 7) if lobes else (8, 4)
    width = length * (0.62 if lobes else 0.48)
    lobe_n = rng.integers(3, 5)
    phase = rng.uniform(0, 0.3)

    def half_width(v):
        w = 0.5 * width * math.sin(math.pi * min(1.0, v ** 0.8)) ** (0.7 if lobes else 0.9)
        if lobes:
            wave = 0.5 + 0.5 * math.cos(2 * math.pi * (v * lobe_n + phase))
            w *= 0.36 + 0.64 * wave ** 0.6 * min(1.0, v / 0.15)
        else:
            w *= 1.0 - 0.06 * (0.5 + 0.5 * math.cos(v * 40))
        return w

    thickness = 0.00025
    crumple = rng.uniform(0.3, 1.0)

    def point(u, v):
        across = (u - 0.5) * 2.0
        x = across * half_width(v)
        lift = curl * length * 0.35 * across * across + 0.002 * crumple * math.sin(u * 7 + v * 11 + seed)
        droop = -0.12 * length * (v - 0.5) ** 2 + rng.uniform(-0.0004, 0.0004)
        return (x, v * length, lift + droop)

    grid, coords = [], []
    for i in range(1, rows):
        v = i / rows
        for j in range(cols):
            u = j / (cols - 1)
            grid.append(point(u, v))
            coords.append((u, v, seed * 0.137 % 7.0))
    petiole = (0.0, -0.08 * length, 0.001)
    tip = point(0.5, 1.0)
    verts = [petiole] + grid + [tip]
    coords = [(0.5, 0.0, coords[0][2])] + coords + [(0.5, 1.0, coords[0][2])]
    last = len(verts) - 1

    def g(i, j):
        return 1 + i * cols + j

    faces = []
    for j in range(cols - 1):
        faces.append((0, g(0, j + 1), g(0, j)))
        faces.append((last, g(rows - 2, j), g(rows - 2, j + 1)))
    for i in range(rows - 2):
        for j in range(cols - 1):
            faces.append((g(i, j), g(i, j + 1), g(i + 1, j + 1), g(i + 1, j)))
    n = len(verts)
    back = [(x, y, z - thickness) for x, y, z in verts]
    loop = [0] + [g(i, 0) for i in range(rows - 1)] + [last] + [g(i, cols - 1) for i in reversed(range(rows - 1))]
    rim = [(a, b, b + n, a + n) for a, b in zip(loop, loop[1:] + loop[:1])]
    faces = faces + [tuple(reversed([f + n for f in face])) for face in faces] + rim
    obj = kit.mesh(name, verts + back, faces, material=material)
    kit.tag_coords(obj.data, [np.array(c) for c in coords + coords])
    # Top-down UVs in the leaf's own frame (both faces share the island).
    uv = obj.data.uv_layers["UVMap"].data
    allv = verts + back
    for poly in obj.data.polygons:
        for loop, vi in zip(poly.loop_indices, poly.vertices):
            uv[loop].uv = (allv[vi][0], allv[vi][1])
    return kit.recalc_normals(obj)


def scatter_leaves(kit, material, objects, count, seed, region, prefix="Leaf", avoid=None, small=False):
    """Drop ``count`` leaves in ``region`` (x0, x1, y0, y1): each rests on the highest
    thing under it (``objects`` rasterised) and tilts with that surface."""
    rng = np.random.default_rng(seed)
    cell = 0.006
    lo = np.array([region[0], region[2]]) - 0.1
    shape = (int((region[1] - region[0] + 0.2) / cell) + 1, int((region[3] - region[2] + 0.2) / cell) + 1)
    raster = C.heights(objects, lo, cell, shape)
    raster = np.where(np.isfinite(raster), raster, 0.0)
    raster = np.maximum(raster, 0.0)
    out = []
    for k in range(count):
        kind = "black" if rng.random() < (0.0 if small else 0.22) else "live"
        length = rng.uniform(0.1, 0.145) if kind == "black" else rng.uniform(0.032, 0.058)
        for _ in range(20):
            x, y = rng.uniform(region[0], region[1]), rng.uniform(region[2], region[3])
            if avoid is None or not avoid(x, y):
                break
        leaf = leaf_mesh(kit, f"{prefix}{k}", kind, length, int(rng.integers(1, 10_000)), material,
                         curl=rng.uniform(0.12, 0.45))
        heading = rng.uniform(0, 2 * math.pi)
        r = max(1, int(length * 0.35 / cell))
        i, j = int((x - lo[0]) / cell), int((y - lo[1]) / cell)
        window = raster[max(i - r, 0):i + r + 1, max(j - r, 0):j + r + 1]
        top = float(window.max()) if window.size else 0.0
        gx = float(raster[min(i + r, shape[0] - 1), j] - raster[max(i - r, 0), j]) / (2 * r * cell)
        gy = float(raster[i, min(j + r, shape[1] - 1)] - raster[i, max(j - r, 0)]) / (2 * r * cell)
        n = C.unit((-np.clip(gx, -0.7, 0.7), -np.clip(gy, -0.7, 0.7), 1.0))
        if rng.random() < 0.3:
            n = C.unit(n + rng.normal(0, 0.25, 3) * (0.3, 0.3, 0.0))
        f = C.unit(np.array([math.cos(heading), math.sin(heading), 0.0]))
        f = C.unit(f - n * n.dot(f))
        rot = np.stack([np.cross(f, n), f, n], axis=1)
        C.place(leaf, rot, (x, y, top + 0.0012), tag=False)
        low = C.vertices(leaf)[:, 2].min()
        if low < 0.0005:
            C.shift([leaf], (0, 0, 0.0005 - low))
        out.append(leaf)
    return out


def moss_cushion(kit, name, centre, radius, material, seed):
    """Low domed moss cushion, lumpy with tufts, its base sunk just below the ground."""
    rng = np.random.default_rng(seed)
    lumps = [sdf.ellipsoid((0, 0, -radius * 0.1), (radius, radius * 0.82, radius * 0.5))]
    for _ in range(5):
        a, r = rng.uniform(0, 2 * math.pi), rng.uniform(0.3, 0.75) * radius
        lumps.append(sdf.ellipsoid((r * math.cos(a), r * math.sin(a), -radius * 0.05),
                                   tuple(radius * rng.uniform(0.3, 0.5) * np.array([1.0, 1.0, 1.25]))))
    node = sdf.smooth_union(lumps, k=radius * 0.18)
    node = sdf.displaced(node, lambda p: radius * 0.07 * sdf.fbm(p / (radius * 0.12), 3, seed), radius * 0.07)
    obj = sdf.mesh(name, node, max(radius / 45, 0.0005), target_tris=int(400 + radius * 16000), material=material)
    a = rng.uniform(0, 2 * math.pi)
    rot = np.array([[math.cos(a), -math.sin(a), 0], [math.sin(a), math.cos(a), 0], [0, 0, 1]])
    C.place(obj, rot, centre)
    return obj


def moss_at_contacts(kit, material, objects, count, seed, radius=(0.018, 0.045)):
    """Cushions centred where bone surfaces come down to the ground."""
    rng = np.random.default_rng(seed)
    low = np.concatenate([C.vertices(o)[C.vertices(o)[:, 2] < 0.004] for o in objects])
    picks = []
    for _ in range(count * 30):
        if len(picks) >= count or not len(low):
            break
        p = low[rng.integers(len(low))]
        if all(np.linalg.norm(p[:2] - q[:2]) > 0.09 for q in picks):
            picks.append(p)
    out = []
    for k, p in enumerate(picks):
        r = rng.uniform(*radius)
        out.append(moss_cushion(kit, f"Moss{seed}_{k}", (p[0], p[1], -r * 0.12), r, material, seed * 31 + k))
    return out


_ = bpy
