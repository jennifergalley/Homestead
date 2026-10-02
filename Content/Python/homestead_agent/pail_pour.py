"""Watering a garden square from the carved water pail (SM_WaterPail) for the MetaHuman heroine.

    from homestead_agent import pail_pour as pp
    anim = pp.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_PailPour
    print(pp.report(anim)) # wrist targets vs the baked wrists, and each arm's reach

She lifts the pail by its bail in front of her, takes its left side just below the rim in her
left hand, moves her right hand from the bail to the other side, and tips the pail forward like a
pot so the water runs from the pouring lip onto the square in a short sweep. Then she rights it,
takes the bail again and lowers it to her side.

The game hangs the pail from the right fist until EVENTS['take'], then lays it against the left
palm (``AHomesteadCharacter::UpdateWaterPail``): the pail's side is at the palm, its axis along
the left fingers, the right hand mirrored on the far side. From EVENTS['give'] it hangs from the
right fist again. The stream runs between EVENTS['pour'] and EVENTS['stop'].

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_PailPour'
ANIM = 'AN_HeroineMH_PailPour'

FRAMES = {
    'stand': 0, 'lift': 12, 'reach': 18, 'take': 26, 'tip': 36, 'sweep': 50, 'back': 64,
    'level': 72, 'rebail': 80, 'release': 88, 'lower': 96, 'end': 104,
}
# Seconds. take: the left hand has the pail (blend from hanging over 16..22); give: back on the bail
# (blend 80..86).
EVENTS = {'take': (16 / 30, 22 / 30), 'pour': FRAMES['tip'] / 30, 'stop': FRAMES['back'] / 30,
          'give': (80 / 30, 86 / 30)}
# SM_WaterPail: pivot at the bail grip, rim 14 cm below it, base 39 cm below; about 11 cm radius
# where she holds it, 18 cm below the pivot.
GRIP_DROP = 18.0
GRIP_RADIUS = 11.5
# Wrist to the palm point the game uses (0.6 of the way to the knuckles, 2 cm off the palm).
PALM_ALONG = 6.0
PALM_OFF = 2.0
# Right wrist to the bail pivot (the fist's grip centre) with the fingers down.
FIST_ALONG = 7.5
FIST_PALM = 3.0

# Where the pail is held between the hands: grip-line centre and forward tip (deg, 0 upright).
HOLD = {
    'reach': ((0.0, 36.0, 100.0), 0.0), 'take': ((0.0, 36.0, 100.0), 0.0),
    'tip': ((5.0, 40.0, 97.0), 100.0), 'sweep': ((0.0, 42.0, 96.0), 112.0),
    'back': ((-5.0, 40.0, 97.0), 105.0), 'level': ((0.0, 36.0, 100.0), 0.0),
    'rebail': ((0.0, 36.0, 100.0), 0.0),
}
# Where the stream lands, relative to her standing pose (the game steps her up so the square's
# middle lands here): below the lip at the middle of the sweep.
SPOT = (0.0, 43.0, 0.0)
# Her fingers lead the pail's axis forward by this much (deg) on the side holds: with the pail upright
# the palms press its sides with the fingers forward, like carrying a pot; fingers straight up its
# axis folded both wrists about 146 degrees back. With the lead equal to the pour's tip her hands hardly
# turn: the pail pivots between her palms as she tips it (between 'take' and 'tip', and back between
# 'back' and 'level'). The lead eases out as she tips it to pour
# (`AHomesteadCharacter::UpdateWaterPail` turns her fingers back by the same lead, PailFingerLead).
LEAD = {'reach': 100.0, 'take': 100.0, 'tip': 0.0, 'sweep': 0.0, 'back': 0.0, 'level': 100.0, 'rebail': 100.0}
# The right fist on the bail (knuckles forward, palm to her left) and the left hand ready to take
# the pail: fingers forward as well as down or up, so the wrists stay near straight.
BAIL_FINGERS = (0.0, 0.95, -0.3)
READY_L_FINGERS = (0.0, 1.0, 0.0)
LEAN = {'stand': 0, 'lift': 6, 'reach': 10, 'take': 14, 'tip': 18, 'sweep': 20, 'back': 20,
        'level': 14, 'rebail': 10, 'release': 8, 'lower': 4, 'end': 0}


def _axis(deg):
    r = math.radians(deg)
    return (0.0, math.sin(r), math.cos(r))


def _side_wrist(side, name):
    centre, deg = HOLD[name]
    f = _axis(deg + LEAD[name])
    sign = 1.0 if side == 'l' else -1.0
    out = GRIP_RADIUS + PALM_OFF
    return (centre[0] + sign * out - PALM_ALONG * f[0], centre[1] - PALM_ALONG * f[1],
            centre[2] - PALM_ALONG * f[2])


def _bail_wrist(name, fingers=BAIL_FINGERS):
    centre, deg = HOLD[name]
    pivot = (centre[0], centre[1], centre[2] + GRIP_DROP)
    n = math.sqrt(sum(c * c for c in fingers))
    f = [c / n for c in fingers]
    # Knuckles forward, palm facing her left (+X).
    return (pivot[0] - FIST_ALONG * f[0] - FIST_PALM, pivot[1] - FIST_ALONG * f[1], pivot[2] - FIST_ALONG * f[2])


def targets():
    right = {'stand': (-24.0, 5.0, 86.0), 'lift': (-10.0, 26.0, 108.0), 'reach': _bail_wrist('reach'),
             'rebail': _bail_wrist('rebail'), 'release': _bail_wrist('rebail'),
             'lower': (-18.0, 14.0, 94.0), 'end': (-24.0, 5.0, 86.0)}
    left = {'stand': (24.0, 5.0, 86.0), 'lift': (18.0, 22.0, 94.0),
            'release': (18.0, 26.0, 92.0), 'lower': (20.0, 10.0, 88.0), 'end': (24.0, 5.0, 86.0)}
    for name in ('take', 'tip', 'sweep', 'back', 'level'):
        right[name] = _side_wrist('r', name)
    for name in ('reach', 'take', 'tip', 'sweep', 'back', 'level', 'rebail'):
        left[name] = _side_wrist('l', name)
    return right, left


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        drop = LEAN[name] * 0.25
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, (0, LEAN[name] * 0.15, -drop)),
                    unreal.Rotator(roll=LEAN[name] * 0.4, pitch=0, yaw=0))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=LEAN[name] * 0.2)
        s.key_rotation(frame, 'neck_01_ctrl', roll=LEAN[name] * 0.2)
        s.key_rotation(frame, 'head_ctrl', roll=LEAN[name] * 0.3 + (10 if F['tip'] <= frame <= F['back'] else 0))
    for f in (F['stand'], F['end']):
        s.key_world(f, 'foot_l_ik_ctrl', kg.FOOT_L)
        s.key_world(f, 'foot_r_ik_ctrl', kg.FOOT_R)

    right, left = targets()
    hang_r = s.hand_turn('r', (0, 0, -1), (1, 0, 0))
    bail_r = s.hand_turn('r', BAIL_FINGERS, (1, 0, 0))
    hang_l = s.hand_turn('l', (0, 0, -1), (-1, 0, 0))
    ready_l = s.hand_turn('l', READY_L_FINGERS, (-1, 0, 0))

    def side(hand, name):
        f = _axis(HOLD[name][1] + LEAD[name])
        return s.hand_turn(hand, f, (1, 0, 0) if hand == 'r' else (-1, 0, 0))

    turns_r = {'stand': hang_r, 'lift': bail_r, 'reach': bail_r, 'rebail': bail_r, 'release': bail_r,
               'lower': bail_r, 'end': hang_r}
    turns_l = {'stand': hang_l, 'lift': ready_l, 'release': ready_l, 'lower': hang_l, 'end': hang_l}
    for name in ('take', 'tip', 'sweep', 'back', 'level'):
        turns_r[name] = side('r', name)
    for name in ('reach', 'take', 'tip', 'sweep', 'back', 'level', 'rebail'):
        turns_l[name] = side('l', name)
    for name, frame in F.items():
        s.key_world(frame, 'hand_r_ik_ctrl', right[name], turns_r[name])
        s.key_world(frame, 'hand_l_ik_ctrl', left[name], turns_l[name])
    # Elbows out to the sides while both hands hold the pail.
    for frame in (F['stand'], F['end']):
        s.key_world(frame, 'arm_r_pv_ik_ctrl', (-45.0, -30.0, 100.0))
        s.key_world(frame, 'arm_l_pv_ik_ctrl', (45.0, -30.0, 100.0))
    for name in ('take', 'tip', 'back', 'level'):
        s.key_world(F[name], 'arm_r_pv_ik_ctrl', (-70.0, -10.0, 95.0))
        s.key_world(F[name], 'arm_l_pv_ik_ctrl', (70.0, -10.0, 95.0))
    return s.bake(ANIM)


def report(anim):
    right, left = targets()
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, ('hand_r', 'hand_l'), frame / 30)
        r, l = b['hand_r'].translation, b['hand_l'].translation
        miss_r = (r - unreal.Vector(*right[name])).length()
        miss_l = (l - unreal.Vector(*left[name])).length()
        lines.append(f"{name:8s} miss R {miss_r:4.1f} L {miss_l:4.1f} cm  span {(r - l).length():5.1f}  "
                     f"L ({l.x:5.1f},{l.y:5.1f},{l.z:5.1f})")
    return '\n'.join(lines)