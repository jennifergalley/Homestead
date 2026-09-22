"""Prepare the admitted Tree Small 02 with Blender's FBX IO and mesh tools."""

import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import sys
import tempfile

import bpy
import numpy as np
from io_scene_fbx import data_types, encode_bin, parse_fbx
from mathutils import Vector


SLOTS = (
    "tree_small_02_branches",
    "tree_small_02_leaves",
    "tree_small_02_trunk",
)
LEVELS = (
    ("LOD1", (0.50, 0.16, 0.75)),
    ("LOD2", (0.25, 0.10, 0.50)),
    ("LOD3", (0.10, 0.08, 0.20)),
)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def verify_sources(source, receipt):
    records = [
        entry for entry in json.loads(receipt.read_text(encoding="utf-8-sig"))["files"]
        if entry["asset"] == "tree_small_02"
    ]
    if len(records) != 14 or len({entry["file"] for entry in records}) != 14:
        raise ValueError("Expected exactly 14 distinct admitted Tree Small 02 files.")
    for entry in records:
        if Path(entry["file"]).name != entry["file"]:
            raise ValueError("Receipt filename must be a basename.")
        path = source / entry["file"]
        if path.stat().st_size != entry["bytes"] or digest(path) != entry["sha256"].lower():
            raise ValueError(f"Source receipt mismatch: {path.name}")
    return records


def geometry_only_fbx(source, destination, model_names=None):
    """Strip image objects before IO; no original texture path reaches the importer."""
    root, version = parse_fbx.parse(str(source))
    objects = next(element for element in root.elems if element.id == b"Objects")
    image_ids = {element.props[0] for element in objects.elems
                 if element.id in {b"Texture", b"Video"}}
    removed_ids = set(image_ids)
    retained = None
    if model_names is not None:
        requested = set(model_names)
        if not requested or len(requested) != len(model_names):
            raise ValueError("Selected FBX model names must be nonempty and unique.")
        by_id = {element.props[0]: element for element in objects.elems}
        if len(by_id) != len(objects.elems) or any(
                element.id not in {b"Model", b"Geometry", b"Material", b"Texture", b"Video"}
                for element in objects.elems):
            raise ValueError("Selected static FBX contains duplicate IDs or unsupported object types.")
        models = {element.props[1].split(b"\x00", 1)[0].decode("utf-8"): element
                  for element in objects.elems if element.id == b"Model"}
        if len(models) != sum(element.id == b"Model" for element in objects.elems) or not requested <= models.keys():
            raise ValueError("Selected FBX model names are missing or ambiguous.")
        selected = {models[name].props[0] for name in requested}
        connections = next(element for element in root.elems if element.id == b"Connections")
        keep = set(selected)
        for model_id in selected:
            bindings = [element.props[1] for element in connections.elems
                        if element.id == b"C" and element.props[0] == b"OO" and element.props[2] == model_id]
            geometries = [value for value in bindings if by_id[value].id == b"Geometry"]
            materials = [value for value in bindings if by_id[value].id == b"Material"]
            if len(geometries) != 1 or not materials or len(bindings) != 1 + len(materials):
                raise ValueError("Expected one geometry and one or more materials per selected static model.")
            keep.update(bindings)
        removed_ids.update(by_id.keys() - keep)
        retained = [element for element in objects.elems if element.props[0] in keep]
        definitions = next(element for element in root.elems if element.id == b"Definitions")
        total = 0
        original_types = {element.id for element in objects.elems}
        for definition in definitions.elems:
            if definition.id != b"ObjectType":
                continue
            count = next(element for element in definition.elems if element.id == b"Count")
            if definition.props[0] in original_types:
                count.props[0] = sum(element.id == definition.props[0] for element in retained)
            total += count.props[0]
        next(element for element in definitions.elems if element.id == b"Count").props[0] = total
    methods = {
        getattr(data_types, name): "add_" + name.lower()
        for name in (
            "BOOL", "CHAR", "INT8", "INT16", "INT32", "INT64", "FLOAT32",
            "FLOAT64", "BYTES", "STRING", "INT32_ARRAY", "INT64_ARRAY",
            "FLOAT32_ARRAY", "FLOAT64_ARRAY", "BOOL_ARRAY", "BYTE_ARRAY",
        )
    }

    def convert(element):
        result = encode_bin.FBXElem(element.id)
        for kind, value in zip(element.props_type, element.props):
            getattr(result, methods[kind])(value)
        for child in element.elems:
            if element.id == b"Objects" and child.props[0] in removed_ids:
                continue
            if element.id == b"Connections" and child.id == b"C":
                if any(value in removed_ids for value in child.props[1:3]):
                    continue
            result.elems.append(convert(child))
        return result

    encode_bin.write(str(destination), convert(root), version)
    if retained is not None:
        actual_root, actual_version = parse_fbx.parse(str(destination))
        actual = next(element for element in actual_root.elems if element.id == b"Objects").elems

        def identical(left, right):
            if left.id != right.id or left.props_type != right.props_type or len(left.props) != len(right.props):
                return False
            for a, b in zip(left.props, right.props):
                if hasattr(a, "tobytes"):
                    if not hasattr(b, "tobytes") or a.tobytes() != b.tobytes():
                        return False
                elif isinstance(a, float):
                    if not isinstance(b, float) or a.hex() != b.hex():
                        return False
                elif a != b:
                    return False
            return len(left.elems) == len(right.elems) and all(
                identical(a, b) for a, b in zip(left.elems, right.elems))

        if actual_version != version or len(actual) != len(retained) or not all(
                identical(a, b) for a, b in zip(retained, actual)):
            raise ValueError("Selected FBX roundtrip changed retained object data/arrays/transforms.")
    return {"fbxVersion": version, "removedImageObjects": len(image_ids),
            "selectedModels": sorted(model_names) if model_names is not None else None,
            "retainedObjectDataVerified": retained is not None}


def import_geometry(path):
    before = set(bpy.data.objects)
    result = bpy.ops.import_scene.fbx(
        filepath=str(path), use_image_search=False, use_anim=False,
        use_custom_props=False, use_custom_normals=True, global_scale=1.0,
    )
    if result != {"FINISHED"}:
        raise RuntimeError(f"FBX import failed: {path}")
    objects = [obj for obj in bpy.data.objects if obj not in before]
    meshes = [obj for obj in objects if obj.type == "MESH"]
    if len(meshes) != 1:
        raise ValueError(f"Expected one tree mesh, found {len(meshes)}.")
    tree = meshes[0]
    names = [slot.material.name for slot in tree.material_slots]
    if names != list(SLOTS):
        raise ValueError(f"Unexpected material order: {names}")
    return tree


def mesh_report(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    coordinates = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", coordinates)
    coordinates = coordinates.reshape(-1, 3)
    matrix = np.array(obj.matrix_world, dtype=np.float64)
    world = coordinates @ matrix[:3, :3].T + matrix[:3, 3]
    material = np.empty(len(mesh.polygons), dtype=np.int32)
    sizes = np.empty(len(mesh.polygons), dtype=np.int32)
    mesh.polygons.foreach_get("material_index", material)
    mesh.polygons.foreach_get("loop_total", sizes)
    triangle_material = np.empty(len(mesh.loop_triangles), dtype=np.int32)
    mesh.loop_triangles.foreach_get("material_index", triangle_material)
    layers = []
    for layer in mesh.uv_layers:
        uv = np.empty(len(layer.data) * 2, dtype=np.float32)
        layer.data.foreach_get("uv", uv)
        uv = uv.reshape(-1, 2)
        layers.append({
            "name": layer.name, "loops": len(uv), "finite": bool(np.isfinite(uv).all()),
            "min": uv.min(axis=0).tolist(), "max": uv.max(axis=0).tolist(),
        })
    return {
        "name": obj.name, "vertices": len(mesh.vertices), "polygons": len(mesh.polygons),
        "triangles": len(mesh.loop_triangles),
        "polygonSizes": {str(size): int((sizes == size).sum()) for size in np.unique(sizes)},
        "materials": [
            {"slot": index, "name": name, "polygons": int((material == index).sum()),
             "triangles": int((triangle_material == index).sum())}
            for index, name in enumerate(SLOTS)
        ],
        "uvLayers": layers,
        "localBounds": {"min": coordinates.min(axis=0).tolist(), "max": coordinates.max(axis=0).tolist()},
        "worldBoundsMeters": {"min": world.min(axis=0).tolist(), "max": world.max(axis=0).tolist()},
        "matrixWorld": matrix.tolist(),
        "originWorldMeters": list(obj.matrix_world.translation),
        "customNormals": mesh.has_custom_normals,
    }


def activate(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def export_tree(obj, path):
    activate(obj)
    result = bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"},
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y", axis_up="Z", bake_anim=False, path_mode="STRIP",
        use_mesh_modifiers=True, mesh_smooth_type="FACE", use_custom_props=False,
    )
    if result != {"FINISHED"}:
        raise RuntimeError(f"FBX export failed: {path}")


def material_contract(records):
    names = {entry["file"] for entry in records}
    contract = []
    for index, (slot, suffix) in enumerate(zip(SLOTS, ("_branch", "_leaves", ""))):
        prefix = "tree_small_02" + suffix
        channels = {
            "baseColor": (prefix + "_diff_2k." + ("jpg" if not suffix else "png"), True),
            "normalDirectX": (prefix + "_nor_dx_2k.png", False),
            "roughness": (prefix + "_rough_2k.png", False),
            "ambientOcclusion": (prefix + "_ao_2k.png", False),
        }
        if index == 1:
            channels["opacityMask"] = ("tree_small_02_leaves_alpha_2k.png", False)
        if any(filename not in names for filename, _ in channels.values()):
            raise ValueError(f"Material {slot} uses a texture outside the admitted whitelist.")
        contract.append({
            "slot": index, "name": slot,
            "blend": "Masked" if index == 1 else "Opaque",
            "twoSided": index == 1,
            "channels": {
                channel: {"file": "Textures/" + filename, "sRGB": srgb}
                for channel, (filename, srgb) in channels.items()
            },
        })
    return contract


def reduced_tree(source, name, ratios, leaf_method, leaf_angle):
    clone = source.copy()
    clone.data = source.data.copy()
    bpy.context.collection.objects.link(clone)
    activate(clone)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.separate(type="MATERIAL")
    bpy.ops.object.mode_set(mode="OBJECT")
    parts = list(bpy.context.selected_objects)
    if len(parts) != 3:
        raise ValueError("Material separation did not produce three tree parts.")
    for part in parts:
        used = {part.data.materials[face.material_index].name for face in part.data.polygons}
        if len(used) != 1 or next(iter(used)) not in SLOTS:
            raise ValueError(f"Unexpected separated materials: {used}")
        slot = SLOTS.index(next(iter(used)))
        activate(part)
        modifier = part.modifiers.new("SourceDerivedCollapse", "DECIMATE")
        if slot == 1 and leaf_method == "planar":
            modifier.decimate_type = "DISSOLVE"
            modifier.angle_limit = math.radians(leaf_angle)
            modifier.delimit = {"UV", "MATERIAL"}
            modifier.use_dissolve_boundaries = False
            description = f"planar {leaf_angle} degrees, preserve boundaries/UV seams"
        else:
            modifier.decimate_type = "COLLAPSE"
            modifier.ratio = ratios[slot]
            modifier.use_collapse_triangulate = True
            description = f"collapse {ratios[slot]:.3f}"
        print(f"{name}: reducing {SLOTS[slot]} with {description}", flush=True)
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    result = bpy.context.object
    old_names = [material.name for material in result.data.materials]
    indices = np.empty(len(result.data.polygons), dtype=np.int32)
    result.data.polygons.foreach_get("material_index", indices)
    remap = np.array([SLOTS.index(value) for value in old_names], dtype=np.int32)
    indices = remap[indices]
    result.data.materials.clear()
    for material in source.data.materials:
        result.data.materials.append(material)
    result.data.polygons.foreach_set("material_index", indices)
    result.name = "TreeSmall02_" + name
    return result


def prepare(tree, output, inspection, leaf_method):
    material_map = material_contract(inspection["sourceFiles"])
    levels = []
    for index, (name, ratios) in enumerate((("SourceLOD0", None), *LEVELS)):
        angle = (0, 1, 5, 12)[index]
        obj = tree if ratios is None else reduced_tree(tree, name, ratios, leaf_method, angle)
        path = output / ("TreeSmall02_" + name + ".fbx")
        report = mesh_report(obj)
        report.update({"level": name, "file": path.name, "collapseRatiosBySlot": ratios})
        if ratios is not None and leaf_method == "planar":
            report["leafPlanarAngleDegrees"] = angle
            report["collapseRatiosBySlot"] = (ratios[0], None, ratios[2])
        if ratios is not None and report["triangles"] >= levels[-1]["triangles"]:
            raise ValueError("Reduced levels must have strictly decreasing triangle counts.")
        if not all(slot["triangles"] > 0 for slot in report["materials"]):
            raise ValueError("A reduced level lost a material region.")
        if [layer["name"] for layer in report["uvLayers"]] != ["UVMap", "UV_map_01"]:
            raise ValueError("The source UV layers were not preserved.")
        export_tree(obj, path)
        report.update({"bytes": path.stat().st_size, "sha256": digest(path)})
        levels.append(report)
        print(f"{name}: {report['triangles']} triangles -> {path.name}", flush=True)
        if ratios is not None:
            mesh = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(mesh)
    write_json(output / "materials.json", material_map)
    return levels


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.use_scripts_auto_execute = False
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0


def verify_exports(output, provenance, write_report=True):
    results = []
    for level in provenance["levels"]:
        path = output / level["file"]
        if digest(path) != level["sha256"]:
            raise ValueError(f"Export changed: {path}")
        reset_scene()
        obj = import_geometry(path)
        actual = mesh_report(obj)
        if actual["triangles"] != level["triangles"]:
            raise ValueError(f"Round-trip triangle mismatch: {path.name}")
        for expected, observed in zip(level["materials"], actual["materials"]):
            if expected["triangles"] != observed["triangles"]:
                raise ValueError(f"Round-trip material mismatch: {path.name}")
        if [layer["name"] for layer in actual["uvLayers"]] != ["UVMap", "UV_map_01"]:
            raise ValueError(f"Round-trip UV layer mismatch: {path.name}")
        if not all(layer["finite"] for layer in actual["uvLayers"]):
            raise ValueError(f"Non-finite round-trip UVs: {path.name}")
        for key in ("min", "max"):
            if not np.allclose(actual["worldBoundsMeters"][key], level["worldBoundsMeters"][key], atol=0.00001):
                raise ValueError(f"Round-trip bounds mismatch: {path.name}")
        for expected, observed in zip(level["uvLayers"], actual["uvLayers"]):
            for key in ("min", "max"):
                if not np.allclose(expected[key], observed[key], atol=0.00001):
                    raise ValueError(f"Round-trip UV range mismatch: {path.name}")
        if not np.allclose(actual["originWorldMeters"], [0, 0, 0], atol=0.00001):
            raise ValueError(f"Round-trip root moved: {path.name}")
        results.append({"file": path.name, "passed": True, "mesh": actual})
    for entry in provenance["sourceFiles"]:
        if entry["file"].endswith((".jpg", ".png")):
            if digest(output / "Textures" / entry["file"]) != entry["sha256"].lower():
                raise ValueError(f"Prepared texture differs from source: {entry['file']}")
    if write_report:
        write_json(output / "roundtrip.json", results)
    return results


def preview(output, provenance):
    views = (("front", Vector((0, -1, 0))), ("side", Vector((1, 0, 0))))
    paths = []
    for level in provenance["levels"]:
        reset_scene()
        tree = import_geometry(output / level["file"])
        for slot, material in enumerate(tree.data.materials):
            material.use_nodes = True
            nodes = material.node_tree.nodes
            nodes.clear()
            target = nodes.new("ShaderNodeOutputMaterial")
            emission = nodes.new("ShaderNodeEmission")
            color = nodes.new("ShaderNodeTexImage")
            mapping = material_contract(provenance["sourceFiles"])[slot]["channels"]
            color.image = bpy.data.images.load(str(output / mapping["baseColor"]["file"]), check_existing=True)
            material.node_tree.links.new(color.outputs["Color"], emission.inputs["Color"])
            if slot == 1:
                alpha = nodes.new("ShaderNodeTexImage")
                alpha.image = bpy.data.images.load(str(output / mapping["opacityMask"]["file"]), check_existing=True)
                alpha.image.colorspace_settings.name = "Non-Color"
                transparent = nodes.new("ShaderNodeBsdfTransparent")
                mix = nodes.new("ShaderNodeMixShader")
                material.node_tree.links.new(alpha.outputs["Color"], mix.inputs[0])
                material.node_tree.links.new(transparent.outputs[0], mix.inputs[1])
                material.node_tree.links.new(emission.outputs[0], mix.inputs[2])
                material.node_tree.links.new(mix.outputs[0], target.inputs[0])
            else:
                material.node_tree.links.new(emission.outputs[0], target.inputs[0])
        scene = bpy.context.scene
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = 8
        scene.cycles.transparent_max_bounces = 32
        scene.render.threads_mode = "FIXED"
        scene.render.threads = 4
        scene.render.resolution_x = 512
        scene.render.resolution_y = 512
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = True
        scene.view_settings.view_transform = "Standard"
        bounds = provenance["levels"][0]["worldBoundsMeters"]
        center = (Vector(bounds["min"]) + Vector(bounds["max"])) * 0.5
        camera_data = bpy.data.cameras.new("SourceComparison")
        camera = bpy.data.objects.new("SourceComparison", camera_data)
        scene.collection.objects.link(camera)
        camera_data.type = "ORTHO"
        camera_data.ortho_scale = 5.25
        scene.camera = camera
        for name, direction in views:
            camera.location = center + direction * 12
            camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
            path = output / "Comparison" / f"{level['level']}-{name}.png"
            path.parent.mkdir(exist_ok=True)
            scene.render.filepath = str(path)
            bpy.ops.render.render(write_still=True)
            paths.append(path)
    metrics = []
    for name, _ in views:
        images = [bpy.data.images.load(str(output / "Comparison" / f"{level['level']}-{name}.png"))
                  for level in provenance["levels"]]
        arrays = []
        reference = None
        for level, image in zip(provenance["levels"], images):
            pixels = np.empty(512 * 512 * 4, dtype=np.float32)
            image.pixels.foreach_get(pixels)
            pixels = pixels.reshape(512, 512, 4)
            alpha = pixels[:, :, 3:4]
            mask = alpha[:, :, 0] >= 0.5
            if reference is None:
                reference = mask
            metrics.append({
                "view": name, "level": level["level"],
                "coveredPixels": int(mask.sum()),
                "coverageRatioToSource": float(mask.sum() / reference.sum()),
                "silhouetteIntersectionOverUnion": float((mask & reference).sum() / (mask | reference).sum()),
            })
            pixels[:, :, :3] = pixels[:, :, :3] * alpha + 0.65 * (1 - alpha)
            pixels[:, :, 3] = 1
            arrays.append(pixels)
        comparison = bpy.data.images.new("Comparison-" + name, 512 * len(images), 512)
        comparison.pixels.foreach_set(np.concatenate(arrays, axis=1).ravel())
        comparison.filepath_raw = str(output / "Comparison" / f"all-{name}.png")
        comparison.file_format = "PNG"
        comparison.save()
    write_json(output / "Comparison" / "metrics.json", {
        "kind": "Internal CPU unlit albedo/alpha orthographic diagnostics, not game acceptance",
        "resolutionPerView": [512, 512], "samples": 8,
        "columnOrder": [level["level"] for level in provenance["levels"]],
        "results": metrics,
    })
    return [str(path.relative_to(output)) for path in paths]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mode", choices=("inspect", "prepare", "verify", "preview"), default="inspect")
    parser.add_argument("--leaf-method", choices=("collapse", "planar"), default="collapse")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    source = args.source_dir.resolve()
    output = args.output.resolve()
    if output == source or source in output.parents or output in source.parents:
        raise ValueError("Output must not overlap the source directory.")
    frozen = (output / "FROZEN.json").exists()
    if frozen and args.mode != "verify":
        raise ValueError("This handoff is frozen; experiments require a new version directory.")
    records = verify_sources(source, args.receipt)
    reset_scene()
    if args.mode in {"verify", "preview"}:
        provenance = json.loads((output / "provenance.json").read_text())
        if args.mode == "verify":
            verify_exports(output, provenance, write_report=not frozen)
        else:
            preview(output, provenance)
        verify_sources(source, args.receipt)
        return
    if args.mode == "prepare" and output.exists() and any(output.iterdir()):
        raise ValueError("Preparation requires a new empty version directory.")
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="tree-fbx-", dir=output) as temporary:
        safe_fbx = Path(temporary) / "geometry-only.fbx"
        sanitization = geometry_only_fbx(source / "tree_small_02_2k.fbx", safe_fbx)
        tree = import_geometry(safe_fbx)
        if any(image.filepath for image in bpy.data.images):
            raise ValueError("Geometry-only import unexpectedly referenced images.")
        report = {
            "blenderVersion": bpy.app.version_string,
            "sourceFiles": records, "sourceReceiptSha256": digest(args.receipt),
            "sanitization": sanitization, "mesh": mesh_report(tree),
            "sceneUnitScaleMeters": bpy.context.scene.unit_settings.scale_length,
            "limitations": "Source-only inspection; no Unreal or runtime performance test.",
        }
        write_json(output / "inspection.json", report)
        if args.mode == "inspect":
            print(json.dumps(report["mesh"], indent=2), flush=True)
        else:
            levels = prepare(tree, output, report, args.leaf_method)
            textures = output / "Textures"
            textures.mkdir()
            for entry in records:
                if entry["file"].endswith((".png", ".jpg")):
                    shutil.copyfile(source / entry["file"], textures / entry["file"])
            provenance = {
                "version": 1, "asset": "Tree Small 02", "author": "Rico Cilliers",
                "sourceUrl": "https://polyhaven.com/a/tree_small_02",
                "license": "CC0-1.0", "licenseUrl": "https://polyhaven.com/license",
                "blenderVersion": bpy.app.version_string, "toolSha256": digest(Path(__file__)),
                "sourceReceiptSha256": digest(args.receipt), "sourceFiles": records,
                "levels": levels, "unitContract": {
                    "blenderMetersPerUnit": 1.0, "fbxExportApplyUnitScale": True,
                    "fbxExportScaleOption": "FBX_SCALE_UNITS",
                    "axisForward": "-Y", "axisUp": "Z",
                    "root": "Original source origin (0,0,0), no recenter or ground translation.",
                },
                "method": {
                    "wood": "Blender Decimate COLLAPSE independently per material.",
                    "leaves": args.leaf_method,
                    "preserved": "Both UV layers and source material slot order.",
                },
                "limitations": "No wind, collision, UE import, runtime LOD thresholds or GPU performance verified.",
            }
            write_json(output / "provenance.json", provenance)
            verify_exports(output, provenance)
    verify_sources(source, args.receipt)


if __name__ == "__main__":
    main()
