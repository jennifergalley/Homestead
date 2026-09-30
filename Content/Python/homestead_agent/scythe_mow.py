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

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import math
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import axe_fell as af

SEQUENCE = 'LS_ScytheMow'
ANIM = 'AN_HeroineMH_ScytheMow'

FRAMES = dict(af.FRAMES)
LOOP_START = af.LOOP_START
LOOP = af.LOOP

# The snath leans back toward her by LEAN (degrees) and the scythe turns about her by the sweep
# yaw (+ to her right). NIB_LOWER is the lower nib grip (the prop pivot) at zero sweep.
LEAN = 45.0
NIB_LOWER = (-10.0, 56.0, 72.0)
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
    'stand': (60.0, -10.0, 90.0), 'address': (60.0, 10.0, 100.0), 'lift': (45.0, 10.0, 100.0),
    'back': (30.0, 10.0, 100.0), 'strike': (60.0, 5.0, 100.0), 'bite': (65.0, 0.0, 100.0),
    'rock': (65.0, -5.0, 100.0), 'recover': (55.0, 0.0, 95.0), 'end': (60.0, -10.0, 90.0),
}
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
    return s.bake(ANIM)


def report(anim):
    """Per key: each fist's grip centre against its nib target, and the blade heel's height."""
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
    return '\n'.join(lines)
