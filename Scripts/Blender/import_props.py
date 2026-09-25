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


def material_instance(spec, dest, parent):
    if "textures" in spec:
        raise NotImplementedError(
            f"{spec['name']} is textured; wire a masked two-sided foliage parent before importing "
            "(see docs\\blender-assets.md, 'Unreal import').")
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
    subsystem.remove_collisions(mesh)
    shape = unreal.ScriptCollisionShapeType.BOX if kind == "box" \
        else unreal.ScriptCollisionShapeType.NDOP26
    if subsystem.add_simple_collisions(mesh, shape) < 0:
        raise RuntimeError(f"Could not add {kind} collision to {mesh.get_name()}")


def import_prop(name, parent):
    folder = PROPS / name
    report = json.loads((folder / "report.json").read_text(encoding="utf-8"))
    dest = f"{DEST_ROOT}/{name}"
    result = {}
    for mesh_name, info in report["meshes"].items():
        source = folder / info["fbx"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != info["sha256"]:
            raise RuntimeError(f"{source} changed after the Blender build; rebuild it first")
        instances = {spec["name"]: material_instance(spec, dest, parent) for spec in info["materials"]}
        mesh = import_mesh(source, dest, mesh_name)
        slots = [str(s.get_editor_property("material_slot_name")) for s in
                 mesh.get_editor_property("static_materials")]
        if sorted(slots) != sorted(instances):
            raise RuntimeError(f"{mesh_name} material slots {slots} != report {sorted(instances)}")
        for index, slot in enumerate(slots):
            mesh.set_material(index, instances[slot])
        add_collision(mesh, report.get("collision", "box"))
        save(mesh)
        extent = mesh.get_bounds().box_extent
        size = [extent.x * 2, extent.y * 2, extent.z * 2]
        if abs(size[2] - info["size_cm"][2]) > 1.0:
            raise RuntimeError(f"{mesh_name} height {size[2]:.1f} cm != Blender {info['size_cm'][2]} cm")
        result[mesh_name] = {"asset": mesh.get_path_name(), "size_cm": [round(v, 2) for v in size],
                             "material_slots": slots, "collision": report.get("collision", "box")}
    return result


def main():
    match = re.search(r"-HomesteadProps=([\w,]+)", unreal.SystemLibrary.get_command_line())
    names = match.group(1).split(",") if match else sorted(
        p.name for p in PROPS.iterdir() if (p / "report.json").exists())
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
