"""Retarget two licensed CMU walk cycles onto the existing in-place heroine rig."""
import hashlib
import json
import math
import statistics
import sys
from pathlib import Path

import bpy
from mathutils import Euler, Matrix, Quaternion, Vector

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_locomotion import aim_bone, prepare_reference, relaxed_upper_body, solve_leg
from export_heroine import export_fbx, reset

ROOT = Path(__file__).resolve().parents[2]
HEAD_LEVEL_TRIAL = "--head-level-trial" in sys.argv
ORIGINAL = ROOT / "Assets" / "Characters" / "HeroineTrials" / "CMUWalk01"
OUT = ORIGINAL.with_name("CMUWalk02") if HEAD_LEVEL_TRIAL else ORIGINAL
SOURCE = ORIGINAL / "Source"
CMU_SCALE = (1.0 / .45) * 2.54 / 100
FPS = 60
CLIPS = (
    ("AN_Heroine_CMUSlowWalk02" if HEAD_LEVEL_TRIAL else "AN_Heroine_CMUSlowWalk01", "07_04.amc", 100, 172),
    ("AN_Heroine_CMUNormalWalk02" if HEAD_LEVEL_TRIAL else "AN_Heroine_CMUNormalWalk01", "07_01.amc", 100, 129),
)


def parse_skeleton(path):
    bones, parents = {}, {}
    section, current = "", None
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith(":"):
            section = line[1:]
        elif section == "bonedata":
            parts = line.split()
            if parts[0] == "begin":
                current = {}
            elif parts[0] == "end":
                bones[current["name"]] = current
                current = None
            elif current is not None and parts[0] in ("name", "direction", "length", "axis", "dof"):
                current[parts[0]] = parts[1] if parts[0] == "name" else parts[1:]
        elif section == "hierarchy":
            parts = line.split()
            if parts[0] not in ("begin", "end"):
                for child in parts[1:]:
                    parents[child] = parts[0]
    if len(bones) != 30 or set(bones) != set(parents):
        raise ValueError("CMU joint definitions differ from the reviewed skeleton")
    return bones, parents


def parse_motion(path):
    frames = []
    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line[0] in "#:":
            continue
        if line.isdigit():
            frames.append({})
        else:
            name, *values = line.split()
            if not frames or name in frames[-1]:
                raise ValueError("Malformed or duplicate CMU motion joint")
            frames[-1][name] = [float(value) for value in values]
    if any(len(frame.get("root", [])) != 6 for frame in frames):
        raise ValueError("The licensed CMU motion has incomplete root channels")
    return frames


def euler(angles):
    return Euler(tuple(math.radians(float(angle)) for angle in angles), "XYZ").to_matrix()


def forward_kinematics(frame, bones, parents):
    source_root = frame["root"]
    points = {"root": Vector(source_root[:3]) * CMU_SCALE}
    orientations = {"root": euler(source_root[3:])}
    todo = ["root"]
    while todo:
        parent = todo.pop()
        for name, owner in parents.items():
            if owner != parent:
                continue
            if parent == "root":
                points[name] = points[parent].copy()
            else:
                source_bone = bones[parent]
                direction = Vector(tuple(float(value) for value in source_bone["direction"]))
                points[name] = points[parent] + orientations[parent] @ (
                    direction * float(source_bone["length"][0]) * CMU_SCALE)
            definition = bones[name]
            channels = {"rx": 0.0, "ry": 0.0, "rz": 0.0}
            for channel, angle in zip(definition.get("dof", []), frame.get(name, [])):
                channels[channel] = angle
            axis = euler(definition["axis"][:3])
            orientations[name] = orientations[parent] @ axis @ euler(
                [channels[channel] for channel in ("rx", "ry", "rz")]) @ axis.inverted()
            todo.append(name)
    return points


def fitted_direction(vector):
    return Vector((vector.x, -vector.z, vector.y))


def smooth(value):
    value = max(0.0, min(1.0, value))
    return value * value * (3.0 - 2.0 * value)


def author(rig, mesh, name, frames, bones, parents, start, period):
    if start + period >= len(frames):
        raise ValueError("The selected CMU loop exceeds the motion source")
    if any(len(frame) != 29 for frame in frames[start:start + period + 1]):
        raise ValueError("The selected CMU motion interval has incomplete joint channels")
    reset(rig)
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(name)
    scene = bpy.context.scene
    scene.render.fps = FPS
    frame_count = round(period / 120 * FPS) + 1
    scene.frame_start, scene.frame_end = 1, frame_count
    first = forward_kinematics(frames[start], bones, parents)
    last = forward_kinematics(frames[start + period], bones, parents)
    source_points = [forward_kinematics(frames[index], bones, parents)
                     for index in range(start, start + period + 1)]
    maximum_leg_span = max((points[foot] - points[hip]).length
                           for points in source_points
                           for hip, foot in (("lfemur", "lfoot"), ("rfemur", "rfoot")))
    limb = rig.data.bones["thigh_l"].length + rig.data.bones["calf_l"].length
    leg_scale = (limb - .02) / maximum_leg_span
    rest_toe_height = rig.data.bones["ball_l"].head_local.z
    heel_to_ground = rig.data.bones["foot_l"].head_local.z - rest_toe_height
    if not .75 < leg_scale < 1.15:
        raise ValueError("The real mocap legs cannot fit the admitted skeleton")
    toes = []
    head_pitch = []
    leveled_head_pitch = []
    maximum_floor_correction = 0
    actor_root = rig.pose.bones["Root"]
    for frame_index in range(1, frame_count + 1):
        phase = (frame_index - 1) / (frame_count - 1)
        index = start + round(phase * period)
        positions = source_points[index - start]
        root_height = float(frames[index]["root"][1]) * CMU_SCALE
        start_height = float(frames[start]["root"][1]) * CMU_SCALE
        end_height = float(frames[start + period]["root"][1]) * CMU_SCALE
        close = smooth((phase - .82) / .18)
        height = root_height - (end_height - start_height) * close - start_height
        first_root = frames[start]["root"]
        end_root = frames[start + period]["root"]
        angles = [frames[index]["root"][column]
                  + (first_root[column] - end_root[column]) * close
                  - first_root[column]
                  for column in (3, 4, 5)]
        lateral = (frames[index]["root"][0] - first_root[0]
                   - (end_root[0] - first_root[0]) * phase) * CMU_SCALE * leg_scale
        corrected = {
            joint: positions[joint] - positions["root"] + (
                (first[joint] - first["root"]) - (last[joint] - last["root"])) * close
            for joint in positions
        }
        scene.frame_set(frame_index)
        for bone in rig.pose.bones:
            bone.rotation_mode = "QUATERNION"
            bone.rotation_quaternion = Quaternion()
            bone.location = (0, 0, 0)
        actor_root.location = actor_root.bone.matrix_local.to_3x3().inverted() @ Vector(
            (lateral, 0, -.02 + height * leg_scale))
        bpy.context.view_layer.update()
        relaxed_upper_body(rig, phase, False)
        pelvis = rig.pose.bones["pelvis"]
        rest = pelvis.bone.matrix_local.to_quaternion()
        turn = (Quaternion((0, 0, 1), math.radians(angles[1]))
                @ Quaternion((0, 1, 0), math.radians(-angles[2] * .8))
                @ Quaternion((1, 0, 0), math.radians(angles[0] * .6)))
        pelvis.rotation_quaternion = rest.inverted() @ turn @ rest
        bpy.context.view_layer.update()
        for target, source_a, source_b in (
            ("spine_01", "lowerback", "upperback"),
            ("spine_02", "upperback", "thorax"),
            ("spine_03", "thorax", "lowerneck"),
            ("neck_01", "lowerneck", "head"),
        ):
            bone = rig.pose.bones[target]
            aim_bone(rig, target, bone.head.copy(),
                     fitted_direction(corrected[source_b] - corrected[source_a]))
        head = rig.pose.bones["head"]
        face_forward = (head.matrix.to_3x3()
                        @ head.bone.matrix_local.to_3x3().inverted()
                        @ Vector((0, -1, 0))).normalized()
        head_pitch.append(math.degrees(math.atan2(
            face_forward.z, math.hypot(face_forward.x, face_forward.y))))
        if HEAD_LEVEL_TRIAL:
            target_pitch = math.radians(-2.0)
            pitch = math.radians(head_pitch[-1])
            horizontal = Vector((face_forward.x, face_forward.y, 0)).normalized()
            right = Vector((0, 0, 1)).cross(horizontal)
            neck = rig.pose.bones["neck_01"]
            neck.matrix = Matrix.LocRotScale(
                neck.head.copy(),
                Quaternion(right, (pitch - target_pitch) * 0.6) @ neck.matrix.to_quaternion(),
                Vector((1, 1, 1)))
            bpy.context.view_layer.update()
            face_forward = (head.matrix.to_3x3()
                            @ head.bone.matrix_local.to_3x3().inverted()
                            @ Vector((0, -1, 0))).normalized()
            pitch = math.atan2(face_forward.z, math.hypot(face_forward.x, face_forward.y))
            horizontal = Vector((face_forward.x, face_forward.y, 0)).normalized()
            right = Vector((0, 0, 1)).cross(horizontal)
            head.matrix = Matrix.LocRotScale(
                head.head.copy(),
                Quaternion(right, pitch - target_pitch) @ head.matrix.to_quaternion(),
                Vector((1, 1, 1)))
            bpy.context.view_layer.update()
            face_forward = (head.matrix.to_3x3()
                            @ head.bone.matrix_local.to_3x3().inverted()
                            @ Vector((0, -1, 0))).normalized()
            leveled_head_pitch.append(math.degrees(math.atan2(
                face_forward.z, math.hypot(face_forward.x, face_forward.y))))
            if abs(leveled_head_pitch[-1] + 2.0) > 0.5:
                raise ValueError("The retarget did not level the moving head")
        for side in ("l", "r"):
            for target, source_a, source_b in (
                ("upperarm_" + side, side + "humerus", side + "radius"),
                ("lowerarm_" + side, side + "radius", side + "wrist"),
            ):
                bone = rig.pose.bones[target]
                aim_bone(rig, target, bone.head.copy(),
                         fitted_direction(corrected[source_b] - corrected[source_a]))
            source_leg = fitted_direction(corrected[side + "foot"] - corrected[side + "femur"])
            hip = rig.pose.bones["thigh_" + side].head.copy()
            solve_leg(rig, side, hip + source_leg * leg_scale)
            foot = rig.pose.bones["foot_" + side]
            aim_bone(rig, foot.name, foot.head.copy(),
                     fitted_direction(corrected[side + "toes"] - corrected[side + "foot"]))
        floor_correction = rest_toe_height - min(
            min(rig.pose.bones["ball_" + side].head.z,
                rig.pose.bones["foot_" + side].head.z - heel_to_ground)
            for side in ("l", "r"))
        if abs(floor_correction) > .04:
            print("CMU_FLOOR", name, frame_index, round(floor_correction, 4), flush=True)
        if abs(floor_correction) > .08:
            raise ValueError(
                f"The captured stance cannot fit the heroine's ground level: {name} "
                f"frame {frame_index} correction {floor_correction:.4f} m")
        actor_root.location += actor_root.bone.matrix_local.to_3x3().inverted() @ Vector(
            (0, 0, floor_correction))
        maximum_floor_correction = max(maximum_floor_correction, abs(floor_correction))
        bpy.context.view_layer.update()
        for bone in rig.pose.bones:
            bone.keyframe_insert(data_path="rotation_quaternion", frame=frame_index, group=bone.name)
        actor_root.keyframe_insert(data_path="location", frame=frame_index, group="Root")
        toes.append({side: rig.pose.bones["ball_" + side].head.copy()
                     for side in ("l", "r")})
    rig.animation_data.action_slot = rig.animation_data.action.slots[0]
    scene.frame_set(1)
    output = OUT / (name + ".fbx")
    export_fbx(output, rig, [mesh], animation=True)
    seam = max((toes[0][side] - toes[-1][side]).length for side in ("l", "r"))
    if seam > .025:
        raise ValueError(f"The mocap loop has an exposed foot seam: {seam:.4f} m")
    stances = {}
    for side in ("l", "r"):
        ground = min(sample[side].z for sample in toes)
        speeds = [abs((b[side].y - a[side].y) * FPS * 100)
                  for a, b in zip(toes, toes[1:])
                  if a[side].z <= ground + .012 and b[side].z <= ground + .012]
        if len(speeds) < 5:
            raise ValueError("Mocap retarget did not show enough near-ground stance frames")
        stances[side] = {
            "contact_samples": len(speeds),
            "median_stance_speed_cm_s": statistics.median(speeds),
            "toe_ground_m": ground,
            "maximum_toe_lift_cm": (max(sample[side].z for sample in toes) - ground) * 100,
        }
    midpoint = statistics.median(row["median_stance_speed_cm_s"] for row in stances.values())
    if not 45 < midpoint < 260:
        raise ValueError(f"The captured stance speed does not fit playable locomotion: {midpoint}")
    return {
        "duration_seconds": (frame_count - 1) / FPS,
        "source_frames": [start, start + period],
        "source_fps": 120,
        "leg_scale": leg_scale,
        "maximum_floor_correction_cm": maximum_floor_correction * 100,
        "toe_loop_error_m": seam,
        "head_pitch_degrees": {
            "minimum": min(head_pitch), "median": statistics.median(head_pitch),
            "maximum": max(head_pitch),
        },
        "leveled_head_pitch_degrees": {
            "minimum": min(leveled_head_pitch), "median": statistics.median(leveled_head_pitch),
            "maximum": max(leveled_head_pitch),
        } if HEAD_LEVEL_TRIAL else None,
        "stance": stances,
        "median_stance_speed_cm_s": midpoint,
        "root_motion": False,
        "notifies": 0,
        "fbx_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
        "license": "CMU Graphics Lab mocap: copying, modification and redistribution allowed",
        "status": "Offline licensed in-place retarget; in-game motion quality unverified",
    }


def main():
    OUT.mkdir(exist_ok=True)
    receipt = json.loads((ORIGINAL / "source-license.json").read_text())
    for name, digest in receipt["sources_sha256"].items():
        source = SOURCE / name
        if hashlib.sha256(source.read_bytes()).hexdigest().upper() != digest.upper():
            raise ValueError("Official CMU text source changed: " + name)
    bones, parents = parse_skeleton(SOURCE / "07.asf")
    rig, mesh = prepare_reference()
    result = {}
    for name, file, start, period in CLIPS:
        result[name] = author(rig, mesh, name, parse_motion(SOURCE / file),
                              bones, parents, start, period)
    (OUT / "retarget-report.json").write_text(json.dumps(result, indent=2) + "\n")
    print("CMU_WALKS_AUTHORED", OUT, flush=True)


if __name__ == "__main__":
    main()
