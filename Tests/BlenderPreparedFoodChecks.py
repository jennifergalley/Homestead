"""Shared closed prepared-food source fixtures, not artistic or engine acceptance.

Usage: from BlenderPreparedFoodChecks import check_source_mesh
"""
import hashlib
import math
from pathlib import Path

import bmesh
import bpy

PIPELINE = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"


def check_source_mesh(name: str, item: str, limits: tuple, components: int,
                      triangle_limits: tuple, material_scales: dict) -> bpy.types.Object:
    shader_hash = hashlib.sha256((PIPELINE / "homestead_food_materials.py").read_bytes()).hexdigest()
    geometry_hash = hashlib.sha256((PIPELINE / "homestead_food_geometry.py").read_bytes()).hexdigest()
    obj = bpy.data.objects.get(name)
    assert obj is not None and obj["food_original"] and obj["food_source_only"]
    assert obj["food_item"] == item
    assert obj["food_geometry_source_sha256"] == geometry_hash
    assert obj.location.length < 1e-7
    assert all(abs(value) < 1e-7 for value in obj.rotation_euler)
    assert all(abs(value - 1) < 1e-7 for value in obj.scale)
    assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co)
    coordinates = obj.data.attributes.get("pcoord")
    assert coordinates is not None and coordinates.data_type == "FLOAT_VECTOR" and coordinates.domain == "POINT"
    assert len(coordinates.data) == len(obj.data.vertices)
    assert all(math.isfinite(value) for entry in coordinates.data for value in entry.vector)
    assert abs(min(vertex.co.z for vertex in obj.data.vertices)) < .00001
    for axis, (minimum, maximum) in enumerate(limits):
        extent = max(vertex.co[axis] for vertex in obj.data.vertices) - min(
            vertex.co[axis] for vertex in obj.data.vertices)
        assert minimum <= extent <= maximum, (name, axis, extent)
    triangles = sum(len(face.vertices) - 2 for face in obj.data.polygons)
    assert triangle_limits[0] <= triangles <= triangle_limits[1], (name, triangles)
    assert obj.data.uv_layers.active.name == "UVMap"
    assert all(math.isfinite(value) and -.00001 <= value <= 1.00001
               for entry in obj.data.uv_layers.active.data for value in entry.uv)
    mesh = bmesh.new()
    try:
        mesh.from_mesh(obj.data)
        mesh.verts.ensure_lookup_table()
        assert all(len(edge.link_faces) == 2 for edge in mesh.edges)
        assert all(face.calc_area() > 1e-14 for face in mesh.faces)
        remaining, count = set(mesh.faces), 0
        while remaining:
            pending, component = [remaining.pop()], []
            while pending:
                face = pending.pop()
                component.append(face)
                for edge in face.edges:
                    for neighbour in edge.link_faces:
                        if neighbour in remaining:
                            remaining.remove(neighbour)
                            pending.append(neighbour)
            volume = 0.0
            for face in component:
                first = face.verts[0].co
                for index in range(1, len(face.verts) - 1):
                    volume += first.dot(face.verts[index].co.cross(face.verts[index + 1].co)) / 6
            assert volume > 1e-12, name + " has a collapsed/inverted component"
            indices = {vertex.index for face in component for vertex in face.verts}
            for axis in range(3):
                span = max(coordinates.data[index].vector[axis] for index in indices) - min(
                    coordinates.data[index].vector[axis] for index in indices)
                assert span > 1e-8, name + " has collapsed part-local shader coordinates"
            count += 1
        assert count == components, (name, count)
    finally:
        mesh.free()
    for slot in obj.material_slots:
        material = slot.material
        assert material.get("food_shader_source_sha256") == shader_hash
        assert not any(node.type == "TEX_IMAGE" for node in material.node_tree.nodes)
        matches = [scales for prefix, scales in material_scales.items() if material.name.startswith(prefix)]
        assert len(matches) == 1, "Unclassified original food material: " + material.name
        scales = {node.inputs["Scale"].default_value for node in material.node_tree.nodes if node.type == "TEX_NOISE"}
        assert scales == matches[0], "Stale food shader graph"
    print("ORIGINAL_PREPARED_FOOD_SOURCE_PASS", name, triangles, "closed/finite/UV/pivot/scale/current-shader")
    return obj
