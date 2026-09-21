"""Read-only frozen FBX attribution; Python only, no bpy or Blender process."""
import hashlib
import importlib
import json
from pathlib import Path
import sys
import types

import numpy as np

SOURCE = Path(r"E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-fuzzy-robot\Assets\Environment\TreeSmall02Prepared\v3\TreeSmall02_LOD2.fbx")
OUTPUT = Path(__file__).with_suffix(".json")
EXPECTED = "03365d16ad77535e28c8f3b7243a8183d30768af3c719589cef58a44ed12e283"
PARSER = Path(r"E:\Tools\blender-4.5.14-windows-x64\4.5\scripts\addons_core\io_scene_fbx")
ROLES = ("tree_small_02_branches", "tree_small_02_leaves", "tree_small_02_trunk")


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def child(parent, name):
    matches = [entry for entry in parent.elems if entry.id == name]
    if len(matches) != 1:
        raise ValueError(f"Expected exactly one {name!r}, got {len(matches)}.")
    return matches[0]


def mapped_indices(layer, direct_count, index_name, corners):
    mapping = child(layer, b"MappingInformationType").props[0]
    reference = child(layer, b"ReferenceInformationType").props[0]
    if mapping != b"ByPolygonVertex":
        raise ValueError(f"Unexpected mapping {mapping!r}; do not guess.")
    if reference == b"IndexToDirect":
        indices = np.asarray(child(layer, index_name).props[0], dtype=np.int64)
    elif reference == b"Direct":
        indices = np.arange(corners, dtype=np.int64)
    else:
        raise ValueError(f"Unexpected reference {reference!r}.")
    if len(indices) != corners or indices.min() < 0 or indices.max() >= direct_count:
        raise ValueError("Corner mapping out of range.")
    return indices, {"mapping": mapping.decode(), "reference": reference.decode()}


if sha256(SOURCE) != EXPECTED:
    raise ValueError("Frozen source hash mismatch.")

# Load only the official standalone binary reader, not the add-on entry point.
package = types.ModuleType("readonly_fbx")
package.__path__ = [str(PARSER)]
sys.modules[package.__name__] = package
parse_fbx = importlib.import_module("readonly_fbx.parse_fbx")
root, version = parse_fbx.parse(str(SOURCE))
objects = child(root, b"Objects")
geometry = child(objects, b"Geometry")
model = child(objects, b"Model")
materials = {
    entry.props[0]: entry.props[1].split(b"\x00")[0].decode()
    for entry in objects.elems if entry.id == b"Material"
}
slots = [
    materials[entry.props[1]]
    for entry in child(root, b"Connections").elems
    if entry.id == b"C" and entry.props[0] == b"OO"
    and entry.props[1] in materials and entry.props[2] == model.props[0]
]
if slots != list(ROLES):
    raise ValueError(f"Unexpected material connection order: {slots}")
vertices = np.asarray(child(geometry, b"Vertices").props[0], dtype=np.float64).reshape(-1, 3)
encoded = np.asarray(child(geometry, b"PolygonVertexIndex").props[0], dtype=np.int64)
ends = np.flatnonzero(encoded < 0)
if not np.array_equal(ends, np.arange(2, len(encoded), 3)):
    raise ValueError("Expected all triangles; no fan triangulation approximation allowed.")
corner_vertices = np.where(encoded < 0, -encoded - 1, encoded)
if corner_vertices.min() < 0 or corner_vertices.max() >= len(vertices):
    raise ValueError("Control point reference out of range.")
triangles = corner_vertices.reshape(-1, 3)
mat_layer = child(geometry, b"LayerElementMaterial")
if child(mat_layer, b"MappingInformationType").props[0] != b"ByPolygon":
    raise ValueError("Unexpected material mapping.")
material_ids = np.asarray(child(mat_layer, b"Materials").props[0], dtype=np.int64)
if len(material_ids) != len(triangles) or material_ids.min() < 0 or material_ids.max() >= len(ROLES):
    raise ValueError("Invalid material assignment.")
normal_layer = child(geometry, b"LayerElementNormal")
normals = np.asarray(child(normal_layer, b"Normals").props[0], dtype=np.float64).reshape(-1, 3)
normal_indices, normal_mapping = mapped_indices(normal_layer, len(normals), b"NormalsIndex", len(encoded))
uv_layer = next(entry for entry in geometry.elems if entry.id == b"LayerElementUV" and entry.props[0] == 0)
uvs = np.asarray(child(uv_layer, b"UV").props[0], dtype=np.float64).reshape(-1, 2)
uv_indices, uv_mapping = mapped_indices(uv_layer, len(uvs), b"UVIndex", len(encoded))
if not all(np.isfinite(array).all() for array in (vertices, normals, uvs)):
    raise ValueError("Non-finite source arrays; separate diagnosis required.")

normal_lengths = np.linalg.norm(normals, axis=1)
positions = vertices[triangles]
areas = np.linalg.norm(np.cross(positions[:, 1] - positions[:, 0],
                               positions[:, 2] - positions[:, 0]), axis=1) * 0.5
triangle_uvs = uvs[uv_indices.reshape(-1, 3)]
du = triangle_uvs[:, 1] - triangle_uvs[:, 0]
dv = triangle_uvs[:, 2] - triangle_uvs[:, 0]
determinants = np.abs(du[:, 0] * dv[:, 1] - du[:, 1] * dv[:, 0])
normal_threshold = 1e-4
area_threshold = 1e-12
uv_threshold = 1e-12
bad_normals = normal_lengths <= normal_threshold
bad_corner_normals = bad_normals[normal_indices]
bad_triangle_normals = bad_corner_normals.reshape(-1, 3).any(axis=1)
zero_area = areas == 0
tiny_area = areas <= area_threshold
zero_uv = determinants == 0
tiny_uv = determinants <= uv_threshold
corner_materials = np.repeat(material_ids, 3)
used_normal_ids = np.unique(normal_indices)
normal_records = []
for normal_id in np.flatnonzero(bad_normals):
    corner_mask = normal_indices == normal_id
    triangle_mask = corner_mask.reshape(-1, 3).any(axis=1)
    normal_records.append({
        "directIndexZeroBased": int(normal_id),
        "vector": normals[normal_id].tolist(),
        "length": float(normal_lengths[normal_id]),
        "referencedCorners": int(corner_mask.sum()),
        "referencedDistinctTriangles": int(triangle_mask.sum()),
        "referenced": bool(corner_mask.any()),
        "byMaterial": {
            role: {"corners": int((corner_mask & (corner_materials == index)).sum()),
                   "distinctTriangles": int((triangle_mask & (material_ids == index)).sum())}
            for index, role in enumerate(ROLES)
        },
        "firstReferencedTrianglesZeroBased": np.flatnonzero(triangle_mask)[:12].tolist(),
    })

report = {
    "sourceFile": SOURCE.name,
    "sourceSha256": EXPECTED,
    "fbxVersion": version,
    "execution": "CPython 3.13 + NumPy; installed official standalone FBX binary reader only; no bpy, Blender process, Unreal, writes to source or repair.",
    "measurements": {
        "normalNearZero": "Euclidean length <= 1e-4 of the direct normal vector; dimensionless. Corner mapping follows FBX reference indices.",
        "geometryZero": "Exact double-precision cross-product area == 0; no welding, tolerance merging or fan triangulation.",
        "geometryNearZero": "Triangle area <= 1e-12 square metres from raw local coordinates (FBX UnitScaleFactor=100 cm/unit).",
        "uv0Zero": "Exact abs(det(UV1-UV0,UV2-UV0)) == 0; determinant equals twice UV area.",
        "uv0NearZero": "Absolute UV0 determinant <= 1e-12, dimensionless; no UV normalization/welding.",
        "scope": "Frozen derived LOD2 only. Does not attribute defects to original acquisition vs decimation, or reproduce UE epsilon/normal/tangent algorithms."
    },
    "controlPoints": {"total": len(vertices), "referenced": len(np.unique(corner_vertices)),
                      "unused": len(vertices) - len(np.unique(corner_vertices))},
    "triangles": len(triangles), "corners": len(encoded),
    "normalMapping": normal_mapping, "uv0Mapping": uv_mapping,
    "uv0Name": child(uv_layer, b"Name").props[0].decode(),
    "directNormals": {
        "total": len(normals), "referenced": len(used_normal_ids),
        "unused": len(normals) - len(used_normal_ids),
        "exactZero": int((normal_lengths == 0).sum()),
        "nearZero": int(bad_normals.sum()),
        "referencedNearZero": int(bad_normals[used_normal_ids].sum()),
        "lengthThresholdCounts": {
            str(epsilon): int((normal_lengths <= epsilon).sum())
            for epsilon in (0, 1e-12, 1e-8, 1e-6, 1e-4)
        },
    },
    "nearZeroNormalAttribution": normal_records,
    "byMaterial": {},
}
for index, role in enumerate(ROLES):
    mask = material_ids == index
    report["byMaterial"][role] = {
        "triangles": int(mask.sum()),
        "nearZeroNormalCorners": int((bad_corner_normals & (corner_materials == index)).sum()),
        "trianglesWithNearZeroNormal": int((mask & bad_triangle_normals).sum()),
        "exactZeroAreaTriangles": int((mask & zero_area).sum()),
        "areaAtMost1eMinus12SquareMeters": int((mask & tiny_area).sum()),
        "exactZeroUv0DeterminantTriangles": int((mask & zero_uv).sum()),
        "uv0DeterminantAtMost1eMinus12": int((mask & tiny_uv).sum()),
        "bothNearZeroAreaAndUv0": int((mask & tiny_area & tiny_uv).sum()),
        "nearZeroNormalAndNearZeroArea": int((mask & bad_triangle_normals & tiny_area).sum()),
        "nearZeroNormalAndNearZeroUv0": int((mask & bad_triangle_normals & tiny_uv).sum()),
    }
report["supplementalUvLayersByRole"] = []
for layer in (e for e in geometry.elems if e.id == b"LayerElementUV"):
    direct = np.asarray(child(layer, b"UV").props[0], dtype=np.float64).reshape(-1, 2)
    indices, mapping = mapped_indices(layer, len(direct), b"UVIndex", len(encoded))
    corners = direct[indices].reshape(-1, 3, 2)
    edge1 = corners[:, 1] - corners[:, 0]
    edge2 = corners[:, 2] - corners[:, 0]
    det = np.abs(edge1[:, 0] * edge2[:, 1] - edge1[:, 1] * edge2[:, 0])
    report["supplementalUvLayersByRole"].append({
        "layerIndex": layer.props[0], "name": child(layer, b"Name").props[0].decode(),
        "byMaterial": {
            role: {
                "exactZeroDeterminants": int(((material_ids == index) & (det == 0)).sum()),
                "determinantsAtMost1eMinus12": int(((material_ids == index) & (det <= 1e-12)).sum()),
                "uvCornerMin": corners[material_ids == index].reshape(-1, 2).min(axis=0).tolist(),
                "uvCornerMax": corners[material_ids == index].reshape(-1, 2).max(axis=0).tolist(),
            }
            for index, role in enumerate(ROLES)
        }
    })
if sha256(SOURCE) != EXPECTED:
    raise ValueError("Frozen source changed during read-only attribution.")
OUTPUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, indent=2))
