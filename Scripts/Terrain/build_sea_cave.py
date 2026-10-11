"""The sea cave's Unreal assets (openspec add-sea-cave). Run in the editor (MCP `py` or the Python console):

    exec(open(r'<repo>/Scripts/Terrain/build_sea_cave.py').read())

1. Imports SM_SeaCave from Assets/Props/SeaCave (New-Prop.ps1 sea_cave) with import_props: Nanite, granite
   parent with the GraniteDetail layer, complex-as-simple collision on LOD2.
2. M_SeaCaveSky: the strip of sky closing the roof fissure (unlit, two-sided; Color x Intensity, set at runtime
   by AHomesteadSeaCave::SetSky).
3. M_SeaCavePoolStill + MI_SeaCavePoolStill: the pool's still, dark green water (translucent; the runtime puts it on
   lighting channel 1 so only the fissure's shaft lights it).
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
AUDIO = "/Game/SurvivalGame/Audio/Ambience"
POOL_COLOR = (0.006, 0.016, 0.012)     # dark green, clear seep water over sand in a dim chamber
POOL_OPACITY = (0.72, 0.95)            # looking straight down .. at a glancing angle

LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def _build_settings(mesh):
    settings = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_lod_build_settings(mesh, 0)
    settings.set_editor_property("distance_field_resolution_scale", 4.0)
    settings.set_editor_property("generate_distance_field_as_if_two_sided", True)
    return settings


def import_mesh():
    sys.path.insert(0, os.path.join(REPO, "Scripts", "Blender"))
    spec = importlib.util.spec_from_file_location("import_props", os.path.join(REPO, "Scripts", "Blender", "import_props.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.main(["SeaCave", "Thrift"])
    # The cave is a one-sided skin seen from inside: give Lumen enough to occlude the sky with. A finer distance
    # field (software Lumen) and a fuller Nanite fallback (the triangles hardware ray tracing traces).
    mesh = unreal.load_asset("/Game/SurvivalGame/Environment/Props/SeaCave/SM_SeaCave")
    unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_lod_build_settings(mesh, 0, _build_settings(mesh))
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("fallback_percent_triangles", 25.0)
    mesh.set_editor_property("nanite_settings", nanite)
    LIB.save_loaded_asset(mesh, False)
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
    """M_SeaCavePoolStill + MI_SeaCavePoolStill: still, dark green water. Translucent, surface-lit (not Single Layer
    Water, which glowed white under the cave's unoccluded sky); see-through looking down onto the bed, a mirror at
    a glancing angle. Tuned in RT-on PIE."""
    path = f"{FOLDER}/M_SeaCavePoolStill"
    m = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        "M_SeaCavePoolStill", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    for e in list(MEL.get_material_expressions(m) if hasattr(MEL, "get_material_expressions") else []):
        MEL.delete_material_expression(m, e)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)

    def vec(name, v, x, y):
        e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", unreal.LinearColor(*v, 1.0))
        return e

    def sc(name, v, x, y):
        e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", v)
        return e
    MEL.connect_material_property(vec("WaterColor", POOL_COLOR, -500, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(sc("Roughness", 0.03, -500, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(sc("Specular", 0.5, -500, 250), "", unreal.MaterialProperty.MP_SPECULAR)
    fresnel = MEL.create_material_expression(m, unreal.MaterialExpressionFresnel, -700, 400)
    lerp = MEL.create_material_expression(m, unreal.MaterialExpressionLinearInterpolate, -400, 450)
    MEL.connect_material_expressions(sc("OpacityDown", POOL_OPACITY[0], -700, 550), "", lerp, "A")
    MEL.connect_material_expressions(sc("OpacityGlancing", POOL_OPACITY[1], -700, 650), "", lerp, "B")
    MEL.connect_material_expressions(fresnel, "", lerp, "Alpha")
    MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(m)
    LIB.save_loaded_asset(m, False)
    ipath = f"{FOLDER}/MI_SeaCavePoolStill"
    mi = LIB.load_asset(ipath) if LIB.does_asset_exist(ipath) else TOOLS.create_asset(
        "MI_SeaCavePoolStill", FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, m)
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
