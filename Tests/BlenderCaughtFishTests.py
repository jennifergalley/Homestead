"""Check the six original catch meshes without clearing the visible scene.

Usage: Scripts\\Blender\\Invoke-BlenderLive.ps1 -File Tests\\BlenderCaughtFishTests.py
"""
import hashlib
import importlib.util
import math
import struct
import sys
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

EXPECTED = {
    "RiverTrout": ("Salmo trutta", .34),
    "RiverSalmon": ("Salmo salar", .62),
    "LakePerch": ("Perca fluviatilis", .30),
    "LakeCarp": ("Cyprinus carpio", .42),
    "SeaMackerel": ("Scomber scombrus", .36),
    "SeaBass": ("Dicentrarchus labrax", .46),
}


def check_fin_ray_attachment() -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_kit as kit

    spec = importlib.util.spec_from_file_location("caught_fish_regression", source / "Recipes" / "caught_fish.py")
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    material = bpy.data.materials.new("CaughtFishRegressionFin")
    parts = []
    try:
        base = lambda s: Vector((0, .04 * s, 0))
        edge = lambda s: base(s) + Vector((0, .015, .05))
        parts = recipe.membrane(kit, "CaughtFishRegressionFin", base, edge, 8,
                                material, .34, material)
        for ray, obj in enumerate(parts[1:], 1):
            s = ray / 8
            for ring in range(8):
                v = ring / 7
                center = sum((vertex.co for vertex in obj.data.vertices[ring*12:(ring+1)*12]), Vector()) / 12
                expected = base(s).lerp(edge(s), v) + Vector((
                    .34 * .0025 * math.sin(math.pi * v) * math.sin(math.pi * s), 0, 0))
                assert (center - expected).length < .000002, "Fin ray leaves its bowed membrane"
        print("ORIGINAL_FISH_FIN_ATTACHMENT_PASS 7 rays, 8 stations")
    finally:
        for obj in parts:
            data = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            if data.users == 0:
                bpy.data.meshes.remove(data)
        if material.users == 0:
            bpy.data.materials.remove(material)


def check_jaw_openings() -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_kit as kit

    spec = importlib.util.spec_from_file_location("caught_fish_jaw_regression", source / "Recipes" / "caught_fish.py")
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    skin = bpy.data.materials.new("CaughtFishRegressionSkin")
    cavity = bpy.data.materials.new("CaughtFishRegressionCavity")
    try:
        for fish in recipe.FISH:
            obj = recipe.body(kit, fish, skin, cavity)
            data = obj.data
            try:
                columns = recipe.BODY_SIDES // 2 + 1
                lower_offset = recipe.BODY_RINGS * columns
                for ring in range(recipe.BODY_RINGS):
                    u = .90 * ring / (recipe.BODY_RINGS - 1)
                    for side in range(2):
                        top = data.vertices[ring * columns + (0 if side == 0 else columns - 1)].co
                        bottom = data.vertices[lower_offset + ring * columns + (columns - 1 if side == 0 else 0)].co
                        if u == 0:
                            nominal_gap = fish["length"] * fish["mouth_gap"]
                            assert .70 * nominal_gap <= top.z - bottom.z <= nominal_gap
                        elif u < fish["mouth_end"] * .5:
                            assert top.z > bottom.z, fish["key"] + " has a sealed mouth"
                        elif u >= fish["mouth_end"]:
                            assert (top - bottom).length < .000002, fish["key"] + " has a split cheek/body"
                cavity_faces = [face for face in data.polygons if face.material_index == 1]
                assert cavity_faces, fish["key"] + " lacks a modeled oral cavity"
                assert all(face.area > 0 and all(math.isfinite(c) for c in face.normal)
                           for face in cavity_faces), fish["key"] + " has collapsed cavity faces"
                lining_start = obj["oral_lining_start"]
                lining_sheet = obj["oral_lining_columns"] * obj["oral_lining_rings"]
                roof_faces = [face for face in cavity_faces if all(
                    lining_start <= index < lining_start + lining_sheet for index in face.vertices)]
                floor_faces = [face for face in cavity_faces if all(
                    index >= lining_start + lining_sheet for index in face.vertices)]
                assert roof_faces and floor_faces, fish["key"] + " lacks a full oral vestibule"
                assert all(face.normal.z < 0 for face in roof_faces), fish["key"] + " has inverted oral roof"
                assert all(face.normal.z > 0 for face in floor_faces), fish["key"] + " has inverted oral floor"
                for face in data.polygons[:2 * (recipe.BODY_RINGS - 1) * (columns - 1)]:
                    if .30 * fish["length"] < face.center.y + .5 * fish["length"] < .80 * fish["length"]:
                        radial = Vector((face.center.x, 0, face.center.z))
                        assert face.normal.dot(radial) > 0, fish["key"] + " has inverted skin normals"
                head = BVHTree.FromPolygons([vertex.co for vertex in data.vertices],
                                           [face.vertices[:] for face in data.polygons])
                for side in (-1, 1):
                    eye = recipe.eye(kit, fish, skin, side)
                    eye_data = eye.data
                    try:
                        for vertex, coord in zip(eye_data.vertices, eye_data.attributes["eyecoord"].data):
                            if math.hypot(coord.vector.y, coord.vector.z) > .75:
                                continue
                            nearest, normal, _, _ = head.find_nearest(vertex.co)
                            assert nearest is not None
                            assert (vertex.co - nearest).dot(normal) >= -.00006, fish["key"] + " clips its pupil into the jaw skin"
                    finally:
                        bpy.data.objects.remove(eye, do_unlink=True)
                        if eye_data.users == 0:
                            bpy.data.meshes.remove(eye_data)
            finally:
                bpy.data.objects.remove(obj, do_unlink=True)
                if data.users == 0:
                    bpy.data.meshes.remove(data)
        print("ORIGINAL_FISH_JAW_PASS 6 rounded jaws/vestibules, inward oral linings, closed seams, 12 unclipped pupils")
    finally:
        for material in (skin, cavity):
            if material.users == 0:
                bpy.data.materials.remove(material)


def check_pectoral_fans() -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_kit as kit

    spec = importlib.util.spec_from_file_location("caught_fish_fan_regression", source / "Recipes" / "caught_fish.py")
    recipe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(recipe)
    material = bpy.data.materials.new("CaughtFishRegressionFan")
    try:
        for fish in recipe.FISH:
            for side in (-1, 1):
                parts = recipe.paired_fin(kit, fish, material, side, False)
                try:
                    membrane = parts[0].data
                    projected_area = sum(abs(face.normal.x) * face.area for face in membrane.polygons) / 2
                    assert projected_area > .002 * fish["length"] ** 2, fish["key"] + " has an edge-on pectoral blade"
                    assert all(math.isfinite(c) for vertex in membrane.vertices for c in vertex.co)
                finally:
                    for obj in parts:
                        data = obj.data
                        bpy.data.objects.remove(obj, do_unlink=True)
                        if data.users == 0:
                            bpy.data.meshes.remove(data)
        print("ORIGINAL_FISH_PECTORAL_PASS 12 rooted broad curved membranes")
    finally:
        if material.users == 0:
            bpy.data.materials.remove(material)


def check_geometry(keys=EXPECTED) -> None:
    signatures = set()
    for key in keys:
        species, length = EXPECTED[key]
        obj = bpy.data.objects.get("SM_" + key)
        assert obj is not None, "Missing catch mesh: " + key
        assert obj.get("fish_species") == species, "Species/catalogue mismatch: " + key
        assert obj.get("fish_original") is True, "Unattributed catch: " + key
        data = obj.data
        assert all(math.isfinite(c) for v in data.vertices for c in v.co), key + " has invalid vertices"
        low = min(v.co.y for v in data.vertices)
        high = max(v.co.y for v in data.vertices)
        assert abs((high - low) - length) < .002, key + " has wrong authored scale"
        assert abs(min(v.co.z for v in data.vertices)) < .00001, key + " has wrong ground pivot"
        triangles = sum(len(p.vertices) - 2 for p in data.polygons)
        assert 25000 <= triangles <= 65000, key + " is empty or exceeds the poly budget"
        assert data.uv_layers.get("UVMap") is not None, key + " has no bake UV"
        assert all(math.isfinite(c) for uv in data.uv_layers["UVMap"].data for c in uv.uv), key + " has invalid UVs"
        skin_coords = data.attributes.get("fishcoord")
        assert skin_coords is not None, key + " lost its authored anatomy coordinates"
        outward_faces = 0
        for face in data.polygons:
            coord = sum((skin_coords.data[index].vector for index in face.vertices), Vector()) / len(face.vertices)
            if .30 < coord.x < .80 and math.hypot(coord.y, coord.z) > .98:
                assert face.normal.dot(Vector((coord.y, 0, coord.z))) > 0, key + " exports inward-facing skin"
                outward_faces += 1
        assert outward_faces > 1000, key + " lacks verifiable outer skin"
        normalized = b"".join(struct.pack("<3f", *(c / length for c in vertex.co)) for vertex in data.vertices)
        signature = hashlib.sha256(normalized).hexdigest()
        assert signature not in signatures, key + " reuses another catch's normalized geometry"
        signatures.add(signature)
        assert len(obj.material_slots) in (1, 5, 6, 7), key + " has missing anatomy materials"
        print(f"ORIGINAL_FISH_PASS {key}: {species}, {triangles} tris, length/pivot/UV/unique geometry")


def check_wet_film(keys=EXPECTED) -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_materials as materials
    for key in keys:
        obj = bpy.data.objects.get("SM_" + key)
        assert obj is not None, "Missing wet-fish mesh: " + key
        for slot in obj.material_slots:
            bsdf = next(node for node in slot.material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
            assert abs(bsdf.inputs["Coat Weight"].default_value - materials.FISH_COAT_WEIGHT) < .000001
            assert abs(bsdf.inputs["Coat Roughness"].default_value - materials.FISH_COAT_ROUGHNESS) < .000001
    print("ORIGINAL_FISH_WET_FILM_PASS", len(keys))


def main() -> None:
    check_fin_ray_attachment()
    check_pectoral_fans()
    check_jaw_openings()
    check_wet_film()
    check_geometry()
    print("ORIGINAL_FISH_FAMILY_PASS 6")


if __name__ == "__main__":
    main()
