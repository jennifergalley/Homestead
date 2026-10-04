"""Check held original meal topology/scale, never its artistic acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderPreparedPotatoTests.py
Open the exported PreparedPotatoesSource blend first.
"""
import math

import bmesh
import bpy

EXPECTED = {
    "SM_BakedPotatoes": ((.23, .234), (.23, .234), (.060, .080)),
    "SM_BakedPotatoesPortion": ((.050, .058), (.075, .080), (.023, .032)),
}


def main() -> None:
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
        if name == "SM_BakedPotatoes":
            assert obj["food_potato_units"] == 2 and obj["food_role"] == "serving"
        else:
            assert obj["food_role"] == "handheld_portion"
        print("ORIGINAL_PREPARED_POTATO_SOURCE_PASS", name, triangles, "closed/finite/UV/pivot/scale")


if __name__ == "__main__":
    main()
