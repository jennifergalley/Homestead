"""Machete hack through underbrush for the MetaHuman heroine.

    from homestead_agent import machete_hack as mh
    anim = mh.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_MacheteHack
    print(mh.report(anim)) # wrist positions and blade directions at each key

Brush-clearing technique: a stable stance with the left foot a little forward and knees bent,
the machete swung from the shoulder, elbow and wrist in 45-degree cuts low on the stems, and the
free hand kept back out of the blade's path. She cuts an X through the bush: a forehand stroke
from high over her right shoulder down across to her left knee, then a backhand return from her
left shoulder down to her right. Her hips and chest wind up and uncoil into each stroke while her
head stays on the target. The game clears the plant when the second stroke lands
(AHomesteadCharacter::MacheteClearSeconds, just after FRAMES['strike2'] / 30).

The machete sits in the right hand's closed grip (FHandGrip, HandGripTransform): its blade runs
along the hand's across-the-knuckles axis (pinky to index) and its edge faces along the knuckles.
Each hand key below therefore names the blade and edge directions directly.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_MacheteHack'
ANIM = 'AN_HeroineMH_MacheteHack'

FRAMES = {
    'stand': 0, 'ready': 7, 'wind1': 15, 'strike1': 20, 'follow1': 24,
    'wind2': 32, 'strike2': 37, 'follow2': 41, 'recover': 50, 'end': 60,
}

# Right wrist targets, blade direction and edge direction per key.
HAND_R = {
    'stand': ((-24.0, 6.0, 86.0), (0.05, 0.8, -0.6), (0.0, -0.6, -0.8)),
    'ready': ((-22.0, 30.0, 100.0), (0.15, 0.55, 0.8), (0.0, 0.85, -0.5)),
    'wind1': ((-30.0, 2.0, 146.0), (0.25, -0.45, 0.85), (0.45, 0.85, 0.1)),
    'strike1': ((4.0, 40.0, 84.0), (0.5, 0.6, -0.6), (0.6, 0.1, -0.8)),
    'follow1': ((12.0, 32.0, 80.0), (0.8, 0.25, -0.5), (0.3, -0.3, -0.9)),
    'wind2': ((20.0, 14.0, 124.0), (0.35, -0.3, 0.85), (-0.65, 0.6, -0.3)),
    'strike2': ((-13.0, 38.0, 86.0), (-0.5, 0.6, -0.6), (-0.7, 0.1, -0.7)),
    'follow2': ((-23.0, 29.0, 81.0), (-0.75, 0.3, -0.55), (-0.3, -0.3, -0.9)),
    'recover': ((-22.0, 24.0, 94.0), (0.1, 0.7, 0.4), (0.0, 0.5, -0.85)),
    'end': ((-24.0, 6.0, 86.0), (0.05, 0.8, -0.6), (0.0, -0.6, -0.8)),
}
# Right elbow pole: out to her right and up for the forehand wind-up, down and in on the backhand.
POLE_R = {
    'stand': (-60.0, -10.0, 90.0), 'ready': (-60.0, -10.0, 90.0), 'wind1': (-80.0, -30.0, 150.0),
    'strike1': (-50.0, 20.0, 90.0), 'follow1': (-30.0, 10.0, 70.0), 'wind2': (10.0, 30.0, 70.0),
    'strike2': (-60.0, 10.0, 80.0), 'follow2': (-70.0, 0.0, 80.0), 'recover': (-60.0, -10.0, 90.0),
    'end': (-60.0, -10.0, 90.0),
}
# Free hand held back at her left hip, out of the blade's path, swinging a little for balance.
HAND_L = {
    'stand': (24.0, 5.0, 86.0), 'ready': (26.0, 10.0, 96.0), 'wind1': (30.0, 22.0, 104.0),
    'strike1': (28.0, -6.0, 98.0), 'follow1': (30.0, -10.0, 100.0), 'wind2': (34.0, -4.0, 100.0),
    'strike2': (30.0, 16.0, 102.0), 'follow2': (28.0, 18.0, 100.0), 'recover': (26.0, 8.0, 94.0),
    'end': (24.0, 5.0, 86.0),
}
# Pelvis offset (cm), torso twist (deg, + turns her chest to her right) and forward lean (deg).
BODY = {
    'stand': ((0, 0, 0), 0, 0), 'ready': ((0, 3, -7), -4, 8), 'wind1': ((-2, -1, -6), 30, 4),
    'strike1': ((2, 8, -14), -24, 30), 'follow1': ((3, 9, -15), -30, 34), 'wind2': ((2, 3, -9), -34, 14),
    'strike2': ((-2, 8, -14), 20, 30), 'follow2': ((-3, 9, -15), 26, 34), 'recover': ((0, 3, -7), 0, 8),
    'end': ((0, 0, 0), 0, 0),
}
# Sign of spine-control yaw that turns her chest to her right.
TWIST_SIGN = 1
FOOT_L_FORWARD = (15.0, 16.0, 8.6)
FOOT_R_BACK = (-15.0, -6.0, 8.6)


def _norm(v):
    length = sum(c * c for c in v) ** 0.5
    return unreal.Vector(*(c / length for c in v))


def grip_turn(s, blade, edge):
    """Extra world rotation for the right hand's IK control so a gripped tool's blade points along
    ``blade`` and its edge along ``edge`` (component-space directions)."""
    hand = s.bone('hand_r').translation
    along = (s.bone('middle_01_r').translation - hand).normal()
    across = s.bone('index_01_r').translation - s.bone('pinky_01_r').translation
    across = (across - along * across.dot(along)).normal()
    rest = unreal.MathLibrary.make_rot_from_xz(along, across.cross(along)).quaternion()
    b = _norm(blade)
    e = _norm(edge)
    e = (e - b * e.dot(b)).normal()
    target = unreal.MathLibrary.make_rot_from_xz(e, b.cross(e)).quaternion()
    return (target * rest.inversed()).rotator()


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        drop, twist, lean = BODY[name]
        twist *= TWIST_SIGN
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop),
                    unreal.Rotator(roll=lean * 0.4, pitch=0, yaw=twist * 0.35))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=lean * 0.2, yaw=twist * 0.22)
        # The head stays on the bush: it undoes most of the torso's twist and looks down at it.
        s.key_rotation(frame, 'neck_01_ctrl', roll=lean * 0.1, yaw=-twist * 0.3)
        s.key_rotation(frame, 'head_ctrl', roll=(8 + lean * 0.2) if frame else 0, yaw=-twist * 0.45)
        position, blade, edge = HAND_R[name]
        s.key_world(frame, 'hand_r_ik_ctrl', position, grip_turn(s, blade, edge))
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R[name])
        s.key_world(frame, 'hand_l_ik_ctrl', HAND_L[name], s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
    # Stance: the left foot steps a little forward and the right slides back, then both return.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 8, 6)))
    s.key_world(F['ready'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 8, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['ready'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    return s.bake(ANIM)


def report(anim):
    """Wrist position and gripped blade/edge directions at each key, from the baked clip."""
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'middle_01_r', 'index_01_r', 'pinky_01_r', 'clavicle_l', 'clavicle_r'), frame / 30)
        hand = b['hand_r'].translation
        along = (b['middle_01_r'].translation - hand).normal()
        across = b['index_01_r'].translation - b['pinky_01_r'].translation
        across = (across - along * across.dot(along)).normal()
        want = HAND_R[name]
        w = _norm(want[1])
        # Shoulder line's turn from square: positive when her left shoulder comes forward (chest turned right).
        shoulders = b['clavicle_l'].translation - b['clavicle_r'].translation
        chest = math.degrees(math.atan2(shoulders.y, shoulders.x))
        lines.append(f"{name:8s} wrist ({hand.x:6.1f},{hand.y:6.1f},{hand.z:6.1f}) want {want[0]}  "
                     f"blade ({across.x:5.2f},{across.y:5.2f},{across.z:5.2f}) want ({w.x:5.2f},{w.y:5.2f},{w.z:5.2f})  "
                     f"edge ({along.x:5.2f},{along.y:5.2f},{along.z:5.2f})  chest yaw {chest:6.1f}")
    return '\n'.join(lines)
