"""Create project-owned camera-safe foliage materials from admitted textures."""

import json
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir())
DEST = "/Game/SurvivalGame/Environment/CameraSafeFoliage"
REPORT = ROOT / "Build" / "Validation" / "camera-safe-foliage-authoring.json"
LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
WHITE = "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"

SURFACES = {
    "FirSaplingBranches": {
        "diffuse": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Branches_Diff",
        "normal": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Branches_NormalDX",
        "roughness": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Branches_Roughness",
        "ao": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Branches_AO",
    },
    "FirSaplingTwigs": {
        "diffuse": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Twigs_Diff",
        "normal": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Twigs_NormalDX",
        "roughness": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Twigs_Roughness",
        "ao": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Twigs_AO",
        "alpha": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FirSapling_Twigs_Alpha",
    },
    "Shrub": {
        "diffuse": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_Shrub04_Diff",
        "normal": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_Shrub04_NormalDX",
        "roughness": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_Shrub04_Roughness",
        "ao": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_Shrub04_AO",
        "alpha": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_Shrub04_Alpha",
    },
    "Flower": {
        "diffuse": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FlowerEmpodium_Diff",
        "normal": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FlowerEmpodium_NormalDX",
        "roughness": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FlowerEmpodium_Roughness",
        "ao": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FlowerEmpodium_AO",
        "alpha": "/Game/Trials/WoodlandResources_20260921_01/Textures/T_FlowerEmpodium_Alpha",
    },
    "Grass": {
        "diffuse": "/Game/Trials/GrassGround_20260921_01/Textures/T_GrassMedium01_Diff",
        "normal": "/Game/Trials/GrassGround_20260921_01/Textures/T_GrassMedium01_NormalDX",
        "roughness": "/Game/Trials/GrassGround_20260921_01/Textures/T_GrassMedium01_Roughness",
        "ao": "/Game/Trials/GrassGround_20260921_01/Textures/T_GrassMedium01_AO",
        "alpha": "/Game/Trials/GrassGround_20260921_01/Textures/T_GrassMedium01_Alpha",
    },
    "Fern": {
        "diffuse": "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_Diff",
        "normal": "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_NormalDX",
        "roughness": "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_Roughness",
        "ao": "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_AO",
        "alpha": "/Game/Trials/Fern02_20260920_01/Textures/T_Fern02_Alpha",
    },
    "TreeSmallLeaves": {
        "diffuse": "/Game/Trials/TreeSmall02_20260921_01/Textures/T_TreeSmall02_Leaves_Diff",
        "normal": "/Game/Trials/TreeSmall02_20260921_01/Textures/T_TreeSmall02_Leaves_NormalDX",
        "roughness": "/Game/Trials/TreeSmall02_20260921_01/Textures/T_TreeSmall02_Leaves_Roughness",
        "ao": "/Game/Trials/TreeSmall02_20260921_01/Textures/T_TreeSmall02_Leaves_AO",
        "alpha": "/Game/Trials/TreeSmall02_20260921_01/Textures/T_TreeSmall02_Leaves_Alpha",
    },
    "MatureFirTwig": {
        "diffuse": "/Game/Trials/MatureFir_20260922_02/Textures/T_MatureFir_Twig_Diff",
        "normal": "/Game/Trials/MatureFir_20260922_02/Textures/T_MatureFir_Twig_NormalDX",
        "roughness": "/Game/Trials/MatureFir_20260922_02/Textures/T_MatureFir_Twig_Roughness",
        "ao": "/Game/Trials/MatureFir_20260922_02/Textures/T_MatureFir_Twig_AO",
        "alpha": "/Game/Trials/MatureFir_20260922_02/Textures/T_MatureFir_Twig_Alpha",
    },
    "JacarandaLeaves": {
        "diffuse": "/Game/Trials/TreePalette_20260921_01/Textures/T_Jacaranda_Leaves_Diff",
        "normal": "/Game/Trials/TreePalette_20260921_01/Textures/T_Jacaranda_Leaves_NormalDX",
        "roughness": "/Game/Trials/TreePalette_20260921_01/Textures/T_Jacaranda_Leaves_Roughness",
        "ao": "/Game/Trials/TreePalette_20260921_01/Textures/T_Jacaranda_Leaves_AO",
        "alpha": "/Game/Trials/TreePalette_20260921_01/Textures/T_Jacaranda_Leaves_Alpha",
    },
    "FirPoleTwigs": {
        "diffuse": "/Game/Trials/TreePalette_20260921_01/Textures/T_FirPole_Twigs_Diff",
        "normal": "/Game/Trials/TreePalette_20260921_01/Textures/T_FirPole_Twigs_NormalDX",
        "roughness": "/Game/Trials/TreePalette_20260921_01/Textures/T_FirPole_Twigs_Roughness",
        "ao": "/Game/Trials/TreePalette_20260921_01/Textures/T_FirPole_Twigs_AO",
        "alpha": "/Game/Trials/TreePalette_20260921_01/Textures/T_FirPole_Twigs_Alpha",
    },
}

MESHES = [
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_a",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FirSapling_c",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_a",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_Shrub04_c",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_b",
    "/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_mid_b",
    "/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b",
    "/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tall_a",
    "/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tiny_a",
    "/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a",
    "/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_b",
    "/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_c",
    "/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_d",
    "/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland",
    "/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir",
    "/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda",
    "/Game/Trials/TreePalette_20260921_01/Meshes/SM_FirPole",
]


def save(asset):
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + asset.get_path_name())


def create_asset(name, cls, factory):
    path = f"{DEST}/{name}"
    existing = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if existing:
        return existing
    asset = TOOLS.create_asset(name, DEST, cls, factory)
    if not asset:
        raise RuntimeError("Could not create " + path)
    return asset


def expression(material, kind, x, y):
    node = EDIT.create_material_expression(material, kind, x, y)
    if not node:
        raise RuntimeError(f"Could not create {kind} in {material.get_name()}")
    return node


def connect(source, output, target, input_name):
    if not EDIT.connect_material_expressions(source, output, target, input_name):
        names = EDIT.get_material_expression_input_names(target)
        raise RuntimeError(f"Could not connect {output} to {input_name}; available={names}")


def property_output(source, output, material_property):
    if not EDIT.connect_material_property(source, output, material_property):
        raise RuntimeError(f"Could not connect {source.get_name()} to {material_property}")


def create_collection():
    collection = create_asset(
        "MPC_CameraSafeFoliage",
        unreal.MaterialParameterCollection,
        unreal.MaterialParameterCollectionFactoryNew(),
    )
    def vector_parameter(name, value):
        parameter = unreal.CollectionVectorParameter()
        parameter.set_editor_property("parameter_name", name)
        parameter.set_editor_property("default_value", value)
        return parameter

    def scalar_parameter(name, value):
        parameter = unreal.CollectionScalarParameter()
        parameter.set_editor_property("parameter_name", name)
        parameter.set_editor_property("default_value", value)
        return parameter

    collection.set_editor_property("vector_parameters", [
        vector_parameter("CameraPosition", unreal.LinearColor(0, 0, -100000, 1)),
        vector_parameter("HeroTargetPosition", unreal.LinearColor(0, 0, -100000, 1)),
    ])
    collection.set_editor_property("scalar_parameters", [
        scalar_parameter("NearRadiusCm", 95.0),
        scalar_parameter("CorridorRadiusCm", 62.0),
        scalar_parameter("FadeBandCm", 45.0),
    ])
    save(collection)
    return collection


def texture_parameter(material, name, default_path, x, y,
                      sampler=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR):
    node = expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", LIB.load_asset(default_path))
    node.set_editor_property("sampler_type", sampler)
    return node


def collection_parameter(material, collection, name, x, y):
    node = expression(material, unreal.MaterialExpressionCollectionParameter, x, y)
    node.set_editor_property("collection", collection)
    node.set_editor_property("parameter_name", name)
    return node


def create_visibility_function(collection):
    path = f"{DEST}/MF_CameraSafeFoliageVisibility"
    function = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if function:
        return function
    function = TOOLS.create_asset(
        "MF_CameraSafeFoliageVisibility",
        DEST,
        unreal.MaterialFunction,
        unreal.MaterialFunctionFactoryNew(),
    )
    if not function:
        raise RuntimeError("Could not create " + path)

    def function_expression(kind, x, y):
        node = EDIT.create_material_expression_in_function(function, kind, x, y)
        if not node:
            raise RuntimeError(f"Could not create {kind} in visibility function")
        return node

    def function_collection(name, x, y):
        node = function_expression(unreal.MaterialExpressionCollectionParameter, x, y)
        node.set_editor_property("collection", collection)
        node.set_editor_property("parameter_name", name)
        return node

    world = function_expression(unreal.MaterialExpressionWorldPosition, -1400, 0)
    camera = function_collection("CameraPosition", -1400, 150)
    target = function_collection("HeroTargetPosition", -1400, 300)
    near_radius = function_collection("NearRadiusCm", -1400, 450)
    corridor_radius = function_collection("CorridorRadiusCm", -1400, 600)
    fade_band = function_collection("FadeBandCm", -1400, 750)
    custom = function_expression(unreal.MaterialExpressionCustom, -700, 250)
    custom.set_editor_property("description", "Finite camera-to-hero corridor visibility")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)

    def custom_input(name):
        value = unreal.CustomInput()
        value.set_editor_property("input_name", name)
        return value

    custom.set_editor_property("inputs", [
        custom_input("PixelWorld"),
        custom_input("CameraWorld"),
        custom_input("TargetWorld"),
        custom_input("NearRadius"),
        custom_input("CorridorRadius"),
        custom_input("FadeBand"),
    ])
    custom.set_editor_property("code", """
float3 a = CameraWorld.xyz;
float3 b = TargetWorld.xyz;
float3 ab = b - a;
float denom = max(dot(ab, ab), 1.0);
float t = saturate(dot(PixelWorld.xyz - a, ab) / denom);
float segmentDistance = length(PixelWorld.xyz - (a + ab * t));
float cameraDistance = length(PixelWorld.xyz - a);
float band = max(FadeBand, 1.0);
float visibility = min(saturate((cameraDistance - NearRadius) / band),
    saturate((segmentDistance - CorridorRadius) / band));
float3 cell = floor(PixelWorld.xyz * 0.35);
float noise = frac(sin(dot(cell, float3(12.9898, 78.233, 37.719))) * 43758.5453);
return step(noise, visibility);
""")
    connect(world, "", custom, "PixelWorld")
    connect(camera, "", custom, "CameraWorld")
    connect(target, "", custom, "TargetWorld")
    connect(near_radius, "", custom, "NearRadius")
    connect(corridor_radius, "", custom, "CorridorRadius")
    connect(fade_band, "", custom, "FadeBand")
    output = function_expression(unreal.MaterialExpressionFunctionOutput, 0, 250)
    output.set_editor_property("output_name", "Visibility")
    connect(custom, "", output, "")
    EDIT.update_material_function(function)
    save(function)
    return function


def create_master(visibility_function, name, use_alpha):
    material = create_asset(
        name,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    EDIT.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    # The woodland trees these dress (small broadleaf, fir, jacaranda) are Nanite meshes.
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("opacity_mask_clip_value", 0.333)

    default = SURFACES["Grass"]
    diffuse = texture_parameter(material, "DiffuseTexture", default["diffuse"], -1800, -500)
    normal = texture_parameter(
        material, "NormalTexture", default["normal"], -1800, -250,
        unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
    )
    roughness = texture_parameter(
        material, "RoughnessTexture", default["roughness"], -1800, 0,
        unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
    )
    ao = texture_parameter(
        material, "AOTexture", default["ao"], -1800, 250,
        unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
    )
    alpha = None
    if use_alpha:
        alpha = texture_parameter(
            material, "AlphaTexture", default["alpha"], -1800, 500,
            unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
        )
    property_output(diffuse, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    property_output(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    property_output(roughness, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    property_output(ao, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    visibility = expression(
        material, unreal.MaterialExpressionMaterialFunctionCall, -700, 850
    )
    visibility.set_editor_property("material_function", visibility_function)
    if use_alpha:
        mask = expression(material, unreal.MaterialExpressionMultiply, -200, 600)
        connect(alpha, "R", mask, "A")
        connect(visibility, "Visibility", mask, "B")
        property_output(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        property_output(visibility, "Visibility", unreal.MaterialProperty.MP_OPACITY_MASK)

    EDIT.recompile_material(material)
    save(material)
    return material


def create_instances(master, opaque_master):
    result = {}
    for name, textures in SURFACES.items():
        instance = create_asset(
            "MI_CameraSafe_" + name,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        parent = opaque_master if name == "FirSaplingBranches" else master
        EDIT.set_material_instance_parent(instance, parent)
        for parameter, path in (
            ("DiffuseTexture", textures["diffuse"]),
            ("NormalTexture", textures["normal"]),
            ("RoughnessTexture", textures["roughness"]),
            ("AOTexture", textures["ao"]),
        ):
            texture = LIB.load_asset(path)
            if not texture:
                raise RuntimeError(f"Missing admitted texture {path}")
            EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
            applied = EDIT.get_material_instance_texture_parameter_value(instance, parameter)
            if not applied or applied.get_path_name() != texture.get_path_name():
                raise RuntimeError(f"Could not verify {parameter} on {name}")
        if "alpha" in textures:
            texture = LIB.load_asset(textures["alpha"])
            EDIT.set_material_instance_texture_parameter_value(
                instance, "AlphaTexture", texture
            )
            applied = EDIT.get_material_instance_texture_parameter_value(
                instance, "AlphaTexture"
            )
            if not applied or applied.get_path_name() != texture.get_path_name():
                raise RuntimeError(f"Could not verify AlphaTexture on {name}")
        EDIT.update_material_instance(instance)
        save(instance)
        result[name] = instance.get_path_name()
    return result


def inventory():
    rows = []
    for path in MESHES:
        mesh = LIB.load_asset(path)
        if not mesh:
            raise RuntimeError("Missing admitted mesh " + path)
        materials = []
        for slot in range(mesh.get_num_sections(0)):
            material = mesh.get_material(slot)
            materials.append({
                "slot": slot,
                "material": material.get_path_name() if material else "",
            })
        rows.append({"mesh": mesh.get_path_name(), "materials": materials})
    return rows


def main():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    if not LIB.does_directory_exist(DEST) and not LIB.make_directory(DEST):
        raise RuntimeError("Could not create " + DEST)
    collection = create_collection()
    visibility_function = create_visibility_function(collection)
    master = create_master(visibility_function, "M_CameraSafeFoliage", True)
    opaque_master = create_master(
        visibility_function, "M_CameraSafeFoliageOpaque", False
    )
    instances = create_instances(master, opaque_master)
    report = {
        "schemaVersion": 1,
        "collection": collection.get_path_name(),
        "function": visibility_function.get_path_name(),
        "master": master.get_path_name(),
        "opaqueMaster": opaque_master.get_path_name(),
        "instances": instances,
        "meshes": inventory(),
        "policy": {
            "nearRadiusCm": 95,
            "corridorRadiusCm": 62,
            "fadeBandCm": 45,
            "blend": "masked stable world-space dither",
        },
    }
    REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("CAMERA_SAFE_FOLIAGE_AUTHORED " + str(REPORT))


main()
