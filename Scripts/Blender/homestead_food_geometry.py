"""Original prepared-food geometry tools, never open fish-sheet normal repair.

Usage: import homestead_food_geometry as shapes; shapes.closed_normals(obj)
Recipes reload this dependency explicitly in live Blender.
"""
import hashlib
import math
import random
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector, noise
from mathutils.bvhtree import BVHTree
from mathutils.geometry import delaunay_2d_cdt

SOURCE_SHA256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()


def closed_normals(obj) -> None:
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        if any(len(edge.link_faces) != 2 for edge in mesh.edges):
            raise ValueError(obj.name + " is not a closed food component")
        bmesh.ops.recalc_face_normals(mesh, faces=list(mesh.faces))
        if mesh.calc_volume(signed=True) < 0:
            bmesh.ops.reverse_faces(mesh, faces=list(mesh.faces))
        if mesh.calc_volume(signed=True) <= 0:
            raise ValueError(obj.name + " has no positive closed volume")
        mesh.to_mesh(obj.data)
    finally:
        mesh.free()
    obj.data.update()


def vessel(kit, name: str, profile: tuple, material, sides: int = 160):
    vertices, rows, faces = [], [], []
    maximum_radius = max(radius for radius, _ in profile)
    for radius, height in profile:
        row = []
        for side in range(sides if radius else 1):
            theta = side * 2 * math.pi / sides
            wobble = .00016 * math.sin(theta * 9 + .71) * (radius / maximum_radius) ** 5
            row.append(len(vertices))
            vertices.append(Vector((radius * math.cos(theta), radius * math.sin(theta), height + wobble)))
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(sides):
            following = (side + 1) % sides
            if len(first) == 1:
                faces.append((first[0], second[following], second[side]))
            elif len(second) == 1:
                faces.append((first[side], first[following], second[0]))
            else:
                faces.append((first[side], first[following], second[following], second[side]))
    obj = kit.mesh(name, vertices, faces, material)
    closed_normals(obj)
    return obj


def world_surface(obj):
    rotation = obj.rotation_euler.to_matrix()
    vertices = [rotation @ vertex.co + obj.location for vertex in obj.data.vertices]
    return BVHTree.FromPolygons(vertices, [tuple(face.vertices) for face in obj.data.polygons])


def carved_spoon(kit, name: str, material, length_m: float = .182):
    """Original elliptical hollow/shoulder construction, scaled before coordinates/UVs."""
    if not .12 <= length_m <= .22:
        raise ValueError("Eating spoon length must remain within the authored 12-22cm range")
    scale = length_m / .182
    sides = 64
    profile = []
    for step in range(41):
        y = -.105 + .055 * step / 40
        relative = (y + .0775) / .0275
        fullness = max(0, 1 - relative * relative)
        width = .0006 + .0134 * math.sqrt(fullness) + .0039 * (step / 40) ** 8
        if step / 40 > .72:
            blend = (step / 40 - .72) / .28
            blend = blend * blend * (3 - 2 * blend)
            shoulder = .0006 + .0134 * math.sqrt(1 - .44 ** 2) + .0039 * .72 ** 8
            width = shoulder * (1 - blend) + .0045 * blend
        profile.append((y, width, .009, .0048 * fullness))
    profile += [(-.047, .0047, .0095, 0), (-.042, .0046, .010, 0)]
    profile += [(y, .0045 + .0003 * math.sin(y * 38), .010, 0)
                for y in (.0, .02, .04, .06, .074, .077)]
    vertices, rows, faces = [], [], []
    for y, width, rim, hollow in profile:
        row = []
        for side in range(sides):
            theta = side * 2 * math.pi / sides
            if y <= -.05:
                depth = (hollow * math.sin(theta) ** 2 if math.sin(theta) >= 0 else
                         (hollow + .0015) * (-math.sin(theta)) ** .8)
                z = rim - depth
            else:
                z = rim + .0018 * math.sin(theta)
            row.append(len(vertices))
            vertices.append(Vector((width * math.cos(theta), y, z)) * scale)
        rows.append(row)
    for first, second in zip(rows, rows[1:]):
        for side in range(sides):
            following = (side + 1) % sides
            faces.append((first[side], second[side], second[following], first[following]))
    faces.extend((tuple(reversed(rows[0])), tuple(rows[-1])))
    obj = kit.mesh(name, vertices, faces, material)
    closed_normals(obj)
    return obj


def cut_food_patch(vertices: list, boundary: list, seed: int, normal_axis: int = 2) -> list:
    """Seal a ragged food section with constrained, nonradial tissue triangles."""
    axes = [axis for axis in range(3) if axis != normal_axis]
    outline = [Vector((vertices[index][axes[0]], vertices[index][axes[1]])) for index in boundary]
    points = list(outline)
    xs, ys = [point.x for point in outline], [point.y for point in outline]
    rng = random.Random(seed)
    for row in range(1, 11):
        for column in range(1, 11):
            x = min(xs) + (max(xs) - min(xs)) * (column + rng.uniform(-.15, .15)) / 11
            y = min(ys) + (max(ys) - min(ys)) * (row + rng.uniform(-.15, .15)) / 11
            inside = False
            for a, b in zip(outline, outline[1:] + outline[:1]):
                if (a.y > y) != (b.y > y) and x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x:
                    inside = not inside
            if inside:
                points.append(Vector((x, y)))
    count = len(boundary)
    cap_points, _, cap_faces, origins, _, _ = delaunay_2d_cdt(
        points, [(i, (i + 1) % count) for i in range(count)],
        [tuple(range(count))], 1, 1e-8)
    height = sum(vertices[index][normal_axis] for index in boundary) / count
    indices = []
    for point, source in zip(cap_points, origins):
        existing = next((i for i in source if i < count), None)
        if existing is not None:
            indices.append(boundary[existing])
        else:
            indices.append(len(vertices))
            relief = .00014 * noise.noise(Vector((point.x * 2600, point.y * 2600, seed)))
            vertex = Vector()
            vertex[axes[0]], vertex[axes[1]], vertex[normal_axis] = point.x, point.y, height + relief
            vertices.append(vertex)
    return [tuple(indices[i] for i in face) for face in cap_faces]


def seat_on_surfaces(obj, x: float, y: float, supports: list, maximum_radius: float) -> None:
    rotation = obj.rotation_euler.to_matrix()
    required_height = None
    for vertex in obj.data.vertices:
        point = rotation @ vertex.co
        px, py = point.x + x, point.y + y
        if math.hypot(px, py) > maximum_radius:
            raise ValueError(obj.name + " extends beyond the bowl's inner aperture")
        supported = False
        for surface in supports:
            hit, _, _, _ = surface.ray_cast(Vector((px, py, 1)), Vector((0, 0, -1)), 1.0)
            if hit is not None:
                supported = True
                offset = hit.z - point.z + .00005
                required_height = offset if required_height is None else max(required_height, offset)
        if not supported:
            raise ValueError(obj.name + " has a vertex outside its authored support surfaces")
    obj.location = (x, y, required_height)


def cooked_muscle_fold(x: float) -> float:
    return .42 * (math.sqrt(x * x + .000003) - math.sqrt(.000003)) + .0008 * math.sin(x * 190)


def cooked_fillet(kit, name: str, index: int, length: float, width: float,
                  height: float, rows: int, flesh_material: bpy.types.Material,
                  skin_material: bpy.types.Material | None,
                  seed: int, whole_fillet: bool, sides: int = 80,
                  spacing_m: float = .0095, depth_m: float = .00065) -> bpy.types.Object:
    """Closed cooked muscle/skin loft; fresh material/profile per meal, no mesh reuse."""
    if (any(not math.isfinite(value) or value <= 0 for value in (length, width, height, spacing_m))
            or not math.isfinite(depth_m) or depth_m < 0 or rows < 4 or sides < 12):
        raise ValueError("Cooked fish dimensions and sampling must support a finite closed loft")
    vertices, rings, faces, skin_flags = [], [], [], []
    rng = random.Random(seed + index)
    partitions = []
    for partition in range(-20, 21):
        partitions.append((partition * spacing_m + rng.uniform(-.0012, .0012),
                           rng.uniform(.60, 1.0)))
    positions = {row / rows for row in range(rows + 1)}
    for centre, _ in partitions:
        for offset in (-.0013, -.00052, 0, .00052, .0013):
            t = (centre + offset + length * .5) / length
            if 0 < t < 1:
                positions.add(t)
    for t in sorted(positions):
        ring = []
        fullness = ((.27 + .73 * math.sin(.35 + t * math.pi * .81) ** .6) * (1 - .65 * t)
                    if whole_fillet else .80 + .20 * math.sin(t * math.pi))
        for side in range(sides):
            theta = side * 2 * math.pi / sides
            c, s = math.cos(theta), math.sin(theta)
            x = width * .5 * fullness * c
            x += .00065 * math.sin(t * 11 + index) * abs(s)
            fold = cooked_muscle_fold(x)
            y = length * (t - .5) - fold
            top = max(0, s) ** .72
            bottom = max(0, -s) ** .80
            z = height * (.70 * top * (1 + .15 * c) - .30 * bottom) * (.7 + .3 * math.sin(t * math.pi))
            point = Vector((x, y, z))
            centre, depth = min(partitions, key=lambda entry: abs(y + fold - entry[0]))
            distance = abs(y + fold - centre)
            groove = depth * math.exp(-(distance / .00052) ** 2)
            point.z -= top * min(depth_m, height * .12) * groove
            point.z += top * .00018 * noise.noise(point * 930 + Vector((index, 4.9, 7.1)))
            point.z += bottom * .00022 * noise.noise(point * 440 + Vector((index, 8.1, 3.9)))
            ring.append(len(vertices))
            vertices.append(point)
        rings.append(ring)
    for first, second in zip(rings, rings[1:]):
        for side in range(sides):
            following = (side + 1) % sides
            faces.append((first[side], first[following], second[following], second[side]))
            angle = (side + .5) * 2 * math.pi / sides
            skin_flags.append(skin_material is not None and
                              (math.sin(angle) < .06 or math.cos(angle) < -.76))
    for cap, boundary in enumerate((rings[0], rings[-1])):
        first_new = len(vertices)
        faces += cut_food_patch(vertices, boundary, seed + index * 2 + cap, normal_axis=1)
        mean_fold = sum(cooked_muscle_fold(vertices[i].x) for i in boundary) / len(boundary)
        for point in vertices[first_new:]:
            point.y += mean_fold - cooked_muscle_fold(point.x)
    obj = kit.mesh(name, vertices, faces, flesh_material)
    if skin_material is not None:
        obj.data.materials.append(skin_material)
        for face, is_skin in zip(obj.data.polygons, skin_flags):
            face.material_index = int(is_skin)
    closed_normals(obj)
    bevel = obj.modifiers.new("CookedFishSoftCutEdges", "BEVEL")
    bevel.width, bevel.segments, bevel.limit_method, bevel.angle_limit = .00035, 3, "ANGLE", .65
    kit.apply_modifiers(obj)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        bmesh.ops.dissolve_degenerate(mesh, dist=1e-8, edges=list(mesh.edges))
        mesh.to_mesh(obj.data)
    finally:
        mesh.free()
    closed_normals(obj)
    return obj
