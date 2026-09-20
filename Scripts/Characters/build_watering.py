"""Author one planted-foot lift/tilt/recover for the original wooden watering can."""
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
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "Watering"
NAME = "AN_Heroine_Water"


def main():
    OUT.mkdir(exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(NAME)
    scene = bpy.context.scene
    scene.render.fps = 60
    scene.frame_start, scene.frame_end = 1, 133
    for frame in range(1, 134):
        scene.frame_set(frame)
        t = (frame - 1) / 60
        lift = smooth(t / 0.4) * (1 - smooth((t - 1.5) / 0.7))
        tilt = math.radians(32) * smooth((t - 0.45) / 0.3) * (1 - smooth((t - 1.25) / 0.25))
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        root.location = root.bone.matrix_local.to_3x3().inverted() @ Vector((0.004, 0, -0.015 - 0.04 * lift))
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, 0, False)
        for name, angle in (("spine_02", 0.08), ("spine_03", 0.04)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle * lift) @ rest
        bpy.context.view_layer.update()
        if lift > 0:
            reach_arm(rig, lift, 0, Vector((-0.24, -0.40, 1.0)))
            hand = rig.pose.bones["hand_r"]
            base = hand.rotation_quaternion.copy()
            aim_bone(rig, hand.name, hand.head.copy(), Vector((0, 0, -1)))
            width = (rig.pose.bones["index_01_r"].head - rig.pose.bones["pinky_01_r"].head).normalized()
            axis = Vector((0, 0, -1))
            width = (width - axis * width.dot(axis)).normalized()
            desired = Vector((1, 0, 0))
            roll = math.atan2(axis.dot(width.cross(desired)), width.dot(desired))
            rotation = Quaternion((1, 0, 0), tilt) @ Quaternion(axis, roll) @ hand.matrix.to_quaternion()
            hand.matrix = Matrix.LocRotScale(hand.head.copy(), rotation, Vector((1, 1, 1)))
            bpy.context.view_layer.update()
            hand.rotation_quaternion = base.slerp(hand.rotation_quaternion, lift)
        for finger in ("index", "middle", "ring", "pinky"):
            for joint in range(1, 4):
                bone = rig.pose.bones[f"{finger}_{joint:02d}_r"]
                rest = bone.bone.matrix_local.to_quaternion()
                bone.rotation_quaternion @= rest.inverted() @ Quaternion((-0.9, 0, 0.4), 0.65 * lift) @ rest
        for side, sign in (("l", 1), ("r", -1)):
            foot = rig.data.bones["foot_" + side]
            solve_leg(rig, side, Vector((sign * 0.08, foot.head_local.y + 0.012 * sign, foot.head_local.z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / (NAME + ".fbx"), rig, [mesh], animation=True)
    contract = {NAME: {
        "duration_seconds": 2.2, "fps": 60, "frames": 133,
        "rig": "Unchanged shared heroine bind; animation-only import",
        "tool": "Original procedural wood/fiber can; hand_r grip, 32-degree authored pour tilt; centimeter geometry with absolute world scale 1",
        "reward": "None; contextual presentation after the existing successful plot-water transaction",
        "contact": "Generic forward watering, not target/terrain IK or precise ground contact",
    }}
    (OUT / "watering-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("WATERING_AUTHORED", OUT / (NAME + ".fbx"))


if __name__ == "__main__":
    main()
