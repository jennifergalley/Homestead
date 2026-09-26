"""Resting stance for the MetaHuman heroine: a grounded, ready idle loop instead of a mall pose.

    from homestead_agent import active_idle as ai
    anim = ai.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_ActiveIdle
    print(ai.report(anim)) # feet and knee spacing, hand clearance from the hip pouch

Feet about shoulder width with the toes turned out a little and the left foot a half step ahead,
knees soft and apart, weight settled onto her right leg so the hips sit over it, chest up and
shoulders back. Her hands hang loose a hand's width out from her thighs and slightly forward,
clear of the forage pouch on her right hip. Over the 6 s loop she breathes (chest rise and a
small shoulder lift), eases her weight a little toward the centre and back, and glances around.
First and last frames match so it loops.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_ActiveIdle'
ANIM = 'AN_HeroineMH_ActiveIdle'
LENGTH = 180  # 6 s at 30 fps
FEET_HALF_WIDTH = 16.5
FOOT_L = (FEET_HALF_WIDTH, 4.0, 8.6)
FOOT_R = (-FEET_HALF_WIDTH, -3.0, 8.6)
TOE_OUT = 8.0
# Weight over the right leg: hips shifted toward it and dropped a touch (soft knees).
BODY = (-2.6, 2.4, 99.6)
HIP_ROLL = 2.5
HAND_L = (27.5, 10.0, 85.0)
HAND_R = (-34.0, 12.0, 85.0)
POUCH = (-19.0, 7.0, 88.0)


def _wave(frame, cycles=1.0, phase=0.0):
    return math.sin(2 * math.pi * (cycles * frame / LENGTH + phase))


def build():
    s = ra.Session(SEQUENCE, frames=LENGTH)
    down = (0, 0, -1)
    s.key_bool(0, 'arm_l_fk_ik_switch', True)
    s.key_bool(0, 'arm_r_fk_ik_switch', True)
    s.key_bool(LENGTH, 'arm_l_fk_ik_switch', True)
    s.key_bool(LENGTH, 'arm_r_fk_ik_switch', True)
    toe_l = unreal.Rotator(roll=0, pitch=0, yaw=-TOE_OUT)
    toe_r = unreal.Rotator(roll=0, pitch=0, yaw=TOE_OUT)
    for frame in (0, LENGTH):
        s.key_world(frame, 'foot_l_ik_ctrl', FOOT_L, toe_l)
        s.key_world(frame, 'foot_r_ik_ctrl', FOOT_R, toe_r)
    # Knees track out over the toes, not inward.
    for frame in (0, LENGTH):
        s.key_world(frame, 'leg_l_pv_ik_ctrl', (FOOT_L[0] + 10.0, 60.0, 50.0))
        s.key_world(frame, 'leg_r_pv_ik_ctrl', (FOOT_R[0] - 10.0, 60.0, 50.0))

    hang_l = s.hand_turn('l', (0.05, 0.12, -1), (-1, 0.1, 0))
    hang_r = s.hand_turn('r', (-0.05, 0.12, -1), (1, 0.1, 0))
    keys = range(0, LENGTH + 1, 15)
    for frame in keys:
        breath = _wave(frame, 2)           # two breaths per loop
        shift = 0.5 - 0.5 * math.cos(2 * math.pi * frame / LENGTH)  # 0 -> 1 -> 0
        body = (BODY[0] + 1.4 * shift, BODY[1] + 0.3 * breath, BODY[2] + 0.35 * breath + 0.3 * shift)
        s.key_world(frame, 'body_ctrl', body, unreal.Rotator(roll=0, pitch=-HIP_ROLL * (1 - 0.4 * shift), yaw=2.0 * shift))
        # Chest rises with the breath; the spine counters the hip tilt so her shoulders stay level.
        for control, share in (('spine_01_ctrl', 0.3), ('spine_02_ctrl', 0.35), ('spine_03_ctrl', 0.35)):
            s.key_rotation(frame, control, roll=-1.2 * breath * share - 1.5 * share,
                           pitch=HIP_ROLL * share * (1 - 0.4 * shift), yaw=-1.5 * shift * share)
        look = _wave(frame, 1, 0.1)
        s.key_rotation(frame, 'neck_01_ctrl', roll=0.5 * breath, yaw=4.0 * look * 0.4)
        s.key_rotation(frame, 'head_ctrl', roll=-1.0, yaw=4.0 * look * 0.6)
        # Hands loose at her sides, swaying a little with the breath and weight shift.
        s.key_world(frame, 'hand_l_ik_ctrl', kg._add(HAND_L, (0.3 * shift, 0.6 * breath, 0.4 * breath)), hang_l)
        s.key_world(frame, 'hand_r_ik_ctrl', kg._add(HAND_R, (0.8 * shift, 0.6 * breath, 0.3 * breath)), hang_r)
        s.key_world(frame, 'arm_l_pv_ik_ctrl', (45.0, -35.0, 100.0))
        s.key_world(frame, 'arm_r_pv_ik_ctrl', (-45.0, -35.0, 100.0))
    return s.bake(ANIM)


def report(anim):
    lines = []
    for frame in (0, LENGTH // 4, LENGTH // 2, 3 * LENGTH // 4):
        b = ra.bone_positions(anim, ('foot_l', 'foot_r', 'calf_l', 'calf_r', 'hand_l', 'hand_r', 'pelvis', 'head'), frame / 30)
        p = {k: v.translation for k, v in b.items()}
        pouch = unreal.Vector(*POUCH)
        lines.append(
            f"f{frame:3d} feet {(p['foot_l'] - p['foot_r']).length():5.1f}  knees {(p['calf_l'] - p['calf_r']).length():5.1f}  "
            f"hand_r to pouch {(p['hand_r'] - pouch).length():5.1f}  pelvis ({p['pelvis'].x:5.1f},{p['pelvis'].y:5.1f},{p['pelvis'].z:6.1f})  "
            f"head z {p['head'].z:6.1f}")
    return '\n'.join(lines)
