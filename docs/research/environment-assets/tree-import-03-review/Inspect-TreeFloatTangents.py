import hashlib
import importlib
import json
from pathlib import Path
import sys
import types

import numpy as np

sys.dont_write_bytecode = True
source = Path(r"E:\Repos\SurvivalGame\Assets\Environment\TreeSmall02Prepared\v3\TreeSmall02_LOD2.fbx")
expected = "03365d16ad77535e28c8f3b7243a8183d30768af3c719589cef58a44ed12e283"
assert hashlib.sha256(source.read_bytes()).hexdigest() == expected
package = types.ModuleType("io_scene_fbx")
package.__path__ = [r"E:\Tools\blender-4.5.14-windows-x64\4.5\scripts\addons_core\io_scene_fbx"]
sys.modules[package.__name__] = package
root, version = importlib.import_module("io_scene_fbx.parse_fbx").parse(str(source))


def child(node, key):
    matches = [entry for entry in node.elems if entry.id == key]
    assert len(matches) == 1
    return matches[0]


geometry = child(child(root, b"Objects"), b"Geometry")
vertices = np.asarray(child(geometry, b"Vertices").props[0]).reshape(-1, 3)
indices = np.asarray(child(geometry, b"PolygonVertexIndex").props[0])
assert np.array_equal(np.flatnonzero(indices < 0), np.arange(2, len(indices), 3))
triangles = np.where(indices < 0, -indices - 1, indices).reshape(-1, 3)
roles = np.asarray(child(child(geometry, b"LayerElementMaterial"), b"Materials").props[0])
uvs = []
for layer in sorted((n for n in geometry.elems if n.id == b"LayerElementUV"), key=lambda n: n.props[0]):
    assert child(layer, b"MappingInformationType").props[0] == b"ByPolygonVertex"
    assert child(layer, b"ReferenceInformationType").props[0] == b"IndexToDirect"
    values = np.asarray(child(layer, b"UV").props[0]).reshape(-1, 2)
    mapping = np.asarray(child(layer, b"UVIndex").props[0])
    mapped = values[mapping].reshape(-1, 3, 2).copy()
    mapped[:, :, 1] = 1 - mapped[:, :, 1]
    uvs.append(mapped)
active = uvs[0].copy()
active[roles == 0] = uvs[1][roles == 0]
uv32 = active.astype(np.float32)
d21 = uv32[:, 1] - uv32[:, 0]
d31 = uv32[:, 2] - uv32[:, 0]
det32 = d21[:, 0] * d31[:, 1] - d21[:, 1] * d31[:, 0]
d21d = uv32[:, 1].astype(np.float64) - uv32[:, 0].astype(np.float64)
d31d = uv32[:, 2].astype(np.float64) - uv32[:, 0].astype(np.float64)
det64 = d21d[:, 0] * d31d[:, 1] - d21d[:, 1] * d31d[:, 0]
rx, rz = np.deg2rad([-9.334666828389418e-6, 180.00000500895632])
rot_x = np.array([[1, 0, 0], [0, np.cos(rx), -np.sin(rx)], [0, np.sin(rx), np.cos(rx)]])
rot_z = np.array([[np.cos(rz), -np.sin(rz), 0], [np.sin(rz), np.cos(rz), 0], [0, 0, 1]])
converted = ((vertices @ (rot_z @ rot_x).T) * [-100, 100, 100]).astype(np.float32)
p = converted[triangles]
cross32 = np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0])
area_zero = np.all(cross32 == 0, axis=1)
result = {
    "sourceSha256": expected,
    "method": "Standalone official FBX parser, NumPy float32 UV arithmetic after UE V flip; predicted float32 model/axis/cm positions. No engine execution or asset changes.",
    "limit": "Source-derived prediction, not identification of the actual12 native invalid tangent corners or a complete MikkTSpace reproduction.",
    "byRole": {},
}
for role, name in enumerate(("branches", "leaves", "trunk")):
    mask = roles == role
    collapse = mask & (det32 == 0)
    result["byRole"][name] = {
        "triangles": int(mask.sum()),
        "uvFloat32DeterminantZero": int(collapse.sum()),
        "uvFloat64DeterminantZeroFromSameFloat32Coordinates": int((mask & (det64 == 0)).sum()),
        "predictedFloat32CrossProductZero": int((mask & area_zero).sum()),
        "zeroFloat32UvTriangleIds": np.flatnonzero(collapse).tolist(),
        "zeroFloat32GeometryTriangleIds": np.flatnonzero(mask & area_zero).tolist(),
    }
target = Path(__file__).with_name("tree-float-tangent-attribution.json")
assert not target.exists()
target.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
