"""Verify the authored tilling FBX against the retained heroine contract."""
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from verify_locomotion import load

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Heroine"


def main():
    reference = load(SOURCE / "SK_Heroine_LongWave.fbx")
    bind = {b.name: (b.parent.name if b.parent else None, b.matrix_local.copy())
            for b in reference.data.bones}
    idle = load(SOURCE / "Locomotion" / "AN_Heroine_RelaxedIdle.fbx")
    bpy.context.scene.frame_set(1)
    idle_pose = {b.name: b.matrix.copy() for b in idle.pose.bones}
    rig = load(SOURCE / "Tilling" / "AN_Heroine_Till.fbx")
    if set(bind) != set(rig.data.bones.keys()):
        raise RuntimeError("Till changed shared bone names")
    bind_error = 0.0
    for bone in rig.data.bones:
        parent, matrix = bind[bone.name]
        if parent != (bone.parent.name if bone.parent else None):
            raise RuntimeError("Till changed a bone parent")
        bind_error = max(bind_error, max(abs(matrix[i][j] - bone.matrix_local[i][j])
                                         for i in range(4) for j in range(4)))
    action = rig.animation_data.action
    start, end = action.frame_range
    if abs((end - start) / 60 - 1.7) > 0.001:
        raise RuntimeError("Unexpected till duration")
    endpoint_error = scale_error = root_travel = foot_drift = hand_travel = 0.0
    first = {}
    for frame in range(int(start), int(end) + 1):
        bpy.context.scene.frame_set(frame)
        bpy.context.view_layer.update()
        if frame in (int(start), int(end)):
            endpoint_error = max(endpoint_error, max(
                abs(idle_pose[b.name][i][j] - b.matrix[i][j])
                for b in rig.pose.bones for i in range(4) for j in range(4)))
        scale_error = max(scale_error, max(abs(s - 1) for b in rig.pose.bones for s in b.scale))
        points = {n: (rig.matrix_world @ rig.pose.bones[n].head).copy()
                  for n in ("Root", "ball_l", "ball_r", "hand_r")}
        if not first:
            first = points
        root_travel = max(root_travel, (points["Root"] - first["Root"]).length)
        foot_drift = max([foot_drift] + [(points[n] - first[n]).length for n in ("ball_l", "ball_r")])
        hand_travel = max(hand_travel, (points["hand_r"] - first["hand_r"]).length)
    if bind_error > 0.0001 or endpoint_error > 0.001 or scale_error > 0.0001:
        raise RuntimeError("Till bind/seam/scale contract failed")
    if root_travel > 0.0001 or foot_drift > 0.001 or hand_travel < 0.4:
        raise RuntimeError("Till planted-foot/action contract failed")
    report = {
        "duration_seconds": 1.7, "bones": len(bind), "maximum_bind_error": bind_error,
        "idle_endpoint_matrix_error": endpoint_error, "maximum_bone_scale_error": scale_error,
        "root_travel_cm": root_travel * 100, "foot_drift_cm": foot_drift * 100,
        "right_wrist_travel_cm": hand_travel * 100, "notifies": 0,
        "limits": "Analytical flat-soil verification; runtime cell direction is separate.",
    }
    out = ROOT / "Build" / "CharacterPreview" / "till-export-validation.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2) + "\n")
    print("TILL_EXPORT_VERIFIED", json.dumps(report))


if __name__ == "__main__":
    main()
