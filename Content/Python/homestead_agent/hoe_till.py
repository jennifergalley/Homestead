"""Hoeing a garden square for the MetaHuman heroine with the stone hoe (SM_StoneHoe).

    from homestead_agent import hoe_till as ht
    anim = ht.build()      # bakes /Game/Characters/Heroine_MH/Animations/AN_HeroineMH_HoeTill
    print(ht.report(anim)) # blade tip against the soil at each key

She keeps the hoe in the grip she carries it with (right hand at the top of the haft, blade out in
front), so nothing turns in her hand as she starts or stops: she sets her feet, bends from the
hips over the square and lays the blade on the soil, brings her left hand onto the haft, lifts the
blade a little, chops it down into the earth in front of her and draws it back toward her along
the ground to turn the soil. A second chop lands a hand's width to the side, then she lets the hoe
back up into the carry. The game shows the square's turned soil from EVENTS['chop1'].

The keys name where the blade's tip is and the direction the haft points toward the blade; the
right wrist follows from the hoe's carried placement in her hand (``HELD``, read from the running
game's ``Held_SM_StoneHoe`` component relative to ``hand_r`` with ``homestead.CarryHoe`` at its
default). SM_StoneHoe (Assets/Props/StoneHoe/report.json "attach"): pivot at the old working grip,
haft along +Z toward its top end, blade tip at (0, -EDGE_OUT, EDGE_ALONG).

Component space: forward +Y, her left +X, up +Z, floor z = 0.
"""
import unreal
from homestead_agent import rig_authoring as ra
from homestead_agent import kneel_gather as kg
from homestead_agent import axe_fell as af

SEQUENCE = 'LS_HoeTill'
ANIM = 'AN_HeroineMH_HoeTill'

FRAMES = {
    'stand': 0, 'set': 14, 'raise1': 24, 'chop1': 31, 'bite1': 34, 'draw1': 44,
    'raise2': 54, 'chop2': 61, 'bite2': 64, 'draw2': 74, 'recover': 88, 'end': 102,
}
EVENTS = {'chop1': FRAMES['chop1'] / 30, 'chop2': FRAMES['chop2'] / 30}
EDGE_ALONG = -75.45
EDGE_OUT = 19.31
# The hoe's transform relative to hand_r while carried (AHomesteadCharacter::UpdateHeldTools):
# location (cm) and rotation (x, y, z, w).
HELD = ((-10.8226, -1.6719, 26.0197), (-0.709149, 0.694266, 0.003177, 0.122849))
# Where her left hand closes on the haft, from the prop pivot toward the blade (cm).
LEFT_ALONG = -4.0

# Blade tip (cm) and the haft's direction toward the blade at each key. The strokes stay low:
# the blade lifts under half a metre, chops into the square and drags back along the soil with
# the haft at a steady slope, so the hoe moves smoothly over the ground.
CHOP = (0.14, 0.70, -0.70)
KEYS = {
    'set': ((-4.0, 76.0, 3.0), (0.14, 0.78, -0.61)),
    'raise1': ((-4.0, 84.0, 44.0), (0.14, 0.94, -0.30)),
    'chop1': ((-4.0, 72.0, 0.5), CHOP),
    'bite1': ((-4.0, 71.0, -1.5), CHOP),
    'draw1': ((-4.0, 60.0, 0.5), CHOP),
    'raise2': ((-10.0, 84.0, 44.0), (0.08, 0.94, -0.30)),
    'chop2': ((-10.0, 72.0, 0.5), (0.08, 0.70, -0.70)),
    'bite2': ((-10.0, 71.0, -1.5), (0.08, 0.70, -0.70)),
    'draw2': ((-10.0, 60.0, 0.5), (0.08, 0.70, -0.70)),
    'recover': ((-7.0, 80.0, 24.0), (0.14, 0.86, -0.50)),
}
# Pelvis drop/forward (cm) and forward bend (deg).
BODY = {
    'stand': ((0, 0, 0), 0), 'set': ((0, 2, -12), 32), 'raise1': ((0, 1, -10), 26),
    'chop1': ((0, 4, -16), 42), 'bite1': ((0, 4, -17), 44), 'draw1': ((0, 1, -15), 40),
    'raise2': ((0, 1, -10), 26), 'chop2': ((0, 4, -16), 42), 'bite2': ((0, 4, -17), 44),
    'draw2': ((0, 1, -15), 40), 'recover': ((0, 1, -6), 14), 'end': ((0, 0, 0), 0),
}
POLE_R = (-60.0, -25.0, 95.0)
POLE_L = (55.0, 10.0, 70.0)
FOOT_L_FORWARD = (14.0, 18.0, 8.6)
FOOT_R_BACK = (-15.0, -12.0, 8.6)


def _v(t):
    return unreal.Vector(*t)


def _frame(key):
    """Haft direction toward the blade and the blade's hang (down and back, square to the haft)."""
    tip, toward = KEYS[key]
    h = _v(toward).normal()
    down = unreal.Vector(0, 0, -1)
    b = (down - h * down.dot(h)).normal()
    return _v(tip), h, b


def prop_transform(key):
    """Component-space transform of the hoe: +Z toward the top end (-h), -Y along the blade (b)."""
    tip, h, b = _frame(key)
    origin = tip - h * (-EDGE_ALONG) - b * EDGE_OUT
    rot = unreal.MathLibrary.make_rot_from_zy(h * -1.0, b * -1.0)
    return unreal.Transform(origin, rot, unreal.Vector(1, 1, 1))


def _held():
    loc, q = HELD
    return unreal.Transform(_v(loc), unreal.Quat(*q).rotator(), unreal.Vector(1, 1, 1))


def right_hand(s, key):
    """hand_r_ik_ctrl location and extra rotation that put the carried hoe at ``prop_transform``."""
    # Prop = Held * Hand, so Hand = Held^-1 * Prop.
    hand = unreal.MathLibrary.compose_transforms(unreal.MathLibrary.invert_transform(_held()), prop_transform(key))
    rest = s.bone('hand_r').rotation
    return hand.translation, (hand.rotation * rest.inversed()).rotator()


def build():
    s = ra.Session(SEQUENCE, frames=FRAMES['end'])
    right, left = af.Hand(s, 'r'), af.Hand(s, 'l')
    F = FRAMES
    for f in (F['stand'], F['end']):
        s.key_bool(f, 'arm_l_fk_ik_switch', True)
        s.key_bool(f, 'arm_r_fk_ik_switch', True)
    for name, frame in F.items():
        drop, bend = BODY[name]
        s.key_world(frame, 'body_ctrl', kg._add(kg.BODY_STAND, drop), unreal.Rotator(roll=bend * 0.45, pitch=0, yaw=0))
        for control in ('spine_01_ctrl', 'spine_02_ctrl', 'spine_03_ctrl'):
            s.key_rotation(frame, control, roll=bend * 0.18)
        s.key_rotation(frame, 'neck_01_ctrl', roll=bend * 0.05)
        s.key_rotation(frame, 'head_ctrl', roll=(8 + bend * 0.12) if frame else 0)
        s.key_world(frame, 'arm_r_pv_ik_ctrl', POLE_R if name not in ('stand', 'end') else (-45.0, -30.0, 100.0))
        s.key_world(frame, 'arm_l_pv_ik_ctrl', POLE_L if name not in ('stand', 'end') else (45.0, -30.0, 100.0))
        if name in ('stand', 'end'):
            position, blade, edge = af.WRIST_R_STAND
            b, e = af._norm(blade), af._norm(edge)
            s.key_world(frame, 'hand_r_ik_ctrl', position, right.turn(b, (e - b * e.dot(b)).normal()))
            s.key_world(frame, 'hand_l_ik_ctrl', af.WRIST_L_STAND, s.hand_turn('l', (0, 0.2, -1), (-1, 0, 0)))
            continue
        position, turn = right_hand(s, name)
        s.key_world(frame, 'hand_r_ik_ctrl', (position.x, position.y, position.z), turn)
        _, h, b = _frame(name)
        grip = prop_transform(name).translation + h * LEFT_ALONG
        # Left hand overhand on the haft: index toward the blade, knuckles down its hang.
        s.key_world(frame, 'hand_l_ik_ctrl', left.wrist((grip.x, grip.y, grip.z), h, b), left.turn(h, b))
    s.key_world(F['stand'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(5, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 9, 6)))
    s.key_world(F['set'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'], 'foot_l_ik_ctrl', FOOT_L_FORWARD)
    s.key_world(F['recover'] + 6, 'foot_l_ik_ctrl', kg._add(kg.FOOT_L, (0, 9, 6)))
    s.key_world(F['end'], 'foot_l_ik_ctrl', kg.FOOT_L)
    s.key_world(F['stand'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['set'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['recover'], 'foot_r_ik_ctrl', FOOT_R_BACK)
    s.key_world(F['end'], 'foot_r_ik_ctrl', kg.FOOT_R)
    s.key_world(F['set'], 'leg_l_pv_ik_ctrl', (25.0, 80.0, 50.0))
    s.key_world(F['set'], 'leg_r_pv_ik_ctrl', (-25.0, 80.0, 50.0))
    return s.bake(ANIM)


def report(anim):
    """Baked blade tip (through the carried hoe in the baked right hand) vs keyed, and hand spacing."""
    bones = ['hand_r', 'hand_l']
    tip_local = unreal.Vector(0, -EDGE_OUT, EDGE_ALONG)
    lines = []
    for name, frame in FRAMES.items():
        b = ra.bone_positions(anim, bones, frame / 30)
        prop = unreal.MathLibrary.compose_transforms(_held(), b['hand_r'])
        tip = unreal.MathLibrary.transform_location(prop, tip_local)
        want = KEYS.get(name, (None,))[0]
        lines.append(f"{name:8s} tip ({tip.x:6.1f},{tip.y:6.1f},{tip.z:6.1f}) want {want}  "
                     f"hands {(b['hand_l'].translation - b['hand_r'].translation).length():5.1f} cm")
    return '\n'.join(lines)
