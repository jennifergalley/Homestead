"""Check held original meal topology/scale, never its artistic acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderPreparedPotatoTests.py
Open the exported PreparedPotatoesSource blend first.
"""
import hashlib
import math
from pathlib import Path

import bmesh
import bpy

FOOD_SOURCE = Path(__file__).resolve().parents[1] / "Scripts" / "Blender" / "homestead_food_materials.py"
SKIN_NOISE_SCALES = {170, 65, 1600, 850}
FLESH_NOISE_SCALES = {230, 1300}
EXPECTED = {
    "SM_BakedPotatoes": ((.23, .234), (.23, .234), (.060, .080)),
    "SM_BakedPotatoesPortion": ((.050, .058), (.075, .080), (.023, .032)),
}


def main() -> None:
    shader_hash = hashlib.sha256(FOOD_SOURCE.read_bytes()).hexdigest()
    for name, limits in EXPECTED.items():
        obj = bpy.data.objects.get(name)
        assert obj is not None
        assert obj["food_original"] and obj["food_source_only"]
        assert obj["food_item"] == "BakedPotatoes"
        assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co)
        assert abs(min(vertex.co.z for vertex in obj.data.vertices)) < .00001
        for dimension, (minimum, maximum) in enumerate(limits):
            extent = (max(v.co[dimension] for v in obj.data.vertices)
                      - min(v.co[dimension] for v in obj.data.vertices))
            assert minimum <= extent <= maximum, name + " has wrong serving/portion scale"
        triangles = sum(len(face.vertices) - 2 for face in obj.data.polygons)
        assert 10000 <= triangles <= 65000
        assert obj.data.uv_layers.active.name == "UVMap"
        assert all(math.isfinite(value) and -.00001 <= value <= 1.00001
                   for entry in obj.data.uv_layers.active.data for value in entry.uv)
        mesh = bmesh.new()
        try:
            mesh.from_mesh(obj.data)
            assert all(len(edge.link_faces) == 2 for edge in mesh.edges), name + " has an open shell"
            assert all(face.calc_area() > 1e-14 for face in mesh.faces), name + " has collapsed faces"
            assert mesh.calc_volume(signed=True) > 0, name + " has inverted volume"
        finally:
            mesh.free()
        assert any("Flesh" in slot.material.name for slot in obj.material_slots)
        assert any("Skin" in slot.material.name for slot in obj.material_slots)
        assert not any(node.type == "TEX_IMAGE" for slot in obj.material_slots
                       for node in slot.material.node_tree.nodes), "Meal references external/reused imagery"
        for slot in obj.material_slots:
            material = slot.material
            if "Skin" not in material.name and "Flesh" not in material.name:
                continue
            assert material.get("food_shader_source_sha256") == shader_hash, "Stale food shader source"
            expected_scales = SKIN_NOISE_SCALES if "Skin" in material.name else FLESH_NOISE_SCALES
            actual_scales = {node.inputs["Scale"].default_value for node in material.node_tree.nodes
                             if node.type == "TEX_NOISE"}
            assert actual_scales == expected_scales, "Stale food shader graph"
        if name == "SM_BakedPotatoes":
            assert obj["food_potato_units"] == 2 and obj["food_role"] == "serving"
        else:
            assert obj["food_role"] == "handheld_portion"
        print("ORIGINAL_PREPARED_POTATO_SOURCE_PASS", name, triangles, "closed/finite/UV/pivot/scale")


if __name__ == "__main__":
    main()
