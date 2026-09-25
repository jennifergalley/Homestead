"""Import the legally reusable CMU walk retarget into a fresh animation-only trial."""
import hashlib
import importlib.util
import json
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
HEAD_LEVEL_TRIAL = "-CMUWalkHeadLevelTrial" in unreal.SystemLibrary.get_command_line()
ORIGINAL = ROOT / "Assets" / "Characters" / "HeroineTrials" / "CMUWalk01"
SOURCE = ORIGINAL.with_name("CMUWalk02") if HEAD_LEVEL_TRIAL else ORIGINAL
DEST = "/Game/Trials/HeroineCMUWalk_20260924_04/Animations" if HEAD_LEVEL_TRIAL else "/Game/Trials/HeroineCMUWalk_20260924_03/Animations"
REPORT = ROOT / "Build" / "CharacterPreview" / "CMUWalk01" / ("import-04.json" if HEAD_LEVEL_TRIAL else "import-03.json")
VERIFY = "-CMUWalkVerifyOnly" in unreal.SystemLibrary.get_command_line()
spec = importlib.util.spec_from_file_location(
    "cmu_walk_import_support", Path(__file__).with_name("import_unreal.py"))
support = importlib.util.module_from_spec(spec)
spec.loader.exec_module(support)
lib = unreal.EditorAssetLibrary


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    receipt = json.loads((ORIGINAL / "source-license.json").read_text())
    for name, expected in receipt["sources_sha256"].items():
        actual = hashlib.sha256((ORIGINAL / "Source" / name).read_bytes()).hexdigest()
        require(actual.upper() == expected.upper(), "Official CMU walk source changed: " + name)
    authored = json.loads((SOURCE / "retarget-report.json").read_text())
    base = lib.load_asset(
        "/Game/SurvivalGame/Characters/Heroine/SK_Heroine_LongWave")
    require(isinstance(base, unreal.SkeletalMesh), "The admitted heroine rig is missing")
    skeleton = base.get_editor_property("skeleton")
    require(isinstance(skeleton, unreal.Skeleton), "The admitted animation skeleton is missing")
    require(REPORT.is_file() if VERIFY else not REPORT.exists(),
            "Choose a fresh trial import or a separately recorded reload verification")
    unreal.SystemLibrary.execute_console_command(
        None, "Interchange.FeatureFlags.Import.FBX 0")
    results = {}
    for name, row in authored.items():
        path = DEST + "/" + name
        fbx = SOURCE / (name + ".fbx")
        require(hashlib.sha256(fbx.read_bytes()).hexdigest() == row["fbx_sha256"],
                "CMU retarget differs from its authoring receipt: " + name)
        if not VERIFY:
            require(not lib.does_asset_exist(path),
                    "CMU walk asset already exists; never overwrite an isolated trial")
            support.import_task(fbx, DEST, name,
                                support.fbx_options(skeleton, animation=True))
        clip = lib.load_asset(path)
        require(isinstance(clip, unreal.AnimSequence)
                and clip.get_editor_property("skeleton") == skeleton,
                "Imported walk cannot animate the admitted heroine: " + name)
        require(abs(clip.get_play_length() - row["duration_seconds"]) < .001
                and not clip.get_editor_property("enable_root_motion")
                and not unreal.AnimationLibrary.get_animation_notify_events(clip),
                "CMU walk duration, in-place or no-notifies contract failed: " + name)
        if not VERIFY:
            support.save_checked(clip)
        results[name] = {
            "asset": clip.get_path_name(),
            "fbx_sha256": row["fbx_sha256"],
            "source": row["source_frames"],
            "duration_seconds": clip.get_play_length(),
            "skeleton": skeleton.get_path_name(),
        }
    if VERIFY:
        require(json.loads(REPORT.read_text()) == results,
                "The CMU walk reload differs from its original import receipt")
    else:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(results, indent=2) + "\n")
    unreal.log("CMU_WALK01_VERIFIED " + str(REPORT))


if __name__ == "__main__":
    main()
