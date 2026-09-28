"""Import SM_EstateOuterLand, build M_EstateOuterLand and place it in the Estate level. Run in the editor
after bake_outer_land.py, with /Game/SurvivalGame/Maps/Estate loaded.

The material is a cheap distant-land stand-in for M_EstateLandscape: world-aligned pasture and moor
textures at the landscape's far tiling and tints, moor on the higher ground and steep faces, and
cliff rock where it drops into the sea.
"""
import os

import unreal

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FOLDER = "/Game/SurvivalGame/Estate/Landscape"
MEL = unreal.MaterialEditingLibrary
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LABEL = "EstateOuterLand"

HLSL = """
float up = abs(Normal.z);
float zm = WorldZ / 100.0;
float2 q = P / 100.0;
float2 out2 = max(abs(q) - 2016.0, 0.0);
float edge = length(out2);
float moor = saturate((zm - 150.0) / 30.0) + saturate((0.93 - up) * 6.0);
float rock = saturate((0.6 - up) * 5.0) + saturate((6.0 - zm) / 5.0);
// A patchwork of hedged fields fades in beyond the map edge, as inland Cornwall reads from a height.
float2 w = q + 45.0 * sin(q.yx / 173.0) + 20.0 * sin(q.yx / 61.0);
float2 cell = floor(w / 230.0);
float2 f = frac(w / 230.0);
float h = frac(sin(dot(cell, float2(127.1, 311.7))) * 43758.5453);
float3 crop = h < 0.55 ? float3(1.0, 1.0, 1.0) : (h < 0.72 ? float3(1.25, 1.1, 0.72) : (h < 0.86 ? float3(0.8, 0.92, 0.78) : float3(0.95, 0.72, 0.55)));
float hedge = 1.0 - smoothstep(0.0, 0.035, min(min(f.x, 1.0 - f.x), min(f.y, 1.0 - f.y)));
float patch = smoothstep(150.0, 900.0, edge) * (1.0 - saturate(moor));
float3 c = Pasture * lerp(float3(1.0, 1.0, 1.0), crop, patch);
c *= lerp(1.0, 0.5, hedge * patch);
c = lerp(c, Moor, saturate(moor));
c = lerp(c, Rock, saturate(rock));
return c * lerp(0.85, 1.05, Vary);
"""


def import_mesh():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(REPO, "Saved", "Terrain", "SM_EstateOuterLand.obj"))
    task.set_editor_property("destination_path", FOLDER)
    task.set_editor_property("destination_name", "SM_EstateOuterLand")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    TOOLS.import_asset_tasks([task])
    mesh = unreal.load_asset(f"{FOLDER}/SM_EstateOuterLand")
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("OBJ import did not produce a static mesh")
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    settings = sms.get_lod_build_settings(mesh, 0)
    settings.set_editor_property("generate_lightmap_u_vs", False)
    settings.set_editor_property("recompute_normals", True)
    settings.set_editor_property("recompute_tangents", True)
    settings.set_editor_property("distance_field_resolution_scale", 0.0)
    sms.set_lod_build_settings(mesh, 0, settings)
    nanite = mesh.get_editor_property("nanite_settings")
    # About 450k triangles, always loaded: Nanite keeps the distant ring cheap.
    nanite.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", nanite)
    sms.remove_collisions(mesh)
    return mesh


def build_material():
    path = f"{FOLDER}/M_EstateOuterLand"
    mat = unreal.load_asset(path) or TOOLS.create_asset("M_EstateOuterLand", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_nanite", True)
    MEL.delete_all_material_expressions(mat)
    for e in list(MEL.get_material_expressions(mat)):
        MEL.delete_material_expression(mat, e)

    wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 0)
    xy = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1250, 0)
    for ch, on in (("r", True), ("g", True), ("b", False), ("a", False)):
        xy.set_editor_property(ch, on)
    MEL.connect_material_expressions(wp, "", xy, "")
    wz = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1250, 120)
    for ch, on in (("r", False), ("g", False), ("b", True), ("a", False)):
        wz.set_editor_property(ch, on)
    MEL.connect_material_expressions(wp, "", wz, "")

    def tex(asset, tiling_m, y, tint):
        div = MEL.create_material_expression(mat, unreal.MaterialExpressionDivide, -1100, y)
        div.set_editor_property("const_b", tiling_m * 100.0)
        MEL.connect_material_expressions(xy, "", div, "A")
        t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -950, y)
        t.set_editor_property("texture", unreal.load_asset(asset))
        t.set_editor_property("sampler_source", unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
        MEL.connect_material_expressions(div, "", t, "UVs")
        k = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -950, y + 120)
        k.set_editor_property("constant", unreal.LinearColor(*tint, 1.0))
        m = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -700, y)
        MEL.connect_material_expressions(t, "RGB", m, "A")
        MEL.connect_material_expressions(k, "", m, "B")
        return m

    # Tilings and tints match M_EstateLandscape's far layer samples (build_landscape_material.py).
    pasture = tex("/Game/Trials/GrassGround_20260921_01/Textures/T_GrassGround_Diff", 3.0 * 4.3, -400, (0.46, 0.74, 0.38))
    moor = tex("/Game/SurvivalGame/Estate/Landscape/Textures/T_Estate_Moorland_D", 5.0 * 4.3, -150, (0.56, 0.58, 0.42))
    rock = tex("/Game/SurvivalGame/Estate/Landscape/Textures/T_Estate_CliffRock_D", 8.0 * 4.3, 100, (0.8, 0.8, 0.8))
    vary = MEL.create_material_expression(mat, unreal.MaterialExpressionNoise, -700, 350)
    vary.set_editor_property("scale", 0.0004)
    vary.set_editor_property("output_min", 0.0)
    vary.set_editor_property("levels", 3)
    MEL.connect_material_expressions(wp, "", vary, "Position")
    normal = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -700, 480)

    custom = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -350, 0)
    custom.set_editor_property("code", HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for name in ("Pasture", "Moor", "Rock", "WorldZ", "Normal", "Vary", "P"):
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", name); ins.append(ci)
    custom.set_editor_property("inputs", ins)
    for node, pin in ((pasture, "Pasture"), (moor, "Moor"), (rock, "Rock"), (wz, "WorldZ"), (normal, "Normal"), (vary, "Vary"), (xy, "P")):
        MEL.connect_material_expressions(node, "", custom, pin)
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 250)
    rough.set_editor_property("r", 0.92)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    LIB.save_loaded_asset(mat, False)
    return mat


def place(mesh):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = next((a for a in actors.get_all_level_actors() if a.get_actor_label() == LABEL), None)
    if actor is None:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        actor.set_actor_label(LABEL)
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    comp.set_editor_property("cast_shadow", False)
    actor.set_actor_location(unreal.Vector(0, 0, 0), False, False)
    actor.set_folder_path("Terrain")
    actor.set_editor_property("is_spatially_loaded", False)
    return actor


mesh = import_mesh()
mat = build_material()
mesh.set_material(0, mat)
LIB.save_loaded_asset(mesh, False)
actor = place(mesh)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("EstateOuterLand placed:", actor.get_path_name())
