"""Import authored motion clips only; never reimport a mesh or skeleton."""
import hashlib
import json
import sys
from pathlib import Path

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_unreal import DEST, LIB, fbx_options, import_task, save_checked

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Heroine" / "Locomotion"
RECEIPT = ROOT / "Build" / "CharacterPreview" / "locomotion-import.json"


def main():
    command = unreal.SystemLibrary.get_command_line()
    group = "Clearing" if "-ClearingAnimations" in command else "Watering" if "-WateringAnimations" in command else "Gathering" if "-GatheringAnimations" in command else "Locomotion"
    source = SOURCE.parent / group
    receipt = RECEIPT.with_name(group.lower() + "-import.json")
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    contract = json.loads((source / (group.lower() + "-contract.json")).read_text())
    mesh = LIB.load_asset(DEST + "/SK_Heroine_LongWave")
    if not mesh:
        raise RuntimeError("Import the existing heroine first")
    skeleton = mesh.get_editor_property("skeleton")
    prior = json.loads(receipt.read_text()) if receipt.exists() else {}
    report = {}
    verify_only = "-LocomotionVerifyOnly" in unreal.SystemLibrary.get_command_line()
    for name, expected in contract.items():
        path = DEST + "/Animations/" + name
        digest = hashlib.sha256((source / (name + ".fbx")).read_bytes()).hexdigest()
        clip = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
        if not verify_only and (clip is None or prior.get(name, {}).get("sha256") != digest):
            if clip:
                data = clip.get_editor_property("asset_import_data")
                data.set_editor_property("convert_scene", True)
                data.set_editor_property("convert_scene_unit", True)
                data.set_editor_property("import_uniform_scale", 1.0)
            import_task(source / (name + ".fbx"), DEST + "/Animations", name,
                        fbx_options(skeleton, animation=True))
            clip = LIB.load_asset(path)
        if not clip or clip.get_editor_property("skeleton") != skeleton:
            raise RuntimeError("Missing or incompatible locomotion: " + name)
        duration = clip.get_play_length()
        if abs(duration - expected["duration_seconds"]) > 0.001:
            raise RuntimeError(f"Wrong duration: {name} {duration}")
        root_motion = clip.get_editor_property("enable_root_motion")
        notify_count = len(unreal.AnimationLibrary.get_animation_notify_events(clip))
        if group != "Locomotion" and (root_motion or notify_count):
            raise RuntimeError(group + " must not extract root motion or carry animation notifies")
        if not verify_only:
            save_checked(clip)
        report[name] = {"sha256": digest, "asset": clip.get_path_name(),
                        "skeleton": skeleton.get_path_name(), "duration_seconds": duration,
                        "root_motion_enabled": root_motion, "notify_count": notify_count}
    receipt.parent.mkdir(parents=True, exist_ok=True)
    target = receipt.with_name(group.lower() + "-reload.json") if verify_only else receipt
    target.write_text(json.dumps(report, indent=2) + "\n")
    unreal.log(group.upper() + "_VERIFIED " + str(target))


if __name__ == "__main__":
    main()
