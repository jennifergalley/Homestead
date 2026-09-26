"""Hand-to-mouth eating for the MetaHuman heroine: a berry (or a piece of root) from the hip pouch.

    from homestead_agent import eat_berry as eb
    anim = eb.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_Eat
    print(eb.report(anim)) # pinch point vs the mouth at each key

The game layers this clip over whatever she is doing (standing or walking) from the right clavicle
and the neck down, so only the right arm and head keys matter. She drops her right hand to the
forage pouch at her hip, fingers in, pinches a berry, brings it up to her lips palm-in, pops it in
with a slight dip of the head to meet it, lowers the hand, and chews with a couple of small nods.
The game shows the food in her fingers between EVENTS['pick'] and EVENTS['bite'].

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import kneel_pouch as kp

SEQUENCE = 'LS_Eat'
ANIM = 'AN_HeroineMH_Eat'

FRAMES = {
    'stand': 0, 'reach': 9, 'dip': 15, 'pinch': 19, 'lift': 27, 'mouth': 36, 'bite': 40,
    'chew1': 45, 'lower': 50, 'chew2': 54, 'end': 62,
}
# Seconds at which the game shows the food in her fingers and when it goes into her mouth.
EVENTS = {'pick': FRAMES['pinch'] / 30, 'bite': FRAMES['bite'] / 30}
# Lips in the standing pose (head joint (0, 0.8, 161.5), mouth a little below and forward).
MOUTH = (0.0, 10.5, 156.5)
# Food held between thumb and fingertips: 8 cm along the fingers, 2.5 cm off the palm.
PINCH_ALONG = 8.0
PINCH_PALM = 2.5
# At the mouth her fingers point up and a little in toward her face, palm facing her.
MOUTH_FINGERS = (0.35, -0.2, 0.92)
MOUTH_PALM = (0.25, -0.95, 0.15)


def _norm(v):
    length = sum(c * c for c in v) ** 0.5
    return tuple(c / length for c in v)


def _wrist_for_pinch(pinch, fingers, palm):
    f = _norm(fingers)
    p = _norm(palm)
    return tuple(pinch[i] - f[i] * PINCH_ALONG - p[i] * PINCH_PALM for i in range(3))


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    down = (0, 0, -1)
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
        s.key_world(f, 'body_ctrl', kg.BODY_STAND)
        s.key_world(f, 'foot_l_ik_ctrl', kg.FOOT_L)
        s.key_world(f, 'foot_r_ik_ctrl', kg.FOOT_R)
        s.key_world(f, 'hand_l_ik_ctrl', (24.0, 5.0, 86.0), s.hand_turn('l', down, (-1, 0, 0)))
        s.key_world(f, 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))

    side_r = (-24.0, 5.0, 86.0)
    hang_r = s.hand_turn('r', down, (1, 0, 0))
    # Fingers down into the pouch, palm toward her hip (as when stowing forage).
    stow_r = s.hand_turn('r', (-0.1, 0.15, -1), (1, 0, 0))
    lift_fingers = (0.1, 0.15, 0.98)
    lift_palm = (0.2, -0.95, 0.2)
    lift_r = s.hand_turn('r', lift_fingers, lift_palm)
    mouth_r = s.hand_turn('r', MOUTH_FINGERS, MOUTH_PALM)
    at_lips = kg._add(MOUTH, (-1.0, 3.0, 0.0))
    in_lips = kg._add(MOUTH, (-0.5, 1.0, 0.0))

    s.key_world(F['stand'], 'hand_r_ik_ctrl', side_r, hang_r)
    s.key_world(F['reach'], 'hand_r_ik_ctrl', kg._add(kp.STOW_WRIST, (-2, 4, 8)), stow_r)
    s.key_world(F['dip'], 'hand_r_ik_ctrl', kp.STOW_WRIST, stow_r)
    s.key_world(F['pinch'], 'hand_r_ik_ctrl', kg._add(kp.STOW_WRIST, (0, 0, 2)), stow_r)
    s.key_world(F['lift'], 'hand_r_ik_ctrl', (-14.0, 24.0, 120.0), lift_r)
    s.key_world(F['mouth'], 'hand_r_ik_ctrl', _wrist_for_pinch(at_lips, MOUTH_FINGERS, MOUTH_PALM), mouth_r)
    s.key_world(F['bite'], 'hand_r_ik_ctrl', _wrist_for_pinch(in_lips, MOUTH_FINGERS, MOUTH_PALM), mouth_r)
    s.key_world(F['chew1'], 'hand_r_ik_ctrl', (-12.0, 23.0, 128.0), lift_r)
    s.key_world(F['lower'], 'hand_r_ik_ctrl', (-21.0, 9.0, 100.0), s.hand_turn('r', (0, 0.35, -0.94), (1, 0, 0)))
    s.key_world(F['end'], 'hand_r_ik_ctrl', side_r, hang_r)
    # Elbow back and out while she reaches into the pouch, then down and forward under the hand.
    s.key_world(F['stand'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
    # Straight back, not flared out to the side, as the hand drops into the pouch at her hip.
    s.key_world(F['dip'], 'arm_r_pv_ik_ctrl', (-38.0, -50.0, 96.0))
    s.key_world(F['pinch'], 'arm_r_pv_ik_ctrl', (-38.0, -50.0, 96.0))
    # Elbow down and a little out in front of her ribs as the hand comes up to her lips.
    s.key_world(F['lift'], 'arm_r_pv_ik_ctrl', (-38.0, 10.0, 70.0))
    s.key_world(F['bite'], 'arm_r_pv_ik_ctrl', (-36.0, 26.0, 88.0))
    s.key_world(F['lower'], 'arm_r_pv_ik_ctrl', (-45.0, -15.0, 90.0))
    s.key_world(F['end'], 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))

    # Head: a glance down at the pouch, then a dip to meet the berry, and two small chewing nods.
    head = {F['stand']: (0, 0), F['dip']: (10, -8), F['pinch']: (9, -6), F['lift']: (2, -2),
            F['mouth']: (6, -3), F['bite']: (8, -3), F['chew1']: (1, 0), F['lower']: (3, 0),
            F['chew2']: (0, 0), 58: (2, 0), F['end']: (0, 0)}
    for frame, (roll, yaw) in head.items():
        s.key_rotation(frame, 'neck_01_ctrl', roll=roll * 0.3, yaw=yaw * 0.4)
        s.key_rotation(frame, 'head_ctrl', roll=roll * 0.7, yaw=yaw * 0.6)
    return s.bake(ANIM)


def report(anim):
    """Pinch point (from the baked hand) against the mouth at each key."""
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'middle_01_r', 'index_01_r', 'pinky_01_r', 'head'), frame / 30)
        hand = b['hand_r'].translation
        fingers = (b['middle_01_r'].translation - hand).normal()
        across = (b['index_01_r'].translation - b['pinky_01_r'].translation).normal()
        palm = fingers.cross(across).normal()
        pinch = hand + fingers * PINCH_ALONG + palm * PINCH_PALM
        head = b['head'].translation
        lines.append(f"{name:6s} pinch ({pinch.x:6.1f},{pinch.y:6.1f},{pinch.z:6.1f})  "
                     f"to mouth {(pinch - unreal.Vector(*MOUTH)).length():5.1f} cm  head ({head.x:5.1f},{head.y:5.1f},{head.z:6.1f})")
    return '\n'.join(lines)
