"""Author a planted-foot downward till and recovery clip."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import prepare_reference, relaxed_upper_body, solve_leg
from build_gathering import smooth, reach_arm
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "Tilling"
NAME = "AN_Heroine_Till"
FPS = 60
DURATION = 1.7


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
        lift = smooth(t / 0.34)
        drive = smooth((t - 0.34) / 0.42)
        recover = smooth((t - 1.02) / 0.68)
        action = smooth(t / 0.14) * (1 - recover)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        root.location = root.bone.matrix_local.to_3x3().inverted() @ Vector((0.004, 0, -0.015))
        relaxed_upper_body(rig, 0, False)
        bend = (-0.08 * lift + 0.28 * drive) * action
        for name, amount in (("spine_02", bend), ("spine_03", bend * 0.8)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), amount) @ rest
        bpy.context.view_layer.update()
        if action > 0:
            raised = Vector((-0.25, -0.05, 1.40))
            soil = Vector((-0.28, -0.36, 0.90))
            settle = Vector((-0.28, -0.33, 0.94))
            target = raised.lerp(soil, drive)
            if t > 0.82:
                target = soil.lerp(settle, smooth((t - 0.82) / 0.20))
            reach_arm(rig, action, 0, rig.pose.bones["hand_r"].head.lerp(target, action))
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
    contract = {NAME: {
        "name": NAME, "duration_seconds": DURATION, "fps": FPS,
        "phases": {"lift": [0.0, 0.34], "drive": [0.34, 0.76],
                   "soil_contact": 0.76, "settle": [0.76, 1.02], "recover": [1.02, 1.70]},
        "authority": "Presentation only after an existing successful Till transaction.",
        "root_motion": False, "notifies": 0,
        "target": "Selected plot-cell center direction; analytical flat-soil height.",
        "samples": samples,
    }}
    (OUT / "till-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("TILL_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
