"""Two-handed axe felling for the MetaHuman heroine.

    from homestead_agent import axe_fell as af
    anim = af.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_AxeFell
    print(af.report(anim)) # grip centres, haft directions and hand spacing at each key

Technique, after felling references: a stance side-on to the trunk, lead foot forward and knees
soft; the axe addressed against the trunk to set the distance; lifted over the shoulder with the
top hand choked up under the head; then the power flows hips, chest, shoulders, arms as the
weight moves onto the lead foot, the top hand sliding down the haft to meet the bottom hand at
impact, which lands a 45-degree cut into the trunk at waist height. After each strike she rocks
the bit free and lifts again. She chops right-handed, from over her right shoulder: the left
hand stays at the knob and the right hand, choked up under the head on the backswing, slides
down the haft to meet it, so her right arm draws back on her right side and drives down and
across to her left. The game carries the hatchet in the left fist while she fells and lays the
haft through both fists (``AHomesteadCharacter::UpdateFellingHatchet``).

The clip holds two identical strokes. ``AHomesteadCharacter`` repeats the stroke cycle
(``LOOP_START``..``LOOP_START + LOOP``) for as many strokes as the tree needs and keeps the intro
and recovery, so strikes land at ``FRAMES['strike1'] + k * LOOP`` (``FellTiming`` in C++).

The hatchet (SM_FlintHatchet) authors its pivot at the knob hand's grip centre with the head
along +Z and the edge toward -Y, the same convention as the machete: in the right hand's closed
grip the haft runs pinky-to-index and the edge faces along the knuckles. Each key below names the
grip centre, the haft direction and the left hand's distance up the haft; the edge follows from
the swing plane.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg

SEQUENCE = 'LS_AxeFell'
ANIM = 'AN_HeroineMH_AxeFell'

FRAMES = {
    'stand': 0, 'address': 12, 'lift1': 20, 'back1': 26, 'strike1': 34, 'bite1': 38, 'rock1': 44,
    'lift2': 56, 'back2': 62, 'strike2': 70, 'bite2': 74, 'rock2': 80, 'recover': 92, 'end': 106,
}
LOOP_START = FRAMES['rock1']
LOOP = FRAMES['rock2'] - FRAMES['rock1']

# Grip centre of the left (knob) hand, haft direction, right hand's distance up the haft (cm).
# At impact the haft points forward and a little to her right, so the bit travels forward, across
# to her left and down into the trunk (a 45-degree notch face), not along it; the tree therefore
# stands a little to her right (FELL_BIT). The wind-up carries the head behind her right shoulder.
STROKE = {
    'lift': ((-16.0, 22.0, 124.0), (-0.3, 0.05, 0.95), 30.0),
    # Top of the backswing: the knob hand stays in front of her right shoulder (so the left arm
    # crosses in front of her chest rather than through it) while the right hand, choked up under
    # the head, draws back and up beside her right ear with the head behind that shoulder.
    'back': ((-14.0, 16.0, 138.0), (-0.4, -0.55, 0.73), 32.0),
    'strike': ((-8.0, 30.0, 102.0), (-0.65, 0.72, -0.2), 11.0),
    'bite': ((-7.0, 31.0, 99.0), (-0.63, 0.72, -0.28), 11.0),
    'rock': ((-10.0, 25.0, 104.0), (-0.62, 0.76, -0.1), 13.0),
}
GRIP = {
    'address': ((-8.0, 27.0, 100.0), (-0.63, 0.75, -0.15), 18.0),
    'recover': ((-12.0, 24.0, 94.0), (-0.3, 0.8, -0.5), 8.0),
}
# SM_FlintHatchet (report.json attach.edge_centre): the bit's centre from the knob grip pivot.
HEAD_ALONG = 43.0
HEAD_EDGE = 13.9
for n in (1, 2):
    for part, value in STROKE.items():
        GRIP[f'{part}{n}'] = value
# The carry she starts and ends in (the machete's resting hand: wrist, blade, edge). The runtime
# FHandGrip carry deviation is layered on top while the clip blends in and out.
WRIST_R_STAND = ((-24.0, 6.0, 86.0), (0.05, 0.8, -0.6), (0.0, -0.6, -0.8))
WRIST_L_STAND = (24.0, 5.0, 86.0)
# Elbow poles: the sliding (right) hand's elbow draws up and back on her right side on the
# backswing and tucks toward her ribs at impact; the knob hand's elbow stays down and out to her left.
POLE_R = {
    'stand': (-60.0, -10.0, 90.0), 'address': (-50.0, 10.0, 75.0), 'lift': (-70.0, 0.0, 120.0),
    'back': (-75.0, -15.0, 135.0), 'strike': (-45.0, 15.0, 72.0), 'bite': (-45.0, 15.0, 70.0),
    'rock': (-50.0, 10.0, 76.0), 'recover': (-55.0, -5.0, 85.0), 'end': (-60.0, -10.0, 90.0),
}
POLE_L = {
    'stand': (60.0, -10.0, 90.0), 'address': (45.0, 15.0, 75.0), 'lift': (30.0, 30.0, 100.0),
    'back': (20.0, 40.0, 105.0), 'strike': (40.0, 20.0, 72.0), 'bite': (40.0, 20.0, 70.0),
    'rock': (45.0, 15.0, 76.0), 'recover': (60.0, -5.0, 85.0), 'end': (60.0, -10.0, 90.0),
}
# Pelvis offset (cm), torso twist (deg, + turns her chest to her right) and forward lean (deg).
# The backswing winds the chest and hips to her right over the back (right) foot; the strike drives
# them through to her left onto the lead (left) foot, bent forward into the cut.
BODY = {
    'stand': ((0, 0, 0), 0, 0), 'address': ((0, 3, -8), -4, 12), 'lift': ((-3, -2, -6), 22, 2),
    'back': ((-5, -5, -6), 38, -4), 'strike': ((4, 7, -14), -24, 26), 'bite': ((4, 8, -15), -26, 28),
    'rock': ((2, 5, -12), -16, 20), 'recover': ((0, 3, -7), -6, 8), 'end': ((0, 0, 0), 0, 0),
}
TWIST_SIGN = 1
# Left foot leads (toward the trunk), right foot back, as for a right-shoulder swing.
FOOT_L_FORWARD = (14.0, 17.0, 8.6)
FOOT_R_BACK = (-16.0, -8.0, 8.6)


def _vec(v):
    return unreal.Vector(*v)


def _norm(v):
    v = _vec(v) if not isinstance(v, unreal.Vector) else v
    return v.normal()


def _part(name):
    return name.rstrip('0123456789')


def swing_normal():
    """Normal of the swing plane (backswing haft x strike haft), used to derive the edge."""
    return _norm(STROKE['back'][1]).cross(_norm(STROKE['strike'][1])).normal()


def edge_for(haft):
    """Edge direction for a haft direction: along the head's travel in the swing plane."""
    h = _norm(haft)
    n = swing_normal()
    # At impact the edge leads the head forward into the trunk.
    if n.cross(_norm(STROKE['strike'][1])).y < 0:
        n = n * -1.0
    e = n.cross(h)
    return (e - h * e.dot(h)).normal()


class Hand:
    """Rest-pose grip geometry of one hand: the closed grip's centre relative to the wrist."""

    def __init__(self, s, side):
        hand = s.bone(f'hand_{side}').translation
        middle = s.bone(f'middle_01_{side}').translation
        self.side = side
        self.reach = (middle - hand).length() * 0.78
        along = (middle - hand).normal()
        across = s.bone(f'index_01_{side}').translation - s.bone(f'pinky_01_{side}').translation
        across = (across - along * across.dot(along)).normal()
        self.rest = unreal.MathLibrary.make_rot_from_xz(along, across.cross(along)).quaternion()

    def turn(self, haft, edge):
        """Extra world rotation that lays the haft pinky-to-index and the knuckles along the edge."""
        target = unreal.MathLibrary.make_rot_from_xz(edge, haft.cross(edge)).quaternion()
        return (target * self.rest.inversed()).rotator()

    def wrist(self, centre, haft, edge):
        """Wrist position that puts the closed grip's centre at ``centre``."""
        # Into the palm, where the handle sits: along x across for the right hand, mirrored on the left.
        palm = edge.cross(haft) if self.side == 'r' else haft.cross(edge)
        w = _vec(centre) - edge * self.reach - palm * 2.6
        return (w.x, w.y, w.z)


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    right, left = Hand(s, 'r'), Hand(s, 'l')
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        part = _part(name)
        drop, twist, lean = BODY[part]
        twist *= TWIST_SIGN
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop),
                    unreal.Rotator(roll=lean * 0.4, pitch=0, yaw=twist * 0.35))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=lean * 0.2, yaw=twist * 0.22)
        # Eyes on the notch: the head undoes most of the torso's wind-up and looks down at the cut.
        s.key_rotation(frame, 'neck_01_ctrl', roll=lean * 0.1, yaw=-twist * 0.3)
        s.key_rotation(frame, 'head_ctrl', roll=(8 + lean * 0.2) if frame else 0, yaw=-twist * 0.45)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R[part])
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L[part])
        if name in ('stand', 'end'):
            position, blade, edge = WRIST_R_STAND
            b, e = _norm(blade), _norm(edge)
            s.key_world(frame, 'hand_r_ik_ctrl', position, right.turn(b, (e - b * e.dot(b)).normal()))
            s.key_world(frame, 'hand_l_ik_ctrl', WRIST_L_STAND, s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
            continue
        centre, haft, slide = GRIP[name]
        h = _norm(haft)
        e = edge_for(h)
        s.key_world(frame, 'hand_l_ik_ctrl', left.wrist(centre, h, e), left.turn(h, e))
        top = _vec(centre) + h * slide
        s.key_world(frame, 'hand_r_ik_ctrl', right.wrist((top.x, top.y, top.z), h, e), right.turn(h, e))
    # Stance: step the left foot toward the trunk and set the right back, then return.
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(5, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 9, 6)))
    s.key_world(F['address'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 6, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 9, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['address'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    return s.bake(ANIM)


def _grip_frame(b, side):
    hand = b[f'hand_{side}'].translation
    along = (b[f'middle_01_{side}'].translation - hand).normal()
    across = b[f'index_01_{side}'].translation - b[f'pinky_01_{side}'].translation
    across = (across - along * across.dot(along)).normal()
    palm = along.cross(across) if side == 'r' else across.cross(along)
    reach = (b[f'middle_01_{side}'].translation - hand).length() * 0.78
    return hand + along * reach + palm * 2.6, across


def report(anim):
    """Per key: left (knob) grip centre vs. wanted, haft (left hand's across axis), the right
    grip's distance up that haft and its offset from it (both hands should wrap the same line)."""
    bones = [f'{b}_{side}' for side in 'lr' for b in ('hand', 'middle_01', 'index_01', 'pinky_01')]
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, bones, frame / 30)
        centre_l, haft = _grip_frame(b, 'l')
        centre_r, haft_r = _grip_frame(b, 'r')
        rel = centre_r - centre_l
        up = rel.dot(haft)
        off = (rel - haft * up).length()
        want = GRIP.get(name, (None,))[0]
        lines.append(f"{name:8s} knob ({centre_l.x:6.1f},{centre_l.y:6.1f},{centre_l.z:6.1f}) want {want}  "
                     f"haft ({haft.x:5.2f},{haft.y:5.2f},{haft.z:5.2f})  right up {up:5.1f} off {off:4.1f}  "
                     f"right haft dot {haft.dot(haft_r):5.2f}")
    return '\n'.join(lines)


def bit_at_strike():
    """Component-space bit centre and the bit's horizontal travel at impact (for the C++ stance)."""
    centre, haft, _ = STROKE['strike']
    h = _norm(haft)
    e = edge_for(h)
    bit = _vec(centre) + h * HEAD_ALONG + e * HEAD_EDGE
    travel = unreal.Vector(e.x, e.y, 0).normal()
    return bit, travel, e
