"""Inspect receipted resources using the existing FBX/image reader; no scene import."""
import argparse
import importlib.util
import json
from pathlib import Path
import sys

import numpy as np

from ResourceAcquisition import ASSETS, contained, digest, run_guard, validate_manifest, write_new


def load_reader(path, expected_sha):
    if digest(path) != expected_sha.upper():
        raise ValueError("Existing source inspector differs from the admitted hash.")
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location("resource_existing_reader", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.KEEP_ARRAYS = module.KEEP_ARRAYS | {b"Normals", b"NormalsIndex", b"UV", b"UVIndex"}
    return module


def mapped_corner_indices(reader, layer, index_name, direct_count, vertex_ids, face_ids):
    mapping = reader.child(layer, "MappingInformationType")["props"][0]
    reference = reader.child(layer, "ReferenceInformationType")["props"][0]
    if mapping == "ByPolygonVertex":
        domain = np.arange(len(vertex_ids))
    elif mapping in ("ByVertice", "ByVertex"):
        domain = vertex_ids
    elif mapping == "ByPolygon":
        domain = face_ids
    elif mapping == "AllSame":
        domain = np.zeros(len(vertex_ids), dtype=np.int64)
    else:
        raise ValueError("Unsupported attribute mapping: " + mapping)
    if reference == "Direct":
        indices = domain
    elif reference == "IndexToDirect":
        table = np.asarray(reader.child(layer, index_name)["props"][0], dtype=np.int64)
        if domain.max() >= len(table):
            raise ValueError("Attribute domain is outside its index table.")
        indices = table[domain]
    else:
        raise ValueError("Unsupported attribute reference: " + reference)
    if indices.min() < 0 or indices.max() >= direct_count:
        raise ValueError("Attribute references outside the direct table.")
    return indices


def geometry_roles(reader, node):
    vertices = np.asarray(reader.child(node, "Vertices")["props"][0], dtype=np.float64).reshape(-1, 3)
    encoded = np.asarray(reader.child(node, "PolygonVertexIndex")["props"][0], dtype=np.int64)
    ends = np.flatnonzero(encoded < 0)
    starts = np.r_[0, ends[:-1] + 1]
    counts = ends - starts + 1
    if not len(ends) or ends[-1] != len(encoded) - 1 or np.any(counts < 3):
        raise ValueError("Malformed source polygons.")
    vertex_ids = np.where(encoded < 0, -encoded - 1, encoded)
    if vertex_ids.min() < 0 or vertex_ids.max() >= len(vertices) or not np.isfinite(vertices).all():
        raise ValueError("Invalid control points or polygon references.")
    face_ids = np.repeat(np.arange(len(counts)), counts)
    layer = reader.child(node, "LayerElementMaterial")
    material_table = np.asarray(reader.child(layer, "Materials")["props"][0], dtype=np.int64)
    mapping = reader.child(layer, "MappingInformationType")["props"][0]
    if mapping == "AllSame" and len(material_table) == 1:
        material_ids = np.full(len(counts), material_table[0], dtype=np.int64)
    elif mapping == "ByPolygon" and len(material_table) == len(counts):
        material_ids = material_table
    else:
        raise ValueError("Unsupported material assignment.")
    if material_ids.min() < 0:
        raise ValueError("Negative material slot.")
    corner_material = material_ids[face_ids]
    fan_corners, fan_faces = [], []
    for size in np.unique(counts):
        selected = np.flatnonzero(counts == size)
        for offset in range(1, int(size) - 1):
            fan_corners.append(starts[selected, None] + np.array([0, offset, offset + 1]))
            fan_faces.append(selected)
    fan_corners = np.concatenate(fan_corners)
    fan_material = material_ids[np.concatenate(fan_faces)]
    tri_positions = vertices[vertex_ids[fan_corners]]
    area = np.linalg.norm(np.cross(tri_positions[:, 1] - tri_positions[:, 0],
                                   tri_positions[:, 2] - tri_positions[:, 0]), axis=1) * 0.5
    roles = {
        str(int(slot)): {
            "polygons": int((material_ids == slot).sum()),
            "fanTriangleEstimate": int((fan_material == slot).sum()),
            "areaAtMost1eMinus12RawUnitsSquared": int(((fan_material == slot) & (area <= 1e-12)).sum()),
            "uvLayers": [], "normalLayers": [],
        } for slot in np.unique(material_ids)
    }
    for attribute in node["children"]:
        kind = attribute["name"]
        if kind not in ("LayerElementNormal", "LayerElementUV"):
            continue
        normal = kind == "LayerElementNormal"
        values = np.asarray(reader.child(attribute, "Normals" if normal else "UV")["props"][0],
                            dtype=np.float64).reshape(-1, 3 if normal else 2)
        if not np.isfinite(values).all():
            raise ValueError("Non-finite UV/normal array.")
        indices = mapped_corner_indices(reader, attribute, "NormalsIndex" if normal else "UVIndex",
                                        len(values), vertex_ids, face_ids)
        if normal:
            near_zero = np.linalg.norm(values, axis=1) <= 1e-4
            for slot, role in roles.items():
                role["normalLayers"].append({
                    "layerIndex": attribute["props"][0],
                    "nearZeroReferencedCorners": int(near_zero[indices[corner_material == int(slot)]].sum()),
                    "directVectors": len(values),
                    "referencedDirectVectors": len(np.unique(indices[corner_material == int(slot)])),
                })
        else:
            corners = values[indices]
            triangles = corners[fan_corners]
            first, second = triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0]
            det = np.abs(first[:, 0] * second[:, 1] - first[:, 1] * second[:, 0])
            for slot, role in roles.items():
                selected = corners[corner_material == int(slot)]
                role["uvLayers"].append({
                    "layerIndex": attribute["props"][0],
                    "name": reader.child(attribute, "Name")["props"][0],
                    "min": selected.min(axis=0).tolist(), "max": selected.max(axis=0).tolist(),
                    "fanTrianglesWithDeterminantAtMost1eMinus12": int(((fan_material == int(slot)) & (det <= 1e-12)).sum()),
                })
    used = np.unique(vertex_ids)
    return {
        "geometryId": node["props"][0],
        "controlPoints": len(vertices), "referencedControlPoints": len(used),
        "unusedControlPoints": len(vertices) - len(used),
        "referencedLocalBounds": {"min": vertices[used].min(axis=0).tolist(),
                                  "max": vertices[used].max(axis=0).tolist()},
        "rolesByMaterialSlot": roles,
        "measurement": "Float64 raw local coordinates; area <=1e-12 raw units squared, normal length <=1e-4, abs UV determinant <=1e-12. Non-triangle polygons use a diagnostic fan estimate, not native triangulation.",
    }


def inspect_fbx(reader, path, guard):
    basic = reader.inspect_fbx(path)
    guard()
    with path.open("rb") as stream:
        parsed = reader.FbxReader(stream, path.stat().st_size).parse()
    objects = reader.child({"children": parsed}, "Objects")["children"]
    roles = []
    for node in objects:
        if node["name"] == "Geometry" and node["props"][2] == "Mesh":
            guard()
            roles.append(geometry_roles(reader, node))
    basic["roleSpecificAttributes"] = roles
    basic["preparationBoundary"] = "Original mesh/model IDs, transforms and material slots only. No evaluated world axes/scale or automatic merging/reduction. Check UV per role, not only layer presence."
    return basic


def inspect_sources(manifest_path, receipt_path, source_root, output, reader, guard):
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    validate_manifest(manifest)
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    if receipt["manifestSha256"] != digest(manifest_path):
        raise ValueError("Acquired receipt differs from the manifest.")
    expected = [(asset["id"], item["name"]) for asset in manifest["assets"] for item in asset["files"]]
    if [(entry["asset"], entry["file"]) for entry in receipt["files"]] != expected:
        raise ValueError("Receipt does not identify the exact four-asset palette.")
    result = {
        "scope": "Read-only four-resource source inventory; no bpy scene, image-reference traversal or Unreal.",
        "manifestSha256": digest(manifest_path), "sourceReceiptSha256": digest(receipt_path),
        "inspectionToolSha256": digest(Path(__file__)),
        "existingReaderSha256": digest(Path(reader.__file__)),
        "files": [],
        "limits": "Parser cancellation is checked before each file/pass/geometry; the reused parser has its existing node/depth/120-second bounds. Raw local bounds are not transformed native render bounds.",
    }
    for entry in receipt["files"]:
        guard()
        path = contained(source_root, entry["asset"], entry["file"])
        if path.stat().st_size != entry["bytes"] or digest(path) != entry["sha256"].upper():
            raise ValueError("Acquired source changed: " + entry["file"])
        info = inspect_fbx(reader, path, guard) if path.suffix == ".fbx" else reader.inspect_image(path)
        if digest(path) != entry["sha256"].upper():
            raise ValueError("Source changed during inspection.")
        result["files"].append({
            "asset": entry["asset"], "file": entry["file"], "sha256": entry["sha256"],
            "sourceInspection": info,
        })
        print("Inspected " + entry["asset"] + " / " + entry["file"], flush=True)
    guard()
    write_new(output, result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--inspector", type=Path, required=True)
    parser.add_argument("--inspector-sha256", required=True)
    args = parser.parse_args()
    guard = lambda: run_guard(args.control, args.run_id)
    guard()
    if json.loads(args.manifest.read_text(encoding="utf-8-sig"))["runId"] != args.run_id:
        raise ValueError("Inspection run ID differs from the admitted manifest.")
    reader = load_reader(args.inspector, args.inspector_sha256)
    result = inspect_sources(args.manifest, args.receipt, args.source_root, args.output, reader, guard)
    print(f"Inspected {len(result['files'])} receipted files; no preparation or import.")


if __name__ == "__main__":
    main()
