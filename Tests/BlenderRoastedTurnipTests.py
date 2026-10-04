"""Check held original turnip geometry and executed shaders, not art acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderRoastedTurnipTests.py
Open the exported RoastedTurnipsSource blend first.
"""
import hashlib
import math
from pathlib import Path

import bmesh
import bpy

FOOD_SOURCE = Path(__file__).resolve().parents[1] / "Scripts" / "Blender" / "homestead_food_materials.py"
EXPECTED = {
    "SM_RoastedTurnips": ((.199, .201), (.199, .201), (.025, .065)),
    "SM_RoastedTurnipsPortion": ((.035, .043), (.075, .077), (.031, .037)),
}


def main() -> None:
    shader_hash = hashlib.sha256(FOOD_SOURCE.read_bytes()).hexdigest()
    for name, limits in EXPECTED.items():
        obj = bpy.data.objects.get(name)
        assert obj is not None
        assert obj["food_original"] and obj["food_source_only"]
        assert obj["food_item"] == "RoastedTurnips"
        assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co)
        assert abs(min(v.co.z for v in obj.data.vertices)) < .00001
        for dimension, (minimum, maximum) in enumerate(limits):
            extent = max(v.co[dimension] for v in obj.data.vertices) - min(v.co[dimension] for v in obj.data.vertices)
            assert minimum <= extent <= maximum, (name, dimension, extent)
        triangles = sum(len(face.vertices) - 2 for face in obj.data.polygons)
        assert 4000 <= triangles <= 65000
        assert obj.data.uv_layers.active.name == "UVMap"
        assert all(math.isfinite(value) and -.00001 <= value <= 1.00001
                   for entry in obj.data.uv_layers.active.data for value in entry.uv)
        mesh = bmesh.new()
        try:
            mesh.from_mesh(obj.data)
            assert all(len(edge.link_faces) == 2 for edge in mesh.edges), name + " has an open shell"
            assert all(face.calc_area() > 1e-14 for face in mesh.faces), name + " has collapsed faces"
            assert mesh.calc_volume(signed=True) > 0, name + " has inverted volume"
            remaining = set(mesh.faces)
            components = 0
            while remaining:
                pending = [remaining.pop()]
                component = []
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
                assert volume > 1e-10, name + " has an inverted individual component"
                components += 1
            assert components == (7 if name == "SM_RoastedTurnips" else 1)
        finally:
            mesh.free()
        for slot in obj.material_slots:
            material = slot.material
            assert material.get("food_shader_source_sha256") == shader_hash, "Stale food shader source"
            assert not any(node.type == "TEX_IMAGE" for node in material.node_tree.nodes)
            scales = {node.inputs["Scale"].default_value for node in material.node_tree.nodes if node.type == "TEX_NOISE"}
            assert scales == ({75, 1800} if "Earthenware" in material.name else {35, 140, 1400})
        assert any("Skin" in slot.material.name for slot in obj.material_slots)
        assert any("Flesh" in slot.material.name for slot in obj.material_slots)
        if name == "SM_RoastedTurnips":
            assert obj["food_role"] == "serving" and obj["food_piece_count"] == 6
            assert obj["food_turnip_units"] == 1
        else:
            assert obj["food_role"] == "handheld_portion"
            assert not any("Earthenware" in slot.material.name for slot in obj.material_slots)
        print("ORIGINAL_ROASTED_TURNIP_SOURCE_PASS", name, triangles, "closed/finite/UV/pivot/scale/current-shader")


if __name__ == "__main__":
    main()
