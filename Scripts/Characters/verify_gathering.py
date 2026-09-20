"""Check the exported action, not just the authoring scene or a posed screenshot."""
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from verify_locomotion import load

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Heroine"


def main():
    rig = load(SOURCE / "SK_Heroine_LongWave.fbx")
    bind = {b.name: (b.parent.name if b.parent else None, b.matrix_local.copy()) for b in rig.data.bones}
    idle = load(SOURCE / "Locomotion" / "AN_Heroine_RelaxedIdle.fbx")
    bpy.context.scene.frame_set(1)
    idle_pose = {b.name: b.matrix.copy() for b in idle.pose.bones}
    rig = load(SOURCE / "Gathering" / "AN_Heroine_Gather.fbx")
    if set(bind) != set(rig.data.bones.keys()):
        raise RuntimeError("Gather changed shared bone names")
    bind_error = 0
    for bone in rig.data.bones:
        parent, matrix = bind[bone.name]
        if parent != (bone.parent.name if bone.parent else None):
            raise RuntimeError("Gather changed a bone parent")
        bind_error = max(bind_error, max(abs(matrix[i][j] - bone.matrix_local[i][j]) for i in range(4) for j in range(4)))
    if bind_error > 0.0001:
        raise RuntimeError("Gather changed the bind matrices")
    action = rig.animation_data.action
    start, end = action.frame_range
    if abs((end - start) / 60 - 1.6) > 0.001:
        raise RuntimeError("Unexpected gathering duration")
    samples, endpoint_error, maximum_scale_error = [], 0, 0
    for frame in range(int(start), int(end) + 1):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        if frame in (int(start), int(end)):
            endpoint_error = max(endpoint_error, max(
                abs(idle_pose[b.name][i][j] - b.matrix[i][j])
                for b in rig.pose.bones for i in range(4) for j in range(4)))
        maximum_scale_error = max(maximum_scale_error, max(abs(s - 1) for b in rig.pose.bones for s in b.scale))
        samples.append({name: (rig.matrix_world @ rig.pose.bones[name].head).copy()
                        for name in ("ball_l", "ball_r", "hand_r", "head")})
    foot_drift = max((sample[foot] - samples[0][foot]).length
                     for sample in samples for foot in ("ball_l", "ball_r"))
    hand_travel = max((sample["hand_r"] - samples[0]["hand_r"]).length for sample in samples)
    if endpoint_error > 0.001 or foot_drift > 0.001 or maximum_scale_error > 0.0001 or hand_travel < 0.20:
        raise RuntimeError(f"Gather pose contract failed: seam={endpoint_error}, feet={foot_drift}, scale={maximum_scale_error}, hand={hand_travel}")
    report = {
        "duration_seconds": 1.6, "sampled_frames": len(samples), "bones": len(bind),
        "maximum_bind_error": bind_error, "idle_endpoint_matrix_error": endpoint_error,
        "foot_drift_cm": foot_drift * 100, "maximum_bone_scale_error": maximum_scale_error,
        "right_wrist_travel_cm": hand_travel * 100,
        "right_wrist_height_cm": [min(s["hand_r"].z for s in samples) * 100, max(s["hand_r"].z for s in samples) * 100],
        "right_wrist_forward_cm": [-max(s["hand_r"].y for s in samples) * 100, -min(s["hand_r"].y for s in samples) * 100],
        "limits": "Planted on a flat authored plane; no terrain/target-aware IK, root-motion movement or reward notifies.",
    }
    path = ROOT / "Build" / "CharacterPreview" / "gathering-export-validation.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("GATHERING_EXPORT_VERIFIED", json.dumps(report))


if __name__ == "__main__":
    main()
