"""Masonry geometry for the stone building kit: split-faced rubble stones (``pillow``),
dressed blocks and timbers (``block``), roofing slates (``plate``) and plain mortar
cores (``box``), accumulated as raw polygons and built into one mesh.

Coordinates are Unreal local space in meters (see ``__init__``); ``Masonry.build``
mirrors Y for Blender. Every stone carries a random ``stone_offset`` (shifts the
procedural patterns so neighbouring stones don't continue each other's markings) and a
``stone_tint`` (per-stone colour variation), read by ``homestead_materials.granite``.
"""
import math

import bmesh
import bpy
import numpy as np

import homestead_rocks as rocks

CM = 0.01
Z = np.array([0.0, 0.0, 1.0])


def unit(v):
    v = np.asarray(v, dtype=np.float64)
    return v / np.linalg.norm(v)


def cm(*values):
    return np.array(values, dtype=np.float64) * CM


class Masonry:
    """Accumulates polygons (Unreal frame, meters) with per-vertex pcoord/offset/tint."""

    def __init__(self, seed, pcoord_shift=(0.0, 0.0, 0.0), tint_mean=1.0):
        self.tint_mean = tint_mean
        self.rng = np.random.default_rng(seed)
        self.noise = rocks.Noise(seed + 101)
        self.pcoord_shift = np.asarray(pcoord_shift, dtype=np.float64)
        self.verts, self.pcoords, self.offsets, self.tints, self.extras = [], [], [], [], {}
        self.faces, self.face_mats = [], []
        self.count = 0

    # ------------------------------------------------------------------ bookkeeping
    def identity(self, variation=0.12):
        """A stone's pattern offset and colour tint (warm/cool and light/dark)."""
        offset = self.rng.uniform(-40.0, 40.0, 3)
        warm = self.rng.normal(0.0, 1.0)
        light = self.tint_mean * float(np.clip(self.rng.normal(1.0, variation), 0.72, 1.3))
        return offset, light * np.array([1 + 0.035 * warm, 1 + 0.008 * warm, 1 - 0.04 * warm])

    def add(self, verts, faces, mat, offset=None, tint=None, pcoord=None, extra=None):
        verts = np.asarray(verts, dtype=np.float64)
        n = len(verts)
        base = self.count
        self.verts.append(verts)
        self.pcoords.append(verts + self.pcoord_shift if pcoord is None else np.asarray(pcoord))
        self.offsets.append(np.tile(np.zeros(3) if offset is None else offset, (n, 1)))
        self.tints.append(np.tile(np.ones(3) if tint is None else tint, (n, 1)))
        for name in set(self.extras) | set(extra or {}):
            values = (extra or {}).get(name)
            if name not in self.extras:
                self.extras[name] = [np.zeros(c) for c in (len(v) for v in self.verts[:-1])]
            self.extras[name].append(np.zeros(n) if values is None else np.broadcast_to(values, (n,)).copy())
        self.faces.extend(tuple(base + i for i in face) for face in faces)
        self.face_mats.extend([mat] * len(faces))
        self.count += n

    # ------------------------------------------------------------------ primitives
    def pillow(self, center, normal, up, hu, hv, mat=0, bulge=0.010, max_out=0.015, skirt=0.035,
               roll=0.005, relief=0.009, exponent=None, recess=(-0.010, 0.0), tilt=0.035,
               identity=None, cuts=(0, 2), sides=None, relief_scale=5.0):
        """A split-faced stone showing one face: a squarish outline ``hu`` x ``hv`` (half
        sizes, m) trimmed by 1-3 straight spall cuts and sheared a little, on the plane through
        ``center`` facing ``normal``; the face is domed by ``bulge`` with fBm relief and a random
        tilt, clamped to ``max_out`` in front of the plane, with a narrow rounded arris
        (``roll``) and a skirt running ``skirt`` deep into the mortar."""
        rng, noise = self.rng, self.noise
        n = unit(normal)
        u = unit(np.cross(up, n))
        v = np.cross(n, u)
        e = exponent or rng.uniform(6.0, 12.0)
        perimeter = 4.0 * (hu + hv)
        m = sides or int(np.clip(perimeter / 0.085, 9, 16))
        theta = np.linspace(0.0, 2 * math.pi, m, endpoint=False) + rng.uniform(0, 2 * math.pi)
        c, s = np.cos(theta), np.sin(theta)
        nx = np.sign(c) * np.abs(c) ** (2.0 / e)
        ny = np.sign(s) * np.abs(s) ** (2.0 / e)
        key = rng.uniform(-50, 50, 2)
        jitter = noise.fbm(np.stack([c * 1.1 + key[0], s * 1.1 + key[1], np.full(m, key[0])], axis=1), 3)
        scale = 1.0 + np.clip(0.06 * jitter, -0.07, 0.01)
        nx, ny = nx * scale, ny * scale
        # Straight spall cuts: project everything beyond a random chord back onto it.
        for _ in range(rng.integers(cuts[0], cuts[1] + 1)):
            phi = rng.uniform(0, 2 * math.pi)
            limit = rng.uniform(0.86, 0.97)
            along = nx * math.cos(phi) + ny * math.sin(phi)
            factor = np.where(along > limit, limit / np.maximum(along, 1e-6), 1.0)
            nx, ny = nx * factor, ny * factor
        # Re-fill the cell after the cuts so joints stay narrow.
        nx = nx / max(np.abs(nx).max(), 1e-6) * 0.99
        ny = ny / max(np.abs(ny).max(), 1e-6) * 0.99
        shear = rng.normal(0.0, 0.06)
        ox = (nx + shear * ny) * hu / (1.0 + abs(shear))
        oy = ny * hv
        tilt_u, tilt_v = rng.normal(0.0, tilt, 2)
        base = rng.uniform(*recess)

        def face_depth(x, y):
            points = center + np.outer(x, u) + np.outer(y, v)
            return base + tilt_u * x + tilt_v * y + relief * noise.fbm(points * relief_scale + key[0], 3)

        rings = []
        for t, k in ((0.975, 0.1), (0.78, 0.6), (0.42, 0.92)):
            depth = np.minimum(face_depth(ox * t, oy * t) + bulge * k, max_out)
            rings.append((t, depth))
        arris = rings[0][1] - roll
        rings = [(1.02, np.minimum(np.full(m, -skirt), arris - 0.01)), (1.0, arris)] + rings
        center_depth = min(float(face_depth(np.zeros(1), np.zeros(1))[0] + bulge), max_out)
        pts = [center + np.outer(ox * t, u) + np.outer(oy * t, v) + np.outer(depth, n) for t, depth in rings]
        pts.append((center + n * center_depth)[None, :])
        verts = np.concatenate(pts)
        faces = []
        for r in range(len(rings) - 1):
            a, b = r * m, (r + 1) * m
            for k in range(m):
                k1 = (k + 1) % m
                faces.append((a + k, a + k1, b + k1, b + k))
        last, tip = (len(rings) - 1) * m, len(rings) * m
        for k in range(m):
            faces.append((last + k, last + (k + 1) % m, tip))
        offset, tint = identity or self.identity()
        self.add(verts, faces, mat, offset, tint)

    def block(self, lo, hi, mat=0, radius=0.02, relief=0.003, step=0.14, skip=(), identity=None,
              pcoord_axis=None, relief_scale=8.0, bulge=0.0):
        """Rounded box between corners ``lo``/``hi`` (dressed quoins, lintels, copings, timbers).
        ``skip`` lists hidden faces as ('x'|'y'|'z', -1|1). ``pcoord_axis`` gives the block a
        local pcoord with that axis along Z (wood grain along a beam); default object coords.
        ``bulge`` domes the faces (pitch-faced stone)."""
        lo, hi = np.asarray(lo, dtype=np.float64), np.asarray(hi, dtype=np.float64)
        center, half = (lo + hi) / 2, (hi - lo) / 2
        r = min(radius, 0.45 * float(half.min()))
        axes = []
        for a in range(3):
            inner = half[a] - r
            count = max(1, int(round(2 * inner / step)))
            axes.append(np.concatenate([[-half[a]], np.linspace(-inner, inner, count + 1), [half[a]]]))
        index, verts = {}, []

        def vertex(i, j, k):
            key = (i, j, k)
            if key not in index:
                index[key] = len(verts)
                verts.append((axes[0][i], axes[1][j], axes[2][k]))
            return index[key]

        faces = []
        names = "xyz"
        for a in range(3):
            b, c = (a + 1) % 3, (a + 2) % 3
            for sign in (-1, 1):
                if (names[a], sign) in skip:
                    continue
                fixed = 0 if sign < 0 else len(axes[a]) - 1
                for i in range(len(axes[b]) - 1):
                    for j in range(len(axes[c]) - 1):
                        quad = []
                        for di, dj in ((0, 0), (1, 0), (1, 1), (0, 1)):
                            ijk = [0, 0, 0]
                            ijk[a], ijk[b], ijk[c] = fixed, i + di, j + dj
                            quad.append(vertex(*ijk))
                        faces.append(tuple(quad) if sign > 0 else tuple(reversed(quad)))
        q = np.array(verts)
        inner = half - r
        core = np.clip(q, -inner, inner)
        d = q - core
        length = np.linalg.norm(d, axis=1, keepdims=True)
        normal = d / np.maximum(length, 1e-9)
        p = core + normal * r
        if bulge:
            # Dome each face: strongest at its middle, zero on the rounded arrises.
            span = np.clip(1.0 - (np.abs(core) / np.maximum(inner, 1e-6)) ** 2, 0.0, 1.0)
            dome = np.ones(len(p))
            for a in range(3):
                others = [x for x in range(3) if x != a]
                on_face = np.abs(normal[:, a]) > 0.99
                dome = np.where(on_face, span[:, others[0]] * span[:, others[1]], dome)
            p = p + normal * (bulge * np.where(np.abs(normal).max(axis=1) > 0.99, dome, 0.0))[:, None]
        world = center + p
        key = self.rng.uniform(-50, 50)
        p = p + normal * (relief * self.noise.fbm(world * relief_scale + key, 3))[:, None]
        world = center + p
        offset, tint = identity or self.identity()
        pcoord = None
        if pcoord_axis is not None:
            order = [x for x in range(3) if x != pcoord_axis] + [pcoord_axis]
            pcoord = p[:, order] + self.rng.uniform(-3, 3, 3)
        self.add(world, faces, mat, offset, tint, pcoord=pcoord)

    def box(self, lo, hi, mat, skip=()):
        """Plain box (mortar cores): six quads minus ``skip``."""
        lo, hi = np.asarray(lo, dtype=np.float64), np.asarray(hi, dtype=np.float64)
        corners = np.array([[x, y, z] for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])])
        quads = {("x", -1): (0, 1, 3, 2), ("x", 1): (4, 6, 7, 5), ("y", -1): (0, 4, 5, 1),
                 ("y", 1): (2, 3, 7, 6), ("z", -1): (0, 2, 6, 4), ("z", 1): (1, 5, 7, 3)}
        self.add(corners, [q for k, q in quads.items() if k not in skip], mat)

    def plate(self, frame, width, length, thickness, mat=0, sides=12, relief=0.002, identity=None,
              extra=None, pcoord_scale=1.0):
        """A roofing slate: an irregular rounded-rectangle slab ``width`` (local x) by ``length``
        (local y, butt at y=0, tail at y=length) by ``thickness`` (local z, bottom at 0).
        ``frame(points)`` maps local points to the Unreal frame (tilt and place)."""
        rng, noise = self.rng, self.noise
        theta = np.linspace(0.0, 2 * math.pi, sides, endpoint=False) + math.pi / sides
        c, s = np.cos(theta), np.sin(theta)
        # Squared slates with small dressed-off corners.
        e = rng.uniform(9.0, 15.0)
        hx, hy = width / 2, length / 2
        ox = hx * np.sign(c) * np.abs(c) ** (2.0 / e)
        oy = hy * np.sign(s) * np.abs(s) ** (2.0 / e)
        key = rng.uniform(-50, 50, 2)
        scale = 1.0 + np.clip(0.06 * noise.fbm(np.stack([c + key[0], s + key[1], np.full(sides, key[1])], 1), 3),
                              -0.07, 0.02)
        # Nibbled edges where the slater dressed them with the hammer.
        scale = scale - 0.014 * np.abs(noise.fbm(np.stack([c * 5 + key[1], s * 5 + key[0], np.full(sides, 3.0)], 1), 2))
        ox, oy = ox * scale, oy * scale + hy
        t = thickness * (1.0 + 0.15 * noise.fbm(np.stack([ox * 6 + key[0], oy * 6, np.zeros(sides)], 1), 2))
        top_outer = np.stack([ox, oy, t], 1)
        inner = np.stack([ox * 0.72, hy + (oy - hy) * 0.72, t + relief * noise.fbm(
            np.stack([ox * 9 + key[1], oy * 9, np.ones(sides)], 1), 2)], 1)
        centre = np.array([[0.0, hy, thickness + relief * 0.5]])
        bottom = np.stack([ox, oy, np.zeros(sides)], 1)
        local = np.concatenate([bottom, top_outer, inner, centre, [[0.0, hy, 0.0]]])
        faces = []
        for k in range(sides):
            k1 = (k + 1) % sides
            faces.append((k, k1, sides + k1, sides + k))                          # edge band
            faces.append((sides + k, sides + k1, 2 * sides + k1, 2 * sides + k))  # top rim
            faces.append((2 * sides + k, 2 * sides + k1, 3 * sides))              # top centre
            faces.append((k1, k, 3 * sides + 1))                                  # underside
        offset, tint = identity or self.identity(0.12)
        extra = {name: fn(local) for name, fn in (extra or {}).items()}
        self.add(frame(local), faces, mat, offset, tint, pcoord=local * pcoord_scale, extra=extra)

    # ------------------------------------------------------------------ build
    def build(self, name, materials):
        """One Blender mesh object (Y mirrored into Blender's frame) with pcoord,
        stone_offset and stone_tint point attributes and one slot per material."""
        verts = np.concatenate(self.verts)
        verts[:, 1] *= -1.0
        faces = [tuple(reversed(f)) for f in self.faces]
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts.tolist(), [], faces)
        mesh.update()
        mesh.uv_layers.new(name="UVMap")
        pcoord = np.concatenate(self.pcoords)
        for attr_name, data in (("pcoord", pcoord), ("stone_offset", np.concatenate(self.offsets)),
                                ("stone_tint", np.concatenate(self.tints))):
            attr = mesh.attributes.new(attr_name, "FLOAT_VECTOR", "POINT")
            attr.data.foreach_set("vector", data.astype(np.float32).ravel())
        for attr_name, parts in self.extras.items():
            attr = mesh.attributes.new(attr_name, "FLOAT", "POINT")
            attr.data.foreach_set("value", np.concatenate(parts).astype(np.float32))
        for mat in materials:
            mesh.materials.append(mat)
        mesh.polygons.foreach_set("material_index", np.array(self.face_mats, dtype=np.int32))
        obj = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def unwrap(obj, shrink=None, margin=0.0025, angle=60.0):
    """Smart-project UVs at true scale, shrink the islands of mostly hidden materials
    (``shrink = {material_index: factor}``, e.g. the mortar core behind the stones) so the
    visible stone gets the texels, then pack everything."""
    for other in bpy.context.scene.objects:
        other.select_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=margin, scale_to_bounds=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    if shrink:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        layer = bm.loops.layers.uv.active
        faces = [f for f in bm.faces if f.material_index in shrink]
        # Flood-fill UV-connected islands among these faces and scale each about its centre.
        seen = set()
        for face in faces:
            if face.index in seen:
                continue
            island, stack = [], [face]
            seen.add(face.index)
            while stack:
                f = stack.pop()
                island.append(f)
                for edge in f.edges:
                    for other in edge.link_faces:
                        if other.index in seen or other.material_index != f.material_index:
                            continue
                        uv_a = {tuple(round(c, 6) for c in l[layer].uv) for l in f.loops if l.edge == edge
                                or l.link_loop_next.edge == edge}
                        uv_b = {tuple(round(c, 6) for c in l[layer].uv) for l in other.loops if l.edge == edge
                                or l.link_loop_next.edge == edge}
                        if uv_a & uv_b:
                            seen.add(other.index)
                            stack.append(other)
            factor = shrink[island[0].material_index]
            uvs = [l[layer].uv for f in island for l in f.loops]
            centre = sum((uv.copy() for uv in uvs), uvs[0].copy() * 0) / len(uvs)
            for f in island:
                for loop in f.loops:
                    loop[layer].uv = centre + (loop[layer].uv - centre) * factor
        bm.to_mesh(obj.data)
        bm.free()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(margin=margin, rotate=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def finish(kit, obj, lod_ratios=(0.45, 0.16), smooth_angle=70.0):
    """Finalize LOD0 in place (authored pivot kept) and add decimated LODs sharing its UVs."""
    kit.finalize(obj, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle)
    meshes = [obj]
    for index, ratio in enumerate(lod_ratios, start=1):
        lod = rocks.lod(kit, obj, f"{obj.name}_LOD{index}", ratio)
        meshes.append(kit.finalize(lod, pivot=None, unwrap=False, reshade=True, smooth_angle=smooth_angle))
    return meshes


def triangles(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)
