"""Prepare two exact tree-palette models with bounded Blender FBX operations."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

import bpy
import numpy as np
from io_scene_fbx import parse_fbx

sys.path.insert(0, str(Path(__file__).parent))
from TreePreparation import geometry_only_fbx


CONFIG = {
    "jacaranda_tree": {
        "model": "jacaranda_tree_LOD0",
        "stem": "Jacaranda",
        "materials": ("jacaranda_tree_branches", "jacaranda_tree_trunk", "jacaranda_tree_leaves"),
        "ratios": (0.12, 0.035, 0.02),
        "trunk_slot": 1,
    },
    "fir_sapling_medium": {
        "model": "fir_sapling_medium_c_LOD0",
        "stem": "FirPole",
        "materials": ("fir_sapling_medium_branches", "fir_sapling_medium_twigs",
                      "fir_sapling_medium_branches_dead"),
        "ratios": (1.0, 0.25, 0.06),
        "trunk_slot": 0,
    },
}
MATURE_FIR_CONFIG = {
    "fir_tree_01": {
        "model": "fir_tree_01_c_LOD0",
        "stem": "MatureFir",
        "materials": ("fir_tree_01_bark", "fir_tree_01_twig",
                      "fir_tree_01_dead_branches", "fir_tree_01_trunk_c"),
        "ratios": (1.0, 0.25, 0.06),
        "trunk_slot": 3,
    },
}


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def array_digest(value):
    return hashlib.sha256(np.ascontiguousarray(value).tobytes()).hexdigest().upper()


def write_new(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=True, allow_nan=False)
        stream.write("\n")


def guard(control, run_id):
    state = json.loads(control.read_text(encoding="utf-8-sig"))
    if state["id"] != run_id or state["state"] != "running" or not state.get("completionPolicy") == "until-complete":
        raise RuntimeError("Live run no longer admits tree preparation.")
    stop = os.environ.get("HOMESTEAD_SOURCE_STOP_PATH")
    if stop and Path(stop).exists():
        raise RuntimeError("Owned tree preparation cancelled at a model boundary.")


def verify_inputs(manifest_path, receipt_path, source_root):
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    profile = manifest.get("profile")
    configs = CONFIG if profile == "tree-palette-source-01" else (
        MATURE_FIR_CONFIG if profile in ("mature-fir-source-01", "mature-fir-uv-source-02") else None)
    if configs is None or receipt["manifestSha256"] != digest(manifest_path):
        raise ValueError("Tree palette manifest/receipt identity differs.")
    selected = [entry for entry in receipt["files"] if entry["asset"] in configs]
    expected = 24 if profile == "tree-palette-source-01" else 1 if profile == "mature-fir-uv-source-02" else 14
    if len(selected) != expected:
        raise ValueError("Expected exact selected tree source closure.")
    for entry in selected:
        path = source_root / entry["asset"] / entry["file"]
        if path.stat().st_size != entry["bytes"] or digest(path) != entry["sha256"].upper():
            raise ValueError("Receipted tree source differs: " + entry["file"])
    return selected, configs, profile


def verify_texture_inputs(receipt_path, source_root):
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    selected = [
        entry for entry in receipt["files"]
        if entry["asset"] == "fir_tree_01" and not entry["file"].endswith((".fbx", ".blend"))
    ]
    if len(selected) != 13:
        raise ValueError("Expected exact retained mature-fir 1K texture closure.")
    for entry in selected:
        path = source_root / entry["asset"] / entry["file"]
        if path.stat().st_size != entry["bytes"] or digest(path) != entry["sha256"].upper():
            raise ValueError("Receipted mature-fir texture differs: " + entry["file"])
    return selected


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.use_scripts_auto_execute = False
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0


def import_selected(path, config):
    reset_scene()
    if bpy.ops.import_scene.fbx(filepath=str(path), use_image_search=False, use_anim=False,
                                use_custom_props=False, use_custom_normals=True,
                                global_scale=1.0) != {"FINISHED"}:
        raise RuntimeError("Selected geometry import failed.")
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if len(meshes) != 1:
        raise ValueError("Selected geometry must import as exactly one mesh.")
    obj = meshes[0]
    names = tuple(slot.material.name for slot in obj.material_slots)
    if names != config["materials"]:
        raise ValueError(f"Material slot order differs: {names}")
    source_transform = {
        "location": list(obj.location), "rotationEuler": list(obj.rotation_euler), "scale": list(obj.scale)}
    obj.location = (0, 0, 0)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return obj, source_transform


def import_selected_blend(path, config, model):
    reset_scene()
    with bpy.data.libraries.load(str(path), link=False) as (available, selected):
        matches = [name for name in available.objects if name == model]
        if matches != [model]:
            raise ValueError("Exact mature-fir Blend object is missing or ambiguous.")
        selected.objects = matches
    objects = [obj for obj in selected.objects if obj is not None]
    if len(objects) != 1 or objects[0].type != "MESH":
        raise ValueError("Exact mature-fir Blend selection must be one mesh.")
    obj = objects[0]
    bpy.context.collection.objects.link(obj)
    if obj.modifiers:
        raise ValueError("Mature-fir Blend selection unexpectedly has modifiers.")
    names = tuple(slot.material.name if slot.material else None for slot in obj.material_slots)
    leading_empty = model.endswith("_LOD0")
    expected_names = (None, *config["materials"]) if leading_empty else config["materials"]
    if names != expected_names:
        raise ValueError(f"Blend material slot order differs: {names}")
    source_transform = {
        "location": list(obj.location), "rotationEuler": list(obj.rotation_euler), "scale": list(obj.scale)}
    if source_transform != {
            "location": [12.0, 0.0, 0.0], "rotationEuler": [0.0, 0.0, 0.0], "scale": [1.0, 1.0, 1.0]}:
        raise ValueError("Mature-fir Blend display-layout transform differs.")

    mesh = obj.data
    mesh.calc_loop_triangles()
    positions = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", positions)
    positions = positions.reshape(-1, 3)
    loop_vertices = np.empty(len(mesh.loops), dtype=np.int32)
    mesh.loops.foreach_get("vertex_index", loop_vertices)
    polygon_starts = np.empty(len(mesh.polygons), dtype=np.int32)
    polygon_totals = np.empty(len(mesh.polygons), dtype=np.int32)
    polygon_materials = np.empty(len(mesh.polygons), dtype=np.int32)
    mesh.polygons.foreach_get("loop_start", polygon_starts)
    mesh.polygons.foreach_get("loop_total", polygon_totals)
    mesh.polygons.foreach_get("material_index", polygon_materials)
    if leading_empty and (polygon_materials == 0).any():
        raise ValueError("Mature-fir source unexpectedly uses the empty leading material slot.")
    source_normals = np.array([tuple(item.vector) for item in mesh.corner_normals], dtype=np.float32)
    attribute = mesh.attributes.get("UVMap")
    if attribute is None or attribute.domain != "CORNER" or attribute.data_type != "FLOAT_VECTOR":
        raise ValueError("Provider-native mature-fir UVMap corner-vector attribute is missing.")
    vectors = np.empty(len(attribute.data) * 3, dtype=np.float32)
    attribute.data.foreach_get("vector", vectors)
    vectors = vectors.reshape(-1, 3)
    if len(vectors) != len(mesh.loops):
        raise ValueError("Provider-native mature-fir UVMap length differs from corner count.")
    if not np.isfinite(vectors).all() or np.abs(vectors[:, 2]).max() > 1e-6:
        raise ValueError("Provider-native mature-fir UVMap values are invalid.")
    source_xy = vectors[:, :2].copy()
    provider_uv = {
        "source": "provider-native CORNER FLOAT_VECTOR attribute UVMap; XY copied exactly, Z required zero",
        "loops": len(vectors),
        "min": vectors[:, :2].min(axis=0).tolist(),
        "max": vectors[:, :2].max(axis=0).tolist(),
        "thirdComponentMaxAbs": float(np.abs(vectors[:, 2]).max()),
        "sourceVectorSha256": array_digest(vectors),
        "sourceXySha256": array_digest(source_xy),
    }
    attribute.name = "UVMap_SourceVector"
    uv = mesh.uv_layers.new(name="UVMap")
    uv.data.foreach_set("uv", source_xy.reshape(-1))
    mesh.uv_layers.active = uv
    uv.active_render = True
    copied_xy = np.empty(len(uv.data) * 2, dtype=np.float32)
    uv.data.foreach_get("uv", copied_xy)
    copied_xy = copied_xy.reshape(-1, 2)
    if uv.name != "UVMap" or mesh.uv_layers.active_index != 0 or not uv.active_render:
        raise ValueError("Recovered mature-fir UV layer is not active/render channel zero.")
    if not np.array_equal(source_xy, copied_xy):
        raise ValueError("Recovered mature-fir UV values differ from provider-native XY values.")
    provider_uv.update({
        "recoveredLayer": uv.name,
        "recoveredLayerIndex": mesh.uv_layers.active_index,
        "activeRender": uv.active_render,
        "recoveredXySha256": array_digest(copied_xy),
        "exactValueEquality": True,
    })
    if leading_empty:
        mesh.materials.pop(index=0)
    obj.location = (0, 0, 0)
    mesh.update()
    if tuple(slot.material.name for slot in obj.material_slots) != config["materials"]:
        raise ValueError("Removing empty Blend slot changed canonical material order.")
    recovered_positions = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", recovered_positions)
    recovered_positions = recovered_positions.reshape(-1, 3)
    recovered_loop_vertices = np.empty(len(mesh.loops), dtype=np.int32)
    mesh.loops.foreach_get("vertex_index", recovered_loop_vertices)
    recovered_starts = np.empty(len(mesh.polygons), dtype=np.int32)
    recovered_totals = np.empty(len(mesh.polygons), dtype=np.int32)
    recovered_materials = np.empty(len(mesh.polygons), dtype=np.int32)
    mesh.polygons.foreach_get("loop_start", recovered_starts)
    mesh.polygons.foreach_get("loop_total", recovered_totals)
    mesh.polygons.foreach_get("material_index", recovered_materials)
    recovered_normals = np.array([tuple(item.vector) for item in mesh.corner_normals], dtype=np.float32)
    if not (np.array_equal(positions, recovered_positions)
            and np.array_equal(loop_vertices, recovered_loop_vertices)
            and np.array_equal(polygon_starts, recovered_starts)
            and np.array_equal(polygon_totals, recovered_totals)
            and np.array_equal(polygon_materials - (1 if leading_empty else 0), recovered_materials)
            and np.array_equal(source_normals, recovered_normals)):
        raise ValueError("UV recovery changed mature-fir geometry, topology, material mapping or normals.")
    provider_uv["preservation"] = {
        "positionsSha256": array_digest(positions),
        "loopVertexIndicesSha256": array_digest(loop_vertices),
        "polygonLoopStartsSha256": array_digest(polygon_starts),
        "polygonLoopTotalsSha256": array_digest(polygon_totals),
        "sourceMaterialIndicesSha256": array_digest(polygon_materials),
        "recoveredMaterialIndicesSha256": array_digest(recovered_materials),
        "sourceCornerNormalsSha256": array_digest(source_normals),
        "recoveredCornerNormalsSha256": array_digest(recovered_normals),
        "positionsExact": True,
        "topologyExact": True,
        "normalsExact": True,
        "materialIndicesChangedOnlyByEmptyLeadingSlotRemoval": leading_empty,
    }
    return obj, source_transform, provider_uv


def report(obj, materials):
    mesh = obj.data
    mesh.calc_loop_triangles()
    coords = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", coords)
    coords = coords.reshape(-1, 3)
    triangle_material = np.empty(len(mesh.loop_triangles), dtype=np.int32)
    mesh.loop_triangles.foreach_get("material_index", triangle_material)
    if len(mesh.uv_layers) != 1:
        raise ValueError("Prepared tree must have exactly one UV layer.")
    uv = np.empty(len(mesh.uv_layers[0].data) * 2, dtype=np.float32)
    mesh.uv_layers[0].data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    loops = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int32)
    mesh.loop_triangles.foreach_get("loops", loops)
    loops = loops.reshape(-1, 3)
    triangle_vertices = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int32)
    mesh.loop_triangles.foreach_get("vertices", triangle_vertices)
    triangle_vertices = triangle_vertices.reshape(-1, 3)
    triangle_positions = coords[triangle_vertices]
    doubled_areas = np.linalg.norm(np.cross(
        triangle_positions[:, 1] - triangle_positions[:, 0],
        triangle_positions[:, 2] - triangle_positions[:, 0]), axis=1)
    triangle_uv = uv[loops]
    determinants = (
        (triangle_uv[:, 1, 0] - triangle_uv[:, 0, 0])
        * (triangle_uv[:, 2, 1] - triangle_uv[:, 0, 1])
        - (triangle_uv[:, 1, 1] - triangle_uv[:, 0, 1])
        * (triangle_uv[:, 2, 0] - triangle_uv[:, 0, 0])
    )
    mesh.calc_tangents(uvmap=mesh.uv_layers[0].name)
    tangents = np.empty(len(mesh.loops) * 3, dtype=np.float32)
    mesh.loops.foreach_get("tangent", tangents)
    tangents = tangents.reshape(-1, 3)
    tangent_lengths = np.linalg.norm(tangents, axis=1)
    normals = np.array([tuple(item.vector) for item in mesh.corner_normals], dtype=np.float32)
    normal_lengths = np.linalg.norm(normals, axis=1)
    bitangent_signs = np.empty(len(mesh.loops), dtype=np.float32)
    mesh.loops.foreach_get("bitangent_sign", bitangent_signs)
    return {
        "vertices": len(mesh.vertices),
        "triangles": len(mesh.loop_triangles),
        "materials": [
            {"slot": index, "name": name, "triangles": int((triangle_material == index).sum())}
            for index, name in enumerate(materials)
        ],
        "uv": {
            "name": mesh.uv_layers[0].name,
            "loops": len(uv),
            "min": uv.min(axis=0).tolist(),
            "max": uv.max(axis=0).tolist(),
            "degenerateTriangles": int((np.abs(determinants) <= 1e-12).sum()),
            "degenerateTrianglesAt1e10": int((np.abs(determinants) <= 1e-10).sum()),
            "minimumAbsoluteDeterminant": float(np.abs(determinants).min()),
            "perMaterial": [
                {
                    "slot": index,
                    "name": name,
                    "triangles": int((triangle_material == index).sum()),
                    "degenerateTriangles": int(
                        (np.abs(determinants[triangle_material == index]) <= 1e-12).sum()),
                    "degenerateTrianglesAt1e10": int(
                        (np.abs(determinants[triangle_material == index]) <= 1e-10).sum()),
                    "minimumAbsoluteDeterminant": float(
                        np.abs(determinants[triangle_material == index]).min()),
                    "min": triangle_uv[triangle_material == index].reshape(-1, 2).min(axis=0).tolist(),
                    "max": triangle_uv[triangle_material == index].reshape(-1, 2).max(axis=0).tolist(),
                    "uniqueSample": len({
                        tuple(value) for value in
                        triangle_uv[triangle_material == index].reshape(-1, 2)[:100000]
                    }),
                }
                for index, name in enumerate(materials)
            ],
        },
        "geometry": {
            "zeroAreaTrianglesAt1e12": int((doubled_areas <= 1e-12).sum()),
            "nearZeroAreaTrianglesAt1e10": int((doubled_areas <= 1e-10).sum()),
            "minimumDoubledArea": float(doubled_areas.min()),
        },
        "basis": {
            "nearZeroTangents": int((tangent_lengths <= 1e-6).sum()),
            "nearZeroTangentsAt1e4": int((tangent_lengths <= 1e-4).sum()),
            "nonFiniteTangents": int((~np.isfinite(tangents).all(axis=1)).sum()),
            "invalidBitangentSigns": int(
                (~np.isfinite(bitangent_signs) | (np.abs(bitangent_signs) < 0.5)).sum()),
            "nearZeroNormals": int((normal_lengths <= 1e-6).sum()),
            "nearZeroNormalsAt1e4": int((normal_lengths <= 1e-4).sum()),
            "nonFiniteNormals": int((~np.isfinite(normals).all(axis=1)).sum()),
        },
        "boundsMeters": {"min": coords.min(axis=0).tolist(), "max": coords.max(axis=0).tolist()},
    }


def collision_proxy(obj, slot):
    mesh = obj.data
    indices = {vertex for polygon in mesh.polygons if polygon.material_index == slot
               for vertex in polygon.vertices}
    points = np.array([mesh.vertices[index].co[:] for index in indices], dtype=np.float64)
    if len(points) < 16 or not np.isfinite(points).all():
        raise ValueError("Insufficient finite lower-trunk geometry.")
    low, high = points[:, 2].min(), points[:, 2].max()
    lower = points[points[:, 2] <= low + min((high - low) * 0.25, 2.5)]
    center = np.median(lower[:, :2], axis=0)
    radius = float(np.quantile(np.linalg.norm(lower[:, :2] - center, axis=1), 0.95) + 0.05)
    height = float(min(max(high - low, 0.4), 5.0))
    if not (0.05 <= radius <= 2.5):
        raise ValueError("Derived lower-trunk radius outside bounded policy.")
    return {
        "shape": "capsule", "centerMeters": [float(center[0]), float(center[1]), low + height * 0.5],
        "radiusMeters": radius, "cylinderHeightMeters": max(0.0, height - radius * 2),
        "source": "95th percentile radius of selected lower-trunk material vertices plus5cm; provisional until game contact review",
    }


def export_fbx(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={"MESH"},
                                global_scale=1.0, apply_unit_scale=True,
                                apply_scale_options="FBX_SCALE_UNITS", axis_forward="-Y", axis_up="Z",
                                bake_anim=False, path_mode="STRIP", use_mesh_modifiers=True,
                                mesh_smooth_type="OFF", use_tspace=True, use_custom_props=False) != {"FINISHED"}:
        raise RuntimeError("Prepared FBX export failed.")


def verify_export_fbx(path, config, expected_triangles):
    root, _ = parse_fbx.parse(str(path))
    uv_elements = []
    tangent_elements = []
    binormal_elements = []

    def visit(element):
        if element.id == b"LayerElementUV":
            uv_elements.append(element)
        elif element.id == b"LayerElementTangent":
            tangent_elements.append(element)
        elif element.id == b"LayerElementBinormal":
            binormal_elements.append(element)
        for child in element.elems:
            visit(child)

    visit(root)
    if len(uv_elements) != 1:
        raise ValueError("Prepared FBX must contain exactly one LayerElementUV.")
    uv_values = next((child for child in uv_elements[0].elems if child.id == b"UV"), None)
    if uv_values is None or not uv_values.props or len(uv_values.props[0]) < 6:
        raise ValueError("Prepared FBX LayerElementUV has no usable values.")
    if len(tangent_elements) != 1 or len(binormal_elements) != 1:
        raise ValueError("Prepared FBX must contain exactly one tangent and binormal layer.")
    tangent_values = next(
        (child for child in tangent_elements[0].elems if child.id == b"Tangents"), None)
    binormal_values = next(
        (child for child in binormal_elements[0].elems if child.id == b"Binormals"), None)
    if (tangent_values is None or not tangent_values.props or len(tangent_values.props[0]) < 9
            or binormal_values is None or not binormal_values.props
            or len(binormal_values.props[0]) < 9):
        raise ValueError("Prepared FBX tangent/binormal arrays are empty.")
    objects = next(element for element in root.elems if element.id == b"Objects")
    exported_materials = tuple(
        element.props[1].split(b"\x00", 1)[0].decode("utf-8")
        for element in objects.elems if element.id == b"Material"
    )
    if exported_materials != config["materials"]:
        raise ValueError("Prepared FBX material object order differs.")

    before = set(bpy.data.objects)
    if bpy.ops.import_scene.fbx(filepath=str(path), use_image_search=False, use_anim=False,
                                use_custom_props=False, use_custom_normals=True,
                                global_scale=1.0) != {"FINISHED"}:
        raise RuntimeError("Prepared FBX verification import failed.")
    imported = [obj for obj in bpy.data.objects if obj not in before and obj.type == "MESH"]
    if len(imported) != 1:
        raise ValueError("Prepared FBX verification must import exactly one mesh.")
    obj = imported[0]
    imported_materials = tuple(slot.material.name for slot in obj.material_slots)
    normalized_materials = tuple(name.rsplit(".", 1)[0] if name.rsplit(".", 1)[-1].isdigit() else name
                                 for name in imported_materials)
    if normalized_materials != config["materials"]:
        raise ValueError("Prepared FBX verification material order differs.")
    result = report(obj, config["materials"])
    if result["triangles"] != expected_triangles or result["uv"]["degenerateTriangles"]:
        raise ValueError("Prepared FBX verification geometry or UV coverage differs.")
    if any(value != 0 for value in result["basis"].values()):
        raise ValueError("Prepared FBX verification tangent basis is invalid.")
    mesh = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.meshes.remove(mesh)
    return {
        "layerElementUvCount": len(uv_elements),
        "layerElementUvValueCount": len(uv_values.props[0]),
        "layerElementTangentCount": len(tangent_elements),
        "layerElementTangentValueCount": len(tangent_values.props[0]),
        "layerElementBinormalCount": len(binormal_elements),
        "layerElementBinormalValueCount": len(binormal_values.props[0]),
        "exportedMaterials": list(exported_materials),
        "freshImportMaterials": list(imported_materials),
        "freshImport": result,
    }


def reduce_by_material(source, ratio, materials):
    clone = source.copy()
    clone.data = source.data.copy()
    bpy.context.collection.objects.link(clone)
    bpy.ops.object.select_all(action="DESELECT")
    clone.select_set(True)
    bpy.context.view_layer.objects.active = clone
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.separate(type="MATERIAL")
    bpy.ops.object.mode_set(mode="OBJECT")
    parts = list(bpy.context.selected_objects)
    if len(parts) != len(materials):
        raise ValueError("Material separation did not preserve every canonical role.")
    for part in parts:
        used = {part.data.materials[face.material_index].name for face in part.data.polygons}
        if len(used) != 1 or next(iter(used)) not in materials:
            raise ValueError("Separated tree material role differs.")
        bpy.context.view_layer.objects.active = part
        bpy.ops.object.select_all(action="DESELECT")
        part.select_set(True)
        modifier = part.modifiers.new("BoundedRoleReduction", "DECIMATE")
        modifier.decimate_type = "COLLAPSE"
        modifier.ratio = ratio
        modifier.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.dissolve_degenerate(threshold=1e-6)
        bpy.ops.object.mode_set(mode="OBJECT")
        part.data.update()
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    result = bpy.context.object
    old_names = [material.name for material in result.data.materials]
    indices = np.empty(len(result.data.polygons), dtype=np.int32)
    result.data.polygons.foreach_get("material_index", indices)
    indices = np.array([materials.index(old_names[value]) for value in indices], dtype=np.int32)
    result.data.materials.clear()
    for material in source.data.materials:
        result.data.materials.append(material)
    result.data.polygons.foreach_set("material_index", indices)
    result.data.polygons.foreach_set("use_smooth", [True] * len(result.data.polygons))
    result.data.update()
    return result


def copy_textures(asset, source_root, output, source_entries):
    textures = []
    for entry in source_entries:
        if entry["asset"] != asset or entry["file"].endswith((".fbx", ".blend")):
            continue
        source = source_root / asset / entry["file"]
        target = output / "Textures" / entry["file"]
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        if digest(target) != entry["sha256"].upper():
            raise ValueError("Prepared texture copy differs.")
        textures.append({
            "file": str(target.relative_to(output)),
            "sha256": digest(target),
            "bytes": target.stat().st_size,
        })
    return textures


def prepare_asset(asset, config, source_root, texture_source_root, output, source_entries, profile):
    if profile == "mature-fir-uv-source-02":
        source_blend = source_root / asset / f"{asset}_1k.blend"
        models = [f"{asset}_c_LOD{index}" for index in range(3)]
        levels = []
        source_transforms = []
        provider_uv = []
        proxy = None
        for index, model in enumerate(models):
            original, source_transform, uv_recovery = import_selected_blend(source_blend, config, model)
            level = report(original, config["materials"])
            if not all(item["triangles"] > 0 for item in level["materials"]):
                raise ValueError("Provider-authored LOD removed a material role.")
            if levels and level["triangles"] >= levels[-1]["triangles"]:
                raise ValueError("Provider-authored LOD triangle counts must decrease.")
            if level["uv"]["degenerateTriangles"] or any(
                    value != 0 for value in level["basis"].values()):
                raise ValueError("Provider-authored LOD has invalid UV or tangent basis.")
            if index == 0:
                proxy = collision_proxy(original, config["trunk_slot"])
            path = output / f"{config['stem']}_LOD{index}.fbx"
            export_fbx(original, path)
            level["exportVerification"] = verify_export_fbx(path, config, level["triangles"])
            level.update({
                "lod": index,
                "sourceModel": model,
                "file": path.name,
                "bytes": path.stat().st_size,
                "sha256": digest(path),
            })
            levels.append(level)
            source_transforms.append(source_transform)
            provider_uv.append(uv_recovery)
            mesh = original.data
            bpy.data.objects.remove(original, do_unlink=True)
            bpy.data.meshes.remove(mesh)
            print(f"{asset} authored LOD{index}: {level['triangles']} triangles", flush=True)
        selection = {
            "sourceFormat": "Blend",
            "selectedModels": models,
            "selectionMode": "bpy.data.libraries.load exact object only; scripts disabled",
            "modifiers": [],
        }
        return {
            "asset": asset,
            "model": config["model"],
            "selection": selection,
            "sourceTransformsBeforeLayoutRemoval": source_transforms,
            "conversion": "Selected provider-authored c LOD0/1/2 objects; removed each 12m display-layout translation once; copied every provider UVMap corner XY exactly into active/render UV0.",
            "providerNativeUvRecovery": provider_uv,
            "materialSlots": list(config["materials"]),
            "levels": levels,
            "collisionProxy": proxy,
            "textures": copy_textures(asset, texture_source_root, output, source_entries),
        }

    provider_uv = None
    source_fbx = source_root / asset / f"{asset}_1k.fbx"
    with tempfile.TemporaryDirectory(prefix=asset + "-", dir=output) as temporary:
        safe = Path(temporary) / "selected.fbx"
        selection = geometry_only_fbx(source_fbx, safe, [config["model"]])
        original, source_transform = import_selected(safe, config)
    levels = []
    proxy = None
    for index, ratio in enumerate(config["ratios"]):
        clone = reduce_by_material(original, ratio, config["materials"]) if ratio < 1 else original.copy()
        if ratio == 1:
            clone.data = original.data.copy()
            bpy.context.collection.objects.link(clone)
        level = report(clone, config["materials"])
        if not all(item["triangles"] > 0 for item in level["materials"]):
            raise ValueError("Reduction removed a material role.")
        if levels and level["triangles"] >= levels[-1]["triangles"]:
            raise ValueError("Prepared LOD triangle counts must decrease.")
        if level["uv"]["degenerateTriangles"] or any(value != 0 for value in level["basis"].values()):
            raise ValueError("Prepared LOD has invalid UV or tangent basis.")
        if index == 0:
            proxy = collision_proxy(clone, config["trunk_slot"])
        path = output / f"{config['stem']}_LOD{index}.fbx"
        export_fbx(clone, path)
        level["exportVerification"] = verify_export_fbx(path, config, level["triangles"])
        level.update({"lod": index, "ratio": ratio, "file": path.name,
                      "bytes": path.stat().st_size, "sha256": digest(path)})
        levels.append(level)
        mesh = clone.data
        bpy.data.objects.remove(clone, do_unlink=True)
        bpy.data.meshes.remove(mesh)
        print(f"{asset} LOD{index}: {level['triangles']} triangles", flush=True)
    bpy.data.objects.remove(original, do_unlink=True)
    return {
        "asset": asset, "model": config["model"], "selection": selection,
        "sourceTransformBeforeLayoutRemoval": source_transform,
        "conversion": "Removed publisher display-layout translation once; baked imported axis/unit rotation and scale once.",
        "providerNativeUvRecovery": provider_uv,
        "materialSlots": list(config["materials"]), "levels": levels,
        "collisionProxy": proxy,
        "textures": copy_textures(asset, texture_source_root, output, source_entries),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--texture-receipt", type=Path)
    parser.add_argument("--texture-source-root", type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    guard(args.control, args.run_id)
    if args.output.exists():
        raise FileExistsError("Fresh prepared palette output required.")
    args.output.mkdir(parents=True)
    entries, configs, profile = verify_inputs(args.manifest, args.receipt, args.source_root)
    if profile == "mature-fir-uv-source-02":
        if args.texture_receipt is None or args.texture_source_root is None:
            raise ValueError("UV repair requires the retained receipted texture closure.")
        entries.extend(verify_texture_inputs(args.texture_receipt, args.texture_source_root))
        texture_root = args.texture_source_root
    else:
        texture_root = args.source_root
    assets = []
    for asset, config in configs.items():
        guard(args.control, args.run_id)
        assets.append(prepare_asset(
            asset, config, args.source_root, texture_root, args.output, entries, profile))
    guard(args.control, args.run_id)
    write_new(args.output / "provenance.json", {
        "schema": 1, "blenderVersion": bpy.app.version_string,
        "manifestSha256": digest(args.manifest), "receiptSha256": digest(args.receipt),
        "toolSha256": digest(Path(__file__)), "assets": assets,
        "limits": "Derived source meshes/material copies only; no Unreal import, shader/runtime/GPU or visual acceptance.",
    })


if __name__ == "__main__":
    main()
