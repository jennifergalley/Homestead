"""Two-handed downward strike for the MetaHuman heroine: the pickaxe into rubble and rock, and the
axe down into stumps and fallen timber.

    from homestead_agent import ground_strike as gs
    anim = gs.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_GroundStrike
    print(gs.report(anim)) # knob grip, haft and right-hand slide at each key
    print(gs.bits())       # pick point and axe bit at impact (for the C++ stance constants)

Technique, after pick and grubbing-axe references: a square, slightly staggered stance facing the
work, knees soft. The left hand stays at the end of the haft; the right hand slides up toward the
head as she lifts the tool over her right shoulder, then slides back down to the left hand as she
drives it down in front of her, bending from the hips so the point (or the axe's bit) lands low,
well ahead of her feet, never toward her shins. After each blow she prises the head free and lifts
again. The game holds the tool in both fists the same way as felling
(``AHomesteadCharacter::UpdateFellingHatchet``).

The clip has exactly the felling clip's structure (``axe_fell.FRAMES``): two identical strokes
with the cycle from the first prise to the second repeated for tougher targets, so the C++
felling timing (``FellTiming``) and stroke loop apply unchanged.

The pickaxe (SM_Pickaxe) and the axe share the hatchet's convention: pivot at the knob hand's grip
centre, head along +Z and the working point or edge toward -Y.

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import axe_fell as af

SEQUENCE = 'LS_GroundStrike'
ANIM = 'AN_HeroineMH_GroundStrike'

FRAMES = dict(af.FRAMES)
LOOP_START = af.LOOP_START
LOOP = af.LOOP

# Left (knob) grip centre, haft direction and right hand's distance up the haft (cm) per key. The
# swing plane is nearly vertical and a little right of centre, over her right shoulder.
STROKE = {
    # Lifting: the right hand runs up the haft toward the head.
    'lift': ((-6.0, 22.0, 118.0), (-0.1, 0.25, 0.96), 30.0),
    # Top of the lift: fists above her right shoulder, the head behind her head.
    'back': ((-7.0, 10.0, 146.0), (-0.12, -0.5, 0.86), 36.0),
    # Impact: fists low in front at the end of the haft, the head down on the ground ahead. The haft lands 15 degrees
    # flatter than first authored, the point on the same spot, so her front wrist isn't cocked sharply at the blow
    # (Jenny, 10-01; joint_limits: right wrist flexion 67 -> 43 degrees).
    'strike': ((-2.4, 18.1, 72.9), (-0.023, 0.928, -0.372), 11.0),
    'bite': ((-2.8, 18.4, 70.4), (-0.023, 0.914, -0.405), 11.0),
    # Prising the head free: the fists rise a little and draw back.
    'rock': ((-3.6, 15.5, 78.4), (-0.034, 0.965, -0.261), 14.0),
}
# Halfway down each blow (between 'back' and 'strike'): the fists swing out in front of her face and chest. Left to
# interpolate, they cut straight from above her head to low in front, through her head and chest, and her right
# elbow folded to 158 degrees (now 90).
DOWN = ((-5.0, 35.0, 105.0), (-0.15, 0.55, 0.82), 18.0)
DOWN_ROLL = (60.0, -30.0)
DOWN_POLE = ((45.0, 15.0, 110.0), (-60.0, 15.0, 110.0))
GRIP = {
    'address': ((-4.0, 30.0, 90.0), (-0.02, 0.82, -0.57), 24.0),
    'recover': ((-10.0, 24.0, 94.0), (-0.2, 0.8, -0.55), 12.0),
}
for n in (1, 2):
    for part, value in STROKE.items():
        GRIP[f'{part}{n}'] = value
WRIST_R_STAND = af.WRIST_R_STAND
WRIST_L_STAND = af.WRIST_L_STAND
# Elbows: out to the sides and up on the lift, in by her ribs at impact.
POLE_R = {
    'stand': (-60.0, -10.0, 90.0), 'address': (-50.0, 10.0, 80.0), 'lift': (-70.0, 0.0, 125.0),
    'back': (-70.0, -10.0, 145.0), 'strike': (-45.0, 10.0, 70.0), 'bite': (-45.0, 10.0, 68.0),
    'rock': (-50.0, 8.0, 76.0), 'recover': (-55.0, -5.0, 85.0), 'end': (-60.0, -10.0, 90.0),
}
POLE_L = {
    'stand': (60.0, -10.0, 90.0), 'address': (45.0, 10.0, 78.0), 'lift': (45.0, 10.0, 115.0),
    'back': (40.0, 5.0, 135.0), 'strike': (45.0, 10.0, 70.0), 'bite': (45.0, 10.0, 68.0),
    'rock': (48.0, 8.0, 76.0), 'recover': (60.0, -5.0, 85.0), 'end': (60.0, -10.0, 90.0),
}
# Pelvis offset (cm), torso twist (deg, + turns her chest to her right) and forward lean (deg):
# upright with a little right turn at the top, bent well forward from the hips at impact.
BODY = {
    'stand': ((0, 0, 0), 0, 0), 'address': ((0, 3, -9), -2, 16), 'lift': ((-1, -1, -5), 8, 4),
    'back': ((-2, -3, -4), 12, -6), 'strike': ((0, 6, -16), -4, 32), 'bite': ((0, 7, -17), -4, 34),
    'rock': ((0, 5, -13), -2, 26), 'recover': ((0, 3, -8), 0, 10), 'end': ((0, 0, 0), 0, 0),
}
TWIST_SIGN = af.TWIST_SIGN
# Each fist's roll on the haft per key (deg, as axe_fell.ROLL_L / ROLL_R). Unrolled, both wrists folded 72-117
# degrees back on the forearms; these keep each wrist and forearm nearest their comfortable ranges
# (joint_limits). The game reads the point's direction from the swing plane (UpdateFellingHatchet's
# SwingNormal is swing_normal()), not her knuckles.
ROLL_L = {'address': 45.0, 'lift': 60.0, 'back': 90.0, 'strike': 45.0, 'bite': 45.0, 'rock': 45.0, 'recover': 60.0}
ROLL_R = {'address': 0.0, 'lift': -45.0, 'back': -30.0, 'strike': -15.0, 'bite': -15.0, 'rock': -30.0, 'recover': -15.0}
FOOT_L_FORWARD = (15.0, 12.0, 8.6)
FOOT_R_BACK = (-15.0, -6.0, 8.6)

# Working ends from the knob grip pivot (report.json sizes): the pick's point and the axe's bit.
PICK_ALONG, PICK_OUT = 70.0, 23.0
AXE_ALONG, AXE_OUT = af.HEAD_ALONG, af.HEAD_EDGE


def swing_normal():
    return af._norm(STROKE['back'][1]).cross(af._norm(STROKE['strike'][1])).normal()


def edge_for(haft):
    """The point's direction for a haft direction: in the swing plane, leading the head's travel, so
    at impact it faces down into the work."""
    h = af._norm(haft)
    n = swing_normal()
    strike = af._norm(STROKE['strike'][1])
    if n.cross(strike).z > 0:
        n = n * -1.0
    e = n.cross(h)
    return (e - h * e.dot(h)).normal()


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    right, left = af.Hand(s, 'r'), af.Hand(s, 'l')
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        part = af._part(name)
        drop, twist, lean = BODY[part]
        twist *= TWIST_SIGN
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop),
                    unreal.Rotator(roll=lean * 0.4, pitch=0, yaw=twist * 0.35))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=lean * 0.2, yaw=twist * 0.22)
        # Eyes on the work on the ground in front of her.
        s.key_rotation(frame, 'neck_01_ctrl', roll=lean * 0.12, yaw=-twist * 0.3)
        s.key_rotation(frame, 'head_ctrl', roll=(12 + lean * 0.25) if frame else 0, yaw=-twist * 0.45)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R[part])
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L[part])
        if name in ('stand', 'end'):
            position, blade, edge = WRIST_R_STAND
            b, e = af._norm(blade), af._norm(edge)
            s.key_world(frame, 'hand_r_ik_ctrl', position, right.turn(b, (e - b * e.dot(b)).normal()))
            s.key_world(frame, 'hand_l_ik_ctrl', WRIST_L_STAND, s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
            continue
        centre, haft, slide = GRIP[name]
        h = af._norm(haft)
        e = edge_for(h)
        e_l, e_r = af.rolled(e, h, ROLL_L[part]), af.rolled(e, h, ROLL_R[part])
        s.key_world(frame, 'hand_l_ik_ctrl', left.wrist(centre, h, e_l), left.turn(h, e_l))
        top = af._vec(centre) + h * slide
        s.key_world(frame, 'hand_r_ik_ctrl', right.wrist((top.x, top.y, top.z), h, e_r), right.turn(h, e_r))
    if DOWN is not None:
        for n in (1, 2):
            f = (F[f'back{n}'] + F[f'strike{n}']) // 2
            centre, haft, slide = DOWN
            h = af._norm(haft)
            e = edge_for(h)
            e_l, e_r = af.rolled(e, h, DOWN_ROLL[0]), af.rolled(e, h, DOWN_ROLL[1])
            s.key_world(f, 'hand_l_ik_ctrl', left.wrist(centre, h, e_l), left.turn(h, e_l))
            top = af._vec(centre) + h * slide
            s.key_world(f, 'hand_r_ik_ctrl', right.wrist((top.x, top.y, top.z), h, e_r), right.turn(h, e_r))
            s.key_world(f, 'arm_l_pv_ik_ctrl', DOWN_POLE[0])
            s.key_world(f, 'arm_r_pv_ik_ctrl', DOWN_POLE[1])
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(5, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 7, 6)))
    s.key_world(F['address'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 6, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (1, 7, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['address'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    return s.bake(ANIM)


def report(anim):
    """Per key: knob grip centre vs. wanted, the haft and how far up it the right fist sits."""
    bones = [f'{b}_{side}' for side in 'lr' for b in ('hand', 'middle_01', 'index_01', 'pinky_01')]
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, bones, frame / 30)
        centre_l, haft = af._grip_frame(b, 'l')
        centre_r, haft_r = af._grip_frame(b, 'r')
        rel = centre_r - centre_l
        up = rel.dot(haft)
        off = (rel - haft * up).length()
        want = GRIP.get(name, (None, None, None))
        lines.append(f"{name:8s} knob ({centre_l.x:6.1f},{centre_l.y:6.1f},{centre_l.z:6.1f}) want {want[0]}  "
                     f"haft ({haft.x:5.2f},{haft.y:5.2f},{haft.z:5.2f})  right up {up:5.1f} want {want[2]} off {off:4.1f}  "
                     f"right haft dot {haft.dot(haft_r):5.2f}")
    return '\n'.join(lines)


def bits():
    """Component-space pick point and axe bit at impact: (left, forward, height) each."""
    centre, haft, _ = STROKE['strike']
    h = af._norm(haft)
    e = edge_for(h)
    pick = af._vec(centre) + h * PICK_ALONG + e * PICK_OUT
    axe = af._vec(centre) + h * AXE_ALONG + e * AXE_OUT
    return {'pick': (pick.x, pick.y, pick.z), 'axe': (axe.x, axe.y, axe.z), 'edge': (e.x, e.y, e.z)}
