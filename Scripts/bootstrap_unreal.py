"""Author reproducible prototype assets using Unreal Editor's Python API."""
from pathlib import Path
import unreal


ROOT = Path(unreal.Paths.project_dir())
CONTENT = "/Game/SurvivalGame"
LIB = unreal.EditorAssetLibrary
MATERIALS = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def import_asset(filename, folder, name, static_mesh=False):
    source = ROOT / "Assets" / "Source" / filename
    if not source.is_file():
        raise RuntimeError(f"Missing licensed source asset: {source}. Run Fetch-Assets.ps1.")
    asset_path = f"{CONTENT}/{folder}/{name}"
    existing = LIB.load_asset(asset_path) if LIB.does_asset_exist(asset_path) else None
    if existing:
        return existing
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", f"{CONTENT}/{folder}")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    if static_mesh:
        task.set_editor_property("factory", unreal.FbxFactory())
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        mesh_data = options.get_editor_property("static_mesh_import_data")
        mesh_data.set_editor_property("combine_meshes", True)
        mesh_data.set_editor_property("auto_generate_collision", True)
        task.set_editor_property("options", options)
    TOOLS.import_asset_tasks([task])
    result = LIB.load_asset(asset_path)
    if not result:
        raise RuntimeError(f"Unreal did not import {source} as {asset_path}.")
    return result


def new_material(name):
    path = f"{CONTENT}/Materials/{name}"
    material = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if material:
        return material, False
    LIB.make_directory(f"{CONTENT}/Materials")
    material = TOOLS.create_asset(name, f"{CONTENT}/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError(f"Could not create material {name}.")
    return material, True


def field_material():
    material, created = new_material("M_Field")
    material.set_editor_property("used_with_instanced_static_meshes", True)
    if not created:
        MATERIALS.recompile_material(material)
        if not LIB.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError("Could not persist the field material's instancing usage.")
        return material
    tint = MATERIALS.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, -150)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.28, 0.39, 0.19, 1))
    roughness = MATERIALS.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 50)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.88)
    glow = MATERIALS.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 200)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0.0)
    multiply = MATERIALS.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, 200)
    connections = (
        MATERIALS.connect_material_expressions(tint, "", multiply, "A"),
        MATERIALS.connect_material_expressions(glow, "", multiply, "B"),
        MATERIALS.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR),
        MATERIALS.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS),
        MATERIALS.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR),
    )
    if not all(connections):
        raise RuntimeError("Could not wire the field material. No successful bootstrap is reported.")
    MATERIALS.recompile_material(material)
    if not LIB.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Could not save the field material.")
    return material


def textured_material(name, source_folder, prefix):
    color = import_asset(f"{source_folder}/{prefix}Color.jpg", "Textures", f"T_{prefix}Color")
    normal = import_asset(f"{source_folder}/{prefix}Normal.jpg", "Textures", f"T_{prefix}Normal")
    roughness = import_asset(f"{source_folder}/{prefix}Roughness.jpg", "Textures", f"T_{prefix}Roughness")
    normal.set_editor_property("srgb", False)
    normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    roughness.set_editor_property("srgb", False)
    for texture in (color, normal, roughness):
        if not LIB.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save texture settings for {texture.get_name()}.")
    material, created = new_material(name)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    if not created:
        MATERIALS.recompile_material(material)
        if not LIB.save_loaded_asset(material, only_if_is_dirty=False):
            raise RuntimeError(f"Could not persist instancing usage for {name}.")
        return material
    for index, (texture, prop) in enumerate((
        (color, unreal.MaterialProperty.MP_BASE_COLOR),
        (normal, unreal.MaterialProperty.MP_NORMAL),
        (roughness, unreal.MaterialProperty.MP_ROUGHNESS),
    )):
        node = MATERIALS.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, index * 250)
        node.set_editor_property("texture", texture)
        if index == 1:
            node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif index == 2:
            node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        if not MATERIALS.connect_material_property(node, "RGB" if index < 2 else "R", prop):
            raise RuntimeError(f"Could not connect texture {texture.get_name()} in {name}.")
    MATERIALS.recompile_material(material)
    if not LIB.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save material {name}.")
    return material


def main():
    LIB.make_directory(CONTENT)
    field_material()
    textured_material("M_Ground", "forest-ground", "Ground")
    rock_material = textured_material("M_Rock", "moss-rocks", "Rock")
    rocks = import_asset("moss-rocks/MossRocks.fbx", "Environment", "MossRocks", static_mesh=True)
    for slot in range(len(rocks.get_editor_property("static_materials"))):
        rocks.set_material(slot, rock_material)
    if not LIB.save_loaded_asset(rocks, only_if_is_dirty=False):
        raise RuntimeError("Could not save the imported rock material assignments.")

    import_asset("evening-harp/EveningHarp.mp3", "Audio/Music", "EveningHarp")
    ambience = import_asset("forest-ambience/ForestAmbience.mp3", "Audio/Ambience", "ForestAmbience")
    ambience.set_editor_property("looping", True)
    if not LIB.save_loaded_asset(ambience, only_if_is_dirty=False):
        raise RuntimeError("Could not save ambience loop settings.")
    for pack, name in (
        ("kenney-impact", "GrassStepA"), ("kenney-impact", "GrassStepB"),
        ("kenney-impact", "WoodTapA"), ("kenney-impact", "WoodTapB"),
        ("kenney-impact", "CraftStrikeA"), ("kenney-impact", "CraftStrikeB"),
        ("kenney-impact", "CraftStrikeC"),
        ("kenney-interface", "UIClick"),
    ):
        cue = import_asset(f"{pack}/{name}.ogg", "Audio/Effects", name)
        cue.set_editor_property("looping", False)
        if not LIB.save_loaded_asset(cue, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save one-shot sound {name}.")

    map_path = f"{CONTENT}/Maps/Homestead"
    if not LIB.does_asset_exist(map_path):
        LIB.make_directory(f"{CONTENT}/Maps")
        level_system = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if not level_system.new_level(map_path):
            raise RuntimeError("Could not create the homestead level.")
        actor_system = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        start = actor_system.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-1000, 0, 180))
        if not start:
            raise RuntimeError("Could not create the player start.")
        start.set_actor_label("HomesteadArrival")
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        game_mode = unreal.load_class(None, "/Script/SurvivalGame.HomesteadGameMode")
        if not game_mode:
            raise RuntimeError("Native game module is unavailable. Build SurvivalGameEditor first.")
        world.get_world_settings().set_editor_property("default_game_mode", game_mode)
        if not level_system.save_current_level():
            raise RuntimeError("Could not save the homestead level.")
    if not LIB.save_directory(CONTENT, only_if_is_dirty=False, recursive=True):
        raise RuntimeError("One or more generated content assets could not be saved.")
    unreal.log("Homestead content bootstrap complete. Technical stand-in remains; heroine is pending.")


if __name__ == "__main__":
    main()
