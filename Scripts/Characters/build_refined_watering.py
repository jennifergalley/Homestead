"""Author a planted-foot lift, aim, pour and recover for plot-directed watering."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import prepare_reference, relaxed_upper_body, solve_leg, aim_bone
from build_gathering import smooth, reach_arm
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "RefinedWatering"
NAME = "AN_Heroine_WaterRefined"
FPS = 60
DURATION = 2.0


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
        lift = smooth(t / 0.35) * (1 - smooth((t - 1.55) / 0.45))
        aim = smooth((t - 0.24) / 0.34)
        pour = smooth((t - 0.55) / 0.25) * (1 - smooth((t - 1.30) / 0.25))
        tilt = math.radians(44) * pour
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        root.location = root.bone.matrix_local.to_3x3().inverted() @ Vector((0.004, 0, -0.015))
        relaxed_upper_body(rig, 0, False)
        for name, angle in (("spine_02", 0.10), ("spine_03", 0.06)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle * lift) @ rest
        bpy.context.view_layer.update()
        if lift > 0:
            carry = Vector((-0.22, -0.30, 1.04))
            directed = Vector((-0.24, -0.34, 0.95))
            reach_arm(rig, lift, 0, rig.pose.bones["hand_r"].head.lerp(carry.lerp(directed, aim), lift))
            hand = rig.pose.bones["hand_r"]
            base = hand.rotation_quaternion.copy()
            aim_bone(rig, hand.name, hand.head.copy(), Vector((0, 0, -1)))
            axis = Vector((0, 0, -1))
            rotation = Quaternion((1, 0, 0), tilt) @ hand.matrix.to_quaternion()
            hand.matrix = Matrix.LocRotScale(hand.head.copy(), rotation, Vector((1, 1, 1)))
            bpy.context.view_layer.update()
            hand.rotation_quaternion = base.slerp(hand.rotation_quaternion, lift)
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
        "phases": {"lift": [0.0, 0.35], "aim": [0.24, 0.58],
                   "pour": [0.55, 1.30], "recover": [1.30, 2.0]},
        "tool": "Existing procedural watering can; runtime target yaw and 44-degree authored pour.",
        "authority": "Presentation only after the existing successful Water transaction.",
        "root_motion": False, "notifies": 0,
        "samples": samples,
    }}
    (OUT / "refined-watering-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("REFINED_WATERING_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
