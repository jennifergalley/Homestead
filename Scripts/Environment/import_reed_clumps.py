"""Import only the two original reed meshes into a new, reversible Unreal path."""
import hashlib
import json
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Environment" / "Reeds"
DEST = "/Game/SurvivalGame/Environment/Reeds"
REPORT = ROOT / "Build" / "Environment" / "reed-import.json"
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
PALETTE = {
    "M_ReedsStem": unreal.LinearColor(0.21, 0.31, 0.15, 1),
    "M_ReedsLeaf": unreal.LinearColor(0.29, 0.39, 0.19, 1),
    "M_ReedsSeed": unreal.LinearColor(0.35, 0.20, 0.09, 1),
}


def save(asset):
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not persist original reed asset " + asset.get_path_name())


def main():
    report = json.loads((SOURCE / "report.json").read_text())
    base = LIB.load_asset("/Game/SurvivalGame/Materials/M_Field")
    if not isinstance(base, unreal.Material):
        raise RuntimeError("Original field material must be installed before reed import")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")
    materials = {}
    for name, color in PALETTE.items():
        path = DEST + "/" + name
        instance = LIB.load_asset(path) if LIB.does_asset_exist(path) else TOOLS.create_asset(
            name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if LIB.does_asset_exist(path) and instance.get_editor_property("parent") != base:
            raise RuntimeError("Existing reed material has an unexpected parent: " + path)
        if not instance:
            raise RuntimeError("Could not create reed material " + name)
        EDIT.set_material_instance_parent(instance, base)
        EDIT.set_material_instance_vector_parameter_value(instance, "Tint", color)
        EDIT.set_material_instance_scalar_parameter_value(instance, "Roughness", 0.86)
        value = EDIT.get_material_instance_vector_parameter_value(instance, "Tint")
        if not value or max(abs(a - b) for a, b in zip(
                (value.r, value.g, value.b), (color.r, color.g, color.b))) > 0.001:
            raise RuntimeError("Reed material tint was not applied: " + name)
        save(instance)
        materials[name] = instance
    result = {}
    for name in ("SM_ReedClump", "SM_ReedStubble"):
        source = SOURCE / (name + ".fbx")
        if hashlib.sha256(source.read_bytes()).hexdigest() != report[name]["sha256"]:
            raise RuntimeError("Reed FBX changed after authoring: " + name)
        existing = LIB.load_asset(DEST + "/" + name) if LIB.does_asset_exist(DEST + "/" + name) else None
        if existing:
            data = existing.get_editor_property("asset_import_data")
            if not data or Path(str(data.get_first_filename())).resolve() != source.resolve():
                raise RuntimeError("Existing reed asset was not imported from the pinned source: " + name)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", existing is not None)
        task.set_editor_property("save", True)
        task.set_editor_property("factory", unreal.FbxFactory())
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        mesh_data = options.get_editor_property("static_mesh_import_data")
        mesh_data.set_editor_property("combine_meshes", True)
        mesh_data.set_editor_property("auto_generate_collision", False)
        mesh_data.set_editor_property("convert_scene", True)
        mesh_data.set_editor_property("convert_scene_unit", True)
        task.set_editor_property("options", options)
        TOOLS.import_asset_tasks([task])
        mesh = LIB.load_asset(DEST + "/" + name)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Reed static mesh import failed: " + name)
        slots = mesh.get_editor_property("static_materials")
        roles = [str(slot.get_editor_property("material_slot_name")) for slot in slots]
        wanted_roles = {"M_ReedsStem"} if name == "SM_ReedStubble" else set(PALETTE)
        if set(roles) != wanted_roles or len(roles) != len(wanted_roles):
            raise RuntimeError(f"Reed material roles differ after FBX import: {name} {roles}")
        for index, role in enumerate(roles):
            mesh.set_material(index, materials[role])
        save(mesh)
        height = mesh.get_bounds().box_extent.z * 2
        expected = report[name]["height_m"] * 100
        if abs(height - expected) > 2:
            raise RuntimeError(f"Reed scale mismatch: {name} {height} vs {expected}")
        result[name] = {"asset": mesh.get_path_name(), "height_cm": height,
                        "material_slots": roles, "source_sha256": report[name]["sha256"]}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(result, indent=2) + "\n")
    unreal.log("REED_CLUMPS_IMPORTED " + str(REPORT))


if __name__ == "__main__":
    main()
