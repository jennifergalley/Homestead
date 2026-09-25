"""Author one five-second, restrained living idle on the admitted CC0 bind."""
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import aim_bone, prepare_reference, relaxed_upper_body, solve_leg
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "IdleRevision"
NAME = "AN_Heroine_LivingIdle02"
FPS = 60
FRAMES = 301


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(NAME)
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, FRAMES
    toes, heads, hands, roots = [], [], [], []
    for frame in range(1, FRAMES + 1):
        scene.frame_set(frame)
        phase = (frame - 1) / (FRAMES - 1)
        breath = math.sin(phase * math.tau)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        shift = Vector((0, 0, -.015))
        root.location = root.bone.matrix_local.to_3x3().inverted() @ shift
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, phase, False)
        for name, bend, twist in (("spine_02", .027, .007), ("spine_03", -.010, -.005)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = (rest.inverted()
                @ Quaternion((1, 0, 0), bend * breath)
                @ Quaternion((0, 1, 0), .010 * breath)
                @ Quaternion((0, 0, 1), twist * breath) @ rest)
        head = rig.pose.bones["head"]
        rest = head.bone.matrix_local.to_quaternion()
        head.rotation_quaternion = (rest.inverted()
            @ Quaternion((0, 0, 1), .012 * breath)
            @ Quaternion((0, 1, 0), .009 * math.sin(phase * math.tau * 2)) @ rest)
        bpy.context.view_layer.update()
        for side, sign in (("l", 1), ("r", -1)):
            upper = rig.pose.bones["upperarm_" + side]
            sway = sign * .010 * breath
            aim_bone(rig, upper.name, upper.head.copy(),
                     Vector((sign * .05, sway, -.23)).normalized())
            lower = rig.pose.bones["lowerarm_" + side]
            aim_bone(rig, lower.name, lower.head.copy(),
                     Vector((sign * .014, sway - .038, -.19)).normalized())
            ankle = rig.data.bones["foot_" + side].head_local
            solve_leg(rig, side, Vector((sign * .08, ankle.y, ankle.z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        toes.append({side: Vector(rig.pose.bones["ball_" + side].head)
                     for side in ("l", "r")})
        heads.append(Vector(head.head))
        hands.append(Vector(rig.pose.bones["hand_r"].head))
        roots.append(shift)
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / f"{NAME}.fbx", rig, [mesh], animation=True)
    toe_drift = max((point[side] - toes[0][side]).length for point in toes
                    for side in ("l", "r"))
    head_travel = max((point - heads[0]).length for point in heads)
    hand_travel = max((point - hands[0]).length for point in hands)
    seam = max((toes[0][side] - toes[-1][side]).length for side in ("l", "r"))
    root_lateral = max(abs(p.x) for p in roots)
    if toe_drift > .001 or seam > .0001 or not .006 < head_travel < .05 \
            or not .007 < hand_travel < .06 or root_lateral > .001:
        raise ValueError(f"Idle toe/head/hand/root contract failed: "
                         f"{toe_drift}, {seam}, {head_travel}, {hand_travel}, {root_lateral}")
    (OUT / "idle-contract.json").write_text(json.dumps({NAME: {
        "duration_seconds": 5, "fps": FPS,
        "toe_drift_m": toe_drift, "head_travel_m": head_travel,
        "hand_travel_m": hand_travel, "root_lateral_m": root_lateral,
        "loop_error_m": seam, "root_motion": False, "notifies": 0,
        "rights": "Original authored motion on the admitted CC0 MakeHuman bind",
    }}, indent=2) + "\n")
    print("LIVING_IDLE_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
