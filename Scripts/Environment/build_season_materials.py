"""Build the seasonal look's materials (rework-farming-calendar-and-period-crafting, lane D).

Creates /Game/SurvivalGame/Environment/Seasons/MPC_Season (scalars SeasonBlend 0-4, Autumn, WinterBare,
Frost; AHomesteadWorld::UpdateSeasonLook writes them) and wires a season stage into the foliage parents'
base colour and opacity mask:
  - Deciduous (0-1, per instance): leaves turn yellow, orange and russet in autumn (clusters turn at
    different times) and drop in winter (an opacity cut in 35 cm clumps; about 4% hang on).
  - Bracken (0-1): russet in autumn, dead brown in winter, never bare.
  - GrassSeason (0-1): spring-fresh, summer-dry, autumn-straw and dull winter tints.
  - Frost on upward faces within 1.5 m of the ground (tree crowns stay clear).
Evergreens (holly, fir, pine, bramble) keep the defaults (0) and only take the frost.
Also sets the per-species instance parameters listed in SPECIES.

Idempotent: re-running updates the stage's code and parameters in place. Re-run it after
Scripts/Environment/create_camera_safe_foliage.py or import_props.py rebuild a foliage parent.
Run in the editor: pyfile Scripts/Environment/build_season_materials.py (McpHelpers), PIE stopped.
"""
import unreal

LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

FOLDER = "/Game/SurvivalGame/Environment/Seasons"
MPC_PATH = FOLDER + "/MPC_Season"
MPC_SCALARS = {"SeasonBlend": 0.0, "Autumn": 0.0, "WinterBare": 0.0, "Frost": 0.0}
PARENTS = [
    "/Game/SurvivalGame/Materials/M_PropFoliage",
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/M_CameraSafeFoliage",
]
# Per-instance season behaviour: material instance -> {parameter: value}.
DECIDUOUS = {"Deciduous": 1.0}
SPECIES = {
    "/Game/SurvivalGame/Environment/Trees/Oak/MI_SM_OakLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/Trees/Beech/MI_SM_BeechLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/Trees/Sycamore/MI_SM_SycamoreLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/Trees/Hawthorn/MI_SM_HawthornLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/Trees/HazelCoppice/MI_SM_HazelCoppiceLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_TreeSmallLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_JacarandaLeaves": DECIDUOUS,
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_Shrub": {"Deciduous": 0.6},
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_Grass": {"GrassSeason": 1.0},
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_Fern": {"Bracken": 0.6},
    "/Game/SurvivalGame/Environment/CameraSafeFoliage/MI_CameraSafe_Flower": {"GrassSeason": 0.5},
}
# Blender props (import_props.py names their instances MI_<Mesh>... ; matched by folder).
PROP_FOLDERS = {
    "Hazel": DECIDUOUS,
    "Thimbleberry": DECIDUOUS,
    "BrackenFern": {"Bracken": 1.0},
    "GrassYarrowTuft": {"GrassSeason": 1.0},
}
STAGE_TAG = "SeasonStage"

COLOR_CODE = """
float3 seasonBase = Base;
float h = frac(sin(dot(floor(Pos / 35.0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
float lum = dot(Base, float3(0.30, 0.59, 0.11));
// Autumn: each clump of leaves turns at its own time, to yellow, orange or russet.
float3 fall = lerp(lerp(float3(0.62, 0.43, 0.07), float3(0.56, 0.21, 0.04), saturate(h * 2.0)),
                   float3(0.30, 0.11, 0.04), saturate(h * 2.0 - 1.0));
float turn = saturate(Autumn * 1.3 - h * 0.35) * Deciduous;
seasonBase = lerp(seasonBase, fall * (lum * 2.2 + 0.04), turn);
// Bracken and ferns: russet in autumn, dead brown in winter.
float3 dead = lerp(float3(0.34, 0.14, 0.05), float3(0.20, 0.12, 0.07), WinterBare);
seasonBase = lerp(seasonBase, dead * (lum * 2.4 + 0.05), Bracken * saturate(Autumn * 1.2));
// Grass: fresh spring, drier summer, straw autumn, dull winter (weights round the 0-4 year).
float4 w = saturate(1.0 - abs(Blend - float4(0.0, 1.0, 2.0, 3.0)));
w.x += saturate(Blend - 3.0);
float3 grassTint = w.x * float3(1.00, 1.05, 0.90) + w.y * float3(1.05, 1.00, 0.85)
                 + w.z * float3(1.12, 0.95, 0.72) + w.w * float3(0.86, 0.86, 0.80);
seasonBase *= lerp(float3(1, 1, 1), grassTint, GrassSeason);
// Frost on upward faces near the ground.
float low = saturate(1.0 - (Pos.z - ObjectZ) / 150.0);
float frost = Frost * low * saturate(Normal.z * 1.5 - 0.2);
seasonBase = lerp(seasonBase, float3(0.70, 0.74, 0.80), frost * 0.75);
return seasonBase;
"""
OPACITY_CODE = """
float h = frac(sin(dot(floor(Pos / 35.0), float3(12.9898, 78.233, 37.719))) * 43758.5453);
// Deciduous leaves drop in clumps through late autumn; about 4% hang on through winter.
float keep = 1.0 - step(h, WinterBare * 0.96) * step(0.5, Deciduous);
return Mask * keep;
"""


def ensure_folder():
    if not LIB.does_directory_exist(FOLDER):
        LIB.make_directory(FOLDER)


def ensure_collection():
    ensure_folder()
    mpc = LIB.load_asset(MPC_PATH) if LIB.does_asset_exist(MPC_PATH) else None
    if not mpc:
        mpc = TOOLS.create_asset("MPC_Season", FOLDER, unreal.MaterialParameterCollection,
                                 unreal.MaterialParameterCollectionFactoryNew())
    params = list(mpc.get_editor_property("scalar_parameters"))
    names = {str(p.get_editor_property("parameter_name")) for p in params}
    changed = False
    for name, default in MPC_SCALARS.items():
        if name in names:
            continue
        p = unreal.CollectionScalarParameter()
        p.set_editor_property("parameter_name", name)
        p.set_editor_property("default_value", default)
        params.append(p)
        changed = True
    if changed:
        mpc.set_editor_property("scalar_parameters", params)
    LIB.save_loaded_asset(mpc, False)
    return mpc


def input_of(material, prop):
    node = EDIT.get_material_property_input_node(material, prop)
    output = ""
    try:
        output = EDIT.get_material_property_input_node_output_name(material, prop)
    except Exception:
        pass
    return node, output


def custom(material, desc, code, output_type, inputs, x, y):
    node = EDIT.create_material_expression(material, unreal.MaterialExpressionCustom, x, y)
    node.set_editor_property("description", desc)
    node.set_editor_property("output_type", output_type)
    values = []
    for name in inputs:
        value = unreal.CustomInput()
        value.set_editor_property("input_name", name)
        values.append(value)
    node.set_editor_property("inputs", values)
    node.set_editor_property("code", code)
    return node


def link(src, out, dst, inp, where):
    if not EDIT.connect_material_expressions(src, out, dst, inp):
        raise RuntimeError(f"{where}: could not connect {out} -> {inp}")


def wire(material, mpc):
    """Insert (or refresh) the season stage between the parent's base colour/opacity and outputs."""
    path = material.get_path_name()
    base_node, base_out = input_of(material, unreal.MaterialProperty.MP_BASE_COLOR)
    mask_node, mask_out = input_of(material, unreal.MaterialProperty.MP_OPACITY_MASK)
    if isinstance(base_node, unreal.MaterialExpressionCustom) and base_node.get_editor_property("description") == STAGE_TAG + "Color":
        base_node.set_editor_property("code", COLOR_CODE)
        if isinstance(mask_node, unreal.MaterialExpressionCustom):
            mask_node.set_editor_property("code", OPACITY_CODE)
        EDIT.recompile_material(material)
        LIB.save_loaded_asset(material, False)
        return "refreshed"
    if not base_node or not mask_node:
        raise RuntimeError(f"{path}: base colour or opacity mask isn't connected")

    def scalar(name, value, y):
        s = EDIT.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1500, y)
        s.set_editor_property("parameter_name", name)
        s.set_editor_property("default_value", value)
        s.set_editor_property("group", "Season")
        return s

    def collection(name, y):
        c = EDIT.create_material_expression(material, unreal.MaterialExpressionCollectionParameter, -1500, y)
        c.set_editor_property("collection", mpc)
        c.set_editor_property("parameter_name", name)
        return c

    pos = EDIT.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1500, -1400)
    pos.set_editor_property("world_position_shader_offset", unreal.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
    obj = EDIT.create_material_expression(material, unreal.MaterialExpressionObjectPositionWS, -1500, -1300)
    objz = EDIT.create_material_expression(material, unreal.MaterialExpressionComponentMask, -1300, -1300)
    objz.set_editor_property("r", False); objz.set_editor_property("g", False); objz.set_editor_property("b", True)
    link(obj, "", objz, "", path)
    normal = EDIT.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS, -1500, -1200)
    color = custom(material, STAGE_TAG + "Color", COLOR_CODE, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                   ["Base", "Pos", "ObjectZ", "Normal", "Deciduous", "Bracken", "GrassSeason",
                    "Blend", "Autumn", "WinterBare", "Frost"], -900, -1200)
    opacity = custom(material, STAGE_TAG + "Opacity", OPACITY_CODE, unreal.CustomMaterialOutputType.CMOT_FLOAT1,
                     ["Mask", "Pos", "Deciduous", "WinterBare"], -900, -900)
    deciduous = scalar("Deciduous", 0.0, -1100)
    bracken = scalar("Bracken", 0.0, -1050)
    grass = scalar("GrassSeason", 0.0, -1000)
    blend, autumn, bare, frost = (collection(n, y) for n, y in
                                  (("SeasonBlend", -950), ("Autumn", -900), ("WinterBare", -850), ("Frost", -800)))
    link(base_node, base_out, color, "Base", path)
    for src, name in ((pos, "Pos"), (objz, "ObjectZ"), (normal, "Normal"), (deciduous, "Deciduous"),
                      (bracken, "Bracken"), (grass, "GrassSeason"), (blend, "Blend"), (autumn, "Autumn"),
                      (bare, "WinterBare"), (frost, "Frost")):
        link(src, "", color, name, path)
    link(mask_node, mask_out, opacity, "Mask", path)
    for src, name in ((pos, "Pos"), (deciduous, "Deciduous"), (bare, "WinterBare")):
        link(src, "", opacity, name, path)
    if not EDIT.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR):
        raise RuntimeError(f"{path}: could not connect the season colour")
    if not EDIT.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY_MASK):
        raise RuntimeError(f"{path}: could not connect the season opacity")
    EDIT.recompile_material(material)
    LIB.save_loaded_asset(material, False)
    return "wired"


def set_species():
    done, missing = [], []
    targets = dict(SPECIES)
    for folder, params in PROP_FOLDERS.items():
        root = f"/Game/SurvivalGame/Environment/Props/{folder}"
        for path in LIB.list_assets(root, recursive=True, include_folder=False):
            asset = LIB.load_asset(path.split(".")[0])
            if isinstance(asset, unreal.MaterialInstanceConstant):
                targets[asset.get_path_name().split(".")[0]] = params
    for path, params in targets.items():
        mi = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
        if not mi:
            missing.append(path)
            continue
        for name, value in params.items():
            EDIT.set_material_instance_scalar_parameter_value(mi, name, value)
        EDIT.update_material_instance(mi)
        LIB.save_loaded_asset(mi, False)
        done.append(path.rsplit("/", 1)[1])
    return done, missing


def main():
    mpc = ensure_collection()
    for parent in PARENTS:
        material = LIB.load_asset(parent)
        if not material:
            raise RuntimeError("Missing foliage parent " + parent)
        print(parent.rsplit("/", 1)[1], wire(material, mpc))
    done, missing = set_species()
    print("species set:", done)
    if missing:
        print("species missing:", missing)


main()
