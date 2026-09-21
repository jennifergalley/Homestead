"""MAIN-only guarded image-reference stripping through the existing FBX helper."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import sys

_spec = importlib.util.spec_from_file_location("resource_acquisition", Path(__file__).with_name("ResourceAcquisition.py"))
_acquisition = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_acquisition)
ASSETS = _acquisition.ASSETS
contained, digest, run_guard = _acquisition.contained, _acquisition.digest, _acquisition.run_guard
validate_manifest, write_new = _acquisition.validate_manifest, _acquisition.write_new


def ordinary_ancestors(path):
    for part in (path, *path.parents):
        if _acquisition.is_reparse_path(part):
            raise ValueError("Source/preparation path contains a link or junction.")


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
    return len(left.elems) == len(right.elems) and all(identical(a, b) for a, b in zip(left.elems, right.elems))


def selected_ids(root, models):
    objects = next(node for node in root.elems if node.id == b"Objects").elems
    by_id = {node.props[0]: node for node in objects}
    if len(by_id) != len(objects) or not models:
        raise ValueError("Empty selection or duplicate source object IDs.")
    if any(node.id not in {b"Geometry", b"Model", b"Material", b"Texture", b"Video"} for node in objects):
        raise ValueError("Unsupported non-static source object type.")
    links = next(node for node in root.elems if node.id == b"Connections").elems
    keep = set()
    seen_models = set()
    for model in models:
        model_id = model["id"]
        if model_id in seen_models or model_id not in by_id:
            raise ValueError("Missing or duplicate selected model.")
        seen_models.add(model_id)
        actual = by_id[model_id]
        if actual.id != b"Model" or actual.props[1].split(b"\0")[0].decode() != model["name"]:
            raise ValueError("Selected model ID/name differs.")
        bindings = [node.props[1] for node in links
                    if node.id == b"C" and node.props[0] == b"OO" and node.props[2] == model_id]
        parents = [node.props[2] for node in links
                   if node.id == b"C" and node.props[0] == b"OO" and node.props[1] == model_id]
        if parents != [0] or any(identifier not in by_id for identifier in bindings):
            raise ValueError("Selected model has an unsupported parent/binding.")
        geometry = [identifier for identifier in bindings if by_id[identifier].id == b"Geometry"]
        materials = [identifier for identifier in bindings if by_id[identifier].id == b"Material"]
        names = [by_id[identifier].props[1].split(b"\0")[0].decode() for identifier in materials]
        if (geometry != [model["geometryId"]] or names != model["materials"]
                or len(materials) not in (1, 2) or len(bindings) != 1 + len(materials)):
            raise ValueError("Selected geometry or ordered one/two-material bindings differ.")
        keep.update([model_id, *bindings])
    return keep


def select_geometry_only(helper, source, target, models):
    """Reuse the existing selector algorithm with explicit one/two-role bindings."""
    root, version = helper.parse_fbx.parse(str(source))
    objects = next(node for node in root.elems if node.id == b"Objects")
    keep = selected_ids(root, models)
    removed = {node.props[0] for node in objects.elems} - keep
    retained = [node for node in objects.elems if node.props[0] in keep]
    definitions = next(node for node in root.elems if node.id == b"Definitions")
    original_types = {node.id for node in objects.elems}
    total = 0
    for definition in definitions.elems:
        if definition.id == b"ObjectType":
            count = next(node for node in definition.elems if node.id == b"Count")
            if definition.props[0] in original_types:
                count.props[0] = sum(node.id == definition.props[0] for node in retained)
            total += count.props[0]
    next(node for node in definitions.elems if node.id == b"Count").props[0] = total
    names = ("BOOL", "CHAR", "INT8", "INT16", "INT32", "INT64", "FLOAT32", "FLOAT64",
             "BYTES", "STRING", "INT32_ARRAY", "INT64_ARRAY", "FLOAT32_ARRAY",
             "FLOAT64_ARRAY", "BOOL_ARRAY", "BYTE_ARRAY")
    methods = {getattr(helper.data_types, name): "add_" + name.lower() for name in names}

    def convert(node):
        result = helper.encode_bin.FBXElem(node.id)
        for kind, value in zip(node.props_type, node.props):
            getattr(result, methods[kind])(value)
        for child in node.elems:
            if node.id == b"Objects" and child.props[0] in removed:
                continue
            if node.id == b"Connections" and child.id == b"C" and any(
                    value in removed for value in child.props[1:3]):
                continue
            result.elems.append(convert(child))
        return result

    helper.encode_bin.write(str(target), convert(root), version)


def verify_retained(helper, source, prepared, models=None):
    before, version = helper.parse_fbx.parse(str(source))
    after, actual_version = helper.parse_fbx.parse(str(prepared))
    old_objects = next(node for node in before.elems if node.id == b"Objects").elems
    new_objects = next(node for node in after.elems if node.id == b"Objects").elems
    allowed = {b"Geometry", b"Model", b"Material", b"Texture", b"Video"}
    if any(node.id not in allowed for node in old_objects):
        raise ValueError("Source has unsupported non-static object types.")
    images = {node.props[0] for node in old_objects if node.id in {b"Texture", b"Video"}}
    keep = selected_ids(before, models) if models is not None else {
        node.props[0] for node in old_objects if node.props[0] not in images}
    removed = {node.props[0] for node in old_objects} - keep
    retained = [node for node in old_objects if node.props[0] in keep]
    if version != actual_version or len(retained) != len(new_objects) or not all(
            identical(a, b) for a, b in zip(retained, new_objects)):
        raise ValueError("Prepared FBX changed retained geometry/model/material data.")
    if not identical(next(node for node in before.elems if node.id == b"GlobalSettings"),
                     next(node for node in after.elems if node.id == b"GlobalSettings")):
        raise ValueError("Prepared FBX changed source unit/axis settings.")
    connections = next(node for node in before.elems if node.id == b"Connections").elems
    expected = [node for node in connections if not (
        node.id == b"C" and any(value in removed for value in node.props[1:3]))]
    actual = next(node for node in after.elems if node.id == b"Connections").elems
    if len(expected) != len(actual) or not all(identical(a, b) for a, b in zip(expected, actual)):
        raise ValueError("Prepared FBX changed non-image connections/material order.")
    return {"fbxVersion": version, "retainedObjects": len(retained), "removedObjects": len(removed),
            "removedImageObjects": len(images), "retainedArraysAndTransformsIdentical": True}


def prepare(manifest_path, receipt_path, inventory_path, selection_path, source_root, output, helper_path, helper_sha, guard):
    for path in (manifest_path, receipt_path, inventory_path, selection_path, source_root, output, helper_path):
        ordinary_ancestors(path)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    validate_manifest(manifest)
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    inventory = json.loads(inventory_path.read_text(encoding="utf-8-sig"))
    selection = json.loads(selection_path.read_text(encoding="utf-8-sig"))
    if receipt["manifestSha256"] != digest(manifest_path) or inventory["manifestSha256"] != digest(manifest_path):
        raise ValueError("Manifest differs from acquired or inspected inputs.")
    if inventory["sourceReceiptSha256"] != digest(receipt_path):
        raise ValueError("Inventory does not identify the acquired receipt.")
    if (selection["manifestSha256"] != digest(manifest_path)
            or selection["sourceReceiptSha256"] != digest(receipt_path)
            or selection["sourceInventorySha256"] != digest(inventory_path)
            or tuple(asset["id"] for asset in selection["assets"]) != ASSETS):
        raise ValueError("Selection differs from the exact acquired/inspected inputs.")
    expected = [(asset["id"], item["name"]) for asset in manifest["assets"] for item in asset["files"]]
    if [(entry["asset"], entry["file"]) for entry in receipt["files"]] != expected:
        raise ValueError("Receipt does not identify the exact four-resource input closure.")
    if digest(helper_path) != helper_sha.upper():
        raise ValueError("Existing geometry helper differs from admitted bytes.")
    if output.exists():
        raise FileExistsError("Preparation requires a fresh candidate namespace.")
    if output.resolve() == source_root.resolve() or output.resolve().is_relative_to(source_root.resolve()):
        raise ValueError("Prepared output cannot be inside raw originals.")
    guard()
    spec = importlib.util.spec_from_file_location("resource_existing_geometry", helper_path)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    output.mkdir(parents=True)
    results = []
    for asset in ASSETS:
        guard()
        originals = [entry for entry in receipt["files"] if entry["asset"] == asset and entry["file"].endswith(".fbx")]
        if len(originals) != 1:
            raise ValueError("Expected one receipted original FBX per asset.")
        original = originals[0]
        chosen = next(entry for entry in selection["assets"] if entry["id"] == asset)
        if chosen["sourceFile"] != original["file"] or chosen["sourceSha256"] != original["sha256"].upper():
            raise ValueError("Selection identifies a different original.")
        inspected = [entry for entry in inventory["files"] if entry["asset"] == asset and entry["file"] == original["file"]]
        if len(inspected) != 1 or inspected[0]["sha256"].upper() != original["sha256"].upper():
            raise ValueError("Missing exact inspected source FBX.")
        source = contained(source_root, asset, original["file"])
        if source.stat().st_size != original["bytes"] or digest(source) != original["sha256"].upper():
            raise ValueError("Original changed since acquisition.")
        observed_models = {model["id"]: model for model in inspected[0]["sourceInspection"]["models"]}
        for model in chosen["models"]:
            observed = observed_models[model["id"]]
            if (observed["name"] != model["name"] or observed["geometryIds"] != [model["geometryId"]]
                    or observed["materialSlotsInConnectionOrder"] != model["materials"]):
                raise ValueError("Selected model differs from measured inventory.")
        target = output / (asset + "_selected.fbx")
        select_geometry_only(helper, source, target, chosen["models"])
        guard()
        result = verify_retained(helper, source, target, chosen["models"])
        if digest(source) != original["sha256"].upper():
            raise ValueError("Original changed during preparation.")
        result.update({
            "asset": asset, "file": target.name, "bytes": target.stat().st_size, "sha256": digest(target),
            "sourceFile": original["file"], "sourceSha256": original["sha256"],
            "sourceModels": [observed_models[model["id"]] for model in chosen["models"]],
            "roleAttributes": [entry for entry in inspected[0]["sourceInspection"]["roleSpecificAttributes"]
                               if entry["geometryId"] in {model["geometryId"] for model in chosen["models"]}],
        })
        results.append(result)
        print("Prepared image-reference-free source: " + asset, flush=True)
    guard()
    write_new(output / "provenance.json", {
        "state": "candidate-source-only-not-unreal-imported",
        "manifestSha256": digest(manifest_path), "sourceReceiptSha256": digest(receipt_path),
        "sourceInventorySha256": digest(inventory_path),
        "selectionSha256": digest(selection_path),
        "toolSha256": digest(Path(__file__)), "geometryHelperSha256": digest(helper_path),
        "files": results,
        "operations": "Existing binary parser/writer selects explicit model/geometry/material IDs with one/two role bindings and removes other objects. All selected arrays, transforms and non-image bindings retained and compared.",
        "limits": "No bpy scene import, merging, mesh reduction, UV/normal repair, material graph or rendering. Main selects actual model/LOD IDs from inventory at native import. These are not final frozen art.",
    })


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ("manifest", "receipt", "inventory", "selection", "source-root", "output", "helper", "control"):
        parser.add_argument("--" + field, type=Path, required=True)
    parser.add_argument("--helper-sha256", required=True)
    parser.add_argument("--run-id", required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else None)
    stop_path = os.environ.get("HOMESTEAD_SOURCE_STOP_PATH")
    if not stop_path:
        raise RuntimeError("The admitted source supervisor stop path is required.")

    def guard():
        run_guard(args.control, args.run_id)
        if Path(stop_path).exists():
            raise RuntimeError("Source preparation cancelled at an asset boundary.")

    if json.loads(args.manifest.read_text(encoding="utf-8-sig"))["runId"] != args.run_id:
        raise ValueError("Preparation run differs from manifest admission.")
    prepare(args.manifest, args.receipt, args.inventory, args.selection, args.source_root, args.output,
            args.helper, args.helper_sha256, guard)


if __name__ == "__main__":
    main()
