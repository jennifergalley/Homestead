"""Author reproducible prototype assets using Unreal Editor's Python API."""
from pathlib import Path
import unreal


ROOT = Path(unreal.Paths.project_dir())
CONTENT = "/Game/SurvivalGame"
LIB = unreal.EditorAssetLibrary
MATERIALS = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def import_asset(filename, folder, name, static_mesh=False, source_root=None):
    source = (source_root or ROOT / "Assets" / "Source") / filename
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


def creek_water_material(rebuild=False, name="M_CreekWater", flow=1.0):
    """Flowing creek water on the Single Layer Water shading model, from the generated ripple and foam maps.

    Single Layer Water renders in the opaque pass, so the surface receives the woodland's shadows and
    Lumen reflections, and the bed shows through tinted by how much water lies over it: the
    absorption and scattering act over the real distance to the bed, which also clears the shallows
    and the shoreline by itself. Mesh UVs are metres (U across the stream, V along it) and vertex
    colour carries the depth over the rendered bed: R reaches 1 at 35 cm deep, G at 8 cm, which
    thins the foam and softens the shoreline highlight. B adds white water (M_EstateRiver, built from
    this graph by Scripts/Terrain/place_water.py, for the estate river's riffles and spring; 0 on the
    woodland creek). flow scales every drift speed (M_EstatePond, the estate lake, uses a small fraction).
    Pass rebuild=True to re-author in place.
    """
    creek = ROOT / "Assets" / "Environment" / "Creek"
    ripples = import_asset("T_CreekRipples_N.png", "Textures", "T_CreekRipples_N", source_root=creek)
    ripples.set_editor_property("srgb", False)
    ripples.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    foam = import_asset("T_CreekFoam.png", "Textures", "T_CreekFoam", source_root=creek)
    foam.set_editor_property("srgb", False)
    foam.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    for texture in (ripples, foam):
        if not LIB.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save texture settings for {texture.get_name()}.")
    material, created = new_material(name)
    if not created and not rebuild:
        return material
    MATERIALS.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    material.set_editor_property("refraction_method", unreal.RefractionMode.RM_PIXEL_NORMAL_OFFSET)

    def node(cls, x, y, **props):
        expression = MATERIALS.create_material_expression(material, cls, x, y)
        for key, value in props.items():
            expression.set_editor_property(key, value)
        return expression

    def link(source, output, target, target_input=""):
        if not MATERIALS.connect_material_expressions(source, output, target, target_input):
            raise RuntimeError(f"Could not wire {source.get_name()}.{output} -> {target.get_name()}.{target_input}.")

    def sample(texture, tiling, speed, x, y, normal=False):
        coords = node(unreal.MaterialExpressionTextureCoordinate, x - 600, y, utiling=tiling, vtiling=tiling)
        # flow scales the drift: 1 for running water, a small fraction for still water stirred by the wind.
        pan = node(unreal.MaterialExpressionPanner, x - 400, y, speed_x=speed[0] * flow, speed_y=speed[1] * flow)
        link(coords, "", pan, "Coordinate")
        tex = node(unreal.MaterialExpressionTextureSample, x - 200, y, texture=texture,
                   sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if normal
                   else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
        link(pan, "", tex, "UVs")
        return tex

    # Two ripple layers drifting downstream (-V) at different scales and speeds.
    broad = sample(ripples, 0.8, (0.015, -0.32), 0, -500, normal=True)
    fine = sample(ripples, 2.1, (-0.03, -0.62), 0, -300, normal=True)
    summed = node(unreal.MaterialExpressionAdd, 50, -420)
    link(broad, "RGB", summed, "A")
    link(fine, "RGB", summed, "B")
    flatten = node(unreal.MaterialExpressionMultiply, 200, -420)
    calm = node(unreal.MaterialExpressionVectorParameter, 50, -300, parameter_name="RippleScale",
                default_value=unreal.LinearColor(0.3, 0.3, 1, 0))
    link(summed, "", flatten, "A")
    link(calm, "", flatten, "B")
    normal = node(unreal.MaterialExpressionNormalize, 350, -420)
    link(flatten, "", normal, "")

    # Parameters keep the look tunable from a material instance while playtesting.
    def scalar(name, value, x, y):
        return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)

    def vector(name, value, x, y):
        return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                    default_value=unreal.LinearColor(*value, 0))

    def lerp(a, b, alpha, x, y):
        result = node(unreal.MaterialExpressionLinearInterpolate, x, y)
        link(a, "", result, "A")
        link(b, "", result, "B")
        link(*alpha, result, "Alpha")
        return result

    color = node(unreal.MaterialExpressionVertexColor, -300, 0)

    flecks = sample(foam, 1.3, (0.0, -0.75), 0, 350)
    froth = lerp(scalar("ShallowFoam", 0.55, -50, 450), scalar("DeepFoam", 0.1, -50, 520), (color, "R"), 150, 300)
    streaming = node(unreal.MaterialExpressionMultiply, 300, 350)
    link(flecks, "R", streaming, "A")
    link(froth, "", streaming, "B")
    # White water where vertex colour B says the water tumbles (the estate river's riffles and its
    # spring): a coarser, faster froth layer over most of the surface. B is 0 on the woodland creek.
    tumble = sample(foam, 0.9, (0.04, -1.4), 0, 650)
    churn = node(unreal.MaterialExpressionMultiply, 150, 650)
    link(tumble, "R", churn, "A")
    link(scalar("WhiteWaterContrast", 1.8, -50, 720), "", churn, "B")
    lift = node(unreal.MaterialExpressionAdd, 250, 650)
    link(churn, "", lift, "A")
    link(scalar("WhiteWaterFloor", 0.2, -50, 790), "", lift, "B")
    white = node(unreal.MaterialExpressionMultiply, 350, 650)
    link(lift, "", white, "A")
    link(color, "B", white, "B")
    white_amount = node(unreal.MaterialExpressionMultiply, 450, 650)
    link(white, "", white_amount, "A")
    link(scalar("WhiteWater", 0.85, 300, 790), "", white_amount, "B")
    combined = node(unreal.MaterialExpressionAdd, 550, 400)
    link(streaming, "", combined, "A")
    link(white_amount, "", combined, "B")
    foam_amount = node(unreal.MaterialExpressionSaturate, 650, 400)
    link(combined, "", foam_amount, "")
    base = node(unreal.MaterialExpressionMultiply, 450, -100)
    link(vector("FoamColor", (0.62, 0.65, 0.62), 300, -20), "", base, "A")
    link(foam_amount, "", base, "B")

    # The highlight and the refraction both fade out over the last few centimetres of shore.
    shine = lerp(scalar("ShoreSpecular", 0.0, 450, 200), scalar("Specular", 0.6, 450, 270), (color, "G"), 600, 220)
    rough = lerp(scalar("Roughness", 0.04, 450, 400), scalar("FoamRoughness", 0.5, 450, 470),
                 (foam_amount, ""), 600, 400)
    bend = lerp(scalar("EdgeRefraction", 1.0, 450, 540), scalar("Refraction", 1.2, 450, 610),
                (color, "G"), 600, 520)

    water = node(unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 900, 700)
    # The coefficients act per centimetre of water: absorbing red fastest leaves a clear green-teal
    # over the 25-38 cm deep channel, and a trace of scattering keeps the deep middle from going black.
    for source, pin in ((vector("Scattering", (0.0003, 0.0006, 0.0007), 700, 650), "ScatteringCoefficients"),
                        (vector("Absorption", (0.062, 0.025, 0.02), 700, 720), "AbsorptionCoefficients"),
                        (scalar("PhaseG", 0.1, 700, 790), "PhaseG"),
                        (scalar("ColorScaleBehindWater", 0.8, 700, 860), "ColorScaleBehindWater")):
        link(source, "", water, pin)

    properties = unreal.MaterialProperty
    for source, prop in ((base, properties.MP_BASE_COLOR), (normal, properties.MP_NORMAL),
                         (foam_amount, properties.MP_OPACITY), (shine, properties.MP_SPECULAR),
                         (rough, properties.MP_ROUGHNESS), (bend, properties.MP_REFRACTION)):
        if not MATERIALS.connect_material_property(source, "", prop):
            raise RuntimeError(f"Could not connect {prop} in {name}.")
    MATERIALS.recompile_material(material)
    if not LIB.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {name}.")
    return material


def main():
    LIB.make_directory(CONTENT)
    field_material()
    creek_water_material()
    textured_material("M_Ground", "forest-ground", "Ground")
    rock_material = textured_material("M_Rock", "moss-rocks", "Rock")
    rocks = import_asset("moss-rocks/MossRocks.fbx", "Environment", "MossRocks", static_mesh=True)
    for slot in range(len(rocks.get_editor_property("static_materials"))):
        rocks.set_material(slot, rock_material)
    if not LIB.save_loaded_asset(rocks, only_if_is_dirty=False):
        raise RuntimeError("Could not save the imported rock material assignments.")

    import_asset("evening-harp/EveningHarp.mp3", "Audio/Music", "EveningHarp")
    for pack, name in (("ascending-the-vale", "AscendingTheVale"), ("teller-of-the-tales", "TellerOfTheTales"),
                       ("meditation-impromptu-02", "MeditationImpromptu02"), ("at-rest", "AtRest")):
        import_asset(f"{pack}/{name}.mp3", "Audio/Music", name)
    ambience = import_asset("forest-ambience/ForestAmbience.mp3", "Audio/Ambience", "ForestAmbience")
    ambience.set_editor_property("looping", True)
    if not LIB.save_loaded_asset(ambience, only_if_is_dirty=False):
        raise RuntimeError("Could not save ambience loop settings.")
    # The creek's burble, cut from a CC0 brook recording by Scripts/generate_creek_assets.py.
    creek = import_asset("CreekLoop.wav", "Audio/Ambience", "CreekLoop", source_root=ROOT / "Assets" / "Audio" / "Ambience")
    creek.set_editor_property("looping", True)
    if not LIB.save_loaded_asset(creek, only_if_is_dirty=False):
        raise RuntimeError("Could not save the creek loop settings.")
    # The standing-room hearth's crackle, cut from a CC0 recording by Scripts/prepare_hearth_crackle.py.
    crackle = import_asset("HearthCrackle.wav", "Audio/Ambience", "HearthCrackle",
                           source_root=ROOT / "Assets" / "Audio" / "Ambience")
    crackle.set_editor_property("looping", True)
    if not LIB.save_loaded_asset(crackle, only_if_is_dirty=False):
        raise RuntimeError("Could not save the hearth crackle loop settings.")
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
    # Original bare-foot footsteps, generated by Scripts/generate_bare_footsteps.py and tracked in git.
    for name in [f"BareStepWalk_{i:02d}" for i in range(6)] + [f"BareStepRun_{i:02d}" for i in range(4)]:
        cue = import_asset(f"{name}.wav", "Audio/Effects", name, source_root=ROOT / "Assets" / "Audio" / "Footsteps")
        cue.set_editor_property("looping", False)
        if not LIB.save_loaded_asset(cue, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save one-shot sound {name}.")
    # Hatchet-on-trunk chops and the trunk landing, cut from CC0 recordings by Scripts/generate_chop_sounds.py.
    for name in ("ChopA", "ChopB", "ChopC", "TreeFall"):
        cue = import_asset(f"{name}.wav", "Audio/Effects", name, source_root=ROOT / "Assets" / "Audio" / "Effects")
        cue.set_editor_property("looping", False)
        if not LIB.save_loaded_asset(cue, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save one-shot sound {name}.")

    # The scythe's mowing swish, original: synthesized by Scripts/generate_scythe_sound.py.
    cue = import_asset("ScytheSwish.wav", "Audio/Effects", "ScytheSwish", source_root=ROOT / "Assets" / "Audio" / "Effects")
    cue.set_editor_property("looping", False)
    if not LIB.save_loaded_asset(cue, only_if_is_dirty=False):
        raise RuntimeError("Could not save one-shot sound ScytheSwish.")

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
