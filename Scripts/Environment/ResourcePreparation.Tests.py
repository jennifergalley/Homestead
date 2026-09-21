"""Offline checks of retained-data comparisons; no Blender or source authoring."""
import array
from collections import namedtuple
import importlib.util
from pathlib import Path
from types import SimpleNamespace
import unittest

SPEC = importlib.util.spec_from_file_location("resource_prepare", Path(__file__).with_name("ResourcePreparation.py"))
prepare = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prepare)
Element = namedtuple("Element", "id props props_type elems")


def element(kind, identifier, children=()):
    return Element(kind, [identifier], b"L", list(children))


def fixture():
    vertex = Element(b"Vertices", [array.array("d", [0, 1, 2])], b"d", [])
    geometry = element(b"Geometry", 1, [vertex])
    model = element(b"Model", 2)
    materials = [element(b"Material", 3), element(b"Material", 4)]
    image = element(b"Texture", 5)
    objects = Element(b"Objects", [], b"", [geometry, model, *materials, image])
    links = [
        Element(b"C", [b"OO", 1, 2], b"SLL", []),
        Element(b"C", [b"OO", 3, 2], b"SLL", []),
        Element(b"C", [b"OO", 4, 2], b"SLL", []),
        Element(b"C", [b"OP", 5, 3, b"DiffuseColor"], b"SLLS", []),
    ]
    settings = Element(b"GlobalSettings", [], b"", [Element(b"UnitScaleFactor", [1.0], b"D", [])])
    before = Element(b"", [], b"", [objects, Element(b"Connections", [], b"", links), settings])
    after = Element(b"", [], b"", [
        Element(b"Objects", [], b"", [geometry, model, *materials]),
        Element(b"Connections", [], b"", links[:-1]),
        settings,
    ])
    return before, after


class RetainedDataTests(unittest.TestCase):
    def verify(self, before, after):
        helper = SimpleNamespace(parse_fbx=SimpleNamespace(
            parse=lambda path: (before if path == "source" else after, 7400)))
        return prepare.verify_retained(helper, "source", "prepared")

    def test_multiple_materials_preserved(self):
        result = self.verify(*fixture())
        self.assertTrue(result["retainedArraysAndTransformsIdentical"])
        self.assertEqual(result["removedImageObjects"], 1)

    def test_array_change_fails(self):
        before, after = fixture()
        original = after.elems[0].elems[0]
        changed = element(b"Geometry", 1, [Element(b"Vertices", [array.array("d", [0, 9, 2])], b"d", [])])
        after.elems[0].elems[0] = changed
        self.assertIsNot(original, changed)
        with self.assertRaises(ValueError):
            self.verify(before, after)

    def test_connection_order_change_fails(self):
        before, after = fixture()
        after.elems[1].elems[1], after.elems[1].elems[2] = after.elems[1].elems[2], after.elems[1].elems[1]
        with self.assertRaises(ValueError):
            self.verify(before, after)

    def test_unexpected_object_fails(self):
        before, after = fixture()
        before.elems[0].elems.append(element(b"AnimationCurve", 8))
        with self.assertRaises(ValueError):
            self.verify(before, after)

    def test_source_axis_units_must_survive(self):
        before, after = fixture()
        after.elems[2] = Element(b"GlobalSettings", [], b"", [Element(b"UnitScaleFactor", [100.0], b"D", [])])
        with self.assertRaises(ValueError):
            self.verify(before, after)

    def selected_fixture(self):
        before, after = fixture()
        for tree in (before, after):
            objects = tree.elems[0].elems
            for index, name in ((1, b"sapling\0Model"), (2, b"branches\0Material"), (3, b"twigs\0Material")):
                old = objects[index]
                objects[index] = Element(old.id, [old.props[0], name], b"LS", old.elems)
            tree.elems[1].elems.insert(0, Element(b"C", [b"OO", 2, 0], b"SLL", []))
        model = {"id": 2, "name": "sapling", "geometryId": 1, "materials": ["branches", "twigs"]}
        return before, after, model

    def test_selected_two_role_model(self):
        before, after, model = self.selected_fixture()
        self.assertEqual(prepare.selected_ids(before, [model]), {1, 2, 3, 4})
        helper = SimpleNamespace(parse_fbx=SimpleNamespace(
            parse=lambda path: (before if path == "source" else after, 7400)))
        self.assertTrue(prepare.verify_retained(helper, "source", "prepared", [model])["retainedArraysAndTransformsIdentical"])

    def test_selected_material_order_is_exact(self):
        before, _, model = self.selected_fixture()
        model["materials"].reverse()
        with self.assertRaises(ValueError):
            prepare.selected_ids(before, [model])

    def test_selected_model_identity_is_exact(self):
        before, _, model = self.selected_fixture()
        with self.assertRaises(ValueError):
            prepare.selected_ids(before, [model | {"name": "wrong"}])
        with self.assertRaises(ValueError):
            prepare.selected_ids(before, [model, model])

    def test_selection_manifest_has_nine_confirmed_models(self):
        import json
        root = Path(__file__).resolve().parents[2]
        selection = json.loads((root / "Assets/Environment/WoodlandResources/candidate01/selection.json").read_text())
        models = [model for asset in selection["assets"] for model in asset["models"]]
        self.assertEqual(len(models), 9)
        self.assertEqual(len({model["id"] for model in models}), 9)
        self.assertEqual(len([model for model in models if len(model["materials"]) == 2]), 2)


if __name__ == "__main__":
    unittest.main()
