"""Mowing with the scythe for the MetaHuman heroine.

    from homestead_agent import scythe_mow as sm
    anim = sm.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_ScytheMow
    print(sm.report(anim)) # both nib grips against where the scythe puts them at each key

Technique, after English mowing references: feet wide and staggered, knees well bent and the body
bowed forward from the hips over the swath, the right hand reaching forward and down to the lower
nib and the left on the upper nib in front of her hip. The stroke is a turn
of the hips and chest, not an arm swing: she winds to her right with the blade's heel riding on the
ground, then turns through to her left with the blade flat and just above the soil, so it cuts a
crescent in front of her from right to left. The return swings it back round to the right to
start again. Nothing lifts high: the blade stays low through the whole cycle.

The scythe is modelled so the whole tool turns rigidly about her: ``scythe_at(yaw)`` places it,
the nib grips of the placed scythe become the fists' targets, and the game lays the prop from the
two fists while she mows (``AHomesteadCharacter::UpdateFellingHatchet``). SM_Scythe
(Scripts/Blender/Recipes/scythe.py): pivot at the lower nib's grip, the snath up +Z, the nibs out
along -Y, the blade along +X from the heel with its edge toward -Y, and the blade set on the snath
at the working lean (``LEAN``) so it lies flat when she holds the snath leaning back toward her.
Everything here is in scythe.py's own coordinates. The Unreal import negates Y, so the game shows
the prop mirrored in its local Y (``HomesteadScythe::Mirror``) to match.

The clip has the felling clip's structure (``axe_fell.FRAMES``): the 'strike' keys are the blade
crossing in front of her, where the grass falls, and the cycle from one stroke's end to the next
repeats for longer mowing, so the C++ felling timing applies unchanged.

The game lays the scythe from her fists every frame (``blade_points``): between sparse keys the fists
interpolate off the rigid scythe's path, and in PIE (09-30) its point dug up to 38 cm into the ground as
each stroke opened and closed. ``build()`` therefore measures the blade that way on every two-handed frame
and, wherever it is below BLADE_CLEARANCE, keys both hands raised together by the shortfall and bakes
again. Raised together, the scythe rises without turning, so the fists stay on the nibs and the swing
keeps its shape. ``report()`` prints the lowest point over the clip.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import axe_fell as af

# The blade, laid from her fists exactly as AHomesteadCharacter::UpdateMowingScythe lays it, must stay at
# least BLADE_CLEARANCE (cm) above the floor (z = 0) on every frame; build() lifts both hands where it doesn't,
# by the shortfall plus LIFT_MARGIN, re-baking up to LIFT_PASSES times. Back and edge along the blade and its
# point, in scythe.py coordinates (HomesteadCharacterEquipment.cpp MowGround::BladeSamples).
BLADE_CLEARANCE = 3.0
LIFT_MARGIN = 1.0
LIFT_PASSES = 3
# The game lays the scythe from her fists whenever the action is blended in (UpdateMowingScythe blends by
# FellWeight): fully from 0.12 s after the clip starts until 0.16 s before it ends (UHomesteadAnimInstance's
# ActionBlend rates), so the getting-into and out-of-grip transitions must clear the ground too.
BLEND_IN_FRAMES = 4
BLEND_OUT_FRAMES = 5
BLADE_SAMPLES = ((6.3, 11.0, -100.9), (6.3, 4.7, -94.5), (45.8, 9.2, -97.8), (45.8, 5.2, -93.8),
                 (67.3, 5.5, -94.5), (67.3, 3.1, -92.2), (88.8, -0.3, -89.7))
_GAME_BONES = [f'{b}_{side}' for side in 'lr' for b in ('hand', 'middle_01', 'index_01', 'pinky_01')]
SEQUENCE = 'LS_ScytheMow'
ANIM = 'AN_HeroineMH_ScytheMow'

FRAMES = dict(af.FRAMES)
LOOP_START = af.LOOP_START
LOOP = af.LOOP

# The snath leans back toward her by LEAN (degrees) and the scythe turns about her by the sweep
# yaw (+ to her right). NIB_LOWER is the lower nib grip (the prop pivot) at zero sweep.
LEAN = 45.0
# 28 cm further out than first authored (Jenny, 10-01: one hand clipped her chest; move the tool in front of her
# so her arms have room): her left fist on the upper nib now clears her chest and belly through the whole stroke,
# and the right arm reaches near straight to the lower nib, as on a real scythe.
NIB_LOWER = (-10.0, 84.0, 72.0)
# The upper nib grip in the prop's frame (scythe.py: 42 cm up the snath).
NIB_UPPER = (1.7, -4.4, 42.0)
HEEL = (2.8, 9.6, -98.0)
# Sweep yaw, lift of the whole tool (cm) and how far it's drawn in toward her (cm) per key: at the
# ends of the sweep and in the recovery her right arm can't reach the full stroke's length.
SWEEP = {
    'address': (8.0, 1.0, 7.0), 'lift': (28.0, 2.0, 7.0), 'back': (48.0, 1.0, 8.0), 'strike': (0.0, 0.0, 0.0),
    'bite': (-28.0, 0.0, 0.0), 'rock': (-48.0, 1.5, 4.0), 'recover': (0.0, 12.0, 14.0),
}
TWIST_SHARE = 0.8
# Pelvis offset (cm), forward lean (deg) per key; the twist follows the sweep. Knees well bent and
# bowed over the swath from the hips; the weight rides on the back (right) foot at the wind-up and
# moves onto the lead (left) foot as the blade crosses and follows through.
BODY = {
    'stand': ((0, 0, 0), 0), 'address': ((0, 4, -16), 22), 'lift': ((-3, 2, -17), 21), 'back': ((-6, 0, -17), 20),
    'strike': ((0, 6, -21), 28), 'bite': ((4, 7, -21), 28), 'rock': ((7, 5, -19), 24),
    'recover': ((0, 3, -9), 10), 'end': ((0, 0, 0), 0),
}
# The right elbow stays low and fairly straight as that arm reaches down to the lower nib; the
# left elbow bends out to her left over the upper nib.
POLE_R = {
    'stand': (-60.0, -10.0, 90.0), 'address': (-50.0, 10.0, 60.0), 'lift': (-60.0, -5.0, 65.0),
    'back': (-55.0, -20.0, 70.0), 'strike': (-40.0, 20.0, 55.0), 'bite': (-25.0, 25.0, 55.0),
    'rock': (-10.0, 30.0, 60.0), 'recover': (-55.0, 0.0, 80.0), 'end': (-60.0, -10.0, 90.0),
}
POLE_L = {
    'stand': (60.0, -10.0, 90.0), 'address': (60.0, 10.0, 100.0), 'lift': (45.0, 40.0, 100.0),
    'back': (30.0, 40.0, 100.0), 'strike': (60.0, 5.0, 100.0), 'bite': (65.0, 0.0, 100.0),
    'rock': (65.0, -5.0, 100.0), 'recover': (55.0, 0.0, 95.0), 'end': (60.0, -10.0, 90.0),
}
# The left fist's roll on its nib per key (deg, as axe_fell.ROLL_L). The game takes only the left grip's
# centre (the snath's line), which a roll doesn't move; the right fist's pinky-to-index axis sets the
# blade, so it isn't rolled. Unrolled, the left wrist bent back up to 118 degrees over the nib; these keep it
# and the forearm inside their comfortable ranges (joint_limits).
ROLL_L = {'address': 60.0, 'lift': 30.0, 'back': 30.0, 'strike': 60.0, 'bite': 60.0, 'rock': 60.0, 'recover': 30.0}
WRIST_R_STAND = af.WRIST_R_STAND
WRIST_L_STAND = af.WRIST_L_STAND
# A wide, staggered stance: left foot forward toward the swath, right foot back.
FOOT_L_FORWARD = (22.0, 16.0, 8.6)
FOOT_R_BACK = (-22.0, -12.0, 8.6)
# Shoulders at rest (component space), for choosing how each fist sits on its nib.
SHOULDER = {'r': (-18.0, -2.0, 138.0), 'l': (18.0, -2.0, 138.0)}


def _rot_z(v, degrees):
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    # + turns toward her right (-X): forward (+Y) swings to -X.
    return (v[0] * c - v[1] * s, v[0] * s + v[1] * c, v[2])


def _rot_x(v, degrees):
    a = math.radians(degrees)
    c, s = math.cos(a), math.sin(a)
    return (v[0], v[1] * c - v[2] * s, v[1] * s + v[2] * c)


def scythe_at(yaw, lift=0.0, pull=0.0):
    """Component-space placement of the scythe at a sweep yaw: a function mapping a prop-frame
    point (cm) to component space, and the prop's X, Y and Z axes there."""
    def point(p):
        q = _rot_x(p, LEAN)
        q = (q[0] + NIB_LOWER[0], q[1] + NIB_LOWER[1] - pull, q[2] + NIB_LOWER[2] + lift)
        return _rot_z(q, yaw)

    def axis(d):
        return _rot_z(_rot_x(d, LEAN), yaw)
    return point, axis((1, 0, 0)), axis((0, 1, 0)), axis((0, 0, 1))


def _knuckles(side, centre, haft, twist):
    """Knuckle direction for a fist at ``centre`` gripping along ``haft``: the forearm's line from
    the (turned) shoulder, square to the nib."""
    shoulder = af._vec(_rot_z(SHOULDER[side], twist))
    d = af._vec(centre) - shoulder
    d = d - haft * d.dot(haft)
    return d.normal()


def grips(name):
    """Right and left grip centres, nib directions (pinky to index, toward the snath) and knuckle
    directions for a key."""
    part = af._part(name)
    yaw, lift, pull = SWEEP[part]
    point, _, y, _ = scythe_at(yaw, lift, pull)
    nib = af._norm(y)
    twist = yaw * TWIST_SHARE
    lower = point((0.0, 0.0, 0.0))
    upper = point(NIB_UPPER)
    return ((lower, nib, _knuckles('r', lower, nib, twist)), (upper, nib, _knuckles('l', upper, nib, twist)))


def build():
    """Bake, then lift both hands wherever the blade (laid from her fists as the game does) dips below
    BLADE_CLEARANCE, and bake again; a few passes settle it. That covers the whole stretch the game lays the
    scythe from her fists, the carry-to-address and recovery transitions included (PIE 10-01: they dug in
    30-40 cm once MaxTipUp no longer tipped it out)."""
    lifts = {}
    anim = _author(lifts)
    for _ in range(LIFT_PASSES):
        low = {f: h for f, h in blade_heights(anim, *laid_frames()).items() if h < BLADE_CLEARANCE}
        if not low:
            break
        for frame, height in low.items():
            lifts[frame] = _lifted_hands(anim, frame, BLADE_CLEARANCE + LIFT_MARGIN - height)
        anim = _author(lifts)
    return anim


def laid_frames():
    """First and last frame the game lays the scythe wholly from her fists."""
    return FRAMES['stand'] + BLEND_IN_FRAMES, FRAMES['end'] - BLEND_OUT_FRAMES


def _author(lifts):
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    right, left = af.Hand(s, 'r'), af.Hand(s, 'l')
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        part = af._part(name)
        drop, lean = BODY[part]
        twist = SWEEP[part][0] * TWIST_SHARE if part in SWEEP else 0.0
        twist *= af.TWIST_SIGN
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop),
                    unreal.Rotator(roll=lean * 0.4, pitch=0, yaw=twist * 0.4))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=lean * 0.2, yaw=twist * 0.2)
        # Eyes on the grass ahead: the head stays pointed at the swath's middle.
        s.key_rotation(frame, 'neck_01_ctrl', roll=lean * 0.1, yaw=-twist * 0.35)
        s.key_rotation(frame, 'head_ctrl', roll=(10 + lean * 0.2) if frame else 0, yaw=-twist * 0.5)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R[part])
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L[part])
        if name in ('stand', 'end'):
            position, blade, edge = WRIST_R_STAND
            b, e = af._norm(blade), af._norm(edge)
            s.key_world(frame, 'hand_r_ik_ctrl', position, right.turn(b, (e - b * e.dot(b)).normal()))
            s.key_world(frame, 'hand_l_ik_ctrl', WRIST_L_STAND, s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
            continue
        (rc, rn, rk), (lc, ln, lk) = grips(name)
        s.key_world(frame, 'hand_r_ik_ctrl', right.wrist(rc, rn, rk), right.turn(rn, rk))
        lk = af.rolled(lk, ln, ROLL_L[part])
        s.key_world(frame, 'hand_l_ik_ctrl', left.wrist(lc, ln, lk), left.turn(ln, lk))
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(5, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (2, 6, 6)))
    s.key_world(F['address'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 6, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (2, 6, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['address'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    # Frames where the blade would cut into the ground: both hands raised together (build()).
    for frame, hands in lifts.items():
        for side, (location, quat) in hands.items():
            rest = s.bone(f'hand_{side}').rotation
            s.key_world(frame, f'hand_{side}_ik_ctrl', location, (quat * rest.inversed()).rotator())
    return s.bake(ANIM)

def _game_grip(b, side):
    """AHomesteadCharacter::UpdateMowingScythe's GripCentre for one fist."""
    hand = b[f'hand_{side}'].translation
    knuckle = b[f'middle_01_{side}'].translation
    across = b[f'index_01_{side}'].translation - b[f'pinky_01_{side}'].translation
    along = (knuckle - hand).normal()
    palm = (across.cross(along) if side == 'l' else along.cross(across)).normal()
    return hand + (knuckle - hand) * 0.78 + palm * 2.6


def blade_points(anim, frame):
    """The blade samples (component cm) as the game lays the scythe from her fists at ``frame``: the lower
    nib in the right fist, its nib along the right fist's pinky-to-index axis, the snath toward the left fist."""
    b = ra.bone_positions(anim, _GAME_BONES, frame / 30)
    lower, upper = _game_grip(b, 'r'), _game_grip(b, 'l')
    nib = (b['index_01_r'].translation - b['pinky_01_r'].translation).normal()
    snath = upper - lower
    snath = (snath - nib * snath.dot(nib)).normal()
    x = nib.cross(snath)
    return [lower + x * p[0] + nib * p[1] + snath * p[2] for p in BLADE_SAMPLES]


def blade_heights(anim, first=None, last=None):
    """Lowest blade sample above the floor on each frame from `first` to `last` (default: the two-handed
    stretch, address to recover)."""
    first = FRAMES['address'] if first is None else first
    last = FRAMES['recover'] if last is None else last
    return {f: min(p.z for p in blade_points(anim, f)) for f in range(first, last + 1)}


def _lifted_hands(anim, frame, lift):
    """Both wrists as baked at ``frame``, raised by ``lift`` cm: the scythe rises with them unturned."""
    b = ra.bone_positions(anim, ('hand_r', 'hand_l'), frame / 30)
    out = {}
    for side in 'rl':
        t = b[f'hand_{side}']
        out[side] = ((t.translation.x, t.translation.y, t.translation.z + lift), t.rotation)
    return out


def report(anim):
    """Per key: each fist's grip centre against its nib target and the blade heel's height; then the blade's
    lowest point over the whole two-handed stretch, laid from her fists as the game does (BLADE_CLEARANCE)."""
    bones = [f'{b}_{side}' for side in 'lr' for b in ('hand', 'middle_01', 'index_01', 'pinky_01')]
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, bones, frame / 30)
        centre_r, across_r = af._grip_frame(b, 'r')
        centre_l, _ = af._grip_frame(b, 'l')
        if name in ('stand', 'end'):
            lines.append(f"{name:8s} right ({centre_r.x:6.1f},{centre_r.y:6.1f},{centre_r.z:6.1f})")
            continue
        (rc, rn, _), (lc, _, _) = grips(name)
        yaw, lift, pull = SWEEP[af._part(name)]
        heel = scythe_at(yaw, lift, pull)[0](HEEL)
        miss_r = (centre_r - af._vec(rc)).length()
        miss_l = (centre_l - af._vec(lc)).length()
        lines.append(f"{name:8s} right miss {miss_r:4.1f} left miss {miss_l:4.1f} nib dot {across_r.dot(rn):5.2f} "
                     f"heel ({heel[0]:6.1f},{heel[1]:6.1f},{heel[2]:5.1f})")
    heights = blade_heights(anim, *laid_frames())
    worst = min(heights, key=heights.get)
    low = [f for f, h in heights.items() if h < BLADE_CLEARANCE]
    first, last = laid_frames()
    lines.append(f"blade lowest {heights[worst]:5.1f} cm at frame {worst} (frames {first}-{last}, transitions included); "
                 f"frames below {BLADE_CLEARANCE:.0f} cm: {low or 'none'}")
    for first, last in ((first, FRAMES['address'] - 1), (FRAMES['recover'] + 1, last)):
        part = {f: heights[f] for f in range(first, last + 1)}
        at = min(part, key=part.get)
        lines.append(f"  transition {first}-{last}: lowest {part[at]:6.1f} cm at frame {at}")
    return '\n'.join(lines)
