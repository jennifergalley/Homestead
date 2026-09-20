"""Import independently authored face/body presets without replacing the preferred heroine."""
import importlib.util
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "BodyPresets"
DEST = "/Game/SurvivalGame/Characters/Heroine"
REPORT = ROOT / "Build" / "CharacterPreview" / "BodyPresets" / "unreal-import-report.json"
spec = importlib.util.spec_from_file_location("body_import_support", Path(__file__).with_name("import_unreal.py"))
support = importlib.util.module_from_spec(spec)
spec.loader.exec_module(support)
LIB = unreal.EditorAssetLibrary


def main():
    REPORT.unlink(missing_ok=True)
    manifest = json.loads((SOURCE / "body-preset-manifest.json").read_text())
    base = LIB.load_asset(DEST + "/SK_Heroine_LongWave")
    if not base:
        raise RuntimeError("The preferred heroine must already be imported.")
    skeleton = base.get_editor_property("skeleton")
    if not skeleton:
        raise RuntimeError("Preferred skeleton is missing.")
    support.save_checked(skeleton)
    component = unreal.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(base)
    expected_names, expected_facing = support.reference_bones(component)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")
    material_names = set()
    for preset in manifest["presets"]:
        material_names.update(json.loads((SOURCE / preset / "materials.json").read_text()))
    materials = {}
    for name in material_names:
        path = DEST + "/Materials/" + name
        material = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
        if not material:
            raise RuntimeError("Missing shared material: " + name)
        materials[name] = material
    result = {"engine": unreal.SystemLibrary.get_engine_version(), "presets": {}, "shared_skeleton": skeleton.get_path_name()}
    for preset, definition in manifest["presets"].items():
        result["presets"][preset] = {}
        for name, entry in definition["exports"].items():
            path = DEST + "/" + name
            mesh = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
            if mesh is None:
                imported = support.import_task(SOURCE / preset / entry["fbx"], DEST, name, support.fbx_options(skeleton))
                mesh = next((LIB.load_asset(p) for p in imported if isinstance(LIB.load_asset(p), unreal.SkeletalMesh)), None)
            if not mesh or mesh.get_editor_property("skeleton") != skeleton:
                raise RuntimeError("Preset skeleton mismatch: " + name)
            # Finish pending load work, then explicitly write modified struct
            # values back into the array; editing iteration copies is insufficient.
            unreal.SystemLibrary.execute_console_command(world, "Editor.AsyncSkinnedAssetCompilationFinishAll")
            slots = mesh.get_editor_property("materials")
            for index, slot in enumerate(slots):
                slot_name = str(slot.get_editor_property("material_slot_name"))
                material = materials.get(slot_name)
                if not material:
                    raise RuntimeError(f"Missing shared material for {name}: {slot_name}")
                slot.set_editor_property("material_interface", material)
                slots[index] = slot
            mesh.set_editor_property("materials", slots)
            unreal.SystemLibrary.execute_console_command(world, "Editor.AsyncSkinnedAssetCompilationFinishAll")
            support.save_checked(mesh)
            for slot in mesh.get_editor_property("materials"):
                if slot.get_editor_property("material_interface") is None:
                    raise RuntimeError("Material assignment did not survive mesh post-processing: " + name
                                       + " " + str(slot.get_editor_property("material_slot_name")))
            check = unreal.SkeletalMeshComponent()
            check.set_skeletal_mesh_asset(mesh)
            names, facing = support.reference_bones(check)
            if names != expected_names:
                raise RuntimeError("Preset bone hierarchy differs: " + name)
            for bone, point in facing["reference_positions_cm"].items():
                if max(abs(a - b) for a, b in zip(point, expected_facing["reference_positions_cm"][bone])) > 0.01:
                    raise RuntimeError("Preset reference pose differs: " + name + " " + bone)
            height = mesh.get_bounds().box_extent.z * 2
            if not 155 < height < 175:
                raise RuntimeError(f"Preset scale invalid: {name} {height}")
            result["presets"][preset][name] = {"asset": mesh.get_path_name(), "height_cm": height, "bone_count": len(names)}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(result, indent=2), encoding="utf-8")
    unreal.log("BODY_PRESETS_VERIFIED " + str(REPORT))


if __name__ == "__main__":
    main()
