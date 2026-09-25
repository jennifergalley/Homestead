"""Author a short planted low-growth cut, distinct from the hatchet chop."""
import json
import sys
from pathlib import Path

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import prepare_reference, relaxed_upper_body, solve_leg
from build_gathering import reach_arm, smooth
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "Assets" / "Characters" / "Heroine" / "KnifeCut"
NAME = "AN_Heroine_KnifeCut"
FPS = 60
DURATION = .9


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    rig, mesh = prepare_reference()
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(NAME)
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, round(DURATION * FPS) + 1
    hands = []
    toes = []
    for frame in range(scene.frame_start, scene.frame_end + 1):
        scene.frame_set(frame)
        t = (frame - 1) / FPS
        enter = smooth(t / .13)
        slash = smooth((t - .24) / .24)
        recover = 1 - smooth((t - .57) / .33)
        amount = enter * recover
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        root = rig.pose.bones["Root"]
        displacement = Vector((.003 * slash * recover, 0, -.015 - .035 * amount))
        root.location = root.bone.matrix_local.to_3x3().inverted() @ displacement
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, 0, False)
        for name, angle in (("spine_02", .12), ("spine_03", .07)):
            bone = rig.pose.bones[name]
            rest = bone.bone.matrix_local.to_quaternion()
            bone.rotation_quaternion = rest.inverted() @ Quaternion((1, 0, 0), angle * amount) @ rest
        bpy.context.view_layer.update()
        if amount > 0:
            windup = Vector((-.23, -.22, 1.14))
            finish = Vector((-.08, -.35, .87))
            reach_arm(rig, amount, 0, windup.lerp(finish, slash))
        for side, sign in (("l", 1), ("r", -1)):
            foot = rig.data.bones["foot_" + side]
            solve_leg(rig, side, Vector((sign * .08, foot.head_local.y + .012 * sign,
                                         foot.head_local.z)))
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame, group=bone.name)
        root.keyframe_insert(data_path="location", frame=frame, group="Root")
        bpy.context.view_layer.update()
        hands.append(list(rig.pose.bones["hand_r"].head))
        toes.append({side: list(rig.pose.bones["ball_" + side].head)
                     for side in ("l", "r")})
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    export_fbx(OUT / f"{NAME}.fbx", rig, [mesh], animation=True)
    wrist_travel = max((Vector(hand) - Vector(hands[0])).length for hand in hands)
    toe_drift = max((Vector(toe[side]) - Vector(toes[0][side])).length
                    for toe in toes for side in ("l", "r"))
    if wrist_travel < .15 or toe_drift > .001:
        raise ValueError(f"Knife cut needs a distinct wrist and planted toes: {wrist_travel}, {toe_drift}")
    contract = {NAME: {
        "duration_seconds": DURATION,
        "fps": FPS,
        "wrist_travel_m": wrist_travel,
        "toe_drift_m": toe_drift,
        "root_motion": False,
        "notifies": 0,
        "rig": "Existing unchanged heroine bind; animation-only import",
        "authority": "Presentation only after existing successful Knife Clear or reed Harvest",
        "rights": "Original authored motion on admitted CC0 MPFB source",
    }}
    (OUT / "knife-cut-contract.json").write_text(json.dumps(contract, indent=2) + "\n")
    print("KNIFE_CUT_AUTHORED", OUT / f"{NAME}.fbx")


if __name__ == "__main__":
    main()
