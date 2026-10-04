"""Isolated opaque-skin/thin-fin parents; called by import_props.textured_instance in UE.

Usage: import_props.main(['CaughtFish']) in Integration's owned editor.
"""
import math

import unreal

PARENT = "/Game/SurvivalGame/Materials/M_CaughtFishWet"
MEMBRANE_PARENT = "/Game/SurvivalGame/Materials/M_CaughtFishMembrane"
PARAMETERS = (
    ("coat_weight", "FishCoatWeight"),
    ("coat_roughness", "FishCoatRoughness"),
)


def _unit_setting(settings: dict, key: str) -> float:
    if not isinstance(settings, dict):
        raise ValueError("Wet-fish material settings must be an object")
    value = settings.get(key)
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or not 0 <= value <= 1:
        raise ValueError("Invalid wet-fish material " + key)
    return float(value)


def _load_parent(base_parent: unreal.Material, path: str) -> unreal.Material:
    library = unreal.EditorAssetLibrary
    material = library.load_asset(path) if library.does_asset_exist(path) else library.duplicate_asset(
        base_parent.get_path_name(), path)
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create/load fish parent: " + path)
    return material


def _save_parent(material: unreal.Material, path: str, changed: bool) -> None:
    if changed:
        unreal.MaterialEditingLibrary.recompile_material(material)
        if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError("Could not save fish parent: " + path)


def wet_fish_parent(base_parent: unreal.Material, settings: dict) -> unreal.Material:
    values = {key: _unit_setting(settings, key) for key, _ in PARAMETERS}
    material = _load_parent(base_parent, PARENT)
    edit = unreal.MaterialEditingLibrary
    roughness = edit.get_material_property_input_node(material, unreal.MaterialProperty.MP_ROUGHNESS)
    if roughness is None:
        raise RuntimeError("Fish wet-skin parent lacks its textured roughness")

    # UE 5.8's Python MaterialProperty enum does not expose ClearCoat Custom Data inputs.
    # Preserve a fish-local wet-film response by scaling the baked roughness instead of attempting
    # unsupported pins, keeping the ordinary prop parent and all non-fish imports unchanged.
    if isinstance(roughness, unreal.MaterialExpressionMultiply):
        return material
    scale = 1.0 - values["coat_weight"] * (1.0 - values["coat_roughness"])
    multiply = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 850)
    scalar = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 1025)
    scalar.set_editor_property("parameter_name", "FishWetRoughnessScale")
    scalar.set_editor_property("default_value", scale)
    if not edit.connect_material_expressions(roughness, "R", multiply, "A"):
        raise RuntimeError("Could not connect baked roughness to fish wet-film scale")
    if not edit.connect_material_expressions(scalar, "", multiply, "B"):
        raise RuntimeError("Could not connect fish wet-film scale")
    if not edit.connect_material_property(multiply, "", unreal.MaterialProperty.MP_ROUGHNESS):
        raise RuntimeError("Could not connect fish wet-film roughness")
    changed = True
    _save_parent(material, PARENT, changed)
    return material


def membrane_fish_parent(base_parent: unreal.Material, settings: dict) -> unreal.Material:
    for key, _ in PARAMETERS:
        _unit_setting(settings, key)
    opacity = _unit_setting(settings, "membrane_opacity")
    material = _load_parent(base_parent, MEMBRANE_PARENT)
    edit = unreal.MaterialEditingLibrary
    changed = False
    for key, value in (("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE),
                       ("two_sided", True)):
        if material.get_editor_property(key) != value:
            material.set_editor_property(key, value)
            changed = True
    color = edit.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    if color is None:
        raise RuntimeError("Fish membrane parent lacks its textured base color")
    subsurface = edit.get_material_property_input_node(material, unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    if subsurface is None:
        if not edit.connect_material_property(color, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR):
            raise RuntimeError("Could not connect fish membrane subsurface color")
        changed = True
    elif subsurface != color:
        raise RuntimeError("Unexpected fish membrane subsurface input")
    node = edit.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY)
    if node is None:
        node = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 1250)
        node.set_editor_property("parameter_name", "FishMembraneOpacity")
        if not edit.connect_material_property(node, "", unreal.MaterialProperty.MP_OPACITY):
            raise RuntimeError("Could not connect fish membrane opacity")
        changed = True
    if not isinstance(node, unreal.MaterialExpressionScalarParameter) or str(
            node.get_editor_property("parameter_name")) != "FishMembraneOpacity":
        raise RuntimeError("Unexpected fish membrane opacity input")
    if abs(float(node.get_editor_property("default_value")) - opacity) > 1e-6:
        node.set_editor_property("default_value", opacity)
        changed = True
    _save_parent(material, MEMBRANE_PARENT, changed)
    return material


def fish_material_parent(base_parent: unreal.Material, settings: dict, name: str) -> unreal.Material:
    if not isinstance(settings, dict):
        raise ValueError("Wet-fish material settings must be an object")
    membranes = settings.get("membrane_materials", [])
    if not isinstance(membranes, list) or any(not isinstance(value, str) or not value for value in membranes):
        raise ValueError("Invalid fish membrane material names")
    if len(set(membranes)) != len(membranes):
        raise ValueError("Duplicate fish membrane material names")
    return membrane_fish_parent(base_parent, settings) if name in membranes else wet_fish_parent(base_parent, settings)
