"""Author a planted-foot anticipation/swing/impact/recover chopping clip."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import prepare_reference, relaxed_upper_body, solve_leg
from build_gathering import smooth, reach_arm
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "Chopping"
NAME = "AN_Heroine_Chop"
FPS = 60
DURATION = 1.8


def interval(t, begin, peak, end):
    return smooth((t - begin) / (peak - begin)) * (1 - smooth((t - peak) / (end - peak)))


def main():
    OUT.mkdir(exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(NAME)
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, round(DURATION * FPS) + 1
    samples = []
    for frame in range(scene.frame_start, scene.frame_end + 1):
        scene.frame_set(frame)
        t = (frame - 1) / FPS
        anticipation = interval(t, 0.0, 0.34, 0.72)
        strike = smooth((t - 0.34) / 0.46)
        recover = smooth((t - 1.12) / 0.68)
        action = smooth(t / 0.16) * (1 - recover)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        root.location = root.bone.matrix_local.to_3x3().inverted() @ Vector((0.004, 0, -0.015))
        relaxed_upper_body(rig, 0, False)
        for name, angle in (("spine_02", -0.10 * anticipation + 0.14 * strike),
                            ("spine_03", -0.08 * anticipation + 0.11 * strike)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle * action) @ rest
        bpy.context.view_layer.update()
        if action > 0:
            windup = Vector((-0.30, 0.03, 1.46))
            impact = Vector((-0.28, -0.42, 1.08))
            follow = Vector((-0.27, -0.39, 1.03))
            target = windup.lerp(impact, strike)
            if t > 0.80:
                target = impact.lerp(follow, smooth((t - 0.80) / 0.28))
            target = rig.pose.bones["hand_r"].head.lerp(target, action)
            reach_arm(rig, action, 0, target)
        for side, sign in (("l", 1), ("r", -1)):
            foot = rig.data.bones["foot_" + side]
            solve_leg(rig, side, Vector((sign * 0.08, foot.head_local.y + 0.012 * sign, foot.head_local.z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        bpy.context.view_layer.update()
        samples.append({
            "frame": frame,
            "hand": list((rig.matrix_world @ rig.pose.bones["hand_r"].head)[:]),
            "left_toe": list((rig.matrix_world @ rig.pose.bones["ball_l"].head)[:]),
            "right_toe": list((rig.matrix_world @ rig.pose.bones["ball_r"].head)[:]),
        })
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / f"{NAME}.fbx", rig, [mesh], animation=True)
    contract = {
        "name": NAME, "duration_seconds": DURATION, "fps": FPS,
        "phases": {"anticipation": [0.0, 0.34], "swing": [0.34, 0.80],
                   "impact": 0.80, "follow_through": [0.80, 1.12], "recover": [1.12, 1.80]},
        "authority": "Presentation only after existing successful Clear/Harvest transaction.",
        "root_motion": False, "notifies": 0, "target": "Bounded runtime facing only; no IK/reach mutation.",
        "samples": samples,
    }
    (OUT / "chop-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("CHOP_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
