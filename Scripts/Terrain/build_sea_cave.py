"""The sea cave's Unreal assets (openspec add-sea-cave). Run in the editor (MCP `py` or the Python console):

    exec(open(r'<repo>/Scripts/Terrain/build_sea_cave.py').read())

1. Imports SM_SeaCave from Assets/Props/SeaCave (New-Prop.ps1 sea_cave) with import_props: Nanite, granite
   parent with the GraniteDetail layer, complex-as-simple collision on LOD2.
2. M_SeaCaveSky: the strip of sky closing the roof fissure (unlit, two-sided; Color x Intensity, set at runtime
   by AHomesteadSeaCave::SetSky).
3. MI_SeaCavePool: the cave pool's still water, an instance of the estate pond's water, clearer and greener
   (seep water over pale sand), with no wind ripple.
4. SeaCaveDrips and SeaCaveSurf from Assets/Audio/Ambience (Scripts/generate_sea_cave_audio.py), looping.
5. The cove cliff's dressing: SM_ThriftClump (Recipes/thrift.py) and darker MI_<Granite>_Cliff instances.
Then saves. The landscape hole is build_landscape_holes.py.
"""
import importlib.util
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
FOLDER = "/Game/SurvivalGame/Environment/SeaCave"
POND = "/Game/SurvivalGame/Materials/M_EstatePond"
AUDIO = "/Game/SurvivalGame/Audio/Ambience"
POOL_LOOK = {"RippleScale": (0.02, 0.02, 1.0), "Specular": 0.9, "Roughness": 0.015,
             "Absorption": (0.03, 0.012, 0.02), "Scattering": (0.0002, 0.0006, 0.0004),
             "ShallowFoam": 0.0, "DeepFoam": 0.0}

LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def import_mesh():
    sys.path.insert(0, os.path.join(REPO, "Scripts", "Blender"))
    spec = importlib.util.spec_from_file_location("import_props", os.path.join(REPO, "Scripts", "Blender", "import_props.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.main(["SeaCave", "Thrift"])
    for name in ("SeaCave", "Thrift"):
        LIB.save_directory(f"/Game/SurvivalGame/Environment/Props/{name}", False, True)


# The cove's west cliff (cove_cliff_rocks.py) reuses the estate's granite with darker, weathered stone: sea cliffs
# are wet, salt-stained and lichen-dark, not the pale sunlit tors inland (M_PropGranite's MacroBrightness is 0.74).
CLIFF_ROCKS = ("GraniteDome", "GraniteSplitBoulder", "GraniteErratic", "GraniteBoulderJointed")
CLIFF_BRIGHTNESS = 0.5


def cliff_materials():
    for name in CLIFF_ROCKS:
        parent = unreal.load_asset(f"/Game/SurvivalGame/Environment/Props/{name}/MI_{name}")
        if not parent:
            print("sea cave: no MI_" + name)
            continue
        path = f"{FOLDER}/MI_{name}_Cliff"
        mi = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
            f"MI_{name}_Cliff", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        MEL.set_material_instance_parent(mi, parent)
        MEL.set_material_instance_scalar_parameter_value(mi, "MacroBrightness", CLIFF_BRIGHTNESS)
        LIB.save_loaded_asset(mi, False)


def sky_material():
    path = f"{FOLDER}/M_SeaCaveSky"
    mat = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        "M_SeaCaveSky", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(mat)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.93, 0.97, 1.0, 1.0))
    glow = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 200)
    glow.set_editor_property("parameter_name", "Intensity")
    glow.set_editor_property("default_value", 14.0)
    mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 100)
    MEL.connect_material_expressions(color, "", mul, "A")
    MEL.connect_material_expressions(glow, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    LIB.save_loaded_asset(mat, False)


def pool_material():
    path = f"{FOLDER}/MI_SeaCavePool"
    mi = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        "MI_SeaCavePool", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, unreal.load_asset(POND))
    for name, value in POOL_LOOK.items():
        if isinstance(value, tuple):
            MEL.set_material_instance_vector_parameter_value(mi, name, unreal.LinearColor(*value, 0.0))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, name, value)
    LIB.save_loaded_asset(mi, False)


def sounds():
    for name in ("SeaCaveDrips", "SeaCaveSurf"):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(REPO, "Assets", "Audio", "Ambience", name + ".wav"))
        task.set_editor_property("destination_path", AUDIO)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        TOOLS.import_asset_tasks([task])
        wave = unreal.load_asset(f"{AUDIO}/{name}")
        wave.set_editor_property("looping", True)
        LIB.save_loaded_asset(wave, False)


import_mesh()
sky_material()
pool_material()
cliff_materials()
sounds()
print("sea cave assets: built")
