"""Import optional cosmetic variants onto the already verified heroine skeleton."""
import importlib.util
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Variants"
DEST = "/Game/SurvivalGame/Characters/Heroine"
REPORT = ROOT / "Build" / "CharacterPreview" / "Variants" / "unreal-import-report.json"
spec = importlib.util.spec_from_file_location("heroine_import_support", Path(__file__).with_name("import_unreal.py"))
support = importlib.util.module_from_spec(spec)
spec.loader.exec_module(support)
LIB = unreal.EditorAssetLibrary


def main():
    REPORT.unlink(missing_ok=True)
    manifest = json.loads((SOURCE / "variant-manifest.json").read_text())
    definitions = json.loads((SOURCE / "materials.json").read_text())
    base = LIB.load_asset(DEST + "/SK_Heroine_LongWave")
    if not base:
        raise RuntimeError("Import the original heroine before cosmetic variants.")
    skeleton = base.get_editor_property("skeleton")
    if not skeleton:
        raise RuntimeError("The base heroine's saved skeleton is missing.")
    support.save_checked(skeleton)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(world, "Interchange.FeatureFlags.Import.FBX 0")
    support.SOURCE = SOURCE
    support.ROLES.update({
        "M_Heroine_Hair_ponytail01": "hair",
        "M_Heroine_ApronLinen": "tunic",
        "M_Heroine_ApronTrim": "tunic_trim",
    })
    new_names = {"M_Heroine_Hair_ponytail01", "M_Heroine_ApronLinen", "M_Heroine_ApronTrim"}
    materials = {}
    for name, record in definitions.items():
        path = DEST + "/Materials/" + name
        if name in new_names:
            materials[name] = support.create_material(name, record)
        elif LIB.does_asset_exist(path):
            materials[name] = LIB.load_asset(path)
        else:
            raise RuntimeError("Shared base material is absent: " + name)
    base_component = unreal.SkeletalMeshComponent()
    base_component.set_skeletal_mesh_asset(base)
    base_names, base_facing = support.reference_bones(base_component)
    result = {"engine": unreal.SystemLibrary.get_engine_version(), "variants": {}, "shared_skeleton": skeleton.get_path_name()}
    for name, entry in manifest["variants"].items():
        path = DEST + "/" + name
        mesh = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
        if mesh is None:
            imported = support.import_task(SOURCE / entry["fbx"], DEST, name, support.fbx_options(skeleton))
            mesh = next((LIB.load_asset(p) for p in imported if isinstance(LIB.load_asset(p), unreal.SkeletalMesh)), None)
        if not mesh or mesh.get_editor_property("skeleton") != skeleton:
            raise RuntimeError("Variant did not retain the shared skeleton: " + name)
        slots = mesh.get_editor_property("materials")
        for index, slot in enumerate(slots):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            if slot_name not in materials:
                raise RuntimeError(f"Unrecognized variant material {slot_name} on {name}")
            slot.set_editor_property("material_interface", materials[slot_name])
            slots[index] = slot
        mesh.set_editor_property("materials", slots)
        support.save_checked(mesh)
        component = unreal.SkeletalMeshComponent()
        component.set_skeletal_mesh_asset(mesh)
        names, facing = support.reference_bones(component)
        if names != base_names:
            raise RuntimeError("Variant bone names/order differ from the original: " + name)
        for bone, point in facing["reference_positions_cm"].items():
            if max(abs(a - b) for a, b in zip(point, base_facing["reference_positions_cm"][bone])) > 0.01:
                raise RuntimeError("Variant reference transform changed: " + name + " " + bone)
        height = mesh.get_bounds().box_extent.z * 2
        if not 155 < height < 175:
            raise RuntimeError(f"Incorrect variant scale: {name} {height}")
        result["variants"][name] = {
            "asset": mesh.get_path_name(), "height_cm": height, "bone_count": len(names),
            "material_slots": [str(slot.get_editor_property("material_slot_name")) for slot in slots],
        }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(result, indent=2), encoding="utf-8")
    unreal.log("HEROINE_VARIANTS_VERIFIED " + str(REPORT))


if __name__ == "__main__":
    main()
