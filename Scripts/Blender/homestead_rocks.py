"""Procedural rock geometry for Homestead: granite boulders, outcrops and stone clusters.

Rocks are implicit fields: ``F(P) -> meters``, negative inside. A field is a
superellipsoid (the rounded corestone), lumped by fBm, cut by smoothly rounded
joint planes (the fracture sets that block out Sierra granite). ``solid`` finds
its surface along rays from a centre, which suits star-shaped stones and keeps
every step vectorised with numpy. Surface passes then carve what weathering
does to granite: exfoliation sheets peeling off in curved steps (``sheets``),
weathering pans on flat tops (``pits``), joint cracks (``crack``) and granular
relief. ``finish`` unions the pieces with a voxel remesh, unwraps the mid-poly
cage with few, large islands, subdivides and adds the fine relief, so decimated
LODs keep LOD0's UVs and share one texture set.

Seeded throughout: the same recipe always rebuilds the same rock.
"""
import math

import bmesh
import bpy
import numpy as np

# ------------------------------------------------------------------ noise

_GRAD = np.array([[1, 1, 0], [-1, 1, 0], [1, -1, 0], [-1, -1, 0], [1, 0, 1], [-1, 0, 1],
                  [1, 0, -1], [-1, 0, -1], [0, 1, 1], [0, -1, 1], [0, 1, -1], [0, -1, -1],
                  [1, 1, 0], [0, -1, 1], [-1, 1, 0], [0, -1, -1]], dtype=np.float64)


class Noise:
    """Vectorised improved Perlin noise (Perlin 2002) with fBm helpers. Values are
    roughly -1..1 (typically within +-0.6)."""

    def __init__(self, seed):
        rng = np.random.default_rng(seed)
        perm = rng.permutation(256)
        self.perm = np.concatenate([perm, perm, perm]).astype(np.int64)
        self.shift = rng.uniform(-300.0, 300.0, 3)

    def perlin(self, points):
        p = np.asarray(points, dtype=np.float64) + self.shift
        cell = np.floor(p)
        f = p - cell
        i = cell.astype(np.int64) & 255
        u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0)
        perm = self.perm
        x_, y_, z_ = i[:, 0], i[:, 1], i[:, 2]
        a = perm[x_] + y_
        b = perm[x_ + 1] + y_
        aa, ab, ba, bb = perm[a] + z_, perm[a + 1] + z_, perm[b] + z_, perm[b + 1] + z_
        x, y, z = f[:, 0], f[:, 1], f[:, 2]

        def grad(h, dx, dy, dz):
            g = _GRAD[perm[h] & 15]
            return g[:, 0] * dx + g[:, 1] * dy + g[:, 2] * dz

        ux, uy, uz = u[:, 0], u[:, 1], u[:, 2]
        n00 = grad(aa, x, y, z) + ux * (grad(ba, x - 1, y, z) - grad(aa, x, y, z))
        n10 = grad(ab, x, y - 1, z) + ux * (grad(bb, x - 1, y - 1, z) - grad(ab, x, y - 1, z))
        n01 = grad(aa + 1, x, y, z - 1) + ux * (grad(ba + 1, x - 1, y, z - 1) - grad(aa + 1, x, y, z - 1))
        n11 = grad(ab + 1, x, y - 1, z - 1) + ux * (grad(bb + 1, x - 1, y - 1, z - 1) - grad(ab + 1, x, y - 1, z - 1))
        n0 = n00 + uy * (n10 - n00)
        n1 = n01 + uy * (n11 - n01)
        return n0 + uz * (n1 - n0)

    def fbm(self, points, octaves=4, lacunarity=2.03, gain=0.5):
        p = np.asarray(points, dtype=np.float64)
        total = np.zeros(len(p))
        amp, norm, freq = 1.0, 0.0, 1.0
        for octave in range(octaves):
            total += amp * self.perlin(p * freq + octave * 17.17)
            norm += amp
            amp *= gain
            freq *= lacunarity
        return total / norm * 1.6

    def ridged(self, points, octaves=3):
        """Sharp creases (1 at the crease) for cracks and seams."""
        p = np.asarray(points, dtype=np.float64)
        total = np.zeros(len(p))
        amp, norm, freq = 1.0, 0.0, 1.0
        for octave in range(octaves):
            total += amp * (1.0 - np.abs(self.perlin(p * freq + octave * 31.7)) * 2.2).clip(0, 1) ** 2
            norm += amp
            amp *= 0.5
            freq *= 2.1
        return total / norm


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def smax(a, b, k):
    """Polynomial smooth maximum (rounded intersection) with blend radius k (m)."""
    if k <= 0:
        return np.maximum(a, b)
    h = np.maximum(k - np.abs(a - b), 0.0) / k
    return np.maximum(a, b) + h * h * k * 0.25


def smin(a, b, k):
    return -smax(-a, -b, k)


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v)


def rotation(yaw=0.0, pitch=0.0, roll=0.0):
    """3x3 rotation (degrees): roll about X, then pitch about Y, then yaw about Z."""
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))
    cr, sr = math.cos(math.radians(roll)), math.sin(math.radians(roll))
    rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
    ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])
    rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])
    return rz @ ry @ rx


# ------------------------------------------------------------------ fields

def boulder(radii, power=2.4, lumps=0.05, lump_scale=1.3, joints=(), seed=0, center=(0, 0, 0),
            rotate=None, twist=0.0, lump_octaves=3, bend=0.0):
    """Rounded granite corestone: superellipsoid ``radii`` (m) with exponent ``power``
    (2 = ellipsoid, 3-4 = loaf/block), fBm ``lumps`` (fraction of mean radius) and
    ``joints``: planes ``(normal, distance_m, rounding_m)`` in the rock's frame that
    cut it with rounded edges. ``rotate`` is a 3x3 matrix applied to the rock;
    ``bend`` (1/m) curves it like a spherical shell of radius 1 / (2 * bend) about
    its local +Z (for exfoliation sheets and slabs)."""
    radii = np.asarray(radii, dtype=np.float64)
    mean = float(radii.mean())
    noise = Noise(seed)
    center = np.asarray(center, dtype=np.float64)
    inverse = np.linalg.inv(rotate) if rotate is not None else None
    planes = [(unit(n), float(d), float(k)) for n, d, k in joints]

    def field(points):
        p = np.asarray(points, dtype=np.float64) - center
        if inverse is not None:
            p = p @ inverse.T
        if twist:
            angle = twist * p[:, 2] / radii[2]
            c, s = np.cos(angle), np.sin(angle)
            p = np.stack([c * p[:, 0] - s * p[:, 1], s * p[:, 0] + c * p[:, 1], p[:, 2]], axis=1)
        if bend:
            p = np.stack([p[:, 0], p[:, 1], p[:, 2] + bend * (p[:, 0] ** 2 + p[:, 1] ** 2)], axis=1)
        q = np.abs(p) / radii
        g = np.maximum((q ** power).sum(axis=1) ** (1.0 / power), 1e-9)
        length = np.linalg.norm(p, axis=1)
        d = length * (g - 1.0) / g
        if lumps:
            d = d - lumps * mean * noise.fbm(p / mean * lump_scale, lump_octaves, gain=0.42)
        for normal, distance, rounding in planes:
            d = smax(d, p @ normal - distance, rounding)
        return d
    field.mean_radius = mean
    field.center = center
    return field


def cut(field, normal, distance, rounding=0.0, wobble=0.0, wobble_scale=1.0, seed=0):
    """Intersect ``field`` with the half-space ``normal . P <= distance`` (world frame),
    optionally with a rough (noisy) fracture face."""
    normal = unit(normal)
    noise = Noise(seed + 911)

    def f(points):
        p = np.asarray(points, dtype=np.float64)
        plane = p @ normal - distance
        if wobble:
            plane = plane + wobble * noise.fbm(p * wobble_scale, 4)
        return smax(field(p), plane, rounding)
    f.mean_radius = getattr(field, "mean_radius", 1.0)
    f.center = getattr(field, "center", np.zeros(3))
    return f


def gradient(field, points, eps):
    p = np.asarray(points, dtype=np.float64)
    g = np.zeros_like(p)
    for axis in range(3):
        offset = np.zeros(3)
        offset[axis] = eps
        g[:, axis] = (field(p + offset) - field(p - offset)) / (2 * eps)
    return g / np.maximum(np.linalg.norm(g, axis=1)[:, None], 1e-12)


# ------------------------------------------------------------------ meshes

_ICO = {}


def icosphere(subdivisions):
    if subdivisions not in _ICO:
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=1.0)
        data = bpy.data.meshes.new("_ico")
        bm.to_mesh(data)
        bm.free()
        verts = np.empty(len(data.vertices) * 3)
        data.vertices.foreach_get("co", verts)
        faces = np.empty(len(data.polygons) * 3, dtype=np.int64)
        data.polygons.foreach_get("vertices", faces)
        bpy.data.meshes.remove(data)
        verts = verts.reshape(-1, 3)
        _ICO[subdivisions] = (verts / np.linalg.norm(verts, axis=1)[:, None], faces.reshape(-1, 3))
    return _ICO[subdivisions]


def mesh_object(name, verts, faces, material=None):
    verts = np.asarray(verts, dtype=np.float64)
    faces = np.asarray(faces, dtype=np.int64)
    data = bpy.data.meshes.new(name)
    data.vertices.add(len(verts))
    data.vertices.foreach_set("co", verts.ravel().astype(np.float32))
    data.loops.add(faces.size)
    data.loops.foreach_set("vertex_index", faces.ravel().astype(np.int32))
    data.polygons.add(len(faces))
    data.polygons.foreach_set("loop_start", np.arange(0, faces.size, faces.shape[1], dtype=np.int32))
    data.update(calc_edges=True)
    data.validate()
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    if material is not None:
        data.materials.append(material)
    return obj


def coords(obj):
    data = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertices.foreach_get("co", data)
    return data.reshape(-1, 3)


def set_coords(obj, points):
    obj.data.vertices.foreach_set("co", np.asarray(points, dtype=np.float32).ravel())
    obj.data.update()


def normals(obj):
    obj.data.update()
    data = np.empty(len(obj.data.vertices) * 3)
    obj.data.vertex_normals.foreach_get("vector", data)
    return data.reshape(-1, 3)


def area(obj):
    p = coords(obj)
    tris = []
    for poly in obj.data.polygons:
        v = poly.vertices
        for k in range(1, len(v) - 1):
            tris.append((v[0], v[k], v[k + 1]))
    t = np.asarray(tris)
    return float(np.linalg.norm(np.cross(p[t[:, 1]] - p[t[:, 0]], p[t[:, 2]] - p[t[:, 0]]), axis=1).sum() / 2)


def solid(name, field, center=None, r_max=None, subdivisions=7, post=None, material=None,
          iterations=30):
    """Mesh the surface of ``field`` by bisection along rays from ``center``
    (default: the field's own centre). ``post(P, N) -> P`` reshapes the surface
    (sheets, pits, cracks) before meshing; N are the field's unit normals."""
    center = np.asarray(field.center if center is None else center, dtype=np.float64)
    r_max = r_max or field.mean_radius * 4.0
    dirs, faces = icosphere(subdivisions)
    lo = np.zeros(len(dirs))
    hi = np.full(len(dirs), float(r_max))
    for _ in range(iterations):
        mid = (lo + hi) * 0.5
        inside = field(center + dirs * mid[:, None]) < 0
        lo = np.where(inside, mid, lo)
        hi = np.where(inside, hi, mid)
    points = center + dirs * ((lo + hi) * 0.5)[:, None]
    if post is not None:
        n = gradient(field, points, field.mean_radius * 0.004)
        points = post(points, n)
    return mesh_object(name, points, faces, material)


def transform(obj, matrix=None, offset=(0, 0, 0)):
    p = coords(obj)
    if matrix is not None:
        p = p @ np.asarray(matrix).T
    set_coords(obj, p + np.asarray(offset))
    return obj


# ------------------------------------------------------------------ surface passes

def tangential_gradient(fn, points, n, eps):
    g = np.zeros_like(points)
    for axis in range(3):
        offset = np.zeros(3)
        offset[axis] = eps
        g[:, axis] = (fn(points + offset) - fn(points - offset)) / (2 * eps)
    g -= (g * n).sum(axis=1)[:, None] * n
    return np.linalg.norm(g, axis=1)


def _hash(cells, seed):
    """64-bit integer hash of integer cell coordinates (N, 3)."""
    c = cells.astype(np.uint64)
    h = (c[:, 0] * np.uint64(0x9E3779B97F4A7C15)) ^ (c[:, 1] * np.uint64(0xC2B2AE3D27D4EB4F)) \
        ^ (c[:, 2] * np.uint64(0x165667B19E3779F9)) ^ np.uint64(seed * 0x27D4EB2F165667C5 & 0xFFFFFFFFFFFFFFFF)
    h ^= h >> np.uint64(31)
    h *= np.uint64(0xBF58476D1CE4E5B9)
    h ^= h >> np.uint64(27)
    h *= np.uint64(0x94D049BB133111EB)
    h ^= h >> np.uint64(33)
    return h


def _unit(h, shift):
    return ((h >> np.uint64(shift)) & np.uint64(0xFFFFF)).astype(np.float64) / float(0x100000)


def voronoi(points, cell, seed, chunk=250000):
    """Jittered-grid 3D Voronoi: nearest/second-nearest distances (m), each cell's
    random value 0..1 and feature point (m) for both."""
    points = np.asarray(points, dtype=np.float64)
    n = len(points)
    f1 = np.full(n, np.inf)
    f2 = np.full(n, np.inf)
    r1, r2 = np.zeros(n), np.zeros(n)
    c1, c2 = np.zeros((n, 3)), np.zeros((n, 3))
    offsets = np.array([(x, y, z) for x in (-1, 0, 1) for y in (-1, 0, 1) for z in (-1, 0, 1)])
    for start in range(0, n, chunk):
        q = points[start:start + chunk] / cell
        base = np.floor(q).astype(np.int64)
        b1 = np.full(len(q), np.inf)
        b2 = np.full(len(q), np.inf)
        v1, v2 = np.zeros(len(q)), np.zeros(len(q))
        p1, p2 = np.zeros((len(q), 3)), np.zeros((len(q), 3))
        for offset in offsets:
            cells = base + offset
            h = _hash(cells, seed)
            feature = cells + np.stack([_unit(h, 0), _unit(h, 20), _unit(h, 40)], axis=1) * 0.9 + 0.05
            d = np.linalg.norm(q - feature, axis=1)
            value = _unit(h, 11)
            closer = d < b1
            second = ~closer & (d < b2)
            b2 = np.where(closer, b1, np.where(second, d, b2))
            v2 = np.where(closer, v1, np.where(second, value, v2))
            p2 = np.where(closer[:, None], p1, np.where(second[:, None], feature, p2))
            b1 = np.where(closer, d, b1)
            v1 = np.where(closer, value, v1)
            p1 = np.where(closer[:, None], feature, p1)
        s = slice(start, start + len(q))
        f1[s], f2[s], r1[s], r2[s], c1[s], c2[s] = b1 * cell, b2 * cell, v1, v2, p1 * cell, p2 * cell
    return f1, f2, r1, r2, c1, c2


def plates(points, n, seed, cell, thickness, coverage, width, wobble=0.25, bias=None, within=None):
    """Exfoliation shell broken into polygonal plates: each Voronoi cell (``cell`` m)
    of the outer shell has spalled off (removing ``thickness`` m) with probability
    ``coverage`` plus ``bias(feature_points)``; boundaries between spalled and intact
    plates become steep steps ``width`` m wide, with ``wobble`` (fraction of a cell)
    of fracture irregularity. ``within`` (0..1 per point) confines a nested, deeper
    shell to where the outer one is gone. Returns (depth_m, spalled 0..1)."""
    warp = np.stack([Noise(seed + k).fbm(points / (cell * 0.45), 3) for k in (1, 2, 3)], axis=1)
    warped = points + warp * (wobble * cell)
    f1, f2, r1, r2, c1, c2 = voronoi(warped, cell, seed)
    k1 = np.full(len(points), coverage)
    k2 = np.full(len(points), coverage)
    if bias is not None:
        k1 = k1 + bias(c1)
        k2 = k2 + bias(c2)
    s1 = (r1 < k1).astype(np.float64)
    s2 = (r2 < k2).astype(np.float64)
    t = 0.5 + 0.5 * smoothstep(0.0, width, (f2 - f1) * 0.5)
    spalled = s1 * t + s2 * (1.0 - t)
    if within is not None:
        spalled = spalled * within
    return thickness * spalled, spalled


def sheets(points, n, seed, scale, layers, width, facing=None, facing_weight=0.0, wobble=0.25):
    """Exfoliation: onion-skin shells spalled off in curved steps. ``layers`` are
    ``(threshold, thickness_m)`` on one fBm field (nested, so successive sheets step
    down like the real shells); ``width`` (m) is the horizontal run of each step.
    ``facing`` (unit vector) with ``facing_weight`` makes sheets peel preferentially
    on faces pointing that way. Returns the depth (m) to remove along -N."""
    noise = Noise(seed)
    jitter = Noise(seed + 5)
    facing = unit(facing) if facing is not None else None

    def field(p):
        return noise.fbm(p / scale, 4) + wobble * jitter.fbm(p / (scale * 0.35), 3)

    base = field(points)
    if facing is not None:
        base = base + facing_weight * (n @ facing)
    slope = np.maximum(tangential_gradient(field, points, n, scale * 0.01), 1e-3)
    depth = np.zeros(len(points))
    for threshold, thickness in layers:
        signed = (base - threshold) / slope
        depth += thickness * smoothstep(-width * 0.5, width * 0.5, signed)
    return depth


def pits(points, n, spots):
    """Weathering pans (gnammas) on flat tops: ``spots`` of ``(x, y, radius, depth)``.
    Flat-floored with steep, slightly irregular walls. Returns depth along -N."""
    depth = np.zeros(len(points))
    up = smoothstep(0.55, 0.85, n[:, 2])
    for index, (x, y, radius, deep) in enumerate(spots):
        near = np.hypot(points[:, 0] - x, points[:, 1] - y)
        mask = (near < radius * 1.5) & (n[:, 2] > 0.4)
        if not mask.any():
            continue
        top = points[mask, 2].max()
        wobble = Noise(900 + index).fbm(points * (3.0 / radius), 3) * 0.18
        d3 = np.sqrt(near ** 2 + (np.minimum(points[:, 2] - top, 0) * 2.0) ** 2) / radius + wobble
        depth += deep * (1.0 - smoothstep(0.35, 1.0, d3)) * up
    return depth


def crack(points, normal, distance, width, deep, seed=0, wobble=0.0, wobble_scale=1.0, taper=None):
    """Narrow V groove where a joint plane meets the surface. ``taper`` = (axis, lo, hi)
    fades it out along a world axis. Returns depth along -N."""
    normal = unit(normal)
    s = points @ normal - distance
    if wobble:
        s = s + wobble * Noise(seed).fbm(points * wobble_scale, 3)
    groove = np.exp(-(s / width) ** 2) + 0.35 * np.exp(-(s / (width * 3.0)) ** 2)
    if taper is not None:
        axis, lo, hi = taper
        groove *= smoothstep(lo, hi, points[:, axis])
    return deep * groove


def relief(points, n, seed, bands):
    """Weathered surface relief: ``bands`` of ``(wavelength_m, amplitude_m)`` fBm,
    plus granular pitting where the amplitude is negative-biased. Returns an
    offset along +N (negative carves)."""
    out = np.zeros(len(points))
    for index, (wavelength, amplitude) in enumerate(bands):
        out += amplitude * Noise(seed + 13 * index).fbm(points / wavelength, 3)
    return out


# ------------------------------------------------------------------ finishing

def _select(obj):
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def join(objects, name):
    for other in bpy.context.scene.objects:
        other.select_set(False)
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    if len(objects) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = obj.data.name = name
    return obj


def remesh(kit, obj, voxel):
    mod = obj.modifiers.new("Remesh", "REMESH")
    mod.mode = "VOXEL"
    mod.voxel_size = voxel
    mod.adaptivity = 0.0
    kit.apply_modifiers(obj)
    return obj


def voxel_for(obj, target_tris, levels=1):
    """Voxel size giving about ``target_tris`` triangles after ``levels`` subdivisions."""
    return math.sqrt(2.6 * area(obj) * 4 ** levels / target_tris)


def _smooth_coords(obj, iterations=12, factor=0.6):
    """Laplacian-smoothed vertex positions (same topology), for noise-free UV projection."""
    p = coords(obj)
    edges = np.empty(len(obj.data.edges) * 2, dtype=np.int64)
    obj.data.edges.foreach_get("vertices", edges)
    edges = edges.reshape(-1, 2)
    count = np.bincount(edges.ravel(), minlength=len(p)).astype(np.float64)[:, None]
    for _ in range(iterations):
        acc = np.zeros_like(p)
        np.add.at(acc, edges[:, 0], p[edges[:, 1]])
        np.add.at(acc, edges[:, 1], p[edges[:, 0]])
        p = p + factor * (acc / np.maximum(count, 1) - p)
    return p


def _shrink_buried_islands(obj, below_z, factor):
    """Scale UV islands lying entirely below ``below_z`` (the buried base) by ``factor``
    so the visible rock gets the texture resolution."""
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv = bm.loops.layers.uv.active
    bm.faces.ensure_lookup_table()
    seen = set()
    for face in bm.faces:
        if face.index in seen:
            continue
        island, stack = [], [face]
        seen.add(face.index)
        while stack:
            f = stack.pop()
            island.append(f)
            for edge in f.edges:
                if edge.seam:
                    continue
                for other in edge.link_faces:
                    if other.index not in seen:
                        seen.add(other.index)
                        stack.append(other)
        zs = [v.co.z for f in island for v in f.verts]
        if sum(zs) / len(zs) > below_z:
            continue
        loops = [loop for f in island for loop in f.loops]
        cu = sum(l[uv].uv.x for l in loops) / len(loops)
        cv = sum(l[uv].uv.y for l in loops) / len(loops)
        for l in loops:
            l[uv].uv.x = cu + (l[uv].uv.x - cu) * factor
            l[uv].uv.y = cv + (l[uv].uv.y - cv) * factor
    bm.to_mesh(mesh)
    bm.free()


def unwrap(obj, angle=62.0, margin=0.003, method="ANGLE_BASED", buried=None):
    """Few, large, low-stretch islands: smart-project a smoothed copy of the surface to
    find islands (the fine noise would shatter them), turn their borders into seams,
    relax each island with ``method``, equalise texel density and pack.
    ``buried=(z, factor)`` shrinks islands entirely below z (the sunken base)."""
    rest = coords(obj)
    set_coords(obj, _smooth_coords(obj))
    _select(obj)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=margin,
                             scale_to_bounds=False)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.seams_from_islands(mark_seams=True)
    bpy.ops.uv.unwrap(method=method, margin=margin)
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode="OBJECT")
    set_coords(obj, rest)
    if buried is not None:
        _shrink_buried_islands(obj, *buried)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def displace(obj, fn):
    """Move vertices along their normals by ``fn(P, N)``, which returns meters or
    ``(meters, {attribute: values})``; the attributes (e.g. ``fresh`` for newly
    spalled surfaces) are stored as float point attributes for the material."""
    p = coords(obj)
    n = normals(obj)
    result = fn(p, n)
    attributes = {}
    if isinstance(result, tuple):
        result, attributes = result
    set_coords(obj, p + n * np.asarray(result)[:, None])
    for name, values in attributes.items():
        attr = obj.data.attributes.get(name) or obj.data.attributes.new(name, "FLOAT", "POINT")
        attr.data.foreach_set("value", np.asarray(values, dtype=np.float32))
    return obj


def place(obj, ground_z):
    """Centre XY on the origin and put the authored ground line ``ground_z`` at z = 0.
    Returns the sink depth (m): how far the stone extends below the origin."""
    p = coords(obj)
    lo, hi = p.min(axis=0), p.max(axis=0)
    shift = np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, ground_z])
    set_coords(obj, p - shift)
    return float(ground_z - lo[2])


def lod(kit, obj, name, ratio):
    copy = obj.copy()
    copy.data = obj.data.copy()
    copy.name = copy.data.name = name
    bpy.context.scene.collection.objects.link(copy)
    mod = copy.modifiers.new("Decimate", "DECIMATE")
    mod.decimate_type = "COLLAPSE"
    mod.ratio = ratio
    mod.use_collapse_triangulate = True
    kit.apply_modifiers(copy)
    return copy


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def scatter(prefix, stones, material, subdivisions=6):
    """One solid per stone spec (dict with center, radii, seed and optional power,
    lumps, lump_scale, rotate=(yaw, pitch, roll), joints, bend, subdivisions), for
    clusters that ``finish`` unions into a single mesh."""
    objects = []
    for index, spec in enumerate(stones):
        field = boulder(spec["radii"], power=spec.get("power", 2.3), lumps=spec.get("lumps", 0.06),
                        lump_scale=spec.get("lump_scale", 1.3), seed=spec["seed"], center=spec["center"],
                        rotate=rotation(*spec.get("rotate", (0, 0, 0))), joints=spec.get("joints", ()),
                        bend=spec.get("bend", 0.0))
        objects.append(solid(f"{prefix}{index}", field, subdivisions=spec.get("subdivisions", subdivisions),
                             material=material, r_max=max(spec["radii"]) * 3.0 + 0.05))
    return objects


def finish(kit, pieces, name, lod0_tris, lod_tris=(), ground_z=0.0, detail=None, levels=1,
           unwrap_method="ANGLE_BASED", uv_angle=62.0, smooth_angle=180.0, union=True):
    """Union ``pieces`` (voxel remesh sized for ``lod0_tris``), unwrap, subdivide,
    apply ``detail(P, N) -> offset`` relief, place the ground line at the origin and
    build decimated LODs that share LOD0's UVs. ``union=False`` remeshes each piece on
    its own before joining, so touching stones in a cluster stay separate instead of
    fusing. Returns ([LOD0, LOD1, ...], sink_m)."""
    if union:
        obj = join(pieces, name)
        remesh(kit, obj, voxel_for(obj, lod0_tris, levels))
    else:
        total_area = sum(area(piece) for piece in pieces)
        voxel = math.sqrt(2.6 * total_area * 4 ** levels / lod0_tris)
        for piece in pieces:
            remesh(kit, piece, voxel)
        obj = join(pieces, name)
    unwrap(obj, angle=uv_angle, method=unwrap_method, buried=(ground_z - 0.03, 0.3))
    if levels:
        kit.subdivide(obj, levels=levels)
    if detail is not None:
        displace(obj, detail)
    sink = place(obj, ground_z)
    kit.tag_coords(obj.data)
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle)
    meshes = [obj]
    total = triangles(obj)
    for index, target in enumerate(lod_tris, start=1):
        meshes.append(kit.finalize(lod(kit, obj, f"{name}_LOD{index}", min(1.0, target / total)),
                                   pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle))
    return meshes, sink
