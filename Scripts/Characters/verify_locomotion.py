"""Read-only FBX round-trip checks for locomotion timing, stance and bind compatibility."""
import json
import sys
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
    sprint = "--sprint" in sys.argv
    knife = "--knife-cut" in sys.argv
    walk_revision = "--walk-revision" in sys.argv
    idle_revision = "--idle-revision" in sys.argv
    if sum((sprint, knife, walk_revision, idle_revision)) > 1:
        raise ValueError("Verify one authored motion set at a time")
    group = "IdleRevision" if idle_revision else "WalkRevision" if walk_revision else "Sprinting" if sprint else "KnifeCut" if knife else "Locomotion"
    contract = json.loads((SOURCE / group /
                           ("idle-contract.json" if idle_revision else
                            "walk-contract.json" if walk_revision else
                            "sprint-contract.json" if sprint else
                            "knife-cut-contract.json" if knife else "locomotion-contract.json")).read_text())
    report = {}
    for name, expected in contract.items():
        rig = load(SOURCE / group / (name + ".fbx"))
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
        hands = []
        heads = []
        for frame in range(int(start), int(end) + 1):
            bpy.context.scene.frame_set(frame)
            bpy.context.view_layer.update()
            samples.append({side: rig.matrix_world @ rig.pose.bones["ball_" + side].head
                            for side in ("l", "r")})
            if knife or idle_revision:
                hands.append(rig.matrix_world @ rig.pose.bones["hand_r"].head)
            if idle_revision:
                heads.append(rig.matrix_world @ rig.pose.bones["head"].head)
        loop_error = max((samples[0][s] - samples[-1][s]).length for s in ("l", "r"))
        width = abs(samples[0]["l"].x - samples[0]["r"].x) * 100
        if loop_error > 0.001 or not 12 < width < 23:
            raise ValueError(f"Bad loop/stance: {loop_error}, {width}")
        result = {"bind_matrix_max_error": error, "duration_seconds": duration,
                  "loop_error_m": loop_error, "toe_separation_cm": width}
        if knife:
            wrist_travel = max((hand - hands[0]).length for hand in hands)
            toe_drift = max((pair[side] - samples[0][side]).length
                            for pair in samples for side in ("l", "r"))
            if wrist_travel < .15 or toe_drift > .001:
                raise ValueError(f"Knife wrist/stance drift: {wrist_travel}, {toe_drift}")
            result.update(wrist_travel_m=wrist_travel, toe_drift_m=toe_drift)
        if idle_revision:
            toe_drift = max((sample[side] - samples[0][side]).length
                            for sample in samples for side in ("l", "r"))
            head_travel = max((head - heads[0]).length for head in heads)
            hand_travel = max((hand - hands[0]).length for hand in hands)
            if toe_drift > .001 or not .006 < head_travel < .05 \
                    or not .007 < hand_travel < .06 \
                    or abs(head_travel - expected["head_travel_m"]) > .002:
                raise ValueError("Living idle lost planted feet or human motion")
            result.update(toe_drift_m=toe_drift, head_travel_m=head_travel,
                          hand_travel_m=hand_travel)
        if "Walk" in name or sprint:
            stance_frames = 15 if sprint else 25 if walk_revision else 30
            speed = expected["stance_speed_cm_s"]
            speeds = [(samples[i + 1]["l"].y - samples[i]["l"].y) * 6000
                      for i in range(stance_frames - 1)]
            deviation = max(abs(v - speed) for v in speeds)
            vertical = max(p["l"].z for p in samples[:stance_frames]) - min(p["l"].z for p in samples[:stance_frames])
            if deviation > 0.2 or vertical > 0.001:
                raise ValueError(f"FBX stance drift: speed={deviation}, z={vertical}")
            result.update(stance_speed_max_error_cm_s=deviation, stance_height_range_cm=vertical * 100)
        report[name] = result
    output = ROOT / "Build" / "CharacterPreview" / (
        "idle-revision-export-validation.json" if idle_revision else
        "walk-revision-export-validation.json" if walk_revision else
        "sprint-export-validation.json" if sprint else
        "knife-cut-export-validation.json" if knife else "locomotion-export-validation.json")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
