"""Read-only FBX round-trip checks for locomotion timing, stance and bind compatibility."""
import json
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Heroine"


def load(path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.render.fps = 60
    bpy.ops.import_scene.fbx(filepath=str(path))
    return next(o for o in bpy.data.objects if o.type == "ARMATURE")


def main():
    reference = load(SOURCE / "SK_Heroine_LongWave.fbx")
    bind = {b.name: b.matrix_local.copy() for b in reference.data.bones}
    contract = json.loads((SOURCE / "Locomotion" / "locomotion-contract.json").read_text())
    report = {}
    for name, expected in contract.items():
        rig = load(SOURCE / "Locomotion" / (name + ".fbx"))
        if set(bind) != set(rig.data.bones.keys()):
            raise ValueError("Locomotion changed the reference bone set")
        error = max(abs(bind[b.name][i][j] - b.matrix_local[i][j])
                    for b in rig.data.bones for i in range(4) for j in range(4))
        if error > 0.0001:
            raise ValueError(f"Changed bind matrices: {error}")
        action = rig.animation_data.action
        start, end = action.frame_range
        duration = (end - start) / 60
        if abs(duration - expected["duration_seconds"]) > 0.001:
            raise ValueError(f"Changed duration: {duration}")
        samples = []
        for frame in range(int(start), int(end) + 1):
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            samples.append({side: rig.matrix_world @ rig.pose.bones["ball_" + side].head
                            for side in ("l", "r")})
        loop_error = max((samples[0][s] - samples[-1][s]).length for s in ("l", "r"))
        width = abs(samples[0]["l"].x - samples[0]["r"].x) * 100
        if loop_error > 0.001 or not 12 < width < 23:
            raise ValueError(f"Bad loop/stance: {loop_error}, {width}")
        result = {"bind_matrix_max_error": error, "duration_seconds": duration,
                  "loop_error_m": loop_error, "toe_separation_cm": width}
        if "Walk" in name:
            speeds = [(samples[i + 1]["l"].y - samples[i]["l"].y) * 6000 for i in range(29)]
            deviation = max(abs(v - 120) for v in speeds)
            vertical = max(p["l"].z for p in samples[:30]) - min(p["l"].z for p in samples[:30])
            if deviation > 0.2 or vertical > 0.001:
                raise ValueError(f"FBX stance drift: speed={deviation}, z={vertical}")
            result.update(stance_speed_max_error_cm_s=deviation, stance_height_range_cm=vertical * 100)
        report[name] = result
    output = ROOT / "Build" / "CharacterPreview" / "locomotion-export-validation.json"
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
