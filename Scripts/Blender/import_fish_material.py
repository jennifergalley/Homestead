"""Opt-in fish wet-film parent; called by import_props.textured_instance in UE.

Usage: import_props.main(['CaughtFish']) in Integration's owned editor.
"""
import math

import unreal

PARENT = "/Game/SurvivalGame/Materials/M_CaughtFishWet"
PARAMETERS = (
    ("coat_weight", "FishCoatWeight", unreal.MaterialProperty.MP_CUSTOM_DATA_0),
    ("coat_roughness", "FishCoatRoughness", unreal.MaterialProperty.MP_CUSTOM_DATA_1),
)


def wet_fish_parent(base_parent: unreal.Material, settings: dict) -> unreal.Material:
    if not isinstance(settings, dict):
        raise ValueError("Wet-fish material settings must be an object")
    values = {}
    for key, _, _ in PARAMETERS:
        value = settings.get(key)
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or not 0 <= value <= 1:
            raise ValueError("Invalid wet-fish material " + key)
        values[key] = float(value)
    library = unreal.EditorAssetLibrary
    edit = unreal.MaterialEditingLibrary
    material = library.load_asset(PARENT) if library.does_asset_exist(PARENT) else library.duplicate_asset(
        base_parent.get_path_name(), PARENT)
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create/load fish wet-film parent: " + PARENT)
    changed = material.get_editor_property("shading_model") != unreal.MaterialShadingModel.MSM_CLEAR_COAT
    if changed:
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_CLEAR_COAT)
    for index, (key, name, target) in enumerate(PARAMETERS):
        node = edit.get_material_property_input_node(material, target)
        if node is None:
            changed = True
            node = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 850 + index * 180)
            node.set_editor_property("parameter_name", name)
            if not edit.connect_material_property(node, "", target):
                raise RuntimeError("Could not connect fish wet-film parameter: " + name)
        if not isinstance(node, unreal.MaterialExpressionScalarParameter) or str(node.get_editor_property("parameter_name")) != name:
            raise RuntimeError("Unexpected fish wet-film parent input: " + name)
        if abs(float(node.get_editor_property("default_value")) - values[key]) > 1e-6:
            changed = True
            node.set_editor_property("default_value", values[key])
    if changed:
        edit.recompile_material(material)
        if not library.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError("Could not save fish wet-film parent: " + PARENT)
    return material
