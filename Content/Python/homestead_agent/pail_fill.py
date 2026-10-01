"""Filling the carved water pail (SM_WaterPail) at the stream for the MetaHuman heroine.

    from homestead_agent import pail_fill as pf
    anim = pf.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_PailFill
    print(pf.report(anim)) # where the bail hand is at each key

She steps her left foot forward and drops onto her right knee at the bank (as in the kneeling
gathers), rests her left hand on the forward knee, reaches out and lowers the pail by its bail
into the water in front of her, lets it tip and sink while it fills, then draws it up out of the
stream, lets it drip for a moment and rises with it hanging at her side. The pail swings freely
from the bail throughout (``AHomesteadCharacter::UpdateHangingPail``); the game shows it full
from EVENTS['full'].

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_PailFill'
ANIM = 'AN_HeroineMH_PailFill'

FRAMES = {
    'stand': 0, 'step': 8, 'kneel': 20, 'reach': 32, 'dip': 44, 'sink': 56, 'hold': 70, 'draw': 82,
    'drip': 92, 'rise': 108, 'end': 118,
}
EVENTS = {'wet': FRAMES['dip'] / 30, 'full': FRAMES['sink'] / 30}
# The spot on the water the pail goes into, relative to her standing pose; the game turns her to
# face the stream so it lands in the water.
SPOT = (-12.0, 59.0, 0.0)
RIGHT = {
    'stand': (-24.0, 5.0, 86.0), 'step': (-22.0, 12.0, 80.0), 'kneel': (-22.0, 20.0, 62.0),
    'reach': (-14.0, 50.0, 50.0), 'dip': (-12.0, 60.0, 36.0), 'sink': (-12.0, 62.0, 30.0),
    'hold': (-12.0, 62.0, 31.0), 'draw': (-14.0, 52.0, 56.0), 'drip': (-16.0, 46.0, 58.0),
    'rise': (-24.0, 10.0, 82.0), 'end': (-24.0, 5.0, 86.0),
}
LEAN = {'stand': 0, 'step': 8, 'kneel': 26, 'reach': 44, 'dip': 54, 'sink': 58, 'hold': 58,
        'draw': 44, 'drip': 36, 'rise': 8, 'end': 0}


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    def tilt(name):
        return unreal.Rotator(roll=LEAN[name] * 0.5, pitch=0, yaw=0)

    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt('stand'))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt('step'))
    for name in ('kneel', 'reach', 'dip', 'sink', 'hold', 'draw', 'drip'):
        forward = (0, 3.0, -5.0) if LEAN[name] > 50 else (0, 1.5, -2.0) if LEAN[name] > 40 else (0, 0, 0)
        s.key_world(F[name], 'body_ctrl', kg._add(kg.BODY_KNEEL, forward), tilt(name))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt('rise'))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt('end'))

    # Feet as in the kneeling gathers.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['drip'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    kg.key_step_back(s, F['step'], F['kneel'], toes)
    s.key_world(F['drip'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    kg.key_rise_steps(s, F['drip'], F['rise'], toes)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    for name, frame in F.items():
        deg = LEAN[name]
        side = (-6 if F['reach'] <= frame <= F['drip'] else 0) * kg.BEND_SIGN
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, pitch=side * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.12)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.1 + (6 if side else 0))

    # Right fist around the bail, knuckles forward, the pail hanging below it.
    hang_r = s.hand_turn('r', (0, 0, -1), (1, 0, 0))
    reach_r = s.hand_turn('r', (0.0, 0.5, -1.0), (1, 0, 0))
    for name, frame in F.items():
        turn = hang_r if name in ('stand', 'step', 'rise', 'end') else reach_r
        s.key_world(frame, 'hand_r_ik_ctrl', RIGHT[name], turn)

    # Left hand braced on the forward knee throughout the kneel.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', (0, 0, -1), (-1, 0, 0))
    knee_l = kg.knee_turn(s)
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, knee_l)
    for name in ('dip', 'hold', 'drip'):
        s.key_world(F[name], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, knee_l)
    s.key_world(F['rise'] - 2, 'hand_l_ik_ctrl', (18.0, 28.0, 66.0), knee_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', side_l, hang_l)
    kg.key_knee_fingers(s, F['stand'], 0)
    for f in (F['kneel'], F['drip']):
        kg.key_knee_fingers(s, f)
    kg.key_knee_fingers(s, F['rise'] + 2, 0)

    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['reach'], 'arm_r_pv_ik_ctrl', (-70.0, 10.0, 70.0))
    s.key_world(F['drip'], 'arm_r_pv_ik_ctrl', (-70.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['drip'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    return s.bake(ANIM)


def report(anim):
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'pelvis'), frame / 30)
        r = b['hand_r'].translation
        lines.append(f"{name:8s} bail hand ({r.x:6.1f},{r.y:6.1f},{r.z:6.1f})  pelvis z {b['pelvis'].translation.z:5.1f}")
    return '\n'.join(lines)
