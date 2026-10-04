"""Original prepared-food geometry tools, never open fish-sheet normal repair.

Usage: import homestead_food_geometry as shapes; shapes.closed_normals(obj)
Recipes reload this dependency explicitly in live Blender.
"""
import hashlib
import math
from pathlib import Path

import bmesh
from mathutils import Vector
from mathutils.bvhtree import BVHTree

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
