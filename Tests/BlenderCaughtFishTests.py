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
        assert sum(count for _, count in recipe.BODY_STATION_SPANS) == recipe.BODY_RINGS - 1
        for sample in range(81):
            angle = 2 * math.pi * sample / 80
            stations = [recipe.landmark_station(recipe.body_station(ring), angle)
                        for ring in range(recipe.BODY_RINGS)]
            assert all(a < b for a, b in zip(stations, stations[1:])), "Head rings cross"
            posterior = .187 + .065 * math.exp(-((math.sin(angle) - .20) / .50) ** 2)
            assert min(abs(u - posterior) for u in stations) < .000001, "Missing opercular landmark ring"
        for fish in recipe.FISH:
            physical = [recipe.anatomy_station(fish, index / 1000) for index in range(1001)]
            assert all(a < b for a, b in zip(physical, physical[1:])), fish["key"] + " folds its head stations"
            assert abs(recipe.anatomy_station(fish, fish["eye_u"])
                       - fish["eye_u"] * fish["head_scale"]) < .000001
            assert recipe.anatomy_station(fish, .50) == .50, fish["key"] + " relocates posterior anatomy"
            assert recipe.ventral_profile(fish, .035) < .65 * recipe.profile(fish["bottom"], .035), (
                fish["key"] + " retains a body-thick anterior dentary")
            obj = recipe.body(kit, fish, skin, cavity)
            data = obj.data
            try:
                chin_u = .035
                rest = recipe.surface(fish, chin_u, 1.5 * math.pi)
                posed = recipe.jaw_surface(fish, chin_u, 1.5 * math.pi, False)
                hinge_u = fish["mouth_end"]
                hinge = Vector((rest.x, fish["length"] * (recipe.anatomy_station(fish, hinge_u) - .5),
                                recipe.surface(fish, hinge_u, recipe.oral_angle(fish, hinge_u)).z
                                + fish["length"] * .008))
                assert posed.y > rest.y and posed.z < rest.z, fish["key"] + " lacks mandibular retreat"
                assert abs((posed - hinge).length - (rest - hinge).length) < .000001, (
                    fish["key"] + " inflates its chin instead of hinging")
                columns = recipe.BODY_SIDES // 2 + 1
                lower_offset = recipe.BODY_RINGS * columns
                for ring in range(recipe.BODY_RINGS):
                    u = recipe.body_station(ring)
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
                for ring in range(obj["oral_lining_rings"]):
                    start = lining_start + ring * obj["oral_lining_columns"]
                    middle = data.vertices[start + obj["oral_lining_columns"] // 2].co
                    edges = (data.vertices[start].co
                             + data.vertices[start + obj["oral_lining_columns"] - 1].co) * .5
                    assert middle.z >= edges.z, fish["key"] + " has a depressed palatal vault"
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
                mouth_parts = recipe.mouth(kit, fish, skin, cavity)
                try:
                    assert len(mouth_parts) == (4 if fish["pattern"] == "carp" else 32)
                    for part in mouth_parts:
                        assert all(math.isfinite(c) for vertex in part.data.vertices for c in vertex.co)
                        sides = 12 if fish["pattern"] == "carp" else 8
                        root = sum((vertex.co for vertex in part.data.vertices[:sides]), Vector()) / sides
                        nearest, _, _, distance = head.find_nearest(root)
                        assert nearest is not None and distance < .00015, (
                            fish["key"] + " has a floating tooth/barbel root")
                finally:
                    for part in mouth_parts:
                        mesh = part.data
                        bpy.data.objects.remove(part, do_unlink=True)
                        if mesh.users == 0:
                            bpy.data.meshes.remove(mesh)
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
        print("ORIGINAL_FISH_JAW_PASS 6 hinged jaws/vestibules, 160 seated teeth/4 barbels, inward linings, closed seams, 12 unclipped pupils")
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
        assert len(obj.material_slots) in (1, 2, 5, 6, 7, 8), key + " has missing anatomy materials"
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


def check_baked_texture_dependencies(keys=EXPECTED) -> None:
    for key in keys:
        obj = bpy.data.objects.get("SM_" + key)
        assert obj is not None, "Missing baked catch mesh: " + key
        images = {node.image for slot in obj.material_slots
                  for node in slot.material.node_tree.nodes
                  if node.type == "TEX_IMAGE" and node.image is not None}
        assert len(images) == 4, key + " lacks four baked material maps"
        expected = {f"T_{key}_{kind}.png" for kind in ("basecolor", "roughness", "normal", "metallic")}
        resolved = set()
        for image in images:
            assert image.source == "FILE", key + " retains an unsaved bake image"
            # Strip Blender's relative prefix before Windows treats it as a UNC path.
            assert image.filepath.startswith("//"), key + " retains an absolute scratch dependency"
            relative = Path(image.filepath[2:])
            assert relative.parts[0] == "Textures", key + " references textures outside its asset folder"
            path = Path(bpy.path.abspath(image.filepath))
            assert path.is_file(), key + " has a missing material map"
            resolved.add(path.name)
        assert resolved == expected, key + " references another catch's textures"
    print("ORIGINAL_FISH_PORTABLE_TEXTURES_PASS", 4 * len(keys))


def check_membrane_materials(keys=EXPECTED) -> None:
    source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender"
    sys.path.insert(0, str(source))
    import homestead_materials as materials
    for key in keys:
        obj = bpy.data.objects.get("SM_" + key)
        assert obj is not None, "Missing membrane catch mesh: " + key
        faces = obj.data.attributes.get("fish_membrane")
        assert faces is not None and any(entry.value for entry in faces.data), key + " lacks fin assignments"
        assert any(not entry.value for entry in faces.data), key + " marks its whole body as membrane"
        for face, entry in zip(obj.data.polygons, faces.data):
            material = obj.data.materials[face.material_index]
            bsdf = next(node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
            assert bool(material.get("fish_membrane", False)) == bool(entry.value), key + " changes fin assignments"
            expected = materials.FISH_MEMBRANE_SSS_WEIGHT if entry.value else 0
            assert abs(bsdf.inputs["Subsurface Weight"].default_value - expected) < .000001, (
                key + " loses fin scattering or spreads it onto opaque anatomy")
    print("ORIGINAL_FISH_MEMBRANE_MATERIALS_PASS", len(keys))


def main() -> None:
    check_fin_ray_attachment()
    check_pectoral_fans()
    check_jaw_openings()
    check_wet_film()
    check_geometry()
    check_membrane_materials()
    print("ORIGINAL_FISH_FAMILY_PASS 6")


if __name__ == "__main__":
    main()
