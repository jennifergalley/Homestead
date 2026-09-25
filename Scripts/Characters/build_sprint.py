"""Author one in-place sprint cycle on the admitted CC0 heroine bind."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import aim_bone, prepare_reference, relaxed_upper_body, solve_leg
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "Sprinting"
NAME = "AN_Heroine_Sprint"
FPS = 60
FRAMES = 31
DURATION = (FRAMES - 1) / FPS
STRIDE = 1.5


def smooth(value):
    value = max(0.0, min(1.0, value))
    return value * value * (3 - 2 * value)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(NAME)
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, FRAMES
    toe_samples = []
    root_samples = []
    for frame in range(1, FRAMES + 1):
        scene.frame_set(frame)
        phase = (frame - 1) / (FRAMES - 1)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        displacement = Vector((
            .012 * math.sin(phase * math.tau), 0,
            -.12 + .035 * math.sin(phase * math.tau) ** 2))
        root.location = root.bone.matrix_local.to_3x3().inverted() @ displacement
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, phase, True)
        for name, lean in (("spine_02", .09), ("spine_03", .055)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), lean) @ rest
        bpy.context.view_layer.update()
        for side, sign in (("l", 1), ("r", -1)):
            upper = rig.pose.bones["upperarm_" + side]
            swing = sign * .135 * math.sin(phase * math.tau)
            aim_bone(rig, upper.name, upper.head.copy(),
                     Vector((sign * .035, swing, -.23)).normalized())
            lower = rig.pose.bones["lowerarm_" + side]
            aim_bone(rig, lower.name, lower.head.copy(),
                     Vector((sign * .015, swing - .035, -.20)).normalized())
            foot = rig.data.bones["foot_" + side]
            p = (phase + (0 if side == "l" else .5)) % 1
            y = foot.head_local.y
            z = foot.head_local.z
            if p <= .5:
                y += -STRIDE / 4 + STRIDE * p
            else:
                u = (p - .5) * 2
                y += STRIDE / 4 - STRIDE / 2 * smooth(u)
                z += .13 * math.sin(math.pi * u) ** 2
            solve_leg(rig, side, Vector((sign * .08, y, z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        toe_samples.append({side: list(rig.pose.bones["ball_" + side].head)
                            for side in ("l", "r")})
        root_samples.append(list(displacement))
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / f"{NAME}.fbx", rig, [mesh], animation=True)
    stance = [(toe_samples[i + 1]["l"][1] - toe_samples[i]["l"][1]) * FPS * 100
              for i in range((FRAMES - 1) // 2 - 1)]
    expected_speed = STRIDE / DURATION * 100
    max_error = max(abs(value - expected_speed) for value in stance)
    seam = max((Vector(toe_samples[0][side]) - Vector(toe_samples[-1][side])).length
               for side in ("l", "r"))
    if (max_error > .1 or seam > .0001
            or max(abs(value[1]) for value in root_samples) > .0001):
        raise ValueError("Sprint velocity, loop or zero root travel failed")
    contract = {NAME: {
        "duration_seconds": DURATION,
        "fps": FPS,
        "stance_speed_cm_s": expected_speed,
        "stance_speed_max_error_cm_s": max_error,
        "loop_error_m": seam,
        "root_motion": False,
        "notifies": 0,
        "rig": "Existing unchanged heroine bind; animation-only import",
        "rights": "Original authored motion on admitted CC0 MPFB source",
    }}
    (OUT / "sprint-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("SPRINT_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
