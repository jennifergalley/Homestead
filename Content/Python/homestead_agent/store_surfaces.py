"""Build the general store's tiling surface materials in the running editor (run with run_python).

Creates M_StoreSurfaceTriplanar (world-aligned triplanar, for the walls, limewash and floor, which
are unrotated boxes of any size) and M_StoreSurfaceUV (mesh UVs times a scale, for the pitched
roof slabs), imports the store_surfaces.py bakes and makes one instance per surface.
"""
import importlib.util
import pathlib
import unreal

REPO = pathlib.Path(__file__).resolve().parents[3]
DEST = "/Game/SurvivalGame/Environment/Props/StoreSurfaces"
SOURCE = REPO / "Assets" / "Props" / "StoreSurfaces" / "Textures"
LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

spec = importlib.util.spec_from_file_location("import_props", str(REPO / "Scripts" / "Blender" / "import_props.py"))
props = importlib.util.module_from_spec(spec)
spec.loader.exec_module(props)

TRIPLANAR = """float3 w = pow(abs(N), 6.0); w /= max(w.x + w.y + w.z, 1e-4);
float3 p = WP / Tile;
float4 a = Texture2DSample(Tex, TexSampler, float2(p.y, -p.z));
float4 b = Texture2DSample(Tex, TexSampler, float2(p.x, -p.z));
float4 c = Texture2DSample(Tex, TexSampler, float2(p.x, p.y));
return a * w.x + b * w.y + c * w.z;"""

# Normal maps are imported DirectX-style (green flipped), so +G tilts toward +v, which is world -Z on
# the vertical planes (their v runs down).
TRIPLANAR_NORMAL = """float3 w = pow(abs(N), 6.0); w /= max(w.x + w.y + w.z, 1e-4);
float3 p = WP / Tile;
float2 ta = Texture2DSample(Tex, TexSampler, float2(p.y, -p.z)).xy * 2 - 1;
float2 tb = Texture2DSample(Tex, TexSampler, float2(p.x, -p.z)).xy * 2 - 1;
float2 tc = Texture2DSample(Tex, TexSampler, float2(p.x, p.y)).xy * 2 - 1;
float3 na = float3(sign(N.x) * sqrt(saturate(1 - dot(ta, ta))), ta.x, -ta.y);
float3 nb = float3(tb.x, sign(N.y) * sqrt(saturate(1 - dot(tb, tb))), -tb.y);
float3 nc = float3(tc.x, tc.y, sign(N.z) * sqrt(saturate(1 - dot(tc, tc))));
return normalize(na * w.x + nb * w.y + nc * w.z + N * 0.001);"""


def fresh_material(name):
    path = f"{DEST}/{name}"
    # Reuse an existing material rather than deleting it: deleting one the renderer holds crashed the RHI thread.
    if LIB.does_asset_exist(path):
        m = LIB.load_asset(path)
        MEL.delete_all_material_expressions(m)
        return m
    return TOOLS.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())


def texture_param(material, name, default, x, y, sampler=None):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", default)
    if sampler is not None:
        node.set_editor_property("sampler_type", sampler)
    return node


def custom(material, code, output_type, inputs, x, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output_type)
    args = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        args.append(entry)
    node.set_editor_property("inputs", args)
    return node


def build_triplanar(defaults):
    m = fresh_material("M_StoreSurfaceTriplanar")
    m.set_editor_property("tangent_space_normal", False)
    wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 0)
    n = MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, 150)
    tile = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 300)
    tile.set_editor_property("parameter_name", "TileSize")
    tile.set_editor_property("default_value", 200.0)
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -300, -200)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    rough_scale = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -300, 500)
    rough_scale.set_editor_property("parameter_name", "RoughnessScale")
    rough_scale.set_editor_property("default_value", 1.0)
    slots = [
        ("BaseColor", unreal.MaterialProperty.MP_BASE_COLOR, TRIPLANAR, unreal.CustomMaterialOutputType.CMOT_FLOAT3),
        ("Normal", unreal.MaterialProperty.MP_NORMAL, TRIPLANAR_NORMAL, unreal.CustomMaterialOutputType.CMOT_FLOAT3),
        ("Roughness", unreal.MaterialProperty.MP_ROUGHNESS, TRIPLANAR, unreal.CustomMaterialOutputType.CMOT_FLOAT1),
        ("AO", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION, TRIPLANAR, unreal.CustomMaterialOutputType.CMOT_FLOAT1),
    ]
    for index, (name, prop, code, out) in enumerate(slots):
        y = -200 + index * 250
        tex = texture_param(m, name, defaults[name], -600, y)
        node = custom(m, code, out, ["Tex", "WP", "N", "Tile"], -300, y + 40)
        MEL.connect_material_expressions(tex, "", node, "Tex")
        MEL.connect_material_expressions(wp, "", node, "WP")
        MEL.connect_material_expressions(n, "", node, "N")
        MEL.connect_material_expressions(tile, "", node, "Tile")
        if name == "BaseColor":
            mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -80, y)
            MEL.connect_material_expressions(node, "", mul, "A")
            MEL.connect_material_expressions(tint, "", mul, "B")
            MEL.connect_material_property(mul, "", prop)
        elif name == "Roughness":
            mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -80, y)
            MEL.connect_material_expressions(node, "", mul, "A")
            MEL.connect_material_expressions(rough_scale, "", mul, "B")
            MEL.connect_material_property(mul, "", prop)
        else:
            MEL.connect_material_property(node, "", prop)
    MEL.recompile_material(m)
    LIB.save_loaded_asset(m, False)
    return m


def build_uv(defaults):
    m = fresh_material("M_StoreSurfaceUV")
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    scale = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 150)
    scale.set_editor_property("parameter_name", "UVScale")
    scale.set_editor_property("default_value", unreal.LinearColor(1, 1, 0, 0))
    mask = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -700, 150)
    mask.set_editor_property("r", True)
    mask.set_editor_property("g", True)
    mask.set_editor_property("b", False)
    mask.set_editor_property("a", False)
    MEL.connect_material_expressions(scale, "", mask, "")
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -550, 50)
    MEL.connect_material_expressions(uv, "", mul, "A")
    MEL.connect_material_expressions(mask, "", mul, "B")
    samplers = {
        "BaseColor": (unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, unreal.MaterialProperty.MP_BASE_COLOR),
        "Normal": (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, unreal.MaterialProperty.MP_NORMAL),
        "Roughness": (unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, unreal.MaterialProperty.MP_ROUGHNESS),
        "AO": (unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
    }
    for index, (name, (sampler, prop)) in enumerate(samplers.items()):
        node = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -300, -200 + index * 250)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("texture", defaults[name])
        node.set_editor_property("sampler_type", sampler)
        MEL.connect_material_expressions(mul, "", node, "UVs")
        MEL.connect_material_property(node, "R" if name in ("Roughness", "AO") else "RGB", prop)
    MEL.recompile_material(m)
    LIB.save_loaded_asset(m, False)
    return m


def textures(surface):
    out = {}
    for role, key in (("basecolor", "BaseColor"), ("normal", "Normal"), ("roughness", "Roughness"), ("ao", "AO")):
        source = SOURCE / f"T_Store_{surface}_{role}.png"
        out[key] = props.import_texture(source, f"{DEST}/Textures", role)
    return out


def instance(name, parent, maps, scalars=None, vectors=None):
    path = f"{DEST}/MI_Store_{name}"
    mi = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
        f"MI_Store_{name}", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for key, texture in maps.items():
        MEL.set_material_instance_texture_parameter_value(mi, key, texture)
    for key, value in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, key, value)
    for key, value in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, key, value)
    LIB.save_loaded_asset(mi, False)
    return mi


def main():
    names = ["WallGranite", "SlateRoof", "Limewash", "Floorboards"]
    # The town buildings' ashlar and render surfaces, once baked.
    names += [n for n in ("WallAshlar", "Render") if (SOURCE / f"T_Store_{n}_basecolor.png").exists()]
    maps = {surface: textures(surface) for surface in names}
    tri = build_triplanar(maps["WallGranite"])
    uv = build_uv(maps["SlateRoof"])
    for name in names:
        if name != "SlateRoof":
            instance(name, tri, maps[name], {"TileSize": 200.0})
    # The roof slabs are 10.8 m along the ridge by 7.2 m up the slope.
    instance("SlateRoof", uv, maps["SlateRoof"], vectors={"UVScale": unreal.LinearColor(5.4, 3.6, 0, 0)})
    unreal.log("STORE_SURFACES_BUILT")


main()
