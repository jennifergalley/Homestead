"""One animation-only, planted-foot reach/pick/recover on the existing adult rig."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import prepare_reference, relaxed_upper_body, solve_leg, aim_bone
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "Gathering"
NAME = "AN_Heroine_Gather"
FPS = 60
DURATION = 1.6


def smooth(t):
    t = min(1, max(0, t))
    return t * t * (3 - 2 * t)


def reach_amount(t):
    return smooth(t / 0.5) if t < 0.5 else 1 - smooth((t - 0.85) / 0.75)


def reach_arm(rig, amount, pick, target_position=None):
    upper = rig.pose.bones["upperarm_r"]
    lower = rig.pose.bones["lowerarm_r"]
    base_upper, base_lower = upper.rotation_quaternion.copy(), lower.rotation_quaternion.copy()
    shoulder = upper.head.copy()
    destination = target_position if target_position is not None else Vector((-0.16, -0.32 + 0.035 * pick, 0.81 + 0.025 * pick))
    target = rig.pose.bones["hand_r"].head.lerp(destination, amount)
    delta = target - shoulder
    distance = delta.length
    a, b = upper.bone.length, lower.bone.length
    if distance >= a + b or distance <= abs(a - b):
        raise ValueError(f"Unreachable wrist without stretching: {distance}, arm={a + b}, amount={amount}, shoulder={shoulder}, target={target}")
    along = delta.normalized()
    hint = Vector((-0.3, 1, 0))
    bend = (hint - along * hint.dot(along)).normalized()
    adjacent = (a * a - b * b + distance * distance) / (2 * distance)
    elbow = shoulder + adjacent * along + math.sqrt(max(0, a * a - adjacent * adjacent)) * bend
    aim_bone(rig, upper.name, shoulder, elbow - shoulder)
    aim_bone(rig, lower.name, elbow, target - elbow)
    upper.rotation_quaternion = base_upper.slerp(upper.rotation_quaternion, amount)
    lower.rotation_quaternion = base_lower.slerp(lower.rotation_quaternion, amount)
    bpy.context.view_layer.update()


def main():
    OUT.mkdir(exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    action = bpy.data.actions.new(NAME)
    rig.animation_data_create()
    rig.animation_data.action = action
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, round(DURATION * FPS) + 1
    samples = []
    for frame in range(scene.frame_start, scene.frame_end + 1):
        scene.frame_set(frame)
        t = (frame - 1) / FPS
        amount = reach_amount(t)
        pick = smooth((t - 0.5) / 0.2) * (1 - smooth((t - 0.75) / 0.2))
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        offset = Vector((0.004, 0.025 * amount, -0.015 - 0.12 * amount))
        root.location = root.bone.matrix_local.to_3x3().inverted() @ offset
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, 0, False)
        for name, angle in (("spine_02", 0.20), ("spine_03", 0.16), ("head", 0.08)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle * amount) @ rest
        bpy.context.view_layer.update()
        if amount > 0:
            reach_arm(rig, amount, pick)
        for finger in ("index", "middle", "ring", "pinky"):
            for joint in range(1, 4):
                bone = rig.pose.bones[f"{finger}_{joint:02d}_r"]
                rest = bone.bone.matrix_local.to_quaternion()
                bone.rotation_quaternion @= rest.inverted() @ Quaternion((-0.9, 0, 0.4), 0.18 * pick) @ rest
        for side, sign in (("l", 1), ("r", -1)):
            foot = rig.data.bones["foot_" + side]
            solve_leg(rig, side, Vector((sign * 0.08, foot.head_local.y + 0.012 * sign, foot.head_local.z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        samples.append({name: list(rig.pose.bones[name].head) for name in ("hand_r", "ball_l", "ball_r", "head")})
    action = rig.animation_data.action
    rig.animation_data.action_slot = action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / (NAME + ".fbx"), rig, [mesh], animation=True)
    result = {NAME: {
        "duration_seconds": DURATION, "fps": FPS, "frames": scene.frame_end,
        "rig": "Unchanged shared heroine bind; animation-only import",
        "contact": "Generic stationary forward picking gesture, not target-aware IK; low ground items and distant/behind targets may not contact",
        "reward": "None; presentation only after the controller's successful non-sapling wild harvest",
    }}
    (OUT / "gathering-contract.json").write_text(json.dumps(result, indent=2) + "\n")
    report = ROOT / "Build" / "CharacterPreview" / "gathering-authoring.json"
    report.write_text(json.dumps({"contract": result, "samples": samples}, indent=2) + "\n")
    print("GATHERING_AUTHORED", OUT / (NAME + ".fbx"))


if __name__ == "__main__":
    main()
