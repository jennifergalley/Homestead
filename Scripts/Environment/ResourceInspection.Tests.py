"""Offline role-UV/bounds checks; no source downloads, scene import or engine."""
import importlib.util
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import MagicMock

import numpy as np

SPEC = importlib.util.spec_from_file_location("resource_inspect", Path(__file__).with_name("ResourceInspection.py"))
inspect = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(inspect)


def node(name, props=(), children=()):
    return {"name": name, "props": list(props), "children": list(children)}


def child(parent, name):
    values = [item for item in parent["children"] if item["name"] == name]
    if len(values) != 1:
        raise ValueError("Expected unique " + name)
    return values[0]


def attribute(kind, index, values, table, name=None):
    children = [
        node("MappingInformationType", ["ByPolygonVertex"]),
        node("ReferenceInformationType", ["IndexToDirect"]),
        node("Normals" if kind == "LayerElementNormal" else "UV", [values]),
        node("NormalsIndex" if kind == "LayerElementNormal" else "UVIndex", [table]),
    ]
    if name:
        children.append(node("Name", [name]))
    return node(kind, [index], children)


def fixture():
    return node("Geometry", [7, "fixture", "Mesh"], [
        node("Vertices", [[0, 0, 0, 1, 0, 0, 0, 1, 0,
                           0, 0, 1, 1, 0, 1, 0, 1, 1, 999, 999, 999]]),
        node("PolygonVertexIndex", [[0, 1, -3, 3, 4, -6]]),
        node("LayerElementMaterial", [0], [
            node("MappingInformationType", ["ByPolygon"]),
            node("Materials", [[0, 1]]),
        ]),
        attribute("LayerElementNormal", 0, [0, 0, 0, 0, 0, 1], [0, 1, 1, 1, 1, 1]),
        attribute("LayerElementUV", 0, [0, 0, 1, 0, 0, 1], [0, 0, 0, 0, 1, 2], "UVMap"),
        attribute("LayerElementUV", 1, [0, 0, 1, 0, 0, 1], [0, 1, 2, 0, 0, 0], "UV_map_01"),
    ])


class SourceRoleTests(unittest.TestCase):
    def test_large_tree_basic_profile_defers_attribute_arrays(self):
        reader = MagicMock()
        reader.inspect_fbx.return_value = {"models": 1}
        result = inspect.inspect_fbx(reader, Path("tree.fbx"), lambda: None, detailed=False)
        self.assertEqual(result["roleSpecificAttributes"], [])
        self.assertIn("must pass detailed UV/normal inspection", result["preparationBoundary"])

    def test_mature_fir_reader_limits_are_opt_in(self):
        path = Path(__file__).resolve().parents[1] / "inspect_woodland_sources.py"
        sha = inspect.digest(path)
        standard = inspect.load_reader(path, sha, detailed=False)
        self.assertEqual(standard.FbxReader.MaxFileBytes, 128 * 1024**2)
        mature = inspect.load_reader(path, sha, detailed=False, mature_fir=True)
        self.assertEqual(mature.FbxReader.MaxFileBytes, 256 * 1024**2)
        self.assertEqual(mature.FbxReader.MaxDecodedBytes, 768 * 1024**2)

    def test_role_specific_uvs_and_zero_normal(self):
        result = inspect.geometry_roles(SimpleNamespace(child=child), fixture())
        branch = result["rolesByMaterialSlot"]["0"]
        leaf = result["rolesByMaterialSlot"]["1"]
        self.assertEqual(branch["uvLayers"][0]["fanTrianglesWithDeterminantAtMost1eMinus12"], 1)
        self.assertEqual(branch["uvLayers"][1]["fanTrianglesWithDeterminantAtMost1eMinus12"], 0)
        self.assertEqual(leaf["uvLayers"][0]["fanTrianglesWithDeterminantAtMost1eMinus12"], 0)
        self.assertEqual(leaf["uvLayers"][1]["fanTrianglesWithDeterminantAtMost1eMinus12"], 1)
        self.assertEqual(branch["normalLayers"][0]["nearZeroReferencedCorners"], 1)
        self.assertEqual(leaf["normalLayers"][0]["nearZeroReferencedCorners"], 0)

    def test_referenced_bounds_exclude_unused_point(self):
        result = inspect.geometry_roles(SimpleNamespace(child=child), fixture())
        self.assertEqual(result["controlPoints"], 7)
        self.assertEqual(result["referencedControlPoints"], 6)
        self.assertEqual(result["unusedControlPoints"], 1)
        self.assertEqual(result["referencedLocalBounds"]["max"], [1, 1, 1])

    def test_invalid_attribute_reference_fails(self):
        mesh = fixture()
        normal = child(mesh, "LayerElementNormal")
        child(normal, "NormalsIndex")["props"][0][0] = 999
        with self.assertRaises(ValueError):
            inspect.geometry_roles(SimpleNamespace(child=child), mesh)

    def test_nonfinite_geometry_fails(self):
        mesh = fixture()
        child(mesh, "Vertices")["props"][0][0] = np.nan
        with self.assertRaises(ValueError):
            inspect.geometry_roles(SimpleNamespace(child=child), mesh)

    def test_unknown_mapping_fails(self):
        mesh = fixture()
        child(child(mesh, "LayerElementNormal"), "MappingInformationType")["props"][0] = "Unexpected"
        with self.assertRaises(ValueError):
            inspect.geometry_roles(SimpleNamespace(child=child), mesh)

    def test_collinear_triangle_is_reported(self):
        mesh = fixture()
        child(mesh, "Vertices")["props"][0][6:9] = [2, 0, 0]
        result = inspect.geometry_roles(SimpleNamespace(child=child), mesh)
        self.assertEqual(result["rolesByMaterialSlot"]["0"]["areaAtMost1eMinus12RawUnitsSquared"], 1)


if __name__ == "__main__":
    unittest.main()
