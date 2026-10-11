"""Landscape holes for the sea cave: the hole material and the Visibility-layer paint (openspec add-sea-cave).

Run in the editor with the Estate map open (MCP `py` or the Python console), after build_landscape_material.py
and after Recipes/sea_cave.py (or Scripts/Blender/sea_cave_fit.py) has written sea_cave_holes.json:

    exec(open(r'<repo>/Scripts/Terrain/build_landscape_holes.py').read())

1. M_EstateLandscapeHoles: a copy of M_EstateLandscape, masked, with a Landscape Visibility Mask on its opacity
   mask, set as the Landscape's LandscapeHoleMaterial (the engine draws it only on components with holes).
   Rebuilt from the current landscape material on every run, so rerun this after build_landscape_material.py.
2. The Visibility layer painted from sea_cave_holes.json's vertices over its paint rectangle (cleared elsewhere
   in it), then the dirty landscape packages saved. Collision holes follow the same layer.

ApplyEstateHeightfield and ApplyEstateWeightmaps don't touch the Visibility layer, so a terrain rebake keeps the
hole; refit the cave (sea_cave_site.json, New-Prop.ps1 sea_cave) and rerun this whenever the ground at the
mouth moves.
"""
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
HOLES = os.path.join(HERE, "sea_cave_holes.json")
SOURCE = "/Game/SurvivalGame/Estate/Landscape/M_EstateLandscape"
HOLE_MATERIAL = "/Game/SurvivalGame/Estate/Landscape/M_EstateLandscapeHoles"

LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary


def landscapes():
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    return [a for a in actors if isinstance(a, unreal.Landscape)]


def hole_material():
    for landscape in landscapes():
        landscape.set_editor_property("landscape_hole_material", None)
    if LIB.does_asset_exist(HOLE_MATERIAL) and not LIB.delete_asset(HOLE_MATERIAL):
        raise RuntimeError("Could not replace " + HOLE_MATERIAL)
    mat = LIB.duplicate_asset(SOURCE, HOLE_MATERIAL)
    if not mat:
        raise RuntimeError("Could not copy " + SOURCE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mask = MEL.create_material_expression(mat, unreal.MaterialExpressionLandscapeVisibilityMask, -700, 1100)
    if not MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK):
        raise RuntimeError("Could not wire the visibility mask")
    MEL.recompile_material(mat)
    LIB.save_loaded_asset(mat, False)
    for landscape in landscapes():
        landscape.modify()
        landscape.set_editor_property("landscape_hole_material", mat)
    return mat


def paint(dry_run=False):
    spec = json.load(open(HOLES))
    vertices = [unreal.IntPoint(int(c), int(r)) for c, r in spec["vertices"]]
    x0, y0, x1, y1 = spec["paint_rect"]
    result = unreal.HomesteadEstateAuthoringLibrary.paint_estate_landscape_holes(vertices, x0, y0, x1, y1, dry_run)
    print("landscape holes:", result)
    if result.startswith("error"):
        raise RuntimeError(result)
    return result


def run(dry_run=False):
    if not dry_run:
        hole_material()
    paint(dry_run)
    if not dry_run:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        print("landscape holes: saved")


run(dry_run=bool(os.environ.get("HOMESTEAD_HOLES_DRY_RUN")))
