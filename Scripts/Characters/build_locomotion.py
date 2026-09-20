"""Author animation-only clips on the existing bind rig, without rebuilding wardrobe meshes."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from export_heroine import export_fbx, normalize, prepare_body, reset, active

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Assets" / "Characters" / "Heroine"
OUT = SOURCE / "Locomotion"
FPS = 60
STRIDE = 1.20


def aim_bone(rig, name, head, direction):
    bone = rig.pose.bones[name]
    rest = bone.bone
    rotation = (rest.tail_local - rest.head_local).rotation_difference(direction)
    bone.matrix = Matrix.LocRotScale(head, rotation @ rest.matrix_local.to_quaternion(), Vector((1, 1, 1)))
    bpy.context.view_layer.update()


def solve_leg(rig, side, ankle):
    thigh = rig.pose.bones["thigh_" + side]
    calf = rig.pose.bones["calf_" + side]
    hip = thigh.head.copy()
    delta = ankle - hip
    distance = delta.length
    upper, lower = thigh.bone.length, calf.bone.length
    if distance >= upper + lower:
        raise ValueError(f"Unreachable {side} ankle: {distance} >= {upper + lower}")
    along = delta.normalized()
    forward = Vector((0, -1, 0))
    bend = (forward - along * forward.dot(along)).normalized()
    a = (upper * upper - lower * lower + distance * distance) / (2 * distance)
    knee = hip + along * a + bend * math.sqrt(max(0, upper * upper - a * a))
    aim_bone(rig, "thigh_" + side, hip, knee - hip)
    aim_bone(rig, "calf_" + side, knee, ankle - knee)
    foot = rig.pose.bones["foot_" + side]
    foot.matrix = Matrix.LocRotScale(ankle, foot.bone.matrix_local.to_quaternion(), Vector((1, 1, 1)))
    bpy.context.view_layer.update()


def relaxed_upper_body(rig, phase, walk):
    breath = math.sin(phase * math.tau)
    for side, sign in (("l", 1), ("r", -1)):
        arm = rig.pose.bones["upperarm_" + side]
        shoulder = arm.head.copy()
        swing = sign * 0.065 * math.sin(phase * math.tau) if walk else 0.002 * breath
        upper_direction = Vector((sign * 0.045, swing, -0.23)).normalized()
        aim_bone(rig, arm.name, shoulder, upper_direction)
        elbow = rig.pose.bones["lowerarm_" + side].head.copy()
        aim_bone(rig, "lowerarm_" + side, elbow, Vector((sign * 0.014, swing - 0.035, -0.20)))
        # Flex around the finger's transverse rest-pose axis, toward the palm.
        for finger, curl in (("index", 0.20), ("middle", 0.28), ("ring", 0.34), ("pinky", 0.40)):
            for joint in range(1, 4):
                bone = rig.pose.bones[f"{finger}_{joint:02d}_{side}"]
                rest = bone.bone.matrix_local.to_quaternion()
                axis = Vector((sign * 0.9, 0, 0.4))
                bone.rotation_quaternion = rest.inverted() @ Quaternion(axis, curl) @ rest
        thumb = rig.pose.bones["thumb_02_" + side]
        thumb.rotation_quaternion = Quaternion((1, 0, 0), 0.12)
    for name, angle in (("spine_02", 0.008 * breath), ("spine_03", -0.006 * breath)):
        bone = rig.pose.bones[name]
        rest = bone.bone.matrix_local.to_quaternion()
        bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle) @ rest


def animate(rig, mesh, name, seconds, walk):
    reset(rig)
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = action
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, int(seconds * FPS) + 1
    samples = []
    for frame in range(scene.frame_start, scene.frame_end + 1):
        scene.frame_set(frame)
        phase = (frame - 1) / (scene.frame_end - 1)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        offset = Vector((0.012 * math.sin(phase * math.tau) if walk else 0.004, 0,
                         -0.075 + 0.024 * math.sin(phase * math.tau) ** 2 if walk else -0.015))
        root.location = root.bone.matrix_local.to_3x3().inverted() @ offset
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, phase, walk)
        for side, sign in (("l", 1), ("r", -1)):
            foot = rig.data.bones["foot_" + side]
            p = (phase + (0 if side == "l" else 0.5)) % 1
            y = foot.head_local.y + (0.012 * sign if not walk else 0)
            z = foot.head_local.z
            if walk:
                if p <= 0.5:
                    y += -STRIDE / 4 + STRIDE * p
                else:
                    u = (p - 0.5) * 2
                    y += STRIDE / 4 - STRIDE / 2 * (u * u * (3 - 2 * u))
                    z += 0.085 * math.sin(math.pi * u) ** 2
            solve_leg(rig, side, Vector((sign * 0.08, y, z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        samples.append({side: list(rig.pose.bones["ball_" + side].head) for side in ("l", "r")})
    action = rig.animation_data.action
    rig.animation_data.action_slot = action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / (name + ".fbx"), rig, [mesh], animation=True)
    loop_error = max((Vector(samples[0][s]) - Vector(samples[-1][s])).length for s in ("l", "r"))
    if loop_error > 0.0001:
        raise ValueError(f"Loop seam: {loop_error}")
    result = {"duration_seconds": seconds, "fps": FPS, "loop_error_m": loop_error,
              "toe_separation_cm": abs(samples[0]["l"][0] - samples[0]["r"][0]) * 100}
    if walk:
        stance = [(samples[i + 1]["l"][1] - samples[i]["l"][1]) * FPS * 100 for i in range(29)]
        result["stance_speed_cm_s"] = sum(stance) / len(stance)
        result["stance_speed_max_error_cm_s"] = max(abs(v - STRIDE * 100) for v in stance)
        if result["stance_speed_max_error_cm_s"] > 0.1:
            raise ValueError("Authored stance velocity is not calibrated")
    return result


def main():
    OUT.mkdir(exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE / "Heroine.blend"))
    rig = bpy.data.objects["Heroine_Rig"]
    reset(rig)
    objects = [o for o in bpy.data.objects if o.type == "MESH" and o.name.startswith("Heroine_")
               and o.name != "Heroine_Hair_Bob"]
    for obj in objects:
        active(obj)
        if obj.name == "Heroine_Body":
            prepare_body(obj)
        for modifier in list(obj.modifiers):
            if modifier.type != "ARMATURE":
                bpy.ops.object.modifier_apply(modifier=modifier.name)
        normalize(obj, rig)
    active(objects[0])
    for obj in objects:
        obj.select_set(True)
    bpy.ops.object.join()
    mesh = bpy.context.object
    mesh.name = "LocomotionReference"
    report = {}
    for name, seconds, walk in (("AN_Heroine_RelaxedIdle", 3, False), ("AN_Heroine_GroundedWalk", 1, True)):
        report[name] = animate(rig, mesh, name, seconds, walk)
    (OUT / "locomotion-contract.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
