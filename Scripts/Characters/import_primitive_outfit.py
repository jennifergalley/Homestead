"""Import the Blender primitive outfit (Assets/Characters/PrimitiveOutfit) onto the MetaHuman heroine.

Run in the editor (MCP run_python):

    exec(open(r'<repo>/Scripts/Characters/import_primitive_outfit.py').read())

Imports SKM_PrimitiveOutfit.fbx (tank top + shorts, two material slots) as a skeletal mesh on
metahuman_base_skel, its 4K textures (OpenGL normals flipped to Unreal's convention) and one
M_PropTextured instance per garment, into /Game/Characters/Heroine_MH/Assembled/Heroine/PrimitiveOutfit.
AHomesteadCharacter wears it over the un-culled body (BodyFull) when it exists.
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


def run():
    props = _props()
    parent = props.textured_parent()
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


run()
