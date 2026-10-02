"""Import the Blender primitive outfit (Assets/Characters/PrimitiveOutfit) onto the MetaHuman heroine.

Run in the editor (MCP run_python):

    exec(open(r'<repo>/Scripts/Characters/import_primitive_outfit.py').read())

Imports SKM_PrimitiveOutfit.fbx (tank top + shorts, two material slots) as a skeletal mesh on
metahuman_base_skel, its 4K textures (OpenGL normals flipped to Unreal's convention) and one
M_HomespunDyeable instance per garment, into /Game/Characters/Heroine_MH/Assembled/Heroine/PrimitiveOutfit.
M_HomespunDyeable is M_PropTextured with a "Tint" vector (default white) multiplying the base colour,
which AHomesteadCharacter::ApplyMetaHumanTunicDye sets for the dyed linen tunic.
AHomesteadCharacter wears it over the un-culled body (BodyFull) when it exists.

To re-parent the existing instances without reimporting the mesh or textures, set
REPARENT_ONLY = True before exec.
"""
import importlib.util
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir())
SOURCE = ROOT / "Assets" / "Characters" / "PrimitiveOutfit"
DEST = "/Game/Characters/Heroine_MH/Assembled/Heroine/PrimitiveOutfit"
SKELETON = "/Game/Characters/Heroine_MH/Common/Female/Medium/NormalWeight/Body/metahuman_base_skel"
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
GARMENTS = {"M_PrimitiveTankTop": "T_PrimitiveTankTop", "M_PrimitiveShorts": "T_PrimitiveShorts"}
DYEABLE_PARENT = f"{DEST}/M_HomespunDyeable"


def _props():
    spec = importlib.util.spec_from_file_location("import_props", ROOT / "Scripts" / "Blender" / "import_props.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _import_mesh():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE / "SKM_PrimitiveOutfit.fbx"))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "SKM_PrimitiveOutfit")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("factory", unreal.FbxFactory())
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("skeleton", unreal.load_asset(SKELETON))
    options.set_editor_property("create_physics_asset", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_animations", False)
    data = options.get_editor_property("skeletal_mesh_import_data")
    # The garments were fitted to the body as Unreal exported it; converting the scene axes would turn them 180 degrees.
    data.set_editor_property("convert_scene", False)
    data.set_editor_property("convert_scene_unit", False)
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("update_skeleton_reference_pose", False)
    data.set_editor_property("use_t0_as_ref_pose", False)
    task.set_editor_property("options", options)
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(f"{DEST}/SKM_PrimitiveOutfit")
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError("Primitive outfit import failed")
    return mesh


def dyeable_parent(props):
    """M_PropTextured with BaseColorTexture multiplied by a "Tint" vector parameter (white = undyed)."""
    if LIB.does_asset_exist(DYEABLE_PARENT):
        return LIB.load_asset(DYEABLE_PARENT)
    source = props.textured_parent()
    material = LIB.duplicate_asset(source.get_path_name(), DYEABLE_PARENT)
    if not isinstance(material, unreal.Material):
        raise RuntimeError("Could not create " + DYEABLE_PARENT)
    albedo = EDIT.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    if not albedo:
        raise RuntimeError(DYEABLE_PARENT + " has no base colour input to tint")
    tint = EDIT.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, -760)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    multiply = EDIT.create_material_expression(material, unreal.MaterialExpressionMultiply, -220, -500)
    if not (EDIT.connect_material_expressions(albedo, "RGB", multiply, "A")
            and EDIT.connect_material_expressions(tint, "", multiply, "B")
            and EDIT.connect_material_property(multiply, "", unreal.MaterialProperty.MP_BASE_COLOR)):
        raise RuntimeError("Could not wire the Tint in " + DYEABLE_PARENT)
    EDIT.recompile_material(material)
    props.save(material)
    return material


def reparent_instances(props):
    parent = dyeable_parent(props)
    for name in GARMENTS:
        instance = LIB.load_asset(f"{DEST}/MI_{name[2:]}")
        EDIT.set_material_instance_parent(instance, parent)
        EDIT.update_material_instance(instance)
        props.save(instance)
    print(json.dumps({"parent": parent.get_path_name(), "instances": [f"MI_{n[2:]}" for n in GARMENTS]}))


def run():
    props = _props()
    parent = dyeable_parent(props)
    mesh = _import_mesh()
    materials = mesh.materials
    for index, slot in enumerate(materials):
        name = str(slot.material_slot_name)
        prefix = GARMENTS.get(name)
        if not prefix:
            raise RuntimeError(f"Unexpected outfit material slot {name}")
        path = f"{DEST}/MI_{name[2:]}"
        instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
            f"MI_{name[2:]}", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(instance, parent)
        for role, parameter in (("basecolor", "BaseColorTexture"), ("normal", "NormalTexture"),
                                ("roughness", "RoughnessTexture"), ("ao", "AOTexture")):
            texture = props.import_texture(SOURCE / "Textures" / f"{prefix}_{role}.png", f"{DEST}/Textures", role)
            EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
        EDIT.update_material_instance(instance)
        props.save(instance)
        slot.material_interface = instance
        materials[index] = slot
    mesh.materials = materials
    props.save(mesh)
    bounds = mesh.get_bounds()
    result = {"mesh": mesh.get_path_name(), "slots": [str(s.material_slot_name) for s in mesh.materials],
              "extent_cm": [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2],
              "skeleton": mesh.skeleton.get_path_name()}
    print(json.dumps(result, indent=2))
    return result


if globals().get("REPARENT_ONLY"):
    reparent_instances(_props())
else:
    run()
