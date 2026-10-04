"""Check held original carrot meal geometry/shader provenance, not acceptance.

Usage: Invoke-BlenderLive.ps1 -File Tests\\BlenderStewedCarrotTests.py
Open the exported StewedCarrotsSource blend first.
"""
import hashlib
import math
from pathlib import Path

import bmesh
import bpy

PIPELINE = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
EXPECTED = {
    "SM_StewedCarrots": ((.183, .185), (.183, .185), (.053, .058)),
    "SM_StewedCarrotsPortion": ((.014, .026), (.045, .047), (.008, .014)),
}


def main() -> None:
    shader_hash = hashlib.sha256((PIPELINE / "homestead_food_materials.py").read_bytes()).hexdigest()
    geometry_hash = hashlib.sha256((PIPELINE / "homestead_food_geometry.py").read_bytes()).hexdigest()
    for name, limits in EXPECTED.items():
        obj = bpy.data.objects.get(name)
        assert obj is not None and obj["food_original"] and obj["food_source_only"]
        assert obj["food_item"] == "StewedCarrots"
        assert obj["food_geometry_source_sha256"] == geometry_hash
        assert all(math.isfinite(value) for vertex in obj.data.vertices for value in vertex.co)
        assert abs(min(v.co.z for v in obj.data.vertices)) < .00001
        for axis, (minimum, maximum) in enumerate(limits):
            extent = max(v.co[axis] for v in obj.data.vertices) - min(v.co[axis] for v in obj.data.vertices)
            assert minimum <= extent <= maximum, (name, axis, extent)
        triangles = sum(len(face.vertices) - 2 for face in obj.data.polygons)
        assert 2500 <= triangles <= 65000
        assert obj.data.uv_layers.active.name == "UVMap"
        assert all(math.isfinite(value) and -.00001 <= value <= 1.00001
                   for entry in obj.data.uv_layers.active.data for value in entry.uv)
        mesh = bmesh.new()
        try:
            mesh.from_mesh(obj.data)
            assert all(len(edge.link_faces) == 2 for edge in mesh.edges)
            assert all(face.calc_area() > 1e-14 for face in mesh.faces)
            remaining, components = set(mesh.faces), 0
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
                assert volume > 1e-10, name + " has an inverted individual component"
                components += 1
            assert components == (14 if name == "SM_StewedCarrots" else 1)
        finally:
            mesh.free()
        for slot in obj.material_slots:
            material = slot.material
            assert material.get("food_shader_source_sha256") == shader_hash
            assert not any(node.type == "TEX_IMAGE" for node in material.node_tree.nodes)
            scales = {node.inputs["Scale"].default_value for node in material.node_tree.nodes if node.type == "TEX_NOISE"}
            expected = ({75, 1800} if "Earthenware" in material.name else
                        {90} if "CookingLiquid" in material.name else {350, 1900})
            assert scales == expected, "Stale food shader graph"
        if name == "SM_StewedCarrots":
            assert obj["food_role"] == "serving" and obj["food_carrot_units"] == 2
            assert obj["food_piece_count"] == 12
        else:
            assert obj["food_role"] == "handheld_portion"
            assert not any("Earthenware" in slot.material.name or "CookingLiquid" in slot.material.name
                           for slot in obj.material_slots)
        print("ORIGINAL_STEWED_CARROT_SOURCE_PASS", name, triangles, "closed/finite/UV/pivot/scale/current-shader")


if __name__ == "__main__":
    main()
