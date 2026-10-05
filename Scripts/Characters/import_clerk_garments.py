"""Import Mr. Trethewey's Blender-fitted garments and pencil onto the clerk MetaHuman.

Run in the editor (MCP run_python):

    exec(open(r'<repo>/Scripts/Characters/import_clerk_garments.py').read())

Every SKM_<Name>.fbx in Assets/Characters/ClerkClothing named in GARMENTS becomes a skeletal mesh
on the clerk's metahuman_base_skel in /Game/Characters/Clerk_MH/Assembled/Clerk/Garments, and
SM_ClerkPencil.fbx a static mesh beside them. Each material slot M_<X> gets an M_PropTextured
instance MI_<X> built from Textures/T_<X>_*.png (OpenGL normals are flipped to Unreal's convention
by import_props). AHomesteadShopkeeper loads them by name (HomesteadShopkeeperMetaHuman.cpp).
"""
import importlib.util
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir())
SOURCE = ROOT / "Assets" / "Characters" / "ClerkClothing"
# Must match GarmentNames in HomesteadShopkeeperMetaHuman.cpp.
GARMENTS = ["SKM_ClerkTrousers", "SKM_ClerkBoots", "SKM_ClerkShirt", "SKM_ClerkWaistcoat",
            "SKM_ClerkNeckerchief", "SKM_ClerkApron"]
PENCIL = "SM_ClerkPencil"
DEST = "/Game/Characters/Clerk_MH/Assembled/Clerk/Garments"
SKELETON = "/Game/Characters/Clerk_MH/Common/Female/Medium/NormalWeight/Body/metahuman_base_skel"
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
ROLES = (("basecolor", "BaseColorTexture"), ("normal", "NormalTexture"),
         ("roughness", "RoughnessTexture"), ("ao", "AOTexture"))


def _props():
    spec = importlib.util.spec_from_file_location("import_props", ROOT / "Scripts" / "Blender" / "import_props.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _import_skeletal(fbx):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(fbx))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", fbx.stem)
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
    # Fitted to the body as Unreal exported it; converting the scene axes would turn them 180 degrees.
    data.set_editor_property("convert_scene", False)
    data.set_editor_property("convert_scene_unit", False)
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("update_skeleton_reference_pose", False)
    data.set_editor_property("use_t0_as_ref_pose", False)
    task.set_editor_property("options", options)
    TOOLS.import_asset_tasks([task])
    mesh = LIB.load_asset(f"{DEST}/{fbx.stem}")
    if not isinstance(mesh, unreal.SkeletalMesh):
        raise RuntimeError(f"{fbx.name} import failed")
    return mesh


def _find(stem):
    """The exported FBX, in its own folder (ClerkApron/SKM_ClerkApron.fbx) or flat under SOURCE."""
    return next(iter(sorted(SOURCE.rglob(f"{stem}.fbx"))), SOURCE / f"{stem}.fbx")


def _materials(props, parent, mesh, name, fbx):
    textures = fbx.parent / "Textures" if (fbx.parent / "Textures").is_dir() else fbx.parent
    materials = mesh.get_editor_property("static_materials" if isinstance(mesh, unreal.StaticMesh) else "materials")
    for index, slot in enumerate(materials):
        slot_name = str(slot.material_slot_name)
        if not slot_name.startswith("M_"):
            raise RuntimeError(f"{name}: unexpected material slot {slot_name}")
        prefix = "T_" + slot_name[2:]
        path = f"{DEST}/MI_{slot_name[2:]}"
        instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
            f"MI_{slot_name[2:]}", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(instance, parent)
        for role, parameter in ROLES:
            source = textures / f"{prefix}_{role}.png"
            if not source.exists():
                raise RuntimeError(f"{name}: missing {source.name}")
            texture = props.import_texture(source, f"{DEST}/Textures", role)
            EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
        EDIT.update_material_instance(instance)
        props.save(instance)
        slot.material_interface = instance
        materials[index] = slot
    return materials


def run():
    props = _props()
    parent = props.textured_parent()
    results = {}
    for name in GARMENTS:
        fbx = _find(name)
        if not fbx.exists():
            unreal.log_warning(f"Clerk garment not yet authored: {name}")
            continue
        mesh = _import_skeletal(fbx)
        mesh.materials = _materials(props, parent, mesh, name, fbx)
        props.save(mesh)
        bounds = mesh.get_bounds()
        results[name] = {"mesh": mesh.get_path_name(), "slots": [str(s.material_slot_name) for s in mesh.materials],
                         "extent_cm": [round(v * 2, 2) for v in (bounds.box_extent.x, bounds.box_extent.y,
                                                                 bounds.box_extent.z)],
                         "skeleton": mesh.skeleton.get_path_name()}
    pencil = _find(PENCIL)
    if pencil.exists():
        mesh = props.import_mesh(pencil, DEST, PENCIL)
        mesh.set_editor_property("static_materials", _materials(props, parent, mesh, PENCIL, pencil))
        props.save(mesh)
        bounds = mesh.get_bounds()
        results[PENCIL] = {"mesh": mesh.get_path_name(),
                           "extent_cm": [round(v * 2, 2) for v in (bounds.box_extent.x, bounds.box_extent.y,
                                                                   bounds.box_extent.z)]}
    print(json.dumps(results, indent=2))
    return results


run()
