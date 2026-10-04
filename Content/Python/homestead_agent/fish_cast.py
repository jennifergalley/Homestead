"""Bank fishing with the hazel pole for the MetaHuman heroine: cast, wait, bite, fight, strike, catch, miss.

    from homestead_agent import fish_cast as fc
    anim = fc.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_Fishing
    print(fc.report(anim)) # wrist, rod direction and left-hand contact at each key

One 240-frame clip holds every segment; UHomesteadAnimInstance plays and loops them by the seconds in
Source/SurvivalGame/HomesteadFishingPresentation.h, which must match SEGMENTS/EVENTS below. Boundary
poses are shared so the loops are seamless: Cast ends on WAIT; Wait and Bite start and end on WAIT;
Fight starts and ends on FIGHT; Strike starts and ends on FIGHT; Catch starts on FIGHT and holds its last
pose; Miss starts on WAIT.

Technique (two-handed overhead cast with a reel-less pole): the right hand grips at the binding, the left
hand holds the butt 14 cm below. She rocks her weight back as the rod rises to one o'clock, then drives
forward from the hips and chest, the right hand pushing and the left pulling the butt to her belly, and the
line releases with the rod at eleven o'clock (EVENTS['release']). The rod follows through low towards the
water and settles at about 35 degrees for the wait. On a bite she tenses and dips the tip; the fight holds
the rod high with her weight back; the strike is a short sharp lift from elbow and shoulder; the catch
lifts the fish clear (EVENTS['lift'], when the game awards it), then the left hand lets go of the butt and
comes up to hold the fish at chest height while the right lowers the pole.

The pole sits in FHandGrip: its tip runs along the right hand's pinky-to-index axis, so each key names the
rod direction and the palm direction; the fingers follow from the diagonal grip (ROD_TILT).

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_Fishing'
ANIM = 'AN_HeroineMH_Fishing'
FPS = 30
# Clip seconds (HomesteadFishingPresentation.h).
SEGMENTS = {'cast': (0.0, 1.6), 'wait': (1.6, 3.6), 'bite': (3.6, 4.4), 'fight': (4.4, 5.6),
            'strike': (5.6, 6.0), 'catch': (6.0, 7.4), 'miss': (7.4, 8.0)}
EVENTS = {'release': 0.62, 'splash': 1.1, 'lift': 6.4}
END = 240

# Key name -> frame. Shared boundary poses reuse the same POSE entry.
KEYS = [
    ('ready', 0), ('lift_rod', 7), ('back', 14), ('release', 19), ('follow', 26), ('settle', 38),
    ('wait', 48), ('wait_breathe', 78), ('wait', 108),
    ('bite_tense', 116), ('bite_dip', 123), ('wait', 131),
    ('fight', 132), ('fight_sway', 150), ('fight', 168),
    ('strike_up', 173), ('fight', 180),
    ('catch_lift', 192), ('catch_swing', 202), ('catch_hold', 212), ('catch_hold', 221),
    ('wait', 222), ('miss_reel', 230), ('miss_end', 240),
]
# Segments meeting at one frame (Bite/Fight at 132, Catch/Miss at 222) can't share a key: the earlier
# segment's closing key sits one frame before. The engine never plays across those boundaries.

# Pose: wrist (cm), rod direction, right palm direction, right elbow pole, body (pelvis offset, chest twist
# deg + to her right, forward lean deg), head (down deg, yaw deg), left hand ('butt', pole) or
# ((wrist), fingers, palm, pole).
POSE = {
    'ready': dict(wrist=(-17, 28, 108), rod=(0.04, 0.72, 0.69), palm=(1, 0, 0.1), pole=(-60, -10, 95),
                  body=((0, 1, -1), 4, 4), head=(10, 0)),
    'lift_rod': dict(wrist=(-19, 22, 128), rod=(0.0, 0.2, 0.98), palm=(1, 0, 0.1), pole=(-65, -15, 105),
                     body=((0, -2, -1), 8, 0), head=(6, -3)),
    'back': dict(left_tilt=16.0, wrist=(-21, 12, 145), rod=(-0.05, -0.62, 0.78), palm=(1, 0, 0.15), pole=(-70, -25, 120),
                 body=((0, -4, -1), 12, -4), head=(5, -5)),
    'release': dict(left_tilt=22.0, wrist=(-18, 32, 132), rod=(0.04, 0.5, 0.87), palm=(1, 0, 0.05), pole=(-65, -10, 105),
                    body=((0, 4, -2), -6, 8), head=(8, 3)),
    'follow': dict(wrist=(-16, 38, 116), rod=(0.06, 0.87, 0.48), palm=(1, 0, 0.05), pole=(-60, -10, 95),
                   body=((0, 5, -3), -8, 10), head=(12, 4)),
    'settle': dict(wrist=(-16, 33, 108), rod=(0.07, 0.83, 0.55), palm=(1, 0, 0.08), pole=(-60, -10, 92),
                   body=((0, 3, -2), -3, 6), head=(12, 2)),
    'wait': dict(wrist=(-16, 31, 106), rod=(0.08, 0.82, 0.57), palm=(1, 0, 0.08), pole=(-60, -10, 92),
                 body=((0, 2, -1.5), -2, 5), head=(12, 1)),
    'wait_breathe': dict(wrist=(-16, 31, 107), rod=(0.09, 0.82, 0.56), palm=(1, 0, 0.08), pole=(-60, -10, 92),
                         body=((0, 2, -1), -1, 4), head=(11, 2)),
    'bite_tense': dict(wrist=(-15, 33, 108), rod=(0.08, 0.84, 0.53), palm=(1, 0, 0.08), pole=(-60, -8, 92),
                       body=((0, 3, -2.5), -2, 8), head=(14, 1)),
    'bite_dip': dict(wrist=(-15, 34, 104), rod=(0.08, 0.88, 0.47), palm=(1, 0, 0.08), pole=(-60, -8, 90),
                     body=((0, 3.5, -3), -2, 9), head=(15, 1)),
    'fight': dict(wrist=(-14, 27, 124), rod=(0.12, 0.42, 0.9), palm=(1, 0, 0.0), pole=(-62, -12, 100),
                  body=((0, -3, -2), 4, -5), head=(9, 0)),
    'fight_sway': dict(wrist=(-18, 26, 122), rod=(-0.18, 0.45, 0.87), palm=(1, 0, 0.0), pole=(-65, -12, 100),
                       body=((-1, -3.5, -2.5), 10, -6), head=(9, -6)),
    'strike_up': dict(wrist=(-15, 22, 138), rod=(0.08, 0.05, 0.99), palm=(1, 0, 0.0), pole=(-66, -18, 112),
                      body=((0, -5, -2), 6, -8), head=(6, 0)),
    'catch_lift': dict(wrist=(-16, 22, 140), rod=(0.06, -0.08, 0.99), palm=(1, 0, 0.0), pole=(-66, -18, 115),
                       body=((0, -4, -1.5), 6, -6), head=(2, 0)),
    'catch_swing': dict(wrist=(-20, 20, 128), rod=(-0.35, -0.15, 0.92), palm=(1, 0, 0.0), pole=(-66, -15, 108),
                        body=((0, -1, -1), 12, 0), head=(14, 10),
                        left=((6, 30, 118), (-0.1, 0.9, 0.1), (-0.6, 0.0, 0.8), (55, -5, 85))),
    'catch_hold': dict(wrist=(-22, 14, 108), rod=(-0.35, 0.3, 0.89), palm=(0.9, 0, 0.1), pole=(-62, -12, 95),
                       body=((0, 0, -0.5), 6, 2), head=(18, 14),
                       left=((4, 32, 120), (-0.15, 0.95, 0.1), (-0.55, 0.0, 0.85), (55, -5, 85))),
    'miss_reel': dict(wrist=(-17, 26, 126), rod=(0.06, 0.4, 0.91), palm=(1, 0, 0.05), pole=(-62, -12, 100),
                      body=((0, -1, -1), 2, 0), head=(10, 0)),
    'miss_end': dict(wrist=(-16, 30, 110), rod=(0.08, 0.72, 0.69), palm=(1, 0, 0.08), pole=(-60, -10, 94),
                     body=((0, 1, -1), 0, 2), head=(16, 0)),
}

# Grip geometry (HandGripTransform): grip centre = wrist + fingers * GRIP_ALONG + palm * GRIP_PALM; the left
# hand holds the butt BUTT_HOLD cm below the grip centre along the rod.
GRIP_ALONG = 7.2
GRIP_PALM = 3.3
BUTT_HOLD = 14.0
# A rod lies diagonally across the fist, not square like a haft: its tip leans ROD_TILT degrees from the
# knuckle line toward the fingers (out past the thumb, butt toward the heel of the hand), which keeps the
# wrist near neutral with the rod forward and up. HomesteadCharacterAppearance.cpp tilts the held pole to match.
ROD_TILT = 40.0
# The lower hand wraps the butt nearer square: its forearm meets the rod lower down.
LEFT_TILT = 28.0
LEFT_POLE = (55.0, -5.0, 85.0)
TWIST_SIGN = 1
FOOT_L = (15.0, 7.0, 8.6)
FOOT_R = (-14.0, -4.0, 8.6)


def _v(t):
    return unreal.Vector(*t)


def _frame(rod, palm, tilt=None):
    """Orthonormal rod, palm and fingers for a hand; rod = knuckle line turned 	ilt (ROD_TILT) toward the fingers."""
    tilt = ROD_TILT if tilt is None else tilt
    r = _v(rod).normal()
    p = _v(palm)
    p = (p - r * p.dot(r)).normal()
    c, s = math.cos(math.radians(tilt)), math.sin(math.radians(tilt))
    return r, p, (r.cross(p) * c + r * s).normal()


def rod_from_hand(across, along):
    """The held rod's direction from the hand's knuckle line and finger direction."""
    c, s = math.cos(math.radians(ROD_TILT)), math.sin(math.radians(ROD_TILT))
    return (across * c + along * s).normal()


def _left(pose):
    """Left wrist, fingers, palm and pole: on the butt below the right hand, or the pose's explicit hold."""
    if 'left' in pose:
        wrist, fingers, palm, pole = pose['left']
        return wrist, fingers, palm, pole
    r, p, f = _frame(pose['rod'], pose['palm'])
    grip = _v(pose['wrist']) + f * GRIP_ALONG + p * GRIP_PALM
    point = grip - r * BUTT_HOLD
    palm_l = p * -1.0
    f = _frame(pose['rod'], pose['palm'], pose.get('left_tilt', LEFT_TILT))[2]
    wrist = point - f * GRIP_ALONG - palm_l * GRIP_PALM
    return (wrist.x, wrist.y, wrist.z), (f.x, f.y, f.z), (palm_l.x, palm_l.y, palm_l.z), LEFT_POLE


def _keys():
    return list(KEYS)


def build():
    s = ra.Session(SEQUENCE, frames=END)
    for f in (0, END):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in _keys():
        pose = POSE[name]
        drop, twist, lean = pose['body']
        twist *= TWIST_SIGN
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop),
                    unreal.Rotator(roll=lean * 0.4, pitch=0, yaw=twist * 0.35))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=lean * 0.2, yaw=twist * 0.22)
        down, look = pose['head']
        # Eyes on the float: the head undoes most of the chest's turn.
        s.key_rotation(frame, 'neck_01_ctrl', roll=down * 0.35, yaw=(look - twist) * 0.4)
        s.key_rotation(frame, 'head_ctrl', roll=down * 0.65, yaw=(look - twist) * 0.5)
        r, p, f = _frame(pose['rod'], pose['palm'])
        s.key_world(frame, 'hand_r_ik_ctrl', pose['wrist'], s.hand_turn('r', (f.x, f.y, f.z), (p.x, p.y, p.z)))
        s.key_world(frame, 'arm_r_pv_ik_ctrl', pose['pole'])
        wrist_l, fingers_l, palm_l, pole_l = _left(pose)
        s.key_world(frame, 'hand_l_ik_ctrl', wrist_l, s.hand_turn('l', fingers_l, palm_l))
        s.key_world(frame, 'arm_l_pv_ik_ctrl', pole_l)
    for frame in (0, END):
        s.key_world(frame, 'foot_l_ik_ctrl', FOOT_L)
        s.key_world(frame, 'foot_r_ik_ctrl', FOOT_R)
    events = {name: frame for name, frame in _keys()}
    events.update({name: round(sec * FPS) for name, sec in EVENTS.items()})
    contacts = [round(sec * FPS) for sec in EVENTS.values()] + [173]
    return s.bake(ANIM, events=events, contacts=contacts)


def report(anim):
    """Baked wrist position, rod direction and left-hand gap to the butt at each key."""
    lines = []
    for name, frame in _keys():
        b = ra.bone_positions(anim, ('hand_r', 'middle_01_r', 'index_01_r', 'pinky_01_r', 'hand_l'), frame / FPS)
        hand = b['hand_r'].translation
        along = (b['middle_01_r'].translation - hand).normal()
        across = b['index_01_r'].translation - b['pinky_01_r'].translation
        across = (across - along * across.dot(along)).normal()
        palm = along.cross(across)
        want = _v(POSE[name]['rod']).normal()
        grip = hand + (b['middle_01_r'].translation - hand) * 0.75 + palm * GRIP_PALM
        rod = rod_from_hand(across, along)
        gap = ''
        if 'left' not in POSE[name]:
            lw = _v(_left(POSE[name])[0])
            gap = f'  left wrist err {(b["hand_l"].translation - lw).length():4.1f}'
        err = math.degrees(math.acos(max(-1.0, min(1.0, rod.dot(want)))))
        lines.append(f'{name:12s} f{frame:3d} wrist ({hand.x:6.1f},{hand.y:6.1f},{hand.z:6.1f}) want {POSE[name]["wrist"]}  '
                     f'rod ({rod.x:5.2f},{rod.y:5.2f},{rod.z:5.2f}) err {err:4.1f} deg  '
                     f'tip z {(grip + rod * 175).z:6.1f}{gap}')
    return '\n'.join(lines)
