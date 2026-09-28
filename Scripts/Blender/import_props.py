"""Import Blender-built Homestead props into Unreal (editor-side, headless).

UnrealEditor-Cmd.exe SurvivalGame.uproject -run=pythonscript -script=<this file>
    -HomesteadProps=ChoppingBlock[,OtherProp]

Reads Assets/Props/<Name>/report.json, verifies FBX hashes, creates one M_Field
instance per material slot (Tint/Roughness from the report) and imports each
SM_* mesh to /Game/SurvivalGame/Environment/Props/<Name>.
"""
import hashlib
import json
import re
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
PROPS = ROOT / "Assets" / "Props"
DEST_ROOT = "/Game/SurvivalGame/Environment/Props"
LOG = ROOT / "Build" / "Logs" / "prop-import.json"
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary


def save(asset):
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + asset.get_path_name())


TEXTURED_PARENT = "/Game/SurvivalGame/Materials/M_PropTextured"
FOLIAGE_PROP_PARENT = "/Game/SurvivalGame/Materials/M_PropFoliage"
CAMERA_SAFE_VISIBILITY = "/Game/SurvivalGame/Environment/CameraSafeFoliage/MF_CameraSafeFoliageVisibility"
FOLIAGE_PARENT = "/Game/SurvivalGame/Environment/CameraSafeFoliage/M_CameraSafeFoliage"
WHITE = "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"
FLAT_NORMAL = "/Engine/EngineMaterials/DefaultNormal.DefaultNormal"
MASK_DEFAULTS = "/Game/SurvivalGame/Materials/Textures"
# Role -> (compression, sRGB). Blender bakes OpenGL (+Y) normals, flipped on import.
TEXTURE_ROLES = {
    "basecolor": (unreal.TextureCompressionSettings.TC_DEFAULT, True),
    "normal": (unreal.TextureCompressionSettings.TC_NORMALMAP, False),
    "roughness": (unreal.TextureCompressionSettings.TC_MASKS, False),
    "metallic": (unreal.TextureCompressionSettings.TC_MASKS, False),
    "ao": (unreal.TextureCompressionSettings.TC_MASKS, False),
    "alpha": (unreal.TextureCompressionSettings.TC_MASKS, False),
}


def mask_default(value):
    """A tiny non-sRGB masks texture. Engine white/black are sRGB colour textures, which a
    Masks sampler rejects (the whole material then falls back to the default grid)."""
    name = "T_PropDefaultWhite" if value else "T_PropDefaultBlack"
    path = f"{MASK_DEFAULTS}/{name}"
    if LIB.does_asset_exist(path):
        return LIB.load_asset(path)
    source = Path(unreal.Paths.project_intermediate_dir()) / f"{name}.bmp"
    source.parent.mkdir(parents=True, exist_ok=True)
    level = 255 if value else 0
    width = height = 4
    row = bytes([level, level, level]) * width
    pixels = row * height
    header = b"BM" + (54 + len(pixels)).to_bytes(4, "little") + b"\0\0\0\0" + (54).to_bytes(4, "little")
    info = ((40).to_bytes(4, "little") + width.to_bytes(4, "little") + height.to_bytes(4, "little")
            + (1).to_bytes(2, "little") + (24).to_bytes(2, "little") + b"\0" * 24)
    source.write_bytes(header + info + pixels)
    return import_texture(source, MASK_DEFAULTS, "roughness")


def repair_textured_parent(material):
    """Older M_PropTextured assets used engine sRGB defaults on Masks samplers and a NULL
    normal, so they never compiled; point every sampler at a default of the right type."""
    defaults = {"NormalTexture": unreal.load_object(None, FLAT_NORMAL),
                "RoughnessTexture": mask_default(True), "MetallicTexture": mask_default(False),
                "AOTexture": mask_default(True)}
    changed = False
    for index in range(8):
        node = unreal.find_object(material, f"MaterialExpressionTextureSampleParameter2D_{index}")
        wanted = node and defaults.get(str(node.get_editor_property("parameter_name")))
        if wanted and node.get_editor_property("texture") != wanted:
            node.set_editor_property("texture", wanted)
            changed = True
    # Worn garments (Scripts\Characters\import_primitive_outfit.py) share this parent, the Estate's
    # scenery draws props as instanced meshes, and the ruin kit is Nanite. A cooked build falls back
    # to the default material for any usage that isn't flagged here.
    for usage in ("used_with_skeletal_mesh", "used_with_instanced_static_meshes", "used_with_nanite"):
        if not material.get_editor_property(usage):
            material.set_editor_property(usage, True)
            changed = True
    if changed:
        EDIT.recompile_material(material)
        save(material)


def textured_parent():
    """Opaque PBR parent for baked Blender props (base color, normal, roughness, metallic, AO)."""
    if LIB.does_asset_exist(TEXTURED_PARENT):
        material = LIB.load_asset(TEXTURED_PARENT)
        repair_textured_parent(material)
        return material
    folder, name = TEXTURED_PARENT.rsplit("/", 1)
    material = TOOLS.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + TEXTURED_PARENT)
    for usage in ("used_with_skeletal_mesh", "used_with_instanced_static_meshes", "used_with_nanite"):
        material.set_editor_property(usage, True)
    masks = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
    samplers = [
        ("BaseColorTexture", WHITE, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, "RGB",
         unreal.MaterialProperty.MP_BASE_COLOR),
        ("NormalTexture", FLAT_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, "RGB",
         unreal.MaterialProperty.MP_NORMAL),
        ("RoughnessTexture", mask_default(True), masks, "R", unreal.MaterialProperty.MP_ROUGHNESS),
        ("MetallicTexture", mask_default(False), masks, "R", unreal.MaterialProperty.MP_METALLIC),
        ("AOTexture", mask_default(True), masks, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
    ]
    for index, (parameter, default, sampler, output, target) in enumerate(samplers):
        node = EDIT.create_material_expression(
            material, unreal.MaterialExpressionTextureSampleParameter2D, -500, index * 260 - 500)
        node.set_editor_property("parameter_name", parameter)
        node.set_editor_property("texture", unreal.load_object(None, default) if isinstance(default, str) else default)
        node.set_editor_property("sampler_type", sampler)
        if not EDIT.connect_material_property(node, output, target):
            raise RuntimeError(f"Could not connect {parameter} in {TEXTURED_PARENT}")
    EDIT.recompile_material(material)
    save(material)
    return material


FOLIAGE_WIND_CODE = """
float h = VertexColor.r;
float phase = VertexColor.g * 6.2832;
float gust = sin(Time * 0.23 + dot(WorldPos.xy, float2(0.0011, 0.0007))) * 0.35 + 0.65;
float sway = sin(Time * 0.8 + phase + dot(WorldPos.xy, float2(0.004, 0.003))) * 0.7
           + sin(Time * 1.45 + phase * 1.7) * 0.3;
float3 offset = normalize(float3(1.0, 0.45, 0.0)) * (sway * gust * Strength * h * h);
offset += float3(sin(Time * 4.1 + phase * 5.0), cos(Time * 3.7 + phase * 3.0),
                 0.3 * sin(Time * 4.9 + phase)) * (Flutter * VertexColor.b * gust);
return offset;
"""
# A slow, small sway: the first pass (3.5 cm, faster flutter) read as bushes bouncing in place.
FOLIAGE_WIND_STRENGTH = 1.6
FOLIAGE_LEAF_FLUTTER = 0.2


def sync_foliage_wind(material):
    """Bring an existing foliage parent's wind node and defaults up to FOLIAGE_WIND_CODE."""
    wind = EDIT.get_material_property_input_node(material, unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    if not isinstance(wind, unreal.MaterialExpressionCustom):
        return material
    changed = wind.get_editor_property("code").strip() != FOLIAGE_WIND_CODE.strip()
    if changed:
        wind.set_editor_property("code", FOLIAGE_WIND_CODE)
    for source in EDIT.get_inputs_for_material_expression(material, wind):
        if isinstance(source, unreal.MaterialExpressionScalarParameter):
            target = {"WindStrength": FOLIAGE_WIND_STRENGTH, "LeafFlutter": FOLIAGE_LEAF_FLUTTER}.get(
                str(source.get_editor_property("parameter_name")))
            if target is not None and abs(source.get_editor_property("default_value") - target) > 1e-4:
                source.set_editor_property("default_value", target)
                changed = True
    if changed:
        EDIT.recompile_material(material)
        save(material)
    return material


def foliage_parent():
    """Masked two-sided foliage parent for Blender-built underbrush. The recipes pack
    R roughness / G translucency / B AO into the roughness map, carry opacity in the base
    colour alpha, and bake wind weights into vertex colour (R height, G phase, B flutter).
    The camera-safe dither keeps shrubs between the camera and heroine from hiding her."""
    if LIB.does_asset_exist(FOLIAGE_PROP_PARENT):
        return sync_foliage_wind(LIB.load_asset(FOLIAGE_PROP_PARENT))
    folder, name = FOLIAGE_PROP_PARENT.rsplit("/", 1)
    material = TOOLS.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + FOLIAGE_PROP_PARENT)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    material.set_editor_property("opacity_mask_clip_value", 0.5)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    def node(kind, x, y):
        created = EDIT.create_material_expression(material, kind, x, y)
        if not created:
            raise RuntimeError(f"Could not create {kind} in {FOLIAGE_PROP_PARENT}")
        return created

    def link(source, output, target, input_name):
        if not EDIT.connect_material_expressions(source, output, target, input_name):
            raise RuntimeError(f"Could not connect {output} -> {input_name} in {FOLIAGE_PROP_PARENT}")

    def out(source, output, prop):
        if not EDIT.connect_material_property(source, output, prop):
            raise RuntimeError(f"Could not connect {output} to {prop} in {FOLIAGE_PROP_PARENT}")

    def sampler(parameter, default, kind, y):
        created = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, y)
        created.set_editor_property("parameter_name", parameter)
        created.set_editor_property("texture", unreal.load_object(None, default) if isinstance(default, str) else default)
        created.set_editor_property("sampler_type", kind)
        return created

    def scalar(parameter, value, x, y):
        created = node(unreal.MaterialExpressionScalarParameter, x, y)
        created.set_editor_property("parameter_name", parameter)
        created.set_editor_property("default_value", value)
        return created

    base = sampler("BaseColorTexture", WHITE, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -600)
    normal = sampler("NormalTexture", FLAT_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -300)
    packed = sampler("PackedTexture", mask_default(True), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 0)
    out(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    out(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    out(packed, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    out(packed, "B", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    tint = node(unreal.MaterialExpressionVectorParameter, -600, 250)
    tint.set_editor_property("parameter_name", "TranslucencyTint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.85, 1.0, 0.45, 1.0))
    translucent = node(unreal.MaterialExpressionMultiply, -350, 150)
    link(base, "RGB", translucent, "A")
    link(packed, "G", translucent, "B")
    subsurface = node(unreal.MaterialExpressionMultiply, -150, 200)
    link(translucent, "", subsurface, "A")
    link(tint, "", subsurface, "B")
    out(subsurface, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    visibility_function = LIB.load_asset(CAMERA_SAFE_VISIBILITY)
    if visibility_function:
        visibility = node(unreal.MaterialExpressionMaterialFunctionCall, -600, 450)
        visibility.set_editor_property("material_function", visibility_function)
        mask = node(unreal.MaterialExpressionMultiply, -300, 450)
        link(base, "A", mask, "A")
        link(visibility, "Visibility", mask, "B")
        out(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        out(base, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    wind = node(unreal.MaterialExpressionCustom, -300, 700)
    wind.set_editor_property("description", "Vertex-colour weighted sway and leaf flutter")
    wind.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for input_name in ("VertexColor", "WorldPos", "Time", "Strength", "Flutter"):
        value = unreal.CustomInput()
        value.set_editor_property("input_name", input_name)
        inputs.append(value)
    wind.set_editor_property("inputs", inputs)
    wind.set_editor_property("code", FOLIAGE_WIND_CODE)
    link(node(unreal.MaterialExpressionVertexColor, -700, 650), "", wind, "VertexColor")
    link(node(unreal.MaterialExpressionWorldPosition, -700, 750), "", wind, "WorldPos")
    link(node(unreal.MaterialExpressionTime, -700, 850), "", wind, "Time")
    link(scalar("WindStrength", FOLIAGE_WIND_STRENGTH, -700, 950), "", wind, "Strength")
    link(scalar("LeafFlutter", FOLIAGE_LEAF_FLUTTER, -700, 1050), "", wind, "Flutter")
    out(wind, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    EDIT.recompile_material(material)
    save(material)
    return material


GRANITE_PARENT = "/Game/SurvivalGame/Materials/M_PropGranite"
# Per-prop overrides of report.json: a convex hull would fill the split boulder's gap, and a box
# would plug the stone doorway's opening.
COLLISION_OVERRIDES = {"GraniteSplitBoulder": "complex", "StoneDoorway": "complex"}
# Which LOD per-poly collision uses (default: the coarsest); the doorway keeps its reveals true.
COLLISION_LOD = {"StoneDoorway": 1}
# Million-triangle house-sized rocks render through Nanite; LOD1/LOD2 stay as the fallback.
NANITE_PROPS = {"GraniteDome", "GraniteSplitBoulder", "RuinWallTall", "RuinWallMid", "RuinWallLow", "RuinChimney",
                "RuinIvy", "RuinSlateScatter", "RuinFallenTimbers"}
# Alpha-free leaf meshes on the opaque textured parent: show both faces of each blade.
TWO_SIDED_PROPS = {"RuinIvy"}
DETAIL_DEST = f"{DEST_ROOT}/GraniteDetail/Textures"
TRIPLANAR_CODE = """
float3 w = pow(abs(normalize(N)), 4.0);
w /= (w.x + w.y + w.z);
float3 p = WorldPos * (Tile / 100.0);
return Texture2DSample(Tex, TexSampler, p.yz).rgb * w.x
     + Texture2DSample(Tex, TexSampler, p.xz).rgb * w.y
     + Texture2DSample(Tex, TexSampler, p.xy).rgb * w.z;
"""
DETAIL_NORMAL_CODE = """
float2 d = DetailRaw.xy * 2.0 - 1.0;
return normalize(float3(Macro.xy + d * K, Macro.z));
"""


def granite_parent():
    """Big Blender granite: a macro bake (no crystals) layered with the shared GraniteDetail
    crystal tiling (triplanar, world-aligned 1 m tiles) where T_<Name>_mask is bare rock:
    k = DetailStrength * mask; base = macro * lerp(1, 2 * detail, k); roughness =
    lerp(macro, detail, 0.5 * k); the detail normal is added over the macro by k."""
    if LIB.does_asset_exist(GRANITE_PARENT):
        return LIB.load_asset(GRANITE_PARENT)
    folder, name = GRANITE_PARENT.rsplit("/", 1)
    material = TOOLS.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + GRANITE_PARENT)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    material.set_editor_property("used_with_nanite", True)
    color = unreal.MaterialSamplerType.SAMPLERTYPE_COLOR
    masks = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS
    normals = unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL

    def node(kind, x, y):
        created = EDIT.create_material_expression(material, kind, x, y)
        if not created:
            raise RuntimeError(f"Could not create {kind} in {GRANITE_PARENT}")
        return created

    def link(source, output, target, input_name):
        if not EDIT.connect_material_expressions(source, output, target, input_name):
            raise RuntimeError(f"Could not connect {output} -> {input_name} in {GRANITE_PARENT}")

    def out(source, output, prop):
        if not EDIT.connect_material_property(source, output, prop):
            raise RuntimeError(f"Could not connect {output} to {prop} in {GRANITE_PARENT}")

    def sampler(parameter, default, kind, y, x=-1200, cls=unreal.MaterialExpressionTextureSampleParameter2D):
        created = node(cls, x, y)
        created.set_editor_property("parameter_name", parameter)
        created.set_editor_property("texture", unreal.load_object(None, default) if isinstance(default, str) else default)
        created.set_editor_property("sampler_type", kind)
        return created

    def scalar(parameter, value, x, y):
        created = node(unreal.MaterialExpressionScalarParameter, x, y)
        created.set_editor_property("parameter_name", parameter)
        created.set_editor_property("default_value", value)
        return created

    def custom(description, code, names, x, y):
        created = node(unreal.MaterialExpressionCustom, x, y)
        created.set_editor_property("description", description)
        created.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs = []
        for input_name in names:
            value = unreal.CustomInput()
            value.set_editor_property("input_name", input_name)
            inputs.append(value)
        created.set_editor_property("inputs", inputs)
        created.set_editor_property("code", code)
        return created

    base = sampler("BaseColorTexture", WHITE, color, -700)
    normal = sampler("NormalTexture", FLAT_NORMAL, normals, -400)
    rough = sampler("RoughnessTexture", mask_default(True), masks, -100)
    ao = sampler("AOTexture", mask_default(True), masks, 200)
    mask = sampler("MaskTexture", mask_default(True), masks, 500)
    world = node(unreal.MaterialExpressionWorldPosition, -1500, 900)
    vertex_normal = node(unreal.MaterialExpressionVertexNormalWS, -1500, 1000)
    tile = scalar("DetailTilesPerMetre", 1.0, -1500, 1100)

    def triplanar(parameter, default, kind, y):
        texture = sampler(parameter, default, kind, y, -1500, unreal.MaterialExpressionTextureObjectParameter)
        sample = custom("Triplanar " + parameter, TRIPLANAR_CODE, ("Tex", "WorldPos", "N", "Tile"), -1100, y)
        link(texture, "", sample, "Tex")
        link(world, "", sample, "WorldPos")
        link(vertex_normal, "", sample, "N")
        link(tile, "", sample, "Tile")
        return sample

    detail_base = triplanar("DetailBaseColor", WHITE, color, 800)
    detail_normal = triplanar("DetailNormal", FLAT_NORMAL, normals, 1300)
    detail_rough = triplanar("DetailRoughness", mask_default(True), masks, 1800)

    k = node(unreal.MaterialExpressionMultiply, -900, 500)
    link(mask, "R", k, "A")
    link(scalar("DetailStrength", 0.5, -1100, 600), "", k, "B")

    doubled = node(unreal.MaterialExpressionMultiply, -800, 800)
    link(detail_base, "", doubled, "A")
    doubled.set_editor_property("const_b", 2.0)
    layer = node(unreal.MaterialExpressionLinearInterpolate, -600, 700)
    layer.set_editor_property("const_a", 1.0)
    link(doubled, "", layer, "B")
    link(k, "", layer, "Alpha")
    tinted = node(unreal.MaterialExpressionMultiply, -600, -700)
    link(base, "RGB", tinted, "A")
    link(scalar("MacroBrightness", 0.74, -800, -600), "", tinted, "B")
    color_out = node(unreal.MaterialExpressionMultiply, -400, -600)
    link(tinted, "", color_out, "A")
    link(layer, "", color_out, "B")
    out(color_out, "", unreal.MaterialProperty.MP_BASE_COLOR)

    half_k = node(unreal.MaterialExpressionMultiply, -800, 1900)
    link(k, "", half_k, "A")
    half_k.set_editor_property("const_b", 0.5)
    rough_out = node(unreal.MaterialExpressionLinearInterpolate, -400, -100)
    link(rough, "R", rough_out, "A")
    link(detail_rough, "", rough_out, "B")
    link(half_k, "", rough_out, "Alpha")
    out(rough_out, "", unreal.MaterialProperty.MP_ROUGHNESS)

    normal_out = custom("Detail normal over macro", DETAIL_NORMAL_CODE, ("Macro", "DetailRaw", "K"), -400, -400)
    link(normal, "RGB", normal_out, "Macro")
    link(detail_normal, "", normal_out, "DetailRaw")
    link(k, "", normal_out, "K")
    out(normal_out, "", unreal.MaterialProperty.MP_NORMAL)
    out(ao, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    EDIT.recompile_material(material)
    save(material)
    return material


def granite_instance(spec, info, folder, dest, report):
    maps = info.get("bake", {}).get("maps", {})
    parameters = {"basecolor": "BaseColorTexture", "normal": "NormalTexture", "roughness": "RoughnessTexture",
                  "ao": "AOTexture", "mask": "MaskTexture"}
    name = "MI_" + spec["name"][2:]
    path = f"{dest}/{name}"
    instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not instance:
        raise RuntimeError("Could not create " + path)
    EDIT.set_material_instance_parent(instance, granite_parent())
    for role, parameter in parameters.items():
        source = folder / "Textures" / maps[role]
        if not source.exists():
            source = folder / maps[role]
        texture = import_texture(source, f"{dest}/Textures", "roughness" if role == "mask" else role)
        EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
    details = {"basecolor": ("DetailBaseColor", "basecolor"), "normal": ("DetailNormal", "normal"),
               "roughness": ("DetailRoughness", "roughness")}
    for relative in report["notes"]["detail_textures"]:
        source = ROOT / relative
        role = source.stem.rsplit("_", 1)[-1]
        if role not in details:
            continue
        parameter, texture_role = details[role]
        texture = LIB.load_asset(f"{DETAIL_DEST}/{source.stem}") if LIB.does_asset_exist(
            f"{DETAIL_DEST}/{source.stem}") else import_texture(source, DETAIL_DEST, texture_role)
        EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
    EDIT.update_material_instance(instance)
    save(instance)
    return instance


def import_texture(source, dest, role):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", source.stem)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    TOOLS.import_asset_tasks([task])
    texture = LIB.load_asset(f"{dest}/{source.stem}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Texture import failed: " + str(source))
    compression, srgb = TEXTURE_ROLES[role]
    texture.set_editor_property("compression_settings", compression)
    texture.set_editor_property("srgb", srgb)
    if role == "normal":
        texture.set_editor_property("flip_green_channel", True)
    save(texture)
    return texture


def textured_instance(spec, info, folder, dest, report=None):
    """Baked props get an M_PropTextured instance; Blender foliage (reports with a ``wind``
    block) gets an M_PropFoliage instance; ones with a separate alpha map get a camera-safe
    masked two-sided foliage instance (the woodland fades those near the camera)."""
    textures = dict(info.get("bake", {}).get("maps", {}))
    textures.update(spec["textures"])
    textures = {role: file for role, file in textures.items() if role in TEXTURE_ROLES}
    packed_foliage = bool(report and report.get("wind"))
    foliage = "alpha" in textures and not packed_foliage
    parent = foliage_parent() if packed_foliage else \
        LIB.load_asset(FOLIAGE_PARENT) if foliage else textured_parent()
    if not parent:
        raise RuntimeError("Missing parent material " + (FOLIAGE_PARENT if foliage else TEXTURED_PARENT))
    parameters = {"basecolor": "DiffuseTexture" if foliage else "BaseColorTexture", "normal": "NormalTexture",
                  "roughness": "PackedTexture" if packed_foliage else "RoughnessTexture",
                  "metallic": "MetallicTexture", "ao": "AOTexture", "alpha": "AlphaTexture"}
    name = "MI_" + spec["name"][2:]
    path = f"{dest}/{name}"
    instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not instance:
        raise RuntimeError("Could not create " + path)
    EDIT.set_material_instance_parent(instance, parent)
    for role, file in textures.items():
        if (foliage or packed_foliage) and role in ("metallic", "ao"):
            continue
        source = folder / "Textures" / file
        if not source.exists():
            source = folder / file
        texture = import_texture(source, f"{dest}/Textures", role)
        EDIT.set_material_instance_texture_parameter_value(instance, parameters[role], texture)
    if report and report.get("name") in TWO_SIDED_PROPS:
        overrides = instance.get_editor_property("base_property_overrides")
        overrides.set_editor_property("override_two_sided", True)
        overrides.set_editor_property("two_sided", True)
        instance.set_editor_property("base_property_overrides", overrides)
    EDIT.update_material_instance(instance)
    save(instance)
    return instance


def material_instance(spec, dest, parent, info=None, folder=None, report=None):
    if report and report.get("notes", {}).get("detail_textures") and "mask" in (info or {}).get("bake", {}).get("maps", {}):
        return granite_instance(spec, info, folder, dest, report)
    if "textures" in spec:
        return textured_instance(spec, info or {}, folder, dest, report)
    name = "MI_" + spec["name"][2:]
    path = f"{dest}/{name}"
    instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not instance:
        raise RuntimeError("Could not create " + path)
    EDIT.set_material_instance_parent(instance, parent)
    r, g, b = spec["color"]
    EDIT.set_material_instance_vector_parameter_value(instance, "Tint", unreal.LinearColor(r, g, b, 1))
    EDIT.set_material_instance_scalar_parameter_value(instance, "Roughness", spec["roughness"])
    save(instance)
    return instance


def import_mesh(source, dest, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("factory", unreal.FbxFactory())
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    data = options.get_editor_property("static_mesh_import_data")
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    task.set_editor_property("options", options)
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(f"{dest}/{name}")
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Static mesh import failed: " + name)
    return mesh


def add_collision(mesh, kind):
    if kind == "none":
        return
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if subsystem is None:
        raise RuntimeError("Collision needs the StaticMeshEditorSubsystem, which -run=pythonscript lacks; "
                           "run import_props.main([...]) inside the editor (Start-EditorMcp.ps1 -AllowPython)")
    subsystem.remove_collisions(mesh)
    if kind == "complex":
        # Walk into splits and slab gaps a convex hull would fill; the collision LOD is set after LODs import.
        body = mesh.get_editor_property("body_setup")
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        return
    if kind == "convex" and subsystem.set_convex_decomposition_collisions(mesh, 1, 24, 100000):
        return
    shape = unreal.ScriptCollisionShapeType.BOX if kind == "box" \
        else unreal.ScriptCollisionShapeType.NDOP26
    if subsystem.add_simple_collisions(mesh, shape) < 0:
        raise RuntimeError(f"Could not add {kind} collision to {mesh.get_name()}")


def import_prop(name, parent):
    folder = PROPS / name
    report = json.loads((folder / "report.json").read_text(encoding="utf-8"))
    dest = f"{DEST_ROOT}/{name}"
    result = {}
    lods = {}
    for mesh_name, info in report["meshes"].items():
        source = folder / info["fbx"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != info["sha256"]:
            raise RuntimeError(f"{source} changed after the Blender build; rebuild it first")
        lod = re.match(r"^(SM_.+)_LOD(\d+)$", mesh_name)
        if lod and lod.group(1) in report["meshes"]:
            # Blender decimates LOD1/LOD2 from LOD0; they become LODs of that mesh.
            lods.setdefault(lod.group(1), []).append((int(lod.group(2)), source))
            continue
        instances = {spec["name"]: material_instance(spec, dest, parent, info, folder, report)
                     for spec in info["materials"]}
        mesh = import_mesh(source, dest, mesh_name)
        slots = [str(s.get_editor_property("material_slot_name")) for s in
                 mesh.get_editor_property("static_materials")]
        if sorted(slots) != sorted(instances):
            raise RuntimeError(f"{mesh_name} material slots {slots} != report {sorted(instances)}")
        for index, slot in enumerate(slots):
            mesh.set_material(index, instances[slot])
        add_collision(mesh, COLLISION_OVERRIDES.get(name, report.get("collision", "box")))
        if name in NANITE_PROPS:
            settings = mesh.get_editor_property("nanite_settings")
            settings.set_editor_property("enabled", True)
            mesh.set_editor_property("nanite_settings", settings)
        save(mesh)
        extent = mesh.get_bounds().box_extent
        size = [extent.x * 2, extent.y * 2, extent.z * 2]
        if abs(size[2] - info["size_cm"][2]) > 1.0:
            raise RuntimeError(f"{mesh_name} height {size[2]:.1f} cm != Blender {info['size_cm'][2]} cm")
        result[mesh_name] = {"asset": mesh.get_path_name(), "size_cm": [round(v, 2) for v in size],
                             "material_slots": slots, "collision": report.get("collision", "box")}
    library = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for base, entries in lods.items():
        mesh = LIB.load_asset(f"{dest}/{base}")
        for index, source in sorted(entries):
            if library.import_lod(mesh, index, str(source)) != index:
                raise RuntimeError(f"Could not import {source.name} as LOD{index} of {base}")
        # Stale separate assets from before LODs were merged.
        for index, _ in entries:
            stale = f"{dest}/{base}_LOD{index}"
            if LIB.does_asset_exist(stale):
                LIB.delete_asset(stale)
        if COLLISION_OVERRIDES.get(name) == "complex":
            mesh.set_editor_property("lod_for_collision",
                                     COLLISION_LOD.get(name, max(index for index, _ in entries)))
        save(mesh)
        result[base]["lods"] = mesh.get_num_lods()
    return result


def main(names=None):
    """Headless via Import-Props.ps1, or in a running editor over MCP (where the static-mesh
    editor subsystem exists for collision): ``import import_props; import_props.main([...])``."""
    match = re.search(r"-HomesteadProps=([\w,]+)", unreal.SystemLibrary.get_command_line())
    names = names or (match.group(1).split(",") if match else sorted(
        p.name for p in PROPS.iterdir() if (p / "report.json").exists()))
    parent = LIB.load_asset("/Game/SurvivalGame/Materials/M_Field")
    if not isinstance(parent, unreal.Material):
        raise RuntimeError("M_Field must exist (run Scripts\\Build-Game.ps1 once) before importing props")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")
    results = {name: import_prop(name, parent) for name in names}
    LOG.parent.mkdir(parents=True, exist_ok=True)
    LOG.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    unreal.log("HOMESTEAD_PROPS_IMPORTED " + str(LOG))


if __name__ == "__main__":
    main()
