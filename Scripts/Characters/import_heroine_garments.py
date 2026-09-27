"""Import the Blender-fitted heroine garments and footwear onto the MetaHuman heroine.

Run in the editor (MCP run_python):

    exec(open(r'<repo>/Scripts/Characters/import_heroine_garments.py').read())

Every SKM_<Name>.fbx under Assets/Characters named in GARMENTS becomes a
skeletal mesh on metahuman_base_skel in /Game/Characters/Heroine_MH/Assembled/Heroine/Garments.
Each material slot M_<X> gets an M_PropTextured instance MI_<X> built from Textures/T_<X>_*.png
(OpenGL normals are flipped to Unreal's convention by import_props). AHomesteadCharacter loads
them by name (MetaHumanGarmentSpecs) over the homespun base layer.
"""
import importlib.util
import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir())
CHARACTERS = ROOT / "Assets" / "Characters"
# Must match MetaHumanGarmentSpecs in HomesteadCharacter.cpp.
GARMENTS = ["SKM_LinenTee", "SKM_LinenLongShirt", "SKM_WoolTrousers", "SKM_FurCoat",
            "SKM_FurBoots", "SKM_WovenSandals", "SKM_TurnShoes"]
DEST = "/Game/Characters/Heroine_MH/Assembled/Heroine/Garments"
SKELETON = "/Game/Characters/Heroine_MH/Common/Female/Medium/NormalWeight/Body/metahuman_base_skel"
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


def _sources(names=None):
    wanted = names or GARMENTS
    found = []
    for fbx in sorted(CHARACTERS.rglob("SKM_*.fbx")):
        if fbx.stem in wanted and "LOD" not in fbx.stem:
            found.append(fbx)
    missing = sorted(set(wanted) - {f.stem for f in found})
    if missing:
        unreal.log_warning("Garments not yet authored: " + ", ".join(missing))
    return found


def _import_mesh(fbx):
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


def _texture_dir(fbx):
    for candidate in (fbx.parent / "Textures", fbx.parent.parent / "Textures"):
        if candidate.exists():
            return candidate
    raise RuntimeError(f"No Textures folder beside {fbx}")


def run(names=None):
    props = _props()
    parent = props.textured_parent()
    results = {}
    for fbx in _sources(names):
        mesh = _import_mesh(fbx)
        textures = _texture_dir(fbx)
        materials = mesh.materials
        for index, slot in enumerate(materials):
            slot_name = str(slot.material_slot_name)
            if not slot_name.startswith("M_"):
                raise RuntimeError(f"{fbx.stem}: unexpected material slot {slot_name}")
            prefix = "T_" + slot_name[2:]
            path = f"{DEST}/MI_{slot_name[2:]}"
            instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
                f"MI_{slot_name[2:]}", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
            EDIT.set_material_instance_parent(instance, parent)
            for role, parameter in ROLES:
                source = textures / f"{prefix}_{role}.png"
                if not source.exists():
                    raise RuntimeError(f"{fbx.stem}: missing {source.name}")
                texture = props.import_texture(source, f"{DEST}/Textures", role)
                EDIT.set_material_instance_texture_parameter_value(instance, parameter, texture)
            EDIT.update_material_instance(instance)
            props.save(instance)
            slot.material_interface = instance
            materials[index] = slot
        mesh.materials = materials
        props.save(mesh)
        bounds = mesh.get_bounds()
        results[fbx.stem] = {"mesh": mesh.get_path_name(), "slots": [str(s.material_slot_name) for s in mesh.materials],
                             "extent_cm": [round(v, 2) for v in (bounds.box_extent.x * 2, bounds.box_extent.y * 2,
                                                                 bounds.box_extent.z * 2)],
                             "skeleton": mesh.skeleton.get_path_name()}
    print(json.dumps(results, indent=2))
    return results


run(globals().get("GARMENT_NAMES"))
