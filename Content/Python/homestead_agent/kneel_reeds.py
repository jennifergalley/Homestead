"""Kneel-and-cut-reeds work animation for the MetaHuman heroine: she saws a fistful of stems free.

    from homestead_agent import kneel_reeds as kr
    anim = kr.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_KneelCutReeds
    print(kr.report(anim)) # knife edge and left fist against the stems at each key

She steps her left foot forward and drops onto her right knee (as in the other kneeling gathers),
reaches out with her left hand and closes it around a bunch of reed stems a hand's width above the
mud, brings the flint knife in low with her right hand (blade out of the thumb side of the fist,
pointing to her left, edge toward the stems) and saws through them with short strokes along the
blade, easing forward as it bites. The stems come free, she lifts the bundle upright in her left
fist and rises holding it at her side. The game shows the knife in her right hand throughout, keeps
the reed clump on the ground until EVENTS['cut'] and shows the cut bundle in her left fist after it.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_KneelCutReeds'
ANIM = 'AN_HeroineMH_KneelCutReeds'

FRAMES = {
    'stand': 0, 'step': 10, 'kneel': 24, 'reach': 32, 'grab': 38, 'knife': 44,
    'saw1': 48, 'saw2': 53, 'saw3': 58, 'saw4': 63, 'saw5': 68, 'cut': 72,
    'lift': 82, 'rise': 98, 'end': 112,
}
# Seconds at which the game closes her left fist on the stems and when they come free.
EVENTS = {'grab': FRAMES['grab'] / 30, 'cut': FRAMES['cut'] / 30}
# Where the stems stand, relative to her standing pose; the game settles her so the reed clump's
# centre lands here (forward, right).
STEMS = (-10.0, 34.0)
GRASP_Z = 40.0
CUT_Z = 26.0
# The fist closes 7 cm along the fingers and 3 cm off the palm from the wrist.
FIST_ALONG = 7.0
FIST_PALM = 3.0
# The knife's blade leaves the thumb side of the fist; its middle is ~8 cm from the fist centre.
BLADE_MID = 8.0


def _norm(v):
    length = sum(c * c for c in v) ** 0.5
    return tuple(c / length for c in v)


def _wrist_for_fist(fist, fingers, palm):
    f, p = _norm(fingers), _norm(palm)
    return tuple(fist[i] - f[i] * FIST_ALONG - p[i] * FIST_PALM for i in range(3))


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    down = (0, 0, -1)
    fwd = (0, 1, 0)
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)

    # Lean in over the stems while cutting (hands work low in front of the forward knee).
    lean = {F['stand']: 0, F['step']: 8, F['kneel']: 30, F['reach']: 46, F['grab']: 52, F['knife']: 56,
            F['saw1']: 58, F['saw2']: 59, F['saw3']: 58, F['saw4']: 59, F['saw5']: 58, F['cut']: 54,
            F['lift']: 34, F['rise']: 8, F['end']: 0}

    def tilt(frame):
        return unreal.Rotator(roll=lean.get(frame, 0) * 0.55, pitch=0, yaw=0)

    s.key_world(F['stand'], 'body_ctrl', kg.BODY_STAND, tilt(F['stand']))
    s.key_world(F['step'], 'body_ctrl', (0.0, 6.0, 96.0), tilt(F['step']))
    for name in ('kneel', 'reach', 'grab', 'knife', 'saw1', 'saw2', 'saw3', 'saw4', 'saw5', 'cut', 'lift'):
        sink = (0, 0, -3.0) if name.startswith('saw') or name in ('grab', 'knife', 'cut') else (0, 0, 0)
        # A small rock of the hips with each stroke.
        rock = (0, 0.8 if name in ('saw1', 'saw3', 'saw5') else -0.8 if name.startswith('saw') else 0, 0)
        s.key_world(F[name], 'body_ctrl', kg._add(kg._add(kg.BODY_KNEEL, sink), rock), tilt(F[name]))
    s.key_world(F['rise'], 'body_ctrl', (0.0, 3.0, 101.0), tilt(F['rise']))
    s.key_world(F['end'], 'body_ctrl', kg.BODY_STAND, tilt(F['end']))

    # Feet as in the kneeling gathers: left steps forward, right slides back onto tucked toes.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 8, 10)))
    s.key_world(F['step'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['lift'] + 2, 'foot_l_ik_ctrl', kg.FOOT_L_FORWARD)
    s.key_world(F['rise'] - 4, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 12, 9)))
    s.key_world(F['rise'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    toes = unreal.Rotator(roll=45, pitch=0, yaw=0)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['step'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['lift'], 'foot_r_ik_ctrl', kg.FOOT_R_KNEEL, toes)
    s.key_world(F['rise'] - 2, 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['kneel'], 'leg_r_pv_ik_ctrl', (-18.0, 60.0, 20.0))
    s.key_world(F['kneel'], 'leg_l_pv_ik_ctrl', (20.0, 90.0, 60.0))

    # Spine: lean, a slight turn toward the knife hand while sawing, eyes on the cut.
    for frame, deg in lean.items():
        turn = -6 if F['knife'] <= frame <= F['cut'] else 0
        # Bend a little to her right, over the stems beside the forward knee.
        side = (10 if F['reach'] <= frame <= F['cut'] else 0) * kg.BEND_SIGN
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=deg * 0.15, pitch=side * 0.3, yaw=turn * 0.3)
        s.key_rotation(frame, 'neck_01_ctrl', roll=deg * 0.15)
        s.key_rotation(frame, 'head_ctrl', roll=deg * 0.1 + (3 if F['grab'] <= frame <= F['cut'] else 0), yaw=turn * 0.5)

    # Left hand: around the stems, knuckles forward, palm toward her right, thumb up.
    side_l = (24.0, 5.0, 86.0)
    hang_l = s.hand_turn('l', down, (-1, 0, 0))
    grip_fingers_l = (0.0, 1.0, -0.1)
    grip_palm_l = (-1.0, 0.0, 0.0)
    grip_l = s.hand_turn('l', grip_fingers_l, grip_palm_l)
    grasp = (STEMS[0], STEMS[1], GRASP_Z)
    wrist_l = _wrist_for_fist(grasp, grip_fingers_l, grip_palm_l)
    s.key_world(F['stand'], 'hand_l_ik_ctrl', side_l, hang_l)
    s.key_world(F['kneel'], 'hand_l_ik_ctrl', kg.KNEE_WRIST_L, kg.knee_turn(s))
    s.key_world(F['reach'], 'hand_l_ik_ctrl', kg._add(wrist_l, (3, -2, 10)), grip_l)
    s.key_world(F['grab'], 'hand_l_ik_ctrl', wrist_l, grip_l)
    # She holds the bunch taut, drawing it back a touch with each pull of the blade.
    for name in ('knife', 'saw2', 'saw4'):
        s.key_world(F[name], 'hand_l_ik_ctrl', kg._add(wrist_l, (0, -1.0, 0)), grip_l)
    for name in ('saw1', 'saw3', 'saw5'):
        s.key_world(F[name], 'hand_l_ik_ctrl', kg._add(wrist_l, (0, -0.4, 0)), grip_l)
    s.key_world(F['cut'], 'hand_l_ik_ctrl', kg._add(wrist_l, (0, -3.0, 3)), grip_l)
    # The freed bundle comes up upright in her fist, then hangs at her left side as she rises.
    s.key_world(F['lift'], 'hand_l_ik_ctrl', (10.0, 24.0, 58.0), grip_l)
    s.key_world(F['rise'], 'hand_l_ik_ctrl', (24.0, 12.0, 86.0), grip_l)
    s.key_world(F['end'], 'hand_l_ik_ctrl', (25.0, 9.0, 88.0), grip_l)

    # Right hand: fist palm down, knuckles toward the stems, blade pointing left under the left fist.
    side_r = (-24.0, 5.0, 86.0)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    saw_fingers_r = (0.1, 1.0, -0.25)
    saw_palm_r = (0.0, 0.25, -1.0)
    saw_r = s.hand_turn('r', saw_fingers_r, saw_palm_r)
    # Fist centre so the blade's middle meets the stems, just short of them.
    fist_r = (STEMS[0] - BLADE_MID, STEMS[1] - 4.0, CUT_Z)
    wrist_r = _wrist_for_fist(fist_r, saw_fingers_r, saw_palm_r)
    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['kneel'], 'hand_r_ik_ctrl', (-24.0, 20.0, 44.0), hang_r)
    s.key_world(F['grab'], 'hand_r_ik_ctrl', kg._add(wrist_r, (-8, -8, 14)), saw_r)
    s.key_world(F['knife'], 'hand_r_ik_ctrl', kg._add(wrist_r, (-2, -1, 1)), saw_r)
    # Strokes along the blade: pull back toward her right, push forward to her left, biting deeper.
    strokes = (('saw1', 6.0, 0.5), ('saw2', -5.0, 1.2), ('saw3', 6.0, 2.0), ('saw4', -5.0, 2.8), ('saw5', 5.0, 3.6))
    for name, x, bite in strokes:
        s.key_world(F[name], 'hand_r_ik_ctrl', kg._add(wrist_r, (x * -1, bite, 0)), saw_r)
    # Through: the blade follows through past the stems.
    s.key_world(F['cut'], 'hand_r_ik_ctrl', kg._add(wrist_r, (-6.0, 7.0, 2.0)), saw_r)
    s.key_world(F['lift'], 'hand_r_ik_ctrl', (-22.0, 22.0, 50.0), s.hand_turn('r', (0.2, 0.8, -0.5), (0.3, 0.2, -0.9)))
    s.key_world(F['rise'], 'hand_r_ik_ctrl', (-24.0, 8.0, 84.0), hang_r)
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)

    # Elbows: right out to her side and back while sawing low; left out and forward holding the bunch.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_r_pv_ik_ctrl', (-60.0, 0.0, 70.0))
    s.key_world(F['knife'], 'arm_r_pv_ik_ctrl', (-65.0, 5.0, 60.0))
    s.key_world(F['cut'], 'arm_r_pv_ik_ctrl', (-65.0, 5.0, 60.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    s.key_world(F['stand'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    s.key_world(F['kneel'], 'arm_l_pv_ik_ctrl', (60.0, 10.0, 70.0))
    s.key_world(F['grab'], 'arm_l_pv_ik_ctrl', (60.0, 20.0, 60.0))
    s.key_world(F['cut'], 'arm_l_pv_ik_ctrl', (60.0, 20.0, 60.0))
    s.key_world(F['rise'], 'arm_l_pv_ik_ctrl', (50.0, -30.0, 100.0))
    s.key_world(F['end'], 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    return s.bake(ANIM)


def report(anim):
    """Left fist and blade middle against the stems (component space) at each key."""
    lines = []
    stems = unreal.Vector(STEMS[0], STEMS[1], 0)
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'middle_01_r', 'index_01_r', 'pinky_01_r',
                                     'hand_l', 'middle_01_l', 'index_01_l', 'pinky_01_l'), frame / 30)
        out = []
        for side in ('l', 'r'):
            hand = b[f'hand_{side}'].translation
            fingers = (b[f'middle_01_{side}'].translation - hand).normal()
            across = (b[f'index_01_{side}'].translation - b[f'pinky_01_{side}'].translation).normal()
            palm = across.cross(fingers).normal() * (-1 if side == 'l' else 1)
            fist = hand + fingers * FIST_ALONG + palm * FIST_PALM
            if side == 'l':
                d = unreal.Vector(fist.x - stems.x, fist.y - stems.y, 0).length()
                out.append(f"L fist ({fist.x:5.1f},{fist.y:5.1f},{fist.z:5.1f}) off stems {d:4.1f}")
            else:
                blade = fist + across * BLADE_MID
                d = unreal.Vector(blade.x - stems.x, blade.y - stems.y, 0).length()
                out.append(f"blade ({blade.x:5.1f},{blade.y:5.1f},{blade.z:5.1f}) off stems {d:4.1f} dir ({across.x:4.2f},{across.y:4.2f},{across.z:4.2f})")
        lines.append(f"{name:6s} " + '  '.join(out))
    return '\n'.join(lines)
