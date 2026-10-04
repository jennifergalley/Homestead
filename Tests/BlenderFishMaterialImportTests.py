"""Offline import-contract tests, not evidence of a compiled Unreal material.

Usage: python Tests\\BlenderFishMaterialImportTests.py
"""
import copy
import importlib.util
import sys
import types
import unittest
from pathlib import Path
from unittest.mock import Mock, patch


class Material:
    def __init__(self):
        self.properties = {"shading_model": "default_lit"}

    def get_path_name(self):
        return "/Game/SurvivalGame/Materials/M_PropTextured"

    def get_editor_property(self, name):
        return self.properties.get(name, False)

    def set_editor_property(self, name, value):
        self.properties[name] = value


class Scalar:
    def __init__(self):
        self.properties = {"default_value": 0.0}

    def get_editor_property(self, name):
        return self.properties[name]

    def set_editor_property(self, name, value):
        self.properties[name] = value


class ImportContractTests(unittest.TestCase):
    def setUp(self):
        self.base = Material()
        self.material = copy.deepcopy(self.base)
        self.nodes = {}
        self.library = Mock()
        self.library.does_asset_exist.return_value = False
        self.library.duplicate_asset.return_value = self.material
        self.library.load_asset.return_value = self.material
        self.library.save_loaded_asset.return_value = True
        self.edit = Mock()
        self.edit.get_material_property_input_node.side_effect = lambda material, target: self.nodes.get(target)
        self.edit.create_material_expression.side_effect = lambda *args: Scalar()

        def connect(node, output, target):
            self.nodes[target] = node
            return True

        self.edit.connect_material_property.side_effect = connect
        unreal = types.SimpleNamespace(
            Material=Material, MaterialExpressionScalarParameter=Scalar,
            MaterialProperty=types.SimpleNamespace(MP_CUSTOM_DATA_0="coat", MP_CUSTOM_DATA_1="coat_roughness",
                                                   MP_BASE_COLOR="basecolor", MP_SUBSURFACE_COLOR="subsurface",
                                                   MP_OPACITY="opacity"),
            MaterialShadingModel=types.SimpleNamespace(MSM_CLEAR_COAT="clear_coat",
                                                       MSM_TWO_SIDED_FOLIAGE="two_sided_foliage"),
            EditorAssetLibrary=self.library, MaterialEditingLibrary=self.edit)
        source = Path(__file__).resolve().parents[1] / "Scripts" / "Blender" / "import_fish_material.py"
        with patch.dict(sys.modules, {"unreal": unreal}):
            spec = importlib.util.spec_from_file_location("fish_import_contract", source)
            self.module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(self.module)

    def test_new_parent_does_not_modify_shared_parent(self):
        self.module.wet_fish_parent(self.base, {"coat_weight": .65, "coat_roughness": .06})
        self.assertEqual(self.base.properties, {"shading_model": "default_lit"})
        self.assertEqual(self.material.properties["shading_model"], "clear_coat")
        self.library.duplicate_asset.assert_called_once_with(self.base.get_path_name(), self.module.PARENT)
        self.assertEqual(self.nodes["coat"].properties, {"parameter_name": "FishCoatWeight", "default_value": .65})
        self.assertEqual(self.nodes["coat_roughness"].properties["default_value"], .06)
        self.edit.recompile_material.assert_called_once_with(self.material)

    def test_reimport_does_not_recompile_unchanged_parent(self):
        settings = {"coat_weight": .65, "coat_roughness": .06}
        self.module.wet_fish_parent(self.base, settings)
        self.library.does_asset_exist.return_value = True
        self.edit.recompile_material.reset_mock()
        self.library.save_loaded_asset.reset_mock()
        self.module.wet_fish_parent(self.base, settings)
        self.edit.recompile_material.assert_not_called()
        self.library.save_loaded_asset.assert_not_called()

    def test_invalid_settings_do_not_write_assets(self):
        for settings in (None, [], {}, {"coat_weight": True, "coat_roughness": .06},
                         {"coat_weight": float("nan"), "coat_roughness": .06},
                         {"coat_weight": .65, "coat_roughness": 1.1}):
            with self.subTest(settings=settings):
                with self.assertRaises(ValueError):
                    self.module.wet_fish_parent(self.base, settings)
        self.library.duplicate_asset.assert_not_called()
        self.edit.recompile_material.assert_not_called()

    def test_parent_input_conflict_and_save_failure_are_explicit(self):
        self.nodes["coat"] = Scalar()
        self.nodes["coat"].properties["parameter_name"] = "UnrelatedInput"
        with self.assertRaisesRegex(RuntimeError, "Unexpected"):
            self.module.wet_fish_parent(self.base, {"coat_weight": .65, "coat_roughness": .06})
        self.nodes.clear()
        self.library.save_loaded_asset.return_value = False
        with self.assertRaisesRegex(RuntimeError, "Could not save"):
            self.module.wet_fish_parent(self.base, {"coat_weight": .65, "coat_roughness": .06})

    def test_membrane_parent_is_isolated_textured_and_idempotent(self):
        color = Scalar()
        self.nodes["basecolor"] = color
        settings = {"coat_weight": .65, "coat_roughness": .06, "membrane_opacity": .35,
                    "membrane_materials": ["M_RiverTroutMembrane"]}
        self.module.fish_material_parent(self.base, settings, "M_RiverTroutMembrane")
        self.assertEqual(self.base.properties, {"shading_model": "default_lit"})
        self.library.duplicate_asset.assert_called_once_with(self.base.get_path_name(), self.module.MEMBRANE_PARENT)
        self.assertEqual(self.material.properties, {"shading_model": "two_sided_foliage", "two_sided": True})
        self.assertIs(self.nodes["subsurface"], color)
        self.assertEqual(self.nodes["opacity"].properties,
                         {"parameter_name": "FishMembraneOpacity", "default_value": .35})
        self.library.does_asset_exist.return_value = True
        self.edit.recompile_material.reset_mock()
        self.library.save_loaded_asset.reset_mock()
        self.module.fish_material_parent(self.base, settings, "M_RiverTroutMembrane")
        self.edit.recompile_material.assert_not_called()
        self.library.save_loaded_asset.assert_not_called()

    def test_opaque_skin_still_uses_clear_coat(self):
        settings = {"coat_weight": .65, "coat_roughness": .06, "membrane_opacity": .35,
                    "membrane_materials": ["M_RiverTroutMembrane"]}
        self.module.fish_material_parent(self.base, settings, "M_RiverTrout")
        self.assertEqual(self.material.properties["shading_model"], "clear_coat")
        self.library.duplicate_asset.assert_called_once_with(self.base.get_path_name(), self.module.PARENT)

    def test_invalid_membrane_metadata_does_not_write_assets(self):
        for names in ("M_RiverTroutMembrane", [False], [""], ["same", "same"]):
            with self.subTest(names=names), self.assertRaises(ValueError):
                self.module.fish_material_parent(self.base, {"membrane_materials": names}, "test")
        for value in (False, float("nan"), -1, 1.1):
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.module.membrane_fish_parent(self.base, {"coat_weight": .65, "coat_roughness": .06,
                                                           "membrane_opacity": value})
        self.library.duplicate_asset.assert_not_called()

    def test_membrane_connections_and_conflicts_fail_explicitly(self):
        settings = {"coat_weight": .65, "coat_roughness": .06, "membrane_opacity": .35}
        with self.assertRaisesRegex(RuntimeError, "lacks"):
            self.module.membrane_fish_parent(self.base, settings)
        self.nodes["basecolor"] = Scalar()
        self.nodes["subsurface"] = Scalar()
        with self.assertRaisesRegex(RuntimeError, "Unexpected"):
            self.module.membrane_fish_parent(self.base, settings)
        del self.nodes["subsurface"]
        self.edit.connect_material_property.return_value = False
        self.edit.connect_material_property.side_effect = None
        with self.assertRaisesRegex(RuntimeError, "Could not connect"):
            self.module.membrane_fish_parent(self.base, settings)

    def test_membrane_save_failure_and_opacity_conflict_are_explicit(self):
        settings = {"coat_weight": .65, "coat_roughness": .06, "membrane_opacity": .35}
        self.nodes["basecolor"] = Scalar()
        self.nodes["opacity"] = Scalar()
        self.nodes["opacity"].properties["parameter_name"] = "UnrelatedOpacity"
        with self.assertRaisesRegex(RuntimeError, "Unexpected"):
            self.module.membrane_fish_parent(self.base, settings)
        del self.nodes["opacity"]
        self.library.save_loaded_asset.return_value = False
        with self.assertRaisesRegex(RuntimeError, "Could not save"):
            self.module.membrane_fish_parent(self.base, settings)


if __name__ == "__main__":
    unittest.main()
